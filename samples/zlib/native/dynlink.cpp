#include <wamr/native.h>
#include <wasm_export.h>
#include <stdexcept>

namespace wasm {
    // TODO - implement dynamic linking with WAMR

    static int32_t dlopen_wrapper(wasm_exec_env_t exec_env, char* filename, int32_t flags) {
        throw std::runtime_error("Native dlopen not implemented");
    }

    static int32_t dlsym_wrapper(wasm_exec_env_t exec_env, void* handle, char* symbol) {
        throw std::runtime_error("Native dlsym not implemented");
    }

    static int32_t dlclose_wrapper(wasm_exec_env_t exec_env, void* handle) {
        throw std::runtime_error("Native dlclose not implemented");
    }

    static NativeSymbol ns[] = {
            REG_NATIVE_FUNC(dlopen, "($i)i"),
            REG_NATIVE_FUNC(dlsym, "(*$)i"),
            REG_NATIVE_FUNC(dlclose, "(*)i"),
    };

    uint32_t getFaasmDynlinkApi(NativeSymbol **nativeSymbols) {
        *nativeSymbols = ns;
        return sizeof(ns) / sizeof(NativeSymbol);
    }
}
