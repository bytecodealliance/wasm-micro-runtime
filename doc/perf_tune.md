# Tune the performance of running wasm/aot file

Normally there are some methods to tune the performance:

## 1. Use `wasm-opt` tool 

Download the [binaryen release](https://github.com/WebAssembly/binaryen/releases), and use the `wasm-opt` tool in it to optimize the wasm file, for example:

```bash
wasm-opt -O4 -o test_opt.wasm test.wasm
```

## 2. Enable `simd128` option when compiling wasm source files

WebAssembly [128-bit SIMD](https://github.com/WebAssembly/simd) is supported by WAMR on x86-64 and aarch64 targets, enabling it when compiling wasm source files may greatly improve the performance. For [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) and [emsdk](https://github.com/emscripten-core/emsdk), please add `-msimd128` flag for `clang` and `emcc/em++`:

```bash
/opt/wasi-sdk/bin/clang -msimd128 -O3 -o <wasm_file> <c/c++ source files>

emcc -msimd128 -O3 -o <wasm_file> <c/c++ source files>
```

## 3. Enable segue optimization for wamrc when generating the aot file

[Segue](https://plas2022.github.io/files/pdf/SegueColorGuard.pdf) is an optimization technology which uses x86 segment register to store the WebAssembly linear memory base address, so as to remove most of the cost of SFI (Software-based Fault Isolation) base addition and free up a general purpose register, by this way it may:
- Improve the performance of JIT/AOT
- Reduce the footprint of JIT/AOT, the JIT/AOT code generated is smaller
- Reduce the compilation time of JIT/AOT

Currently it is supported on linux x86-64, developer can use `--enable-segue=[<flags>]` for wamrc:
```bash
wamrc --enable-segue -o aot_file wasm_file
# or
wamrc --enable-segue=[<flags>] -o aot_file wasm_file
```
`flags` can be: i32.load, i64.load, f32.load, f64.load, v128.load, i32.store, i64.store, f32.store, f64.store and v128.store, use comma to separate them, e.g. `--enable-segue=i32.load,i64.store`, and `--enable-segue` means all flags are added.

> Note: Normally for most cases, using `--enable-segue` is enough, but for some cases, using `--enable-segue=<flags>` may be better, for example for CoreMark benchmark, `--enable-segue=i32.store` may lead to better performance than `--enable-segue`.

## 4. Enable segue optimization for iwasm when running wasm file

Similar to segue optimization for wamrc, run:
``` bash
iwasm --enable-segue wasm_file      (iwasm is built with llvm-jit enabled)
# or
iwasm --enable-segue=[<flags>] wasm_file
```

## 5. Use the AOT static PGO method

LLVM PGO (Profile-Guided Optimization) allows the compiler to better optimize code for how it actually runs. WAMR supports AOT static PGO, currently it is tested on Linux x86-64 and x86-32. The basic steps are:

1. Use `wamrc --enable-llvm-pgo -o <aot_file_of_pgo> <wasm_file>` to generate an instrumented aot file.

2. Compile iwasm with `cmake -DWAMR_BUILD_STATIC_PGO=1` and run `iwasm --gen-prof-file=<raw_profile_file> <aot_file_of_pgo>` to generate the raw profile file.

> Note: Directly dumping raw profile data to file system may be unsupported in some environments, developer can dump the profile data into memory buffer instead and try outputting it through network (e.g. uart or socket):
```C
uint32_t
wasm_runtime_get_pgo_prof_data_size(wasm_module_inst_t module_inst);

uint32_t
wasm_runtime_dump_pgo_prof_data_to_buf(wasm_module_inst_t module_inst, char *buf, uint32_t len);
```

3. Install or compile `llvm-profdata` tool，refer to [here](../tests/benchmarks/README.md#install-llvm-profdata) for the details.

4. Run `llvm-profdata merge -output=<profile_file> <raw_profile_file>` to merge the raw profile file into the profile file.

5. Run `wamrc --use-prof-file=<profile_file> -o <aot_file> <wasm_file>` to generate the optimized aot file.

6. Run the optimized aot_file: `iwasm <aot_file>`.

Developer can refer to the `test_pgo.sh` files under each benchmark folder for more details, e.g. [test_pgo.sh](../tests/benchmarks/coremark/test_pgo.sh) of CoreMark benchmark.
