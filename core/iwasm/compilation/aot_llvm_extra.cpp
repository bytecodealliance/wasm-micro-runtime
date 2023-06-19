/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/Error.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Initialization.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
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
#include "../aot/aot_runtime.h"
#include "aot_llvm.h"

using namespace llvm;
using namespace llvm::orc;

LLVM_C_EXTERN_C_BEGIN

bool
aot_check_simd_compatibility(const char *arch_c_str, const char *cpu_c_str);

void
aot_add_expand_memory_op_pass(LLVMPassManagerRef pass);

void
aot_add_simple_loop_unswitch_pass(LLVMPassManagerRef pass);

void
aot_apply_llvm_new_pass_manager(AOTCompContext *comp_ctx, LLVMModuleRef module);

LLVM_C_EXTERN_C_END

ExitOnError ExitOnErr;

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
    reinterpret_cast<legacy::PassManager *>(pass)->add(
        new ExpandMemoryOpPass());
}

void
aot_add_simple_loop_unswitch_pass(LLVMPassManagerRef pass)
{
    reinterpret_cast<legacy::PassManager *>(pass)->add(
        createSimpleLoopUnswitchLegacyPass());
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
aot_apply_llvm_new_pass_manager(AOTCompContext *comp_ctx, LLVMModuleRef module)
{
    TargetMachine *TM =
        reinterpret_cast<TargetMachine *>(comp_ctx->target_machine);
    PipelineTuningOptions PTO;
    PTO.LoopVectorization = true;
    PTO.SLPVectorization = true;
    PTO.LoopUnrolling = true;

#ifdef DEBUG_PASS
    PassInstrumentationCallbacks PIC;
    PassBuilder PB(TM, PTO, None, &PIC);
#else
#if LLVM_VERSION_MAJOR == 12
    PassBuilder PB(false, TM, PTO);
#else
#if WASM_ENABLE_DYNAMIC_PGO != 0
    Optional<PGOOptions> PGOOpt;
    PassBuilder PB(TM, PTO, PGOOpt);
#else
    PassBuilder PB(TM, PTO);
#endif /* WASM_ENABLE_DYNAMIC_PGO != 0 */
#endif /* LLVM_VERSION_MAJOR == 12 */
#endif

    /* Register all the basic analyses with the managers */
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    /* Register the target library analysis directly and give it a
       customized preset TLI */
    std::unique_ptr<TargetLibraryInfoImpl> TLII(
        new TargetLibraryInfoImpl(Triple(TM->getTargetTriple())));
    FAM.registerPass([&] { return TargetLibraryAnalysis(*TLII); });

    /* Register the AA manager first so that our version is the one used */
    AAManager AA = PB.buildDefaultAAPipeline();
    FAM.registerPass([&] { return std::move(AA); });

#ifdef DEBUG_PASS
    StandardInstrumentations SI(true, false);
    SI.registerCallbacks(PIC, &FAM);
#endif

    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

#if LLVM_VERSION_MAJOR <= 13
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
#else
    OptimizationLevel OL;

    switch (comp_ctx->opt_level) {
        case 0:
            OL = OptimizationLevel::O0;
            break;
        case 1:
            OL = OptimizationLevel::O1;
            break;
        case 2:
            OL = OptimizationLevel::O2;
            break;
        case 3:
        default:
            OL = OptimizationLevel::O3;
            break;
    }
#endif /* end of LLVM_VERSION_MAJOR */

    bool disable_llvm_lto = comp_ctx->disable_llvm_lto;
#if WASM_ENABLE_SPEC_TEST != 0
    disable_llvm_lto = true;
#endif

    Module *M = reinterpret_cast<Module *>(module);
    if (disable_llvm_lto) {
        for (Function &F : *M) {
            F.addFnAttr("disable-tail-calls", "true");
        }
    }

    ModulePassManager MPM;
    if (comp_ctx->is_jit_mode) {
#if WASM_ENABLE_DYNAMIC_PGO != 0
        /* FIXME: tuning default pipelines and customized pipelines */
        // MPM.addPass(PB.buildFunctionSimplificationPipeline(
        //     OL, ThinOrFullLTOPhase::None));
        MPM.addPass(PB.buildPerModuleDefaultPipeline(OL));
#else
        const char *Passes =
            "mem2reg,instcombine,simplifycfg,jump-threading,indvars";
        ExitOnErr(PB.parsePassPipeline(MPM, Passes));
#endif
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

        if (!disable_llvm_lto) {
            /* Apply LTO for AOT mode */
            if (comp_ctx->comp_data->func_count >= 10)
                /* Adds the pre-link optimizations if the func count
                   is large enough */
                MPM.addPass(PB.buildLTOPreLinkDefaultPipeline(OL));
            else
                MPM.addPass(PB.buildLTODefaultPipeline(OL, NULL));
        }
        else {
            MPM.addPass(PB.buildPerModuleDefaultPipeline(OL));
        }
    }

    MPM.run(*M, MAM);

    /*TODO: dump modification */
}

#if WASM_ENABLE_DYNAMIC_PGO != 0
/*TODO: maybe move below to a new file? */
/* return true if broken */
bool
verify_module_and_debug_info(LLVMModuleRef module)
{
    bool broken_debug_info = false;
    Module *M = reinterpret_cast<Module *>(module);
    bool broken_module = verifyModule(*M, &dbgs(), &broken_debug_info);

    if (broken_debug_info || broken_module)
        LOG_ERROR("broken_module=%d, broken_debug_info=%d", broken_module,
                  broken_debug_info);

    return broken_debug_info || broken_module;
}

/* append !{!"function_entry_count", i64 NN} */
void
wasm_dpgo_set_function_entry_count(LLVMValueRef function, uint32 count)
{
    Function *F = unwrap<Function>(function);
    if (F->hasProfileData()) {
        uint64 current_count = F->getEntryCount().getValue().getCount();
        if (current_count == count)
            return;
    }

    F->setEntryCount(count, Function::PCT_Real);
    F->addFnAttr(Attribute::Hot);
}

/* append !{!"branch_weights", i32 NN, i32 MM} */
void
wasm_dpgo_set_branch_weights(LLVMContextRef context, LLVMValueRef instruction,
                             uint32 *counts, uint32 counts_size)
{
    bh_assert(LLVMGetValueKind(instruction) == LLVMInstructionValueKind);
    MDBuilder MDB(*reinterpret_cast<LLVMContext *>(context));
    Instruction *I = unwrap<Instruction>(instruction);

    // FIXME: add more counters when meeting switch
    LOG_DEBUG("%s --> branch_weights {%u, %u}",
              LLVMPrintValueToString(instruction), counts[0], counts[1]);

    if (counts_size == 2) {
        I->setMetadata(LLVMContext::MD_prof,
                       MDB.createBranchWeights(counts[0], counts[1]));
    }
    else {
        SmallVector<unsigned, 4> Weights;
        for (unsigned i = 0; i < counts_size; i++) {
            Weights.push_back(counts[i]);
        }
        I->setMetadata(LLVMContext::MD_prof, MDB.createBranchWeights(Weights));
    }
}

void
wasm_dpgo_unlike_true_branch(LLVMContextRef context, LLVMValueRef cond_br)
{
    bh_assert(LLVMIsABranchInst(cond_br));
    bh_assert(LLVMIsConditional(cond_br));

    uint32 weights[2] = { 1, 1000 };
    return wasm_dpgo_set_branch_weights(context, cond_br, weights, 2);
}

void
wasm_dpgo_unlike_false_branch(LLVMContextRef context, LLVMValueRef cond_br)
{
    bh_assert(LLVMIsABranchInst(cond_br));
    bh_assert(LLVMIsConditional(cond_br));

    uint32 weights[2] = { 1000, 1 };
    return wasm_dpgo_set_branch_weights(context, cond_br, weights, 2);
}

/* append !{!"loop_header_weight", i64 NN} */
void
wasm_dpgo_set_irr_loop(LLVMContextRef context, LLVMValueRef instruction,
                       uint32 count)
{
    MDBuilder MDB(*reinterpret_cast<LLVMContext *>(context));

    bh_assert(LLVMGetValueKind(instruction) == LLVMInstructionValueKind);
    Instruction *I = unwrap<Instruction>(instruction);
    I->setMetadata(llvm::LLVMContext::MD_irr_loop,
                   MDB.createIrrLoopHeaderWeight(count));
}

void
wasm_dpgo_set_vp(LLVMModuleRef module, LLVMValueRef instruction, uint32 *counts,
                 uint32 counts_size)
{
    /*
    TODO:
    using
    void annotateValueSite(Module &M, Instruction &Inst,
                           const InstrProfRecord &InstrProfR,
                           InstrProfValueKind ValueKind, uint32 SiteIndx,
                           uint32 MaxMDCount = 3);

    Valuekind is 0(IPVK_IndirectCallTarget),
    MaxMDCount needs to be defined
    */
}

static bool
wasm_dpgo_get_offset_of_instr(WASMModule *wasm_module, LLVMValueRef instruction,
                              uint32 *line)
{
    /* always have some Debug Location */
    LLVMMetadataRef dbg_location = LLVMInstructionGetDebugLoc(instruction);
    bh_assert(dbg_location && "No Debug Location");
    bh_assert(LLVMGetMetadataKind(dbg_location) == LLVMDILocationMetadataKind
              && "Not a DILocation");
    if (!dbg_location)
        return false;

    *line = LLVMDILocationGetLine(dbg_location);
    LOG_DEBUG("  DILocation. LINE: %u", *line);
    return true;
}

static void
wasm_dpgo_visit_select(WASMModule *wasm_module, LLVMModuleRef module,
                       LLVMValueRef function, uint32 func_idx)
{
    for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function);
         basic_block != NULL;
         basic_block = LLVMGetNextBasicBlock(basic_block)) {
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block);
             instruction != NULL;
             instruction = LLVMGetNextInstruction(instruction)) {

            if (!LLVMIsASelectInst(instruction))
                continue;

            LOG_DEBUG("Select : %s  .", LLVMPrintValueToString(instruction));

            /* locate profiling counters */
            uint32 line;
            bool ret =
                wasm_dpgo_get_offset_of_instr(wasm_module, instruction, &line);
            if (!ret)
                return;

            uint32 weights[2] = { 0 };
            ret = wasm_dpgo_get_select_counts(wasm_module, func_idx, line,
                                              &weights[0], &weights[1]);
            bh_assert(ret && "Failed to get profiling counters for select");
            if (!ret)
                return;

            wasm_dpgo_set_branch_weights(LLVMGetModuleContext(module),
                                         instruction, weights, 2);
        }
    }
}

static void
wasm_dpgo_visit_terminator(WASMModule *wasm_module, LLVMModuleRef module,
                           LLVMValueRef function, uint32 func_idx)
{
    LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function);
    while (basic_block) {
        LLVMValueRef terminator = LLVMGetBasicBlockTerminator(basic_block);
        basic_block = LLVMGetNextBasicBlock(basic_block);

        if (LLVMGetNumSuccessors(terminator) <= 1)
            continue;

        LOG_DEBUG("Terminator: %s  .", LLVMPrintValueToString(terminator));

        /* only take care condbr and switch */
        if (!LLVMIsABranchInst(terminator) && !LLVMIsASwitchInst(terminator))
            continue;

        /* bypass non-CondBr */
        if (LLVMIsABranchInst(terminator) && !LLVMIsConditional(terminator))
            continue;

        LLVMValueRef prof_md =
            LLVMGetMetadata(terminator, LLVMContext::MD_prof);
        if (prof_md) {
            LOG_DEBUG("  Already has Prof meta");
            continue;
        }

        /* locate profiling counters */
        uint32 line;
        bool ret =
            wasm_dpgo_get_offset_of_instr(wasm_module, terminator, &line);
        if (!ret)
            return;

        if (LLVMIsABranchInst(terminator)) {
            uint32 weights[2] = { 0 };
            ret = wasm_dpgo_get_cond_br_counts(wasm_module, func_idx, line,
                                               &weights[0], &weights[1]);
            bh_assert(ret && "Failed to get profiling counters for condbr");
            if (!ret)
                return;

            wasm_dpgo_set_branch_weights(LLVMGetModuleContext(module),
                                         terminator, weights, 2);
            continue;
        }

        // FIXME: implement switch
    }
}

void
wasm_dpgo_set_prof_meta(WASMModule *wasm_module, LLVMModuleRef module,
                        LLVMValueRef function, uint32 func_idx)
{
    bh_assert(LLVMGetValueKind(function) == LLVMFunctionValueKind);
    bh_assert(func_idx < wasm_module->import_function_count
                             + wasm_module->function_count);

    size_t size;
    LOG_DEBUG("  --> Add prof meta into %s",
              LLVMGetValueName2(function, &size));

    wasm_dpgo_visit_terminator(wasm_module, module, function, func_idx);
    wasm_dpgo_visit_select(wasm_module, module, function, func_idx);
}
#endif /* WASM_ENABLE_DYNAMIC_PGO != 0 */
