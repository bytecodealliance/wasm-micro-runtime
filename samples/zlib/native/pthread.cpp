#include <wamr/native.h>
#include <wasm_export.h>


namespace wasm {
    // ------------------------------------------
    // NOTE: these functions are just here to stub out bits that are missing
    // from the existing WAMR pthread implementation. This can be found here:
    // https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/core/iwasm/libraries/lib-pthread
    // ------------------------------------------
    static int32_t pthread_cond_broadcast_wrapper(wasm_exec_env_t exec_env, int32_t a) {
        return 0;
    }

    static int32_t pthread_equal_wrapper(wasm_exec_env_t exec_env, int32_t a, int32_t b) {
        return 0;
    }

    static NativeSymbol ns[] = {
            REG_NATIVE_FUNC(pthread_cond_broadcast, "(i)i"),
            REG_NATIVE_FUNC(pthread_equal, "(ii)i"),
    };

    uint32_t getFaasmPthreadApi(NativeSymbol **nativeSymbols) {
        *nativeSymbols = ns;
        return sizeof(ns) / sizeof(NativeSymbol);
    }
}
