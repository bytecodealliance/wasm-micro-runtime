/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_ORC_LAZINESS_H_
#define _AOT_ORC_LAZINESS_H_

#include "llvm-c/Error.h"
#include "llvm-c/ExternC.h"
#include "llvm-c/LLJIT.h"
#include "llvm-c/Orc.h"
#include "llvm-c/Types.h"

LLVM_C_EXTERN_C_BEGIN

typedef struct LLVMOrcOpaqueLLLazyJITBuilder *LLVMOrcLLLazyJITBuilderRef;
typedef struct LLVMOrcOpaqueLLLazyJIT *LLVMOrcLLLazyJITRef;

void
LLVMOrcLLJITBuilderSetNumCompileThreads(LLVMOrcLLJITBuilderRef Builder,
                                        unsigned NumCompileThreads);

/**
 * Create an LLVMOrcLLJITBuilder.
 *
 * The client owns the resulting LLJITBuilder and should dispose of it using
 * LLVMOrcDisposeLLJITBuilder once they are done with it.
 */
LLVMOrcLLLazyJITBuilderRef
LLVMOrcCreateLLLazyJITBuilder(void);

/**
 * Dispose of an LLVMOrcLLJITBuilderRef. This should only be called if ownership
 * has not been passed to LLVMOrcCreateLLJIT (e.g. because some error prevented
 * that function from being called).
 */
void
LLVMOrcDisposeLLLazyJITBuilder(LLVMOrcLLLazyJITBuilderRef Builder);

void
LLVMOrcLLLazyJITBuilderSetNumCompileThreads(LLVMOrcLLLazyJITBuilderRef Builder,
                                            unsigned NumCompileThreads);

/**
 * Create an LLJIT instance from an LLJITBuilder.
 *
 * This operation takes ownership of the Builder argument: clients should not
 * dispose of the builder after calling this function (even if the function
 * returns an error). If a null Builder argument is provided then a
 * default-constructed LLJITBuilder will be used.
 *
 * On success the resulting LLJIT instance is uniquely owned by the client and
 * automatically manages the memory of all JIT'd code and all modules that are
 * transferred to it (e.g. via LLVMOrcLLJITAddLLVMIRModule). Disposing of the
 * LLJIT instance will free all memory managed by the JIT, including JIT'd code
 * and not-yet compiled modules.
 */
LLVMErrorRef
LLVMOrcCreateLLLazyJIT(LLVMOrcLLLazyJITRef *Result,
                       LLVMOrcLLLazyJITBuilderRef Builder);

/**
 * Dispose of an LLJIT instance.
 */
LLVMErrorRef
LLVMOrcDisposeLLLazyJIT(LLVMOrcLLLazyJITRef J);

/**
 * Add an IR module to the given JITDylib in the given LLJIT instance. This
 * operation transfers ownership of the TSM argument to the LLJIT instance.
 * The TSM argument should not be disposed of or referenced once this
 * function returns.
 *
 * Resources associated with the given Module will be tracked by the given
 * JITDylib's default resource tracker.
 */
LLVMErrorRef
LLVMOrcLLLazyJITAddLLVMIRModule(LLVMOrcLLLazyJITRef J, LLVMOrcJITDylibRef JD,
                                LLVMOrcThreadSafeModuleRef TSM);

/**
 * Look up the given symbol in the main JITDylib of the given LLJIT instance.
 *
 * This operation does not take ownership of the Name argument.
 */
LLVMErrorRef
LLVMOrcLLLazyJITLookup(LLVMOrcLLLazyJITRef J, LLVMOrcExecutorAddress *Result,
                       const char *Name);

/**
 * Mangles the given string according to the LLJIT instance's DataLayout, then
 * interns the result in the SymbolStringPool and returns a reference to the
 * pool entry. Clients should call LLVMOrcReleaseSymbolStringPoolEntry to
 * decrement the ref-count on the pool entry once they are finished with this
 * value.
 */
LLVMOrcSymbolStringPoolEntryRef
LLVMOrcLLLazyJITMangleAndIntern(LLVMOrcLLLazyJITRef J,
                                const char *UnmangledName);

/**
 * Return a reference to the Main JITDylib.
 *
 * The JITDylib is owned by the LLJIT instance. The client is not responsible
 * for managing its memory.
 */
LLVMOrcJITDylibRef
LLVMOrcLLLazyJITGetMainJITDylib(LLVMOrcLLLazyJITRef J);

/**
 * Return the target triple for this LLJIT instance. This string is owned by
 * the LLJIT instance and should not be freed by the client.
 */
const char *
LLVMOrcLLLazyJITGetTripleString(LLVMOrcLLLazyJITRef J);

/**
 * Get a reference to the ExecutionSession for this LLJIT instance.
 *
 * The ExecutionSession is owned by the LLJIT instance. The client is not
 * responsible for managing its memory.
 */
LLVMOrcExecutionSessionRef
LLVMOrcLLLazyJITGetExecutionSession(LLVMOrcLLLazyJITRef J);

/**
 * Returns a non-owning reference to the LLJIT instance's IR transform layer.
 */
LLVMOrcIRTransformLayerRef
LLVMOrcLLLazyJITGetIRTransformLayer(LLVMOrcLLLazyJITRef J);

LLVM_C_EXTERN_C_END
#endif
