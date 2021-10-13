/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_llvm_lazyjit.h"

LLVMOrcJITTargetMachineBuilderRef
LLVMOrcJITTargetMachineBuilderFromTargetMachine(LLVMTargetMachineRef TM);

LLVMOrcLLJITBuilderRef
LLVMOrcCreateLLJITBuilder(void);

void
LLVMOrcDisposeLLJITBuilder(LLVMOrcLLJITBuilderRef Builder);

LLVMErrorRef
LLVMOrcCreateLLJIT(LLVMOrcLLJITRef *Result, LLVMOrcLLJITBuilderRef Builder);

LLVMErrorRef
LLVMOrcDisposeLLJIT(LLVMOrcLLJITRef J);

LLVMOrcJITDylibRef
LLVMOrcLLJITGetMainJITDylib(LLVMOrcLLJITRef J);

const char *
LLVMOrcLLJITGetTripleString(LLVMOrcLLJITRef J);

char
LLVMOrcLLJITGetGlobalPrefix(LLVMOrcLLJITRef J);

LLVMErrorRef
LLVMOrcLLJITAddLLVMIRModule(LLVMOrcLLJITRef J, LLVMOrcJITDylibRef JD,
                            LLVMOrcThreadSafeModuleRef TSM);

LLVMErrorRef
LLVMOrcLLJITLookup(LLVMOrcLLJITRef J, LLVMOrcJITTargetAddress *Result,
                   const char *Name);

const char *
LLVMOrcLLJITGetTripleString(LLVMOrcLLJITRef J);

void
LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(
    LLVMOrcLLJITBuilderRef Builder, LLVMOrcJITTargetMachineBuilderRef JTMB);

char
LLVMOrcLLJITGetGlobalPrefix(LLVMOrcLLJITRef J);

#if LLVM_VERSION_MAJOR < 12
LLVMOrcJITTargetMachineBuilderRef
LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(LLVMTargetMachineRef TM)
{
    return LLVMOrcJITTargetMachineBuilderFromTargetMachine(TM);
}
#endif

LLVMOrcJITDylibRef
LLVMOrcLLLazyJITGetMainJITDylib(LLVMOrcLLLazyJITRef J)
{
    return LLVMOrcLLJITGetMainJITDylib(J);
}

LLVMOrcLLLazyJITBuilderRef
LLVMOrcCreateLLLazyJITBuilder(void)
{
    return LLVMOrcCreateLLJITBuilder();
}

void
LLVMOrcDisposeLLLazyJITBuilder(LLVMOrcLLLazyJITBuilderRef Builder)
{
    return LLVMOrcDisposeLLJITBuilder(Builder);
}

LLVMErrorRef
LLVMOrcCreateLLLazyJIT(LLVMOrcLLLazyJITRef *Result,
                       LLVMOrcLLLazyJITBuilderRef Builder)
{
    return LLVMOrcCreateLLJIT(Result, Builder);
}

LLVMErrorRef
LLVMOrcDisposeLLLazyJIT(LLVMOrcLLLazyJITRef J)
{
    return LLVMOrcDisposeLLJIT(J);
}

LLVMErrorRef
LLVMOrcLLLazyJITAddLLVMIRModule(LLVMOrcLLLazyJITRef J, LLVMOrcJITDylibRef JD,
                                LLVMOrcThreadSafeModuleRef TSM)
{
    return LLVMOrcLLJITAddLLVMIRModule(J, JD, TSM);
}

LLVMErrorRef
LLVMOrcLLLazyJITLookup(LLVMOrcLLLazyJITRef J, LLVMOrcJITTargetAddress *Result,
                       const char *Name)
{
    return LLVMOrcLLJITLookup(J, Result, Name);
}

const char *
LLVMOrcLLLazyJITGetTripleString(LLVMOrcLLLazyJITRef J)
{
    return LLVMOrcLLJITGetTripleString(J);
}

void
LLVMOrcLLLazyJITBuilderSetJITTargetMachineBuilder(
    LLVMOrcLLLazyJITBuilderRef Builder, LLVMOrcJITTargetMachineBuilderRef JTMB)
{
    return LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(Builder, JTMB);
}

char
LLVMOrcLLLazyJITGetGlobalPrefix(LLVMOrcLLLazyJITRef J)
{
    return LLVMOrcLLJITGetGlobalPrefix(J);
}
