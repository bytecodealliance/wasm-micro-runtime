# ts2wasm

ts2wasm is a compiler which compiles TypeScript source code directly into WebAssembly bytecode. It will leverage WebAssembly's GC proposal. The strategy is to apply static compilation for those with type information, while supporting dynamic type (any) through a builtin runtime library.

> **Note: This project is highly experimental and under active development, DO NOT use in production**

## Build and run

1. install the dependencies
    ``` bash
    npm install
    ```

2. build

    ``` bash
    npm run build
    ```

3. use the compiler

    ``` bash
    cd build
    node cli/ts2wasm.js <source> -o out.wasm
    ```

## Execute the generated module

### Execute on WAMR
Refer to [iwasm_gc](./runtime-library/README.md)

### Execute on browser
Refer to [ts2wasm playground](./tools/playground/README.md)

## Test

``` bash
npm run test
```
