#include <wamr/native.h>
#include <wasm_export.h>
#include <stdexcept>
#include <faabric/util/logging.h>

namespace wasm {
    // ------------------------------------------
    // NOTE: All of these functions are just stubs to get things to run. We do not
    // expect them to be called, so throw errors when they are.
    // ------------------------------------------

    static int32_t syscall_wrapper(wasm_exec_env_t exec_env, int32_t syscallNo, int32_t syscallArgs) {
        switch (syscallNo) {
            case 224:
                // We only support gettid here
                faabric::util::getLogger()->warn("Using syscall to call gettid (syscall no. {})", syscallNo);
                return 0;
            default:
                throw std::runtime_error("Native syscall not implemented");
        }
    }

    static int32_t __cxa_allocate_exception_wrapper(wasm_exec_env_t exec_env, int32_t a) {
        throw std::runtime_error("Native __cxa_allocate_exception not implemented");
    }

    static void __cxa_throw_wrapper(wasm_exec_env_t exec_env, int32_t a, int32_t b, int32_t c) {
        throw std::runtime_error("Native __cxa_throw not implemented");
    }

    static int32_t shm_open_wrapper(wasm_exec_env_t exec_env, char* a, int32_t b, int32_t c) {
        throw std::runtime_error("Native shm_open not implemented");
    }

    static NativeSymbol ns[] = {
            REG_NATIVE_FUNC(__cxa_allocate_exception, "(i)i"),
            REG_NATIVE_FUNC(__cxa_throw, "(iii)"),
            REG_NATIVE_FUNC(shm_open, "($ii)i"),
            REG_NATIVE_FUNC(syscall, "(ii)i"),
    };

    uint32_t getFaasmStubs(NativeSymbol **nativeSymbols) {
        *nativeSymbols = ns;
        return sizeof(ns) / sizeof(NativeSymbol);
    }
}
