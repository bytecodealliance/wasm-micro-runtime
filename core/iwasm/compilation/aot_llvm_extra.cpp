/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Initialization.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#include <llvm/Transforms/Vectorize/LoopVectorize.h>
#include <llvm/Transforms/Vectorize/LoadStoreVectorizer.h>
#include <llvm/Transforms/Vectorize/SLPVectorizer.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/SimpleLoopUnswitch.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#if LLVM_VERSION_MAJOR >= 12
#include <llvm/Analysis/AliasAnalysis.h>
#endif
#include <cstring>

#include "aot_llvm.h"

using namespace llvm;

extern "C" {

LLVMBool
WAMRCreateMCJITCompilerForModule(LLVMExecutionEngineRef *OutJIT,
                                 LLVMModuleRef M,
                                 LLVMMCJITCompilerOptions *PassedOptions,
                                 size_t SizeOfPassedOptions, char **OutError);

bool
aot_check_simd_compatibility(const char *arch_c_str, const char *cpu_c_str);

void
aot_add_expand_memory_op_pass(LLVMPassManagerRef pass);

void
aot_func_disable_tce(LLVMValueRef func);

void
aot_apply_llvm_new_pass_manager(AOTCompContext *comp_ctx);
}

static TargetMachine *
unwrap(LLVMTargetMachineRef P)
{
    return reinterpret_cast<TargetMachine *>(P);
}

LLVMBool
WAMRCreateMCJITCompilerForModule(LLVMExecutionEngineRef *OutJIT,
                                 LLVMModuleRef M,
                                 LLVMMCJITCompilerOptions *PassedOptions,
                                 size_t SizeOfPassedOptions, char **OutError)
{
    LLVMMCJITCompilerOptions options;
    // If the user passed a larger sized options struct, then they were compiled
    // against a newer LLVM. Tell them that something is wrong.
    if (SizeOfPassedOptions > sizeof(options)) {
        *OutError = strdup("Refusing to use options struct that is larger than "
                           "my own; assuming LLVM library mismatch.");
        return 1;
    }

    // Defend against the user having an old version of the API by ensuring that
    // any fields they didn't see are cleared. We must defend against fields
    // being set to the bitwise equivalent of zero, and assume that this means
    // "do the default" as if that option hadn't been available.
    LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
    memcpy(&options, PassedOptions, SizeOfPassedOptions);

    TargetOptions targetOptions;
    targetOptions.EnableFastISel = options.EnableFastISel;
    std::unique_ptr<Module> Mod(unwrap(M));

    if (Mod) {
        // Set function attribute "frame-pointer" based on
        // NoFramePointerElim.
        for (auto &F : *Mod) {
            auto Attrs = F.getAttributes();
            StringRef Value = options.NoFramePointerElim ? "all" : "none";
            Attrs =
                Attrs.addAttribute(F.getContext(), AttributeList::FunctionIndex,
                                   "frame-pointer", Value);
            F.setAttributes(Attrs);
        }
    }

    std::string Error;
    bool JIT;
    char *host_cpu = LLVMGetHostCPUName();

    if (!host_cpu) {
        *OutError = NULL;
        return false;
    }

    std::string mcpu(host_cpu);
    LLVMDisposeMessage(host_cpu);

    EngineBuilder builder(std::move(Mod));
    builder.setEngineKind(EngineKind::JIT)
        .setErrorStr(&Error)
        .setMCPU(mcpu)
        .setOptLevel((CodeGenOpt::Level)options.OptLevel)
        .setTargetOptions(targetOptions);
    if (Optional<CodeModel::Model> CM = unwrap(options.CodeModel, JIT))
        builder.setCodeModel(*CM);
    if (options.MCJMM)
        builder.setMCJITMemoryManager(
            std::unique_ptr<RTDyldMemoryManager>(unwrap(options.MCJMM)));
    if (ExecutionEngine *JIT = builder.create()) {
        *OutJIT = wrap(JIT);
        return 0;
    }
    *OutError = strdup(Error.c_str());
    return 1;
}

class ExpandMemoryOpPass : public llvm::ModulePass
{
  public:
    static char ID;

    ExpandMemoryOpPass()
      : ModulePass(ID)
    {}

    bool runOnModule(Module &M) override;

    bool expandMemIntrinsicUses(Function &F);
    StringRef getPassName() const override
    {
        return "Expand memory operation intrinsics";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override
    {
        AU.addRequired<TargetTransformInfoWrapperPass>();
    }
};

char ExpandMemoryOpPass::ID = 0;

bool
ExpandMemoryOpPass::expandMemIntrinsicUses(Function &F)
{
    Intrinsic::ID ID = F.getIntrinsicID();
    bool Changed = false;

    for (auto I = F.user_begin(), E = F.user_end(); I != E;) {
        Instruction *Inst = cast<Instruction>(*I);
        ++I;

        switch (ID) {
            case Intrinsic::memcpy:
            {
                auto *Memcpy = cast<MemCpyInst>(Inst);
                Function *ParentFunc = Memcpy->getParent()->getParent();
                const TargetTransformInfo &TTI =
                    getAnalysis<TargetTransformInfoWrapperPass>().getTTI(
                        *ParentFunc);
                expandMemCpyAsLoop(Memcpy, TTI);
                Changed = true;
                Memcpy->eraseFromParent();
                break;
            }
            case Intrinsic::memmove:
            {
                auto *Memmove = cast<MemMoveInst>(Inst);
                expandMemMoveAsLoop(Memmove);
                Changed = true;
                Memmove->eraseFromParent();
                break;
            }
            case Intrinsic::memset:
            {
                auto *Memset = cast<MemSetInst>(Inst);
                expandMemSetAsLoop(Memset);
                Changed = true;
                Memset->eraseFromParent();
                break;
            }
            default:
                break;
        }
    }

    return Changed;
}

bool
ExpandMemoryOpPass::runOnModule(Module &M)
{
    bool Changed = false;

    for (Function &F : M) {
        if (!F.isDeclaration())
            continue;

        switch (F.getIntrinsicID()) {
            case Intrinsic::memcpy:
            case Intrinsic::memmove:
            case Intrinsic::memset:
                if (expandMemIntrinsicUses(F))
                    Changed = true;
                break;

            default:
                break;
        }
    }

    return Changed;
}

void
aot_add_expand_memory_op_pass(LLVMPassManagerRef pass)
{
    unwrap(pass)->add(new ExpandMemoryOpPass());
}

bool
aot_check_simd_compatibility(const char *arch_c_str, const char *cpu_c_str)
{
#if WASM_ENABLE_SIMD != 0
    if (!arch_c_str || !cpu_c_str) {
        return false;
    }

    llvm::SmallVector<std::string, 1> targetAttributes;
    llvm::Triple targetTriple(arch_c_str, "", "");
    auto targetMachine =
        std::unique_ptr<llvm::TargetMachine>(llvm::EngineBuilder().selectTarget(
            targetTriple, "", std::string(cpu_c_str), targetAttributes));
    if (!targetMachine) {
        return false;
    }

    const llvm::Triple::ArchType targetArch =
        targetMachine->getTargetTriple().getArch();
    const llvm::MCSubtargetInfo *subTargetInfo =
        targetMachine->getMCSubtargetInfo();
    if (subTargetInfo == nullptr) {
        return false;
    }

    if (targetArch == llvm::Triple::x86_64) {
        return subTargetInfo->checkFeatures("+sse4.1");
    }
    else if (targetArch == llvm::Triple::aarch64) {
        return subTargetInfo->checkFeatures("+neon");
    }
    else {
        return false;
    }
#else
    (void)arch_c_str;
    (void)cpu_c_str;
    return true;
#endif /* WASM_ENABLE_SIMD */
}

void
aot_func_disable_tce(LLVMValueRef func)
{
    Function *F = unwrap<Function>(func);
    auto Attrs = F->getAttributes();

    Attrs = Attrs.addAttribute(F->getContext(), AttributeList::FunctionIndex,
                               "disable-tail-calls", "true");
    F->setAttributes(Attrs);
}

void
aot_apply_llvm_new_pass_manager(AOTCompContext *comp_ctx)
{
    Module *M = unwrap(comp_ctx->module);
    TargetMachine *TM = unwrap(comp_ctx->target_machine);

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    PipelineTuningOptions PTO;
    PTO.LoopVectorization = true;
    PTO.SLPVectorization = true;
    PTO.LoopUnrolling = true;

#if LLVM_VERSION_MAJOR == 12
    PassBuilder PB(false, TM, PTO);
#else
    PassBuilder PB(TM, PTO);
#endif

    // Register the target library analysis directly and give it a
    // customized preset TLI.
    std::unique_ptr<TargetLibraryInfoImpl> TLII(
        new TargetLibraryInfoImpl(Triple(TM->getTargetTriple())));
    FAM.registerPass([&] { return TargetLibraryAnalysis(*TLII); });

    // Register the AA manager first so that our version is the one used.
    AAManager AA = PB.buildDefaultAAPipeline();
    FAM.registerPass([&] { return std::move(AA); });

    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    ModulePassManager MPM;

    PassBuilder::OptimizationLevel OL;

    switch (comp_ctx->opt_level) {
        case 0:
            OL = PassBuilder::OptimizationLevel::O0;
            break;
        case 1:
            OL = PassBuilder::OptimizationLevel::O1;
            break;
        case 2:
            OL = PassBuilder::OptimizationLevel::O2;
            break;
        case 3:
        default:
            OL = PassBuilder::OptimizationLevel::O3;
            break;
    }

    if (comp_ctx->disable_llvm_lto) {
        uint32 i;

        for (i = 0; i < comp_ctx->func_ctx_count; i++) {
            aot_func_disable_tce(comp_ctx->func_ctxes[i]->func);
        }
    }

    if (comp_ctx->is_jit_mode) {
        /* Apply normal pipeline for JIT mode, without
           Vectorize related passes, without LTO */
        MPM.addPass(PB.buildPerModuleDefaultPipeline(OL));
    }
    else {
        FunctionPassManager FPM;

        /* Apply Vectorize related passes for AOT mode */
        FPM.addPass(LoopVectorizePass());
        FPM.addPass(SLPVectorizerPass());
        FPM.addPass(LoadStoreVectorizerPass());

        /*
        FPM.addPass(createFunctionToLoopPassAdaptor(LICMPass()));
        FPM.addPass(createFunctionToLoopPassAdaptor(LoopRotatePass()));
        FPM.addPass(createFunctionToLoopPassAdaptor(SimpleLoopUnswitchPass()));
        */

        MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));

        if (!comp_ctx->disable_llvm_lto)
            /* Apply LTO for AOT mode */
            MPM.addPass(PB.buildLTODefaultPipeline(OL, NULL));
        else
            MPM.addPass(PB.buildPerModuleDefaultPipeline(OL));
    }

    MPM.run(*M, MAM);
}
