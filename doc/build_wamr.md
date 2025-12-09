# Build WAMR vmcore

WAMR vmcore is a set of runtime libraries for loading and running Wasm modules. This document introduces how to build the WAMR vmcore.

References:

- [how to build iwasm](../product-mini/README.md): building different target platforms such as Linux, Windows, Mac etc
- [Blog: Introduction to WAMR running modes](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-running-modes/)

## building configurations

By including the script `runtime_lib.cmake` under folder [build-scripts](../build-scripts) in CMakeList.txt, it is easy to use vmcore to build host software with cmake.

```cmake
# add this into your CMakeList.txt
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
```

The script `runtime_lib.cmake` defines a number of variables for configuring the WAMR runtime features. You can set these variables in your CMakeList.txt or pass the configurations from cmake command line.

### All compilation flags

| Description                          | Compilation flags                                                                                        | Tiered | Default | on Ubuntu |
| ------------------------------------ | -------------------------------------------------------------------------------------------------------- | ------ | ------- | --------- |
| Maximum stack size for app threads   | [WAMR_APP_THREAD_STACK_SIZE_MAX](#set-maximum-app-thread-stack-size)                                     | B      | ND[^1]  |           |
| Host defined logging                 | [WAMR_BH_LOG](#wamr_bh_log)                                                                              | B      | ND      |           |
| Host defined vprintf                 | [WAMR_BH_VPRINTF](#set-vprintf-callback)                                                                 | B      | ND      |           |
| Allocation with usage tracking       | [WAMR_BUILD_ALLOC_WITH_USAGE](#user-defined-linear-memory-allocator)                                     | B      | ND      |           |
| Allocation with user data            | [WAMR_BUILD_ALLOC_WITH_USER_DATA](#user-defined-linear-memory-allocator)                                 | B      | ND      |           |
| AoT compilation                      | [WAMR_BUILD_AOT](#configure-aot)                                                                         | A      | ND      | 1         |
| AoT intrinsics                       | [WAMR_BUILD_AOT_INTRINSICS](#enable-aot-intrinsics)                                                      | A      | 1[^2]   |           |
| AoT stack frame                      | [WAMR_BUILD_AOT_STACK_FRAME](#enable-aot-stack-frame-feature)                                            | A      | ND      |           |
| AoT validator                        | [WAMR_BUILD_AOT_VALIDATOR]()                                                                             | B      | ND      |           |
| bulk memory                          | [WAMR_BUILD_BULK_MEMORY](#enable-bulk-memory-feature)                                                    | A      | 1       |           |
| copy call stack                      | WAMR_BUILD_COPY_CALL_STACK                                                                               | B      | ND      |           |
| custom name section                  | [WAMR_BUILD_CUSTOM_NAME_SECTION](#configure-debug)                                                       | B      | ND      |           |
| debug AoT                            | WAMR_BUILD_DEBUG_AOT                                                                                     | C      | ND      |           |
| debug interpreter                    | [WAMR_BUILD_DEBUG_INTERP](#enable-source-debugging-features)                                             | B      | ND      |           |
| dump call stack                      | [WAMR_BUILD_DUMP_CALL_STACK](#enable-dump-call-stack-feature)                                            | B      | ND      |           |
| dynamic AoT debugging                | WAMR_BUILD_DYNAMIC_AOT_DEBUG                                                                             | C      | ND      |           |
| exception handling                   | [WAMR_BUILD_EXCE_HANDLING](#enable-exception-handling)                                                   | C      | 0       |           |
| extended constant expressions        | [WAMR_BUILD_EXTENDED_CONST_EXPR](#enable-extended-constant-expression)                                   | A      | 0       |           |
| fast interpreter                     | [WAMR_BUILD_FAST_INTERP](#configure-interpreters)                                                        | A      | ND      | 1         |
| fast JIT                             | [WAMR_BUILD_FAST_JIT](#configure-fast-jit)                                                               | B      | ND      |           |
| fast JIT dump                        | WAMR_BUILD_FAST_JIT_DUMP                                                                                 | B      | ND      |           |
| garbage collection                   | [WAMR_BUILD_GC](#enable-garbage-collection)                                                              | B      | 0       |           |
| garbage collection heap verification | WAMR_BUILD_GC_HEAP_VERIFY                                                                                | B      | ND      |           |
| global heap pool                     | [WAMR_BUILD_GLOBAL_HEAP_POOL](#enable-the-global-heap)                                                   | A      | ND      |           |
| global heap size                     | [WAMR_BUILD_GLOBAL_HEAP_SIZE](#set-the-global-heap-size)                                                 | A      | ND      |           |
| instruction metering                 | [WAMR_BUILD_INSTRUCTION_METERING](#instruction-metering)                                                 | C      | ND      |           |
| interpreter                          | [WAMR_BUILD_INTERP](#configure-interpreters)                                                             | A      | ND      | 1         |
| native general invocation            | WAMR_BUILD_INVOKE_NATIVE_GENERAL                                                                         | B      | ND      |           |
| JIT compilation                      | [WAMR_BUILD_JIT](#configure-llvm-jit)                                                                    | B      | ND      |           |
| lazy JIT compilation                 | WAMR_BUILD_LAZY_JIT                                                                                      | B      | 1[^3]   |           |
| libc builtin functions               | [WAMR_BUILD_LIBC_BUILTIN](#configure-libc)                                                               | A      | ND      | 1         |
| libc emcc compatibility              | WAMR_BUILD_LIBC_EMCC                                                                                     | C      | ND      |           |
| libc uvwasi compatibility            | [WAMR_BUILD_LIBC_UVWASI](#configure-libc)                                                                | C      | ND      |           |
| wasi libc                            | [WAMR_BUILD_LIBC_WASI](#configure-libc)                                                                  | A      | ND      | 1         |
| pthread library                      | [WAMR_BUILD_LIB_PTHREAD](#enable-lib-pthread)                                                            | B      | ND      |           |
| pthread semaphore support            | [WAMR_BUILD_LIB_PTHREAD_SEMAPHORE](#enable-lib-pthread-semaphore)                                        | B      | ND      |           |
| RATS library                         | WAMR_BUILD_LIB_RATS                                                                                      | C      | ND      |           |
| wasi threads                         | [WAMR_BUILD_LIB_WASI_THREADS](#enable-lib-wasi-threads)                                                  | B      | ND      |           |
| Linux performance counters           | [WAMR_BUILD_LINUX_PERF](#enable-linux-perf-support)                                                      | B      | ND      |           |
| LIME1 runtime                        | [WAMR_BUILD_LIME1](#enable-lime1-target)                                                                 | A      | NO      |           |
| loading custom sections              | [WAMR_BUILD_LOAD_CUSTOM_SECTION](#enable-load-wasm-custom-sections)                                      | A      | ND      |           |
| memory64 support                     | [WAMR_BUILD_MEMORY64](#enable-memory64-feature)                                                          | A      | 0       |           |
| memory profiling                     | [WAMR_BUILD_MEMORY_PROFILING](#enable-memory-profiling-experiment)                                       | B      | ND      |           |
| mini loader                          | [WAMR_BUILD_MINI_LOADER](#enable-wasm-mini-loader)                                                       | B      | ND      |           |
| module instance context              | [WAMR_BUILD_MODULE_INST_CONTEXT](#module-instance-context-apis)                                          | B      | ND      | 1         |
| multi-memory support                 | [WAMR_BUILD_MULTI_MEMORY](#enable-multi-memory)                                                          | C      | 0       |           |
| multi-module support                 | [WAMR_BUILD_MULTI_MODULE](#enable-multi-module-feature)                                                  | B      | ND      |           |
| performance profiling                | [WAMR_BUILD_PERF_PROFILING](#enable-performance-profiling-experiment)                                    | B      | ND      |           |
| Default platform                     | [WAMR_BUILD_PLATFORM](#configure-platform-and-architecture)                                              | -      | ND      | linux     |
| quick AOT entry                      | [WAMR_BUILD_QUICK_AOT_ENTRY](#enable-quick-aotjti-entries)                                               | A      | 1[^4]   |           |
| reference types                      | [WAMR_BUILD_REF_TYPES](#enable-reference-types-feature)                                                  | A      | ND      | 1         |
| sanitizer                            | WAMR_BUILD_SANITIZER                                                                                     | B      | ND      |           |
| SGX IPFS support                     | WAMR_BUILD_SGX_IPFS                                                                                      | C      | ND      |           |
| shared heap                          | [WAMR_BUILD_SHARED_HEAP](#shared-heap-among-wasm-apps-and-host-native)                                   | A      | ND      |           |
| shared memory                        | [WAMR_BUILD_SHARED_MEMORY](#enable-shared-memory-feature)                                                | A      | 0       | 1         |
| shrunk memory                        | [WAMR_BUILD_SHRUNK_MEMORY](#shrunk-the-memory-usage)                                                     | A      | ND      | 1         |
| SIMD support                         | [WAMR_BUILD_SIMD](#enable-128-bit-simd-feature)                                                          | A      | ND      | 1         |
| SIMD E extensions                    | WAMR_BUILD_SIMDE                                                                                         | A      | ND      | 1         |
| spec test                            | WAMR_BUILD_SPEC_TEST                                                                                     | A      | ND      |           |
| Stack guard size                     | [WAMR_BUILD_STACK_GUARD_SIZE](#stack-guard-size)                                                         | B      | ND      |           |
| Static PGO                           | [WAMR_BUILD_STATIC_PGO](#enable-running-pgoprofile-guided-optimization-instrumented-aot-file)            | B      | ND      |           |
| String reference support             | [WAMR_BUILD_STRINGREF](#configure-debug)                                                                 | B      | 0       |           |
| Tail call optimization               | [WAMR_BUILD_TAIL_CALL](#enable-tail-call-feature)                                                        | A      | 0       | 1         |
| Default target architecture          | [WAMR_BUILD_TARGET](#configure-platform-and-architecture)                                                | -      | ND      | X86-64    |
| Thread manager                       | [WAMR_BUILD_THREAD_MGR](#enable-thread-manager)                                                          | A      | ND      |           |
| WAMR compiler                        | WAMR_BUILD_WAMR_COMPILER                                                                                 | A      | ND      |           |
| WASI ephemeral NN                    | [WAMR_BUILD_WASI_EPHEMERAL_NN](#enable-lib-wasi-nn-with-wasi_ephemeral_nn-module-support)                | B      | ND      |           |
| WASI NN                              | [WAMR_BUILD_WASI_NN](#enable-lib-wasi-nn)                                                                | B      | ND      |           |
| external delegate for WASI NN        | [WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE](#enable-lib-wasi-nn-external-delegate-mode)                | B      | ND      |           |
| GPU support for WASI NN              | [WAMR_BUILD_WASI_NN_ENABLE_GPU](#enable-lib-wasi-nn-gpu-mode)                                            | B      | ND      |           |
| External delegate path for WASI NN   | [WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH](#enable-lib-wasi-nn-external-delegate-mode)                  | B      | ND      |           |
| LLAMA CPP for WASI NN                | WAMR_BUILD_WASI_NN_LLAMACPP                                                                              | B      | ND      |           |
| ONNX for WASI NN                     | WAMR_BUILD_WASI_NN_ONNX                                                                                  | B      | ND      |           |
| OpenVINO for WASI NN                 | WAMR_BUILD_WASI_NN_OPENVINO                                                                              | B      | ND      |           |
| TFLite for WASI NN                   | WAMR_BUILD_WASI_NN_TFLITE                                                                                | B      | ND      |           |
| WASM cache                           | WAMR_BUILD_WASM_CACHE                                                                                    | B      | ND      |           |
| Configurable bounds checks           | [WAMR_CONFIGURABLE_BOUNDS_CHECKS](#configurable-memory-access-boundary-check)                            | C      | ND      |           |
| Disable app entry                    | [WAMR_DISABLE_APP_ENTRY](#exclude-wamr-application-entry-functions)                                      | A      | ND      |           |
| Disable hardware bound check         | [WAMR_DISABLE_HW_BOUND_CHECK](#disable-boundary-check-with-hardware-trap)                                | A      | ND      |           |
| Disable stack hardware bound check   | [WAMR_DISABLE_STACK_HW_BOUND_CHECK](#disable-native-stack-boundary-check-with-hardware-trap)             | A      | ND      |           |
| Disable wakeup blocking operation    | [WAMR_DISABLE_WAKEUP_BLOCKING_OP](#disable-async-wakeup-of-blocking-operation)                           | B      | ND      |           |
| Disable write GS base                | [WAMR_DISABLE_WRITE_GS_BASE](#disable-writing-the-linear-memory-base-address-to-x86-gs-segment-register) | B      | ND      |           |
| Test garbage collection              | WAMR_TEST_GC                                                                                             | B      | ND      |           |

### **Configure platform and architecture**

- **WAMR_BUILD_PLATFORM**: set the target platform. It can be set to any platform name (folder name) under folder [core/shared/platform](../core/shared/platform).

- **WAMR_BUILD_TARGET**: set the target CPU architecture. Current supported targets are: X86_64, X86_32, AARCH64, ARM, THUMB, XTENSA, ARC, RISCV32, RISCV64 and MIPS.
  - For ARM and THUMB, the format is \<arch>\[\<sub-arch>]\[\_VFP], where \<sub-arch> is the ARM sub-architecture and the "\_VFP" suffix means using VFP coprocessor registers s0-s15 (d0-d7) for passing arguments or returning results in standard procedure-call. Both \<sub-arch> and "\_VFP" are optional, e.g. ARMV7, ARMV7_VFP, THUMBV7, THUMBV7_VFP and so on.
  - For AARCH64, the format is\<arch>[\<sub-arch>], VFP is enabled by default. \<sub-arch> is optional, e.g. AARCH64, AARCH64V8, AARCH64V8.1 and so on.
  - For RISCV64, the format is \<arch\>[_abi], where "\_abi" is optional, currently the supported formats are RISCV64, RISCV64_LP64D and RISCV64_LP64: RISCV64 and RISCV64_LP64D are identical, using [LP64D](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) as abi (LP64 with hardware floating-point calling convention for FLEN=64). And RISCV64_LP64 uses [LP64](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) as abi (Integer calling-convention only, and hardware floating-point calling convention is not used).
  - For RISCV32, the format is \<arch\>[_abi], where "\_abi" is optional, currently the supported formats are RISCV32, RISCV32_ILP32D, RISCV32_ILP32F and RISCV32_ILP32: RISCV32 and RISCV32_ILP32D are identical, using [ILP32D](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) as abi (ILP32 with hardware floating-point calling convention for FLEN=64). RISCV32_ILP32F uses [ILP32F](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) as abi (ILP32 with hardware floating-point calling convention for FLEN=32). And RISCV32_ILP32 uses [ILP32](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) as abi (Integer calling-convention only, and hardware floating-point calling convention is not used).

```bash
cmake -DWAMR_BUILD_PLATFORM=linux -DWAMR_BUILD_TARGET=ARM
```

### **Configure interpreters**

- **WAMR_BUILD_INTERP**=1/0: enable or disable WASM interpreter

- **WAMR_BUILD_FAST_INTERP**=1/0: build fast (default) or classic WASM interpreter.

> [!NOTE]
> the fast interpreter runs ~2X faster than classic interpreter, but consumes about 2X memory to hold the pre-compiled code.

### **Configure AOT**

- **WAMR_BUILD_AOT**=1/0, enable AOT or not, default to enable if not set

### **Configure LLVM JIT**

- **WAMR_BUILD_JIT**=1/0, enable LLVM JIT or not, default to disable if not set

### **Configure Fast JIT**

the fast JIT is a lightweight JIT compiler which generates machine code quickly with optimizations for hot functions. Only covers few architectures (x86_64) currently.

- **WAMR_BUILD_FAST_JIT**=1/0, enable Fast JIT or not, default to disable if not set

> [!NOTE]
>
> - **WAMR_BUILD_FAST_JIT**=1 and **WAMR_BUILD_JIT**=1, enable Multi-tier JIT, default to disable if not set

### **Configure LIBC**

- **WAMR_BUILD_LIBC_BUILTIN**=1/0, build the built-in libc subset for WASM app, default to enable if not set

- **WAMR_BUILD_LIBC_WASI**=1/0, build the [WASI](https://github.com/WebAssembly/WASI) libc subset for WASM app, default to enable if not set

- **WAMR_BUILD_LIBC_UVWASI**=1/0 (Experiment), build the [WASI](https://github.com/WebAssembly/WASI) libc subset for WASM app based on [uvwasi](https://github.com/nodejs/uvwasi) implementation, default to disable if not set

> [!WARNING]
> WAMR doesn't support a safe sandbox on all platforms. For platforms that do not support **WAMR_BUILD_LIBC_WASI**, e.g. Windows, developers can try using an unsafe uvwasi-based WASI implementation by using **WAMR_BUILD_LIBC_UVWASI**.

### **Enable Multi-Module feature**

- **WAMR_BUILD_MULTI_MODULE**=1/0, default to disable if not set

> [!NOTE]
> See [Multiple Modules as Dependencies](./multi_module.md) for more details.

> [!WARNING]
> Currently, the multi-module feature is not supported in fast-jit and llvm-jit modes.

### **Enable WASM mini loader**

- **WAMR_BUILD_MINI_LOADER**=1/0, default to disable if not set

> [!NOTE]
> the mini loader doesn't check the integrity of the WASM binary file, developer must ensure that the WASM file is well-formed.

### **Enable shared memory feature**

- **WAMR_BUILD_SHARED_MEMORY**=1/0, default to disable if not set

### **Enable bulk memory feature**

- **WAMR_BUILD_BULK_MEMORY**=1/0, default to disable if not set

### **Enable memory64 feature**

- **WAMR_BUILD_MEMORY64**=1/0, default to disable if not set

> [!WARNING]
> Currently, the memory64 feature is only supported in classic interpreter running mode and AOT mode.

### **Enable thread manager**

- **WAMR_BUILD_THREAD_MGR**=1/0, default to disable if not set

### **Enable lib-pthread**

- **WAMR_BUILD_LIB_PTHREAD**=1/0, default to disable if not set

> [!NOTE]
> The dependent feature of lib pthread such as the `shared memory` and `thread manager` will be enabled automatically.
> See [WAMR pthread library](./pthread_library.md) for more details.

### **Enable lib-pthread-semaphore**

- **WAMR_BUILD_LIB_PTHREAD_SEMAPHORE**=1/0, default to disable if not set

> [!NOTE]
> This feature depends on `lib-pthread`, it will be enabled automatically if this feature is enabled.

### **Enable lib wasi-threads**

- **WAMR_BUILD_LIB_WASI_THREADS**=1/0, default to disable if not set

> [!NOTE]
> The dependent feature of lib wasi-threads such as the `shared memory` and `thread manager` will be enabled automatically.
> See [wasi-threads](./pthread_impls.md#wasi-threads-new) and [Introduction to WAMR WASI threads](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-wasi-threads) for more details.

### **Enable lib wasi-nn**

- **WAMR_BUILD_WASI_NN**=1/0, default to disable if not set

> [!NOTE]
> WAMR_BUILD_WASI_NN without WAMR_BUILD_WASI_EPHEMERAL_NN is deprecated and will likely be removed in future versions of WAMR. Please consider to enable WAMR_BUILD_WASI_EPHEMERAL_NN as well.
> See [WASI-NN](../core/iwasm/libraries/wasi-nn) for more details.

### **Enable lib wasi-nn GPU mode**

- **WAMR_BUILD_WASI_NN_ENABLE_GPU**=1/0, default to disable if not set

### **Enable lib wasi-nn external delegate mode**

- **WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE**=1/0, default to disable if not set

- **WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH**=Path to the external delegate shared library (e.g. `libedgetpu.so.1.0` for Coral USB)

### **Enable lib wasi-nn with `wasi_ephemeral_nn` module support**

- **WAMR_BUILD_WASI_EPHEMERAL_NN**=1/0, default to enable if not set

### **Disable boundary check with hardware trap**

- **WAMR_DISABLE_HW_BOUND_CHECK**=1/0, default to enable if not set and supported by platform

> [!NOTE]
> by default only platform [linux/darwin/android/windows/vxworks 64-bit](https://github.com/bytecodealliance/wasm-micro-runtime/blob/5fb5119239220b0803e7045ca49b0a29fe65e70e/core/shared/platform/linux/platform_internal.h#L81) will enable the boundary check with hardware trap feature, for 32-bit platforms it's automatically disabled even when the flag is set to 0, and the wamrc tool will generate AOT code without boundary check instructions in all 64-bit targets except SGX to improve performance. The boundary check includes linear memory access boundary and native stack access boundary, if `WAMR_DISABLE_STACK_HW_BOUND_CHECK` below isn't set.

### **Disable native stack boundary check with hardware trap**

- **WAMR_DISABLE_STACK_HW_BOUND_CHECK**=1/0, default to enable if not set and supported by platform, same as `WAMR_DISABLE_HW_BOUND_CHECK`.

> [!NOTE]
> When boundary check with hardware trap is disabled, or `WAMR_DISABLE_HW_BOUND_CHECK` is set to 1, the native stack boundary check with hardware trap will be disabled too, no matter what value is set to `WAMR_DISABLE_STACK_HW_BOUND_CHECK`. And when boundary check with hardware trap is enabled, the status of this feature is set according to the value of `WAMR_DISABLE_STACK_HW_BOUND_CHECK`.

### **Disable async wakeup of blocking operation**

- **WAMR_DISABLE_WAKEUP_BLOCKING_OP**=1/0, default to enable if supported by the platform

> [!NOTE]
> The feature helps async termination of blocking threads. If you disable it, the runtime can wait for termination of blocking threads possibly forever.

### **Enable tail call feature**

- **WAMR_BUILD_TAIL_CALL**=1/0, default to disable if not set

### **Enable 128-bit SIMD feature**

- **WAMR_BUILD_SIMD**=1/0, default to enable if not set

> [!WARNING]
> supported in AOT mode, JIT mode, and fast-interpreter mode with SIMDe library.

### **Enable SIMDe library for SIMD in fast interpreter**

- **WAMR_BUILD_LIB_SIMDE**=1/0, default to disable if not set

> [!NOTE]
> If enabled, SIMDe (SIMD Everywhere) library will be used to implement SIMD operations in fast interpreter mode.

### **Enable Exception Handling**

- **WAMR_BUILD_EXCE_HANDLING**=1/0, default to disable if not set

> [!WARNING]
> Currently, the exception handling feature is only supported in classic interpreter running mode.

### **Enable Garbage Collection**

- **WAMR_BUILD_GC**=1/0, default to disable if not set

> [!WARNING]
> Currently, the exception handling feature is not supported in fast-jit running mode.

### **Set the Garbage Collection heap size**

- **WAMR_BUILD_GC_HEAP_SIZE_DEFAULT**=n, default to 128 kB (131072) if not set

### **Enable Multi Memory**

- **WAMR_BUIL_MULTI_MEMORY**=1/0, default to disable if not set

> [!WARNING]
> Currently, the multi memory feature is only supported in classic interpreter running mode.

### **Configure Debug**

- **WAMR_BUILD_CUSTOM_NAME_SECTION**=1/0, load the function name from custom name section, default to disable if not set

### **Enable AOT stack frame feature**

- **WAMR_BUILD_AOT_STACK_FRAME**=1/0, default to disable if not set

> [!NOTE]
> if it is enabled, the AOT or JIT stack frames (like stack frame of classic interpreter but only necessary data is committed) will be created for AOT or JIT mode in function calls. And please add `--enable-dump-call-stack` option to wamrc during compiling AOT module.

### **Enable dump call stack feature**

- **WAMR_BUILD_DUMP_CALL_STACK**=1/0, default to disable if not set

> [!NOTE]
> if it is enabled, the call stack will be dumped when exception occurs.
>
> - For interpreter mode, the function names are firstly extracted from _custom name section_, if this section doesn't exist or the feature is not enabled, then the name will be extracted from the import/export sections
> - For AOT/JIT mode, the function names are extracted from import/export section, please export as many functions as possible (for `wasi-sdk` you can use `-Wl,--export-all`) when compiling wasm module, and add `--enable-dump-call-stack --emit-custom-sections=name` option to wamrc during compiling AOT module.

### **Enable memory profiling (Experiment)**

- **WAMR_BUILD_MEMORY_PROFILING**=1/0, default to disable if not set

> [!NOTE]
> if it is enabled, developer can use API `void wasm_runtime_dump_mem_consumption(wasm_exec_env_t exec_env)` to dump the memory consumption info.
> Currently we only profile the memory consumption of module, module_instance and exec_env, the memory consumed by other components such as `wasi-ctx`, `multi-module` and `thread-manager` are not included.
>
> Also refer to [Memory usage estimation for a module](./memory_usage.md).

### **Enable performance profiling (Experiment)**

- **WAMR_BUILD_PERF_PROFILING**=1/0, default to disable if not set

> [!NOTE]
> if it is enabled, developer can use API `void wasm_runtime_dump_perf_profiling(wasm_module_inst_t module_inst)` to dump the performance consumption info. Currently we only profile the performance consumption of each WASM function.
> The function name searching sequence is the same with dump call stack feature.
> Also refer to [Tune the performance of running wasm/aot file](./perf_tune.md).

### **Enable the global heap**

- **WAMR_BUILD_GLOBAL_HEAP_POOL**=1/0, default to disable if not set for all _iwasm_ applications, except for the platforms Alios and Zephyr.

> [!NOTE] > **WAMR_BUILD_GLOBAL_HEAP_POOL** is used in the _iwasm_ applications provided in the directory `product-mini`. When writing your own host application using WAMR, if you want to use a global heap and allocate memory from it, you must set the initialization argument `mem_alloc_type` to `Alloc_With_Pool`.
> The global heap is defined in the documentation [Memory model and memory usage tunning](memory_tune.md).

### **Set the global heap size**

- **WAMR_BUILD_GLOBAL_HEAP_SIZE**=n, default to 10 MB (10485760) if not set for all _iwasm_ applications, except for the platforms Alios (256 kB), Riot (256 kB) and Zephyr (128 kB).

> [!NOTE] > **WAMR_BUILD_GLOBAL_HEAP_SIZE** is used in the _iwasm_ applications provided in the directory `product-mini`. When writing your own host application using WAMR, if you want to set the amount of memory dedicated to the global heap pool, you must set the initialization argument `mem_alloc_option.pool` with the appropriate values.
> The global heap is defined in the documentation [Memory model and memory usage tunning](memory_tune.md).

### **Set maximum app thread stack size**

- **WAMR_APP_THREAD_STACK_SIZE_MAX**=n, default to 8 MB (8388608) if not set

> [!NOTE]
> the AOT boundary check with hardware trap mechanism might consume large stack since the OS may lazily grow the stack mapping as a guard page is hit, we may use this configuration to reduce the total stack usage, e.g. -DWAMR_APP_THREAD_STACK_SIZE_MAX=131072 (128 KB).

### **Set vprintf callback**

- **WAMR_BH_VPRINTF**=<vprintf_callback>, default to disable if not set

> [!NOTE]
> if the vprintf_callback function is provided by developer, the os_printf() and os_vprintf() in Linux, Darwin, Windows, VxWorks, Android and esp-idf platforms, besides WASI Libc output will call the callback function instead of libc vprintf() function to redirect the stdout output. For example, developer can define the callback function like below outside runtime lib:
>
> ```C
> int my_vprintf(const char *format, va_list ap)
> {
>     /* output to pre-opened file stream */
>     FILE *my_file = ...;
>     return vfprintf(my_file, format, ap);
>     /* or output to pre-opened file descriptor */
>     int my_fd = ...;
>     return vdprintf(my_fd, format, ap);
>     /* or output to string buffer and print the string */
>     char buf[128];
>     vsnprintf(buf, sizeof(buf), format, ap);
>     return my_printf("%s", buf);
> }
> ```
>
> and then use `cmake -DWAMR_BH_VPRINTF=my_vprintf ..` to pass the callback function, or add `BH_VPRINTF=my_vprintf` macro for the compiler, e.g. add line `add_definitions(-DBH_VPRINTF=my_vprintf)` in CMakeLists.txt. See [basic sample](../samples/basic/src/main.c) for a usage example.

### **WAMR_BH_LOG**=<log_callback>, default to disable if not set

> [!NOTE]
> if the log_callback function is provided by the developer, WAMR logs are redirected to such callback. For example:
>
> ```C
> void my_log(uint32 log_level, const char *file, int line, const char *fmt, ...)
> {
>     /* Usage of custom logger */
> }
> ```
>
> See [basic sample](../samples/basic/src/main.c) for a usage example.

### **Enable reference types feature**

- **WAMR_BUILD_REF_TYPES**=1/0, default to enable if not set

### **Exclude WAMR application entry functions**

- **WAMR_DISABLE_APP_ENTRY**=1/0, default to disable if not set

> [!NOTE]
> The WAMR application entry (`core/iwasm/common/wasm_application.c`) encapsulate some common process to instantiate, execute the wasm functions and print the results. Some platform related APIs are used in these functions, so you can enable this flag to exclude this file if your platform doesn't support those APIs.
> _Don't enable this flag if you are building `product-mini`_

### **Enable source debugging features**

- **WAMR_BUILD_DEBUG_INTERP**=1/0, default to 0 if not set

> [!NOTE]
> There are some other setup required by source debugging, please refer to [source_debugging.md](./source_debugging.md) and [WAMR source debugging basic](https://bytecodealliance.github.io/wamr.dev/blog/wamr-source-debugging-basic) for more details.

### **Enable load wasm custom sections**

- **WAMR_BUILD_LOAD_CUSTOM_SECTION**=1/0, default to disable if not set

> [!NOTE]
> By default, the custom sections are ignored. If the embedder wants to get custom sections from `wasm_module_t`, then `WAMR_BUILD_LOAD_CUSTOM_SECTION` should be enabled, and then `wasm_runtime_get_custom_section` can be used to get a custom section by name.
>
> If `WAMR_BUILD_CUSTOM_NAME_SECTION` is enabled, then the `custom name section` will be treated as a special section and consumed by the runtime, not available to the embedder.
> For AoT file, must use `--emit-custom-sections` to specify which sections need to be emitted into AoT file, otherwise all custom sections will be ignored.

### **Stack guard size**

- **WAMR_BUILD_STACK_GUARD_SIZE**=n, default to N/A if not set.

> [!NOTE]
> By default, the stack guard size is 1K (1024) or 24K (if uvwasi enabled).

### **Disable writing the linear memory base address to x86 GS segment register**

- **WAMR_DISABLE_WRITE_GS_BASE**=1/0, default to enable if not set and supported by platform

> [!NOTE]
> by default only platform [linux x86-64](https://github.com/bytecodealliance/wasm-micro-runtime/blob/5fb5119239220b0803e7045ca49b0a29fe65e70e/core/shared/platform/linux/platform_internal.h#L67) will enable this feature, for 32-bit platforms it's automatically disabled even when the flag is set to 0. In linux x86-64, writing the linear memory base address to x86 GS segment register may be used to speedup the linear memory access for LLVM AOT/JIT, when `--enable-segue=[<flags>]` option is added for `wamrc` or `iwasm`.

> See [Enable segue optimization for wamrc when generating the aot file](./perf_tune.md#3-enable-segue-optimization-for-wamrc-when-generating-the-aot-file) for more details.

### **User defined linear memory allocator**

- **WAMR_BUILD_ALLOC_WITH_USAGE**=1/0, default to disable if not set
- **WAMR_BUILD_ALLOC_WITH_USER_DATA**=1/0, default to disable if not set

> [!NOTE]
> by default, the linear memory is allocated by system. when it's set to 1 and Alloc_With_Allocator is selected, it will be allocated by customer.

### **Enable running PGO(Profile-Guided Optimization) instrumented AOT file**

- **WAMR_BUILD_STATIC_PGO**=1/0, default to disable if not set

> [!NOTE]
> See [Use the AOT static PGO method](./perf_tune.md#5-use-the-aot-static-pgo-method) for more details.

### **Enable linux perf support**

- **WAMR_BUILD_LINUX_PERF**=1/0, enable linux perf support to generate the flamegraph to analyze the performance of a wasm application, default to disable if not set

> [!NOTE]
> See [Use linux-perf](./perf_tune.md#7-use-linux-perf) for more details.

### **Enable module instance context APIs**

- **WAMR_BUILD_MODULE_INST_CONTEXT**=1/0, enable module instance context APIs which can set one or more contexts created by the embedder for a wasm module instance, default to enable if not set:

```C
    wasm_runtime_create_context_key
    wasm_runtime_destroy_context_key
    wasm_runtime_set_context
    wasm_runtime_set_context_spread
    wasm_runtime_get_context
```

> [!NOTE]
> See [wasm_export.h](../core/iwasm/include/wasm_export.h) for more details.

### **Enable quick AOT/JTI entries**

- **WAMR_BUILD_QUICK_AOT_ENTRY**=1/0, enable registering quick call entries to speedup the aot/jit func call process, default to enable if not set

> [!NOTE]
> See [Refine callings to AOT/JIT functions from host native](./perf_tune.md#83-refine-callings-to-aotjit-functions-from-host-native) for more details.

### **Enable AOT intrinsics**

- **WAMR_BUILD_AOT_INTRINSICS**=1/0, enable the AOT intrinsic functions, default to enable if not set. These functions can be called from the AOT code when `--disable-llvm-intrinsics` flag or `--enable-builtin-intrinsics=<intr1,intr2,...>` flag is used by wamrc to generate the AOT file.

> [!NOTE]
> See [Tuning the XIP intrinsic functions](./xip.md#tuning-the-xip-intrinsic-functions) for more details.

### **Enable extended constant expression**

- **WAMR_BUILD_EXTENDED_CONST_EXPR**=1/0, default to disable if not set.

> [!NOTE]
> See [Extended Constant Expressions](https://github.com/WebAssembly/extended-const/blob/main/proposals/extended-const/Overview.md) for more details.

### **Enable bulk-memory-opt**

- **WAMR_BUILD_BULK_MEMORY_OPT**=1/0, default to disable if not set.

> [!NOTE]
> See [bulk-memory-opt](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#bulk-memory-opt) for more details.

### **Enable call-indirect-overlong**

- **WAMR_BUILD_CALL_INDIRECT_OVERLONG**=1/0, default to disable if not set.

> [!NOTE]
> See [call-indirect-overlong](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#call-indirect-overlong) for more details.

### **Enable Lime1 target**

- **WAMR_BUILD_LIME1**=1/0, default to disable if not set.

> [!NOTE]
> See [Lime1](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#lime1) for more details.

### **Configurable memory access boundary check**

- **WAMR_CONFIGURABLE_BOUNDS_CHECKS**=1/0, default to disable if not set

> [!NOTE]
> If it is enabled, allow to run `iwasm --disable-bounds-checks` to disable the memory access boundary checks for interpreter mode.

### **Module instance context APIs**

- **WAMR_BUILD_MODULE_INST_CONTEXT**=1/0, default to disable if not set

> [!NOTE]
> If it is enabled, allow to set one or more contexts created by embedder for a module instance, the below APIs are provided:
>
> ```C
>     wasm_runtime_create_context_key
>     wasm_runtime_destroy_context_key
>     wasm_runtime_set_context
>     wasm_runtime_set_context_spread
>     wasm_runtime_get_context
> ```

### **Shared heap among wasm apps and host native**

- **WAMR_BUILD_SHARED_HEAP**=1/0, default to disable if not set

> [!NOTE]
> If it is enabled, allow to create one or more shared heaps, and attach one to a module instance, the belows APIs ared provided:
>
> ```C
>    wasm_runtime_create_shared_heap
>    wasm_runtime_attach_shared_heap
>    wasm_runtime_detach_shared_heap
>    wasm_runtime_shared_heap_malloc
>    wasm_runtime_shared_heap_free
> ```
>
> And the wasm app can calls below APIs to allocate/free memory from/to the shared heap if it is attached to the app's module instance:
>
> ```C
>    void *shared_heap_malloc();
>    void shared_heap_free(void *ptr);
> ```

> [!WARNING]
> Currently, the shared-heap feature is not supported in fast-jit mode.

### **Shrunk the memory usage**

- **WAMR_BUILD_SHRUNK_MEMORY**=1/0, default to enable if not set

> [!NOTE]
> When enabled, this feature will reduce memory usage by decreasing the size of the linear memory, particularly when the `memory.grow` opcode is not used and memory usage is somewhat predictable.

## **Instruction metering**

- **WAMR_BUILD_INSTRUCTION_METERING**=1/0, default to disable if not set

> [!NOTE]
> Enabling this feature allows limiting the number of instructions a wasm module instance can execute. Use the `wasm_runtime_set_instruction_count_limit(...)` API before calling `wasm_runtime_call_*(...)` APIs to enforce this limit.

## **Combination of configurations:**

We can combine the configurations. For example, if we want to disable interpreter, enable AOT and WASI, we can run command:

```Bash
cmake .. -DWAMR_BUILD_INTERP=0 -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_LIBC_WASI=1 -DWAMR_BUILD_PLATFORM=linux
```

Or if we want to enable interpreter, disable AOT and WASI, and build as X86_32, we can run command:

```Bash
cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_BUILD_TARGET=X86_32
```

When enabling SIMD for fast interpreter mode, you'll need to enable both SIMD and the SIMDe library:

```Bash

cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=1 -DWAMR_BUILD_SIMD=1 -DWAMR_BUILD_LIB_SIMDE=1
```

For Valgrind, begin with the following configurations and add additional ones as needed:

```Bash
  #...
  -DCMAKE_BUILD_TYPE=Debug \
  -DWAMR_DISABLE_HW_BOUND_CHECK=0 \
  -DWAMR_DISABLE_WRITE_GS_BASE=0
  #...
```

To enable the minimal Lime1 feature set, we need to disable some features that are on by default, such as
bulk memory and reference types:

```Bash
cmake .. -DWAMR_BUILD_LIME1=1 -DWAMR_BUILD_BULK_MEMORY=0 -DWAMR_BUILD_REF_TYPES=0 -DDWAMR_BUILD_SIMD=0
```
