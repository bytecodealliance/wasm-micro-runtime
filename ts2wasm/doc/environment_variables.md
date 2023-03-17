# ts2wasm environment variables

This document record useful environment variables used during development, these variables may or may not be included in final release.

- **TS2WASM_DUMP_SCOPE**

    if not none, ts2wasm will dump the scope information before code generation

- **TS2WASM_VALIDATE**

    if not none, ts2wasm will run binaryen's validation function to print any error found in the generated wasm module
