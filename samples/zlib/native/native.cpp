#include <wamr/native.h>
#include <wasm_export.h>
#include <wasm_native.h>

namespace wasm {
    void doSymbolRegistration(uint32_t (*f)(NativeSymbol **ns)) {
        NativeSymbol *symbols;
        uint32_t nSymbols = f(&symbols);
        wasm_native_register_natives("env", symbols, nSymbols);
    }

    void initialiseWAMRNatives() {
        doSymbolRegistration(getFaasmDynlinkApi);
        doSymbolRegistration(getFaasmFilesystemApi);
        doSymbolRegistration(getFaasmFunctionsApi);
        doSymbolRegistration(getFaasmPthreadApi);
        doSymbolRegistration(getFaasmStateApi);
        doSymbolRegistration(getFaasmStubs);
    }
}