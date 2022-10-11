/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "llvm-c/LLJIT.h"
#include "llvm-c/Orc.h"
#include "llvm-c/OrcEE.h"
#include "llvm-c/TargetMachine.h"

#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/CBindingWrapping.h"

#include "aot_orc_extra.h"

using namespace llvm;
using namespace llvm::orc;

namespace llvm {
namespace orc {

class InProgressLookupState;

class OrcV2CAPIHelper
{
  public:
    using PoolEntry = SymbolStringPtr::PoolEntry;
    using PoolEntryPtr = SymbolStringPtr::PoolEntryPtr;

    // Move from SymbolStringPtr to PoolEntryPtr (no change in ref count).
    static PoolEntryPtr moveFromSymbolStringPtr(SymbolStringPtr S)
    {
        PoolEntryPtr Result = nullptr;
        std::swap(Result, S.S);
        return Result;
    }

    // Move from a PoolEntryPtr to a SymbolStringPtr (no change in ref count).
    static SymbolStringPtr moveToSymbolStringPtr(PoolEntryPtr P)
    {
        SymbolStringPtr S;
        S.S = P;
        return S;
    }

    // Copy a pool entry to a SymbolStringPtr (increments ref count).
    static SymbolStringPtr copyToSymbolStringPtr(PoolEntryPtr P)
    {
        return SymbolStringPtr(P);
    }

    static PoolEntryPtr getRawPoolEntryPtr(const SymbolStringPtr &S)
    {
        return S.S;
    }

    static void retainPoolEntry(PoolEntryPtr P)
    {
        SymbolStringPtr S(P);
        S.S = nullptr;
    }

    static void releasePoolEntry(PoolEntryPtr P)
    {
        SymbolStringPtr S;
        S.S = P;
    }

    static InProgressLookupState *extractLookupState(LookupState &LS)
    {
        return LS.IPLS.release();
    }

    static void resetLookupState(LookupState &LS, InProgressLookupState *IPLS)
    {
        return LS.reset(IPLS);
    }
};

} // namespace orc
} // namespace llvm

// ORC.h
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(ExecutionSession, LLVMOrcExecutionSessionRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(IRTransformLayer, LLVMOrcIRTransformLayerRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(JITDylib, LLVMOrcJITDylibRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(JITTargetMachineBuilder,
                                   LLVMOrcJITTargetMachineBuilderRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(ObjectTransformLayer,
                                   LLVMOrcObjectTransformLayerRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(OrcV2CAPIHelper::PoolEntry,
                                   LLVMOrcSymbolStringPoolEntryRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(SymbolStringPool, LLVMOrcSymbolStringPoolRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(ThreadSafeModule, LLVMOrcThreadSafeModuleRef)

// LLJIT.h
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(LLJITBuilder, LLVMOrcLLJITBuilderRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(LLLazyJITBuilder, LLVMOrcLLLazyJITBuilderRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(LLLazyJIT, LLVMOrcLLLazyJITRef)

void
LLVMOrcLLJITBuilderSetNumCompileThreads(LLVMOrcLLJITBuilderRef Builder,
                                        unsigned NumCompileThreads)
{
    unwrap(Builder)->setNumCompileThreads(NumCompileThreads);
}

LLVMOrcLLLazyJITBuilderRef
LLVMOrcCreateLLLazyJITBuilder(void)
{
    return wrap(new LLLazyJITBuilder());
}

void
LLVMOrcDisposeLLLazyJITBuilder(LLVMOrcLLLazyJITBuilderRef Builder)
{
    delete unwrap(Builder);
}

void
LLVMOrcLLLazyJITBuilderSetNumCompileThreads(LLVMOrcLLLazyJITBuilderRef Builder,
                                            unsigned NumCompileThreads)
{
    unwrap(Builder)->setNumCompileThreads(NumCompileThreads);
}

void
LLVMOrcLLLazyJITBuilderSetJITTargetMachineBuilder(
    LLVMOrcLLLazyJITBuilderRef Builder, LLVMOrcJITTargetMachineBuilderRef JTMP)
{
    unwrap(Builder)->setJITTargetMachineBuilder(*unwrap(JTMP));
}

LLVMErrorRef
LLVMOrcCreateLLLazyJIT(LLVMOrcLLLazyJITRef *Result,
                       LLVMOrcLLLazyJITBuilderRef Builder)
{
    assert(Result && "Result can not be null");

    if (!Builder)
        Builder = LLVMOrcCreateLLLazyJITBuilder();

    auto J = unwrap(Builder)->create();
    LLVMOrcDisposeLLLazyJITBuilder(Builder);

    if (!J) {
        Result = nullptr;
        return wrap(J.takeError());
    }

    *Result = wrap(J->release());
    return LLVMErrorSuccess;
}

LLVMErrorRef
LLVMOrcDisposeLLLazyJIT(LLVMOrcLLLazyJITRef J)
{
    delete unwrap(J);
    return LLVMErrorSuccess;
}

LLVMErrorRef
LLVMOrcLLLazyJITAddLLVMIRModule(LLVMOrcLLLazyJITRef J, LLVMOrcJITDylibRef JD,
                                LLVMOrcThreadSafeModuleRef TSM)
{
    std::unique_ptr<ThreadSafeModule> TmpTSM(unwrap(TSM));
    return wrap(unwrap(J)->addLazyIRModule(*unwrap(JD), std::move(*TmpTSM)));
}

LLVMErrorRef
LLVMOrcLLLazyJITLookup(LLVMOrcLLLazyJITRef J, LLVMOrcExecutorAddress *Result,
                       const char *Name)
{
    assert(Result && "Result can not be null");

    auto Sym = unwrap(J)->lookup(Name);
    if (!Sym) {
        *Result = 0;
        return wrap(Sym.takeError());
    }

    *Result = Sym->getAddress();
    return LLVMErrorSuccess;
}

LLVMOrcSymbolStringPoolEntryRef
LLVMOrcLLLazyJITMangleAndIntern(LLVMOrcLLLazyJITRef J,
                                const char *UnmangledName)
{
    return wrap(OrcV2CAPIHelper::moveFromSymbolStringPtr(
        unwrap(J)->mangleAndIntern(UnmangledName)));
}

LLVMOrcJITDylibRef
LLVMOrcLLLazyJITGetMainJITDylib(LLVMOrcLLLazyJITRef J)
{
    return wrap(&unwrap(J)->getMainJITDylib());
}

const char *
LLVMOrcLLLazyJITGetTripleString(LLVMOrcLLLazyJITRef J)
{
    return unwrap(J)->getTargetTriple().str().c_str();
}

LLVMOrcExecutionSessionRef
LLVMOrcLLLazyJITGetExecutionSession(LLVMOrcLLLazyJITRef J)
{
    return wrap(&unwrap(J)->getExecutionSession());
}

LLVMOrcIRTransformLayerRef
LLVMOrcLLLazyJITGetIRTransformLayer(LLVMOrcLLLazyJITRef J)
{
    return wrap(&unwrap(J)->getIRTransformLayer());
}

LLVMOrcObjectTransformLayerRef
LLVMOrcLLLazyJITGetObjTransformLayer(LLVMOrcLLLazyJITRef J)
{
    return wrap(&unwrap(J)->getObjTransformLayer());
}