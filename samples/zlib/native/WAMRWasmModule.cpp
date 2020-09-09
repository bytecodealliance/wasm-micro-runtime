#include "WAMRWasmModule.h"

#include <wamr/native.h>
#include <storage/FileLoader.h>
#include <wasm_export.h>
#include <faabric/util/locks.h>

namespace wasm {
    static bool wamrInitialised = false;
    std::mutex wamrInitMx;

    static thread_local WAMRWasmModule *executingModule;

    void WAMRWasmModule::initialiseWAMRGlobally() {
        if(wamrInitialised) {
            return;
        } else {
            faabric::util::UniqueLock lock(wamrInitMx);

            if(wamrInitialised) {
                return;
            }

            // Initialise runtime
            bool success = wasm_runtime_init();
            if(!success) {
                throw std::runtime_error("Failed to initialise WAMR");
            }

            faabric::util::getLogger()->debug("Successfully initialised WAMR");

            // Initialise native functions
            initialiseWAMRNatives();
        }
    }

    void tearDownWAMRGlobally() {
        wasm_runtime_destroy();
    }

    WAMRWasmModule *getExecutingWAMRModule() {
        return executingModule;
    }

    void setExecutingModule(WAMRWasmModule *executingModuleIn) {
        executingModule = executingModuleIn;
    }

    WAMRWasmModule::WAMRWasmModule() {
        // Lazily initialise WAMR
        initialiseWAMRGlobally();
    }

    WAMRWasmModule::~WAMRWasmModule() {
        tearDown();
    }

    // ----- Module lifecycle -----
    void WAMRWasmModule::bindToFunction(const faabric::Message &msg) {
        const std::shared_ptr<spdlog::logger> &logger = faabric::util::getLogger();
        
        // Set up the module
        boundUser = msg.user();
        boundFunction = msg.function();
        _isBound = true;

        // Prepare the filesystem
        filesystem.prepareFilesystem();

        // Load the wasm file
        storage::FileLoader &functionLoader = storage::getFileLoader();
        std::vector<uint8_t> aotFileBytes = functionLoader.loadFunctionWamrAotFile(msg);

        // Load wasm
        errorBuffer.reserve(ERROR_BUFFER_SIZE);
        wasmModule = wasm_runtime_load(
                aotFileBytes.data(),
                aotFileBytes.size(),
                errorBuffer.data(),
                ERROR_BUFFER_SIZE
        );

        if(wasmModule == nullptr) {
            std::string errorMsg = std::string(errorBuffer.data());
            logger->error("Failed to instantiate WAMR module: \n{}", errorMsg);
            throw std::runtime_error("Failed to instantiate WAMR module");
        }
        
        // Instantiate module
        moduleInstance = wasm_runtime_instantiate(
                wasmModule,
                STACK_SIZE_KB,
                HEAP_SIZE_KB,
                errorBuffer.data(),
                ERROR_BUFFER_SIZE
        );

        if(moduleInstance->module_type != Wasm_Module_AoT) {
            throw std::runtime_error("WAMR module had unexpected type: " + std::to_string(moduleInstance->module_type));
        }
    }

    void WAMRWasmModule::bindToFunctionNoZygote(const faabric::Message &msg) {
        // WAMR does not support zygotes yet so it's
        // equivalent to binding with zygote
        bindToFunction(msg);
    }

    bool WAMRWasmModule::execute(faabric::Message &msg, bool forceNoop) {
        setExecutingCall(&msg);
        setExecutingModule(this);

        executionEnv = wasm_runtime_create_exec_env(moduleInstance, STACK_SIZE);

        // Run wasm initialisers
        executeFunction(WASM_CTORS_FUNC_NAME);

        // Run the main function
        executeFunction(ENTRY_FUNC_NAME);

        return true;
    }

    void WAMRWasmModule::executeFunction(const std::string &funcName) {
        const std::shared_ptr<spdlog::logger> &logger = faabric::util::getLogger();

        WASMFunctionInstanceCommon *func = wasm_runtime_lookup_function(
                moduleInstance, funcName.c_str(), nullptr
        );

        // Invoke the function
        bool success = wasm_runtime_call_wasm(executionEnv, func, 0, nullptr);
        if (success) {
            logger->debug("{} finished", funcName);
        } else {
            std::string errorMessage(errorBuffer.data());
            logger->error("Function failed: {}", errorMessage);
        }
    }

    bool WAMRWasmModule::isBound() {
        return _isBound;
    }

    void WAMRWasmModule::tearDown() {
        wasm_runtime_destroy_exec_env(executionEnv);
        wasm_runtime_deinstantiate(moduleInstance);
    }

    uint32_t WAMRWasmModule::mmapMemory(uint32_t length) {
        void *nativePtr;
        wasm_runtime_module_malloc(moduleInstance, length, &nativePtr);
        int32 wasmPtr = wasm_runtime_addr_native_to_app(moduleInstance, nativePtr);
        return wasmPtr;
    }

    uint32_t WAMRWasmModule::mmapPages(uint32_t pages) {
        uint32_t bytes = pages * WASM_BYTES_PER_PAGE;
        return mmapMemory(bytes);
    }

    uint8_t* WAMRWasmModule::wasmPointerToNative(int32_t wasmPtr) {
        void *nativePtr = wasm_runtime_addr_app_to_native(moduleInstance, wasmPtr);
        return static_cast<uint8_t *>(nativePtr);
    }

    uint32_t WAMRWasmModule::mmapFile(uint32_t fp, uint32_t length) {
        // TODO - implement
        return 0;
    }
}