#pragma once

#include <cstdint>
#include <lib_export.h>

#define REG_NATIVE_FUNC(func_name, signature)  \
    { #func_name, (void*) func_name##_wrapper, signature, nullptr }

/*
 * -- WAMR native signatures --
 *
 * When defining WAMR native functions you have to specify the function
 * signature. This uses the following scheme, where capitals mean a 64-bit
 * version:
 *
 * - $ = string
 * - * = pointer
 * - F,f = float
 * - I,i = integer
 *
 * For example:
 *
 * int32_t myFunc(int32_t i, char* s) = "(i$)i"
 * int32_t myBigIntFunc(int64_t i, char* s) = "(I$)i"
 * void fooBar(*int32_t i, char* s, float32_t f) = "(*$f)"
 * void nothing() = "()"
 */

namespace wasm {
    void initialiseWAMRNatives();

    uint32_t getFaasmDynlinkApi(NativeSymbol **nativeSymbols);

    uint32_t getFaasmFilesystemApi(NativeSymbol **nativeSymbols);

    uint32_t getFaasmFunctionsApi(NativeSymbol **nativeSymbols);

    uint32_t getFaasmPthreadApi(NativeSymbol **nativeSymbols);

    uint32_t getFaasmStateApi(NativeSymbol **nativeSymbols);

    uint32_t getFaasmStubs(NativeSymbol **nativeSymbols);
}