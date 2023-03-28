# Run generated module

This document describe how to run generated .wasm module

## Run generated module by V8

### Prerequisites
   - v8 version 11.3.0 or higher
      > Please refer to [V8 doc](https://v8.dev/docs/build) to build the `d8` and ensure `d8` command is available in the terminal.

### Run

   - run `runWasm.js` by d8 with `experimental-wasm-gc` option:

      `d8 --experimental-wasm-gc runWasm.js -- xxx.wasm funcName`

      the parameter `xxx.wasm` is the module you want to run, and `funcName` is the export function you want to execute in the module.

   - export function parameters

      the export function maybe accept parameters, you can pass them by pairs, the first one of the pair represents the type of argument, 0: boolean, 1: number 2: string(**currently V8 doesn't support**). For example, the export function you want to execute is:

      `export function foo(x: number, y: boolean, z: number)`

      so the command to run this function is:

      `d8 --experimental-wasm-gc runWasm.js -- xxx.wasm foo 1 10 0 0 1 11`

      if you build V8 with snapshot, you need to add `--snapshot_blob` to specify the path of `snapshot_blob.bin`, for example:

      `d8 --experimental-wasm-gc --snapshot_blob="/path/to/snapshot_blob.bin" runWasm.js -- xxx.wasm foo 1 10 0 0 1 11`

      the above command is expected equal to call:

      `foo(10, false, 11)`

## Auto validate module by V8

This section describe how to add test cases into the auto validation script. If you just want to run a single wasm module, skip this section.

- add test files that you want to validate into `validate_res.txt`, the format is:

   `moduleName  validateFlag(0: not validate, 1 validate) result(the format as above) exportFunction functionParameters(the format as above)`

   for example:

   ``` c++
   //for module foo.wasm, export function funcFoo, which accept parameter(number, boolean), here passes(1, false), return value is 1(number) but we dont want to validate it(validate flag is 0)
   foo.wasm 0 1 1 funcFoo 1 1 0 0

   // we want to validate module bar's export function funcBar, whose return value is 10, and it doesn't accept parameter
   bar.wasm 1 1 10 funcBar
   ```

   then run the command below:

   ```bash
   bash validate.sh
   ```

   it accepts optimize level as argument, by default, optimize level is 0.

   For example, the command below will compile .ts file to .wasm in optimize level o3.

   ```bash
   bash validate.sh 3
   ```

   the result will be saved in `result.txt`
