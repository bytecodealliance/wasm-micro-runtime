/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Twine.h>
#include <llvm/ADT/Triple.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Target/TargetOptions.h>
#include <cstring>

using namespace llvm;

extern "C" LLVMBool
WAMRCreateMCJITCompilerForModule(LLVMExecutionEngineRef *OutJIT,
                                 LLVMModuleRef M,
                                 LLVMMCJITCompilerOptions *PassedOptions,
                                 size_t SizeOfPassedOptions,
                                 char **OutError);

extern "C" bool
aot_check_simd_compatibility(const char *arch_c_str, const char *cpu_c_str);

LLVMBool
WAMRCreateMCJITCompilerForModule(LLVMExecutionEngineRef *OutJIT,
                                 LLVMModuleRef M,
                                 LLVMMCJITCompilerOptions *PassedOptions,
                                 size_t SizeOfPassedOptions,
                                 char **OutError)
{
    LLVMMCJITCompilerOptions options;
    // If the user passed a larger sized options struct, then they were compiled
    // against a newer LLVM. Tell them that something is wrong.
    if (SizeOfPassedOptions > sizeof(options)) {
        *OutError = strdup(
                "Refusing to use options struct that is larger than my own; assuming "
                "LLVM library mismatch.");
        return 1;
    }

    // Defend against the user having an old version of the API by ensuring that
    // any fields they didn't see are cleared. We must defend against fields being
    // set to the bitwise equivalent of zero, and assume that this means "do the
    // default" as if that option hadn't been available.
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
            Attrs = Attrs.addAttribute(F.getContext(), AttributeList::FunctionIndex,
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

bool
aot_check_simd_compatibility(const char *arch_c_str, const char *cpu_c_str)
{
#if WASM_ENABLE_SIMD != 0
    if (!arch_c_str || !cpu_c_str) {
        return false;
    }

    llvm::SmallVector<std::string, 1> targetAttributes;
    llvm::Triple targetTriple(arch_c_str, "", "");
    llvm::TargetMachine *targetMachine = llvm::EngineBuilder().selectTarget(
      targetTriple, "", std::string(cpu_c_str), targetAttributes);
    if (targetMachine == nullptr) {
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

