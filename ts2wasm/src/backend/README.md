# ts2wasm backend

ts2wasm is designed to support multiple backends, every backend should inherit the `Ts2wasmBackend` class defined in index.ts.

The backend class can access the `ParserContext` to get all necessary information.

``` TypeScript
/* custom_backend.ts */
class CustomBackend extends Ts2wasmBackend {
    // ...
}

/* cli.ts */
const parserCtx = new ParserContext();
parserCtx.parse();

const backend = new CustomBackend(parserCtx);
backend.codegen();
backend.emitBinary();
```
