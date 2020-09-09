#include "WAMRWasmModule.h"

#include <faabric/util/files.h>
#include <stdexcept>
#include <faabric/util/logging.h>

#include <wasm_export.h>
#include <aot_export.h>

#include <boost/filesystem.hpp>


namespace wasm {
    std::vector<uint8_t> wamrCodegen(std::vector<uint8_t> &wasmBytes) {
        const std::shared_ptr<spdlog::logger> &logger = faabric::util::getLogger();

        // Make sure WAMR is initialised
        WAMRWasmModule::initialiseWAMRGlobally();

        if (!faabric::util::isWasm(wasmBytes)) {
            throw std::runtime_error("File is not a wasm binary");
        }

        // Load the module
        char errorBuffer[128];
        wasm_module_t wasmModule = wasm_runtime_load(
                wasmBytes.data(), wasmBytes.size(),
                errorBuffer, sizeof(errorBuffer)
        );

        if (wasmModule == nullptr) {
            logger->error("Failed to import module: {}", errorBuffer);
            throw std::runtime_error("Failed to load module");
        }

        aot_comp_data_t compileData = aot_create_comp_data(wasmModule);
        if(compileData == nullptr) {
            logger->error("Failed to generat AOT data: {}", aot_get_last_error());
            throw std::runtime_error("Failed to generate AOT data");
        }

        AOTCompOption option = { 0 };
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;

        aot_comp_context_t compileContext =  aot_create_comp_context(compileData, &option);
            
        if(compileContext == nullptr) {
            logger->error("Failed to generat AOT context: {}", aot_get_last_error());
                throw std::runtime_error("Failed to generate AOT context");
        }

        bool compileSuccess = aot_compile_wasm(compileContext);
        if(!compileSuccess) {
            logger->error("Failed to run codegen on wasm: {}", aot_get_last_error());
            throw std::runtime_error("Failed to run codegen");
        }

        // TODO - avoid using a temp file here
        // Note - WAMR doesn't let us generate an array of bytes, so we have to
        // write to a temp file, then load the bytes again
        boost::filesystem::path temp = boost::filesystem::unique_path();
        bool aotSuccess = aot_emit_aot_file(compileContext, compileData, temp.c_str());
        if(!aotSuccess) {
            logger->error("Failed to write AOT file to {}", temp.string());
            throw std::runtime_error("Failed to emit AOT file");
        }

        // Read object bytes back in
        std::vector<uint8_t> objBytes = faabric::util::readFileToBytes(temp.c_str());
        return objBytes;
    }
}
