/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef AOT_LLVM_LAZYJIT_H
#define AOT_LLVM_LAZYJIT_H

#include "llvm-c/Error.h"
#include "llvm-c/Orc.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"
#if LLVM_VERSION_MAJOR >= 12
#include "llvm-c/LLJIT.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef LLVMOrcLLJITBuilderRef LLVMOrcLLLazyJITBuilderRef;

typedef LLVMOrcLLJITRef LLVMOrcLLLazyJITRef;

LLVMOrcJITTargetMachineBuilderRef
LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(LLVMTargetMachineRef TM);

LLVMOrcLLLazyJITBuilderRef
LLVMOrcCreateLLLazyJITBuilder(void);

void
LLVMOrcDisposeLLLazyJITBuilder(LLVMOrcLLLazyJITBuilderRef Builder);

LLVMErrorRef
LLVMOrcCreateLLLazyJIT(LLVMOrcLLLazyJITRef *Result,
                       LLVMOrcLLLazyJITBuilderRef Builder);

LLVMErrorRef
LLVMOrcDisposeLLLazyJIT(LLVMOrcLLLazyJITRef J);

LLVMOrcJITDylibRef
LLVMOrcLLLazyJITGetMainJITDylib(LLVMOrcLLLazyJITRef J);

const char *
LLVMOrcLLLazyJITGetTripleString(LLVMOrcLLLazyJITRef J);

char
LLVMOrcLLLazyJITGetGlobalPrefix(LLVMOrcLLLazyJITRef J);

LLVMErrorRef
LLVMOrcLLLazyJITAddLLVMIRModule(LLVMOrcLLLazyJITRef J,
                                LLVMOrcJITDylibRef JD,
                                LLVMOrcThreadSafeModuleRef TSM);

LLVMErrorRef
LLVMOrcLLLazyJITLookup(LLVMOrcLLLazyJITRef J,
                       LLVMOrcJITTargetAddress *Result,
                       const char *Name);

const char *
LLVMOrcLLLazyJITGetTripleString(LLVMOrcLLLazyJITRef J);

void
LLVMOrcLLLazyJITBuilderSetJITTargetMachineBuilder(
                        LLVMOrcLLLazyJITBuilderRef Builder,
                        LLVMOrcJITTargetMachineBuilderRef JTMB);

char
LLVMOrcLLLazyJITGetGlobalPrefix(LLVMOrcLLLazyJITRef J);

#ifdef __cplusplus
}
#endif

#endif /* end of AOT_LLVM_LAZYJIT_H */

