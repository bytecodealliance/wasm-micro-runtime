#include <wamr/native.h>
#include <wasm_export.h>
#include <stdexcept>


namespace wasm {
    static int32_t __wasi_fd_dup_wrapper(wasm_exec_env_t exec_env, int32_t a) {
        throw std::runtime_error("Native WASI fd dup not implemented");
    }

    static NativeSymbol ns[] = {
            REG_NATIVE_FUNC(__wasi_fd_dup, "(i*)i"),
    };

    uint32_t getFaasmFilesystemApi(NativeSymbol **nativeSymbols) {
        *nativeSymbols = ns;
        return sizeof(ns) / sizeof(NativeSymbol);
    }
}
