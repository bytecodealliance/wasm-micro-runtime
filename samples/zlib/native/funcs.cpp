#include <wamr/native.h>
#include <wasm_export.h>
#include <proto/faabric.pb.h>
#include <wasm/WasmModule.h>
#include <faabric/util/bytes.h>


namespace wasm {
    /**
     * Returns the index of the current function (this is zero unless it's a
     * function chained from within the same module).
     */
    static int32_t __faasm_get_idx_wrapper(wasm_exec_env_t exec_env) {
        faabric::util::getLogger()->debug("S - faasm_get_idx");

        faabric::Message *call = getExecutingCall();
        int idx = call->idx();
        return idx;
    }

    /**
     * Read the function input
     */
    static int32_t __faasm_read_input_wrapper(wasm_exec_env_t exec_env, char* inBuff, int32_t inLen) {
        faabric::util::getLogger()->debug("S - faasm_read_input {} {}", inBuff, inLen);

        faabric::Message *call = getExecutingCall();
        std::vector<uint8_t> inputBytes = faabric::util::stringToBytes(call->inputdata());

        // If nothing, return nothing
        if (inputBytes.empty()) {
            return 0;
        }

        // Write to the wasm buffer
        int inputSize = faabric::util::safeCopyToBuffer(inputBytes, reinterpret_cast<uint8_t *>(inBuff), inLen);
        return inputSize;
    }

    /**
     * Set the function output
     */
    static void __faasm_write_output_wrapper(wasm_exec_env_t exec_env, char* outBuff, int32_t outLen) {
        faabric::util::getLogger()->debug("S - faasm_write_output {} {}", outBuff, outLen);

        faabric::Message *call = getExecutingCall();
        call->set_outputdata(outBuff, outLen);
    }

    static NativeSymbol ns[] = {
            REG_NATIVE_FUNC(__faasm_get_idx, "()i"),
            REG_NATIVE_FUNC(__faasm_write_output, "($i)"),
            REG_NATIVE_FUNC(__faasm_read_input, "($i)i"),
    };

    uint32_t getFaasmFunctionsApi(NativeSymbol **nativeSymbols) {
        *nativeSymbols = ns;
        return sizeof(ns) / sizeof(NativeSymbol);
    }
}
