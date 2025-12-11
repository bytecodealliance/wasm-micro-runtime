# Build WAMR vmcore

WAMR vmcore is the runtime library set that loads and runs Wasm modules. This guide walks you through building the WAMR vmcore.

References:

- [how to build iwasm](../product-mini/README.md): build for Linux, Windows, macOS, and more
- [Blog: Introduction to WAMR running modes](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-running-modes/)

## building configurations

Drop the script `runtime_lib.cmake` from [build-scripts](../build-scripts) into your CMakeLists.txt to pull vmcore into your build.

```cmake
# add this into your CMakeLists.txt
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
```

The `runtime_lib.cmake` script exposes variables that control WAMR runtime features. Set them in CMakeLists.txt or pass them on the cmake command line.

```cmake
# Set flags in CMakeLists.txt
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)
# Include the runtime lib script
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
```

### All compilation flags

| Description                          | Compilation flags                                                                                        |
| ------------------------------------ | -------------------------------------------------------------------------------------------------------- |
| Maximum stack size for app threads   | [WAMR_APP_THREAD_STACK_SIZE_MAX](#set-maximum-app-thread-stack-size)                                     |
| Host defined logging                 | [WAMR_BH_LOG](#wamr_bh_log)                                                                              |
| Host defined vprintf                 | [WAMR_BH_VPRINTF](#set-vprintf-callback)                                                                 |
| Allocation with usage tracking       | [WAMR_BUILD_ALLOC_WITH_USAGE](#user-defined-linear-memory-allocator)                                     |
| Allocation with user data            | [WAMR_BUILD_ALLOC_WITH_USER_DATA](#user-defined-linear-memory-allocator)                                 |
| AoT compilation(wamrc)               | [WAMR_BUILD_AOT](#configure-aot)                                                                         |
| AoT runtime                          | [WAMR_BUILD_AOT](#configure-aot)                                                                         |
| AoT intrinsics                       | [WAMR_BUILD_AOT_INTRINSICS](#enable-aot-intrinsics)                                                      |
| AoT stack frame                      | [WAMR_BUILD_AOT_STACK_FRAME](#enable-aot-stack-frame-feature)                                            |
| AoT validator                        | [WAMR_BUILD_AOT_VALIDATOR]()                                                                             |
| bulk memory                          | [WAMR_BUILD_BULK_MEMORY](#enable-bulk-memory-feature)                                                    |
| copy call stack                      | WAMR_BUILD_COPY_CALL_STACK                                                                               |
| name section                         | [WAMR_BUILD_CUSTOM_NAME_SECTION](#configure-debug)                                                       |
| debug AoT                            | WAMR_BUILD_DEBUG_AOT                                                                                     |
| debug interpreter                    | [WAMR_BUILD_DEBUG_INTERP](#enable-source-debugging-features)                                             |
| dump call stack                      | [WAMR_BUILD_DUMP_CALL_STACK](#enable-dump-call-stack-feature)                                            |
| dynamic AoT debugging                | WAMR_BUILD_DYNAMIC_AOT_DEBUG                                                                             |
| exception handling                   | [WAMR_BUILD_EXCE_HANDLING](#enable-exception-handling)                                                   |
| extended constant expressions        | [WAMR_BUILD_EXTENDED_CONST_EXPR](#enable-extended-constant-expression)                                   |
| fast interpreter                     | [WAMR_BUILD_FAST_INTERP](#configure-interpreters)                                                        |
| fast JIT                             | [WAMR_BUILD_FAST_JIT](#configure-fast-jit)                                                               |
| fast JIT dump                        | WAMR_BUILD_FAST_JIT_DUMP                                                                                 |
| garbage collection                   | [WAMR_BUILD_GC](#enable-garbage-collection)                                                              |
| garbage collection heap verification | WAMR_BUILD_GC_HEAP_VERIFY                                                                                |
| global heap pool                     | [WAMR_BUILD_GLOBAL_HEAP_POOL](#enable-the-global-heap)                                                   |
| global heap size                     | [WAMR_BUILD_GLOBAL_HEAP_SIZE](#set-the-global-heap-size)                                                 |
| instruction metering                 | [WAMR_BUILD_INSTRUCTION_METERING](#instruction-metering)                                                 |
| interpreter                          | [WAMR_BUILD_INTERP](#configure-interpreters)                                                             |
| native general invocation            | WAMR_BUILD_INVOKE_NATIVE_GENERAL                                                                         |
| JIT compilation                      | [WAMR_BUILD_JIT](#configure-llvm-jit)                                                                    |
| lazy JIT compilation                 | WAMR_BUILD_LAZY_JIT                                                                                      |
| libc builtin functions               | [WAMR_BUILD_LIBC_BUILTIN](#configure-libc)                                                               |
| libc emcc compatibility              | WAMR_BUILD_LIBC_EMCC                                                                                     |
| libc uvwasi compatibility            | [WAMR_BUILD_LIBC_UVWASI](#configure-libc)                                                                |
| wasi libc                            | [WAMR_BUILD_LIBC_WASI](#configure-libc)                                                                  |
| pthread library                      | [WAMR_BUILD_LIB_PTHREAD](#enable-lib-pthread)                                                            |
| pthread semaphore support            | [WAMR_BUILD_LIB_PTHREAD_SEMAPHORE](#enable-lib-pthread-semaphore)                                        |
| RATS library                         | WAMR_BUILD_LIB_RATS                                                                                      |
| wasi threads                         | [WAMR_BUILD_LIB_WASI_THREADS](#enable-lib-wasi-threads)                                                  |
| Linux performance counters           | [WAMR_BUILD_LINUX_PERF](#enable-linux-perf-support)                                                      |
| LIME1 runtime                        | [WAMR_BUILD_LIME1](#enable-lime1-target)                                                                 |
| loading custom sections              | [WAMR_BUILD_LOAD_CUSTOM_SECTION](#enable-load-wasm-custom-sections)                                      |
| memory64 support                     | [WAMR_BUILD_MEMORY64](#enable-memory64-feature)                                                          |
| memory profiling                     | [WAMR_BUILD_MEMORY_PROFILING](#enable-memory-profiling-experiment)                                       |
| mini loader                          | [WAMR_BUILD_MINI_LOADER](#enable-wasm-mini-loader)                                                       |
| module instance context              | [WAMR_BUILD_MODULE_INST_CONTEXT](#module-instance-context-apis)                                          |
| multi-memory support                 | [WAMR_BUILD_MULTI_MEMORY](#enable-multi-memory)                                                          |
| multi-module support                 | [WAMR_BUILD_MULTI_MODULE](#enable-multi-module-feature)                                                  |
| performance profiling                | [WAMR_BUILD_PERF_PROFILING](#enable-performance-profiling-experiment)                                    |
| Default platform                     | [WAMR_BUILD_PLATFORM](#configure-platform-and-architecture)                                              |
| quick AOT entry                      | [WAMR_BUILD_QUICK_AOT_ENTRY](#enable-quick-aotjti-entries)                                               |
| reference types                      | [WAMR_BUILD_REF_TYPES](#enable-reference-types-feature)                                                  |
| sanitizer                            | WAMR_BUILD_SANITIZER                                                                                     |
| SGX IPFS support                     | WAMR_BUILD_SGX_IPFS                                                                                      |
| shared heap                          | [WAMR_BUILD_SHARED_HEAP](#shared-heap-among-wasm-apps-and-host-native)                                   |
| shared memory                        | [WAMR_BUILD_SHARED_MEMORY](#enable-shared-memory-feature)                                                |
| shrunk memory                        | [WAMR_BUILD_SHRUNK_MEMORY](#shrunk-the-memory-usage)                                                     |
| SIMD support                         | [WAMR_BUILD_SIMD](#enable-128-bit-simd-feature)                                                          |
| SIMD E extensions                    | WAMR_BUILD_SIMDE                                                                                         |
| spec test                            | WAMR_BUILD_SPEC_TEST                                                                                     |
| Stack guard size                     | [WAMR_BUILD_STACK_GUARD_SIZE](#stack-guard-size)                                                         |
| Static PGO                           | [WAMR_BUILD_STATIC_PGO](#enable-running-pgoprofile-guided-optimization-instrumented-aot-file)            |
| String reference support             | [WAMR_BUILD_STRINGREF](#configure-debug)                                                                 |
| Tail call optimization               | [WAMR_BUILD_TAIL_CALL](#enable-tail-call-feature)                                                        |
| Default target architecture          | [WAMR_BUILD_TARGET](#configure-platform-and-architecture)                                                |
| Thread manager                       | [WAMR_BUILD_THREAD_MGR](#enable-thread-manager)                                                          |
| WAMR compiler                        | WAMR_BUILD_WAMR_COMPILER                                                                                 |
| WASI ephemeral NN                    | [WAMR_BUILD_WASI_EPHEMERAL_NN](#enable-lib-wasi-nn-with-wasi_ephemeral_nn-module-support)                |
| WASI NN                              | [WAMR_BUILD_WASI_NN](#enable-lib-wasi-nn)                                                                |
| External delegate path for WASI NN   | [WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH](#enable-lib-wasi-nn-external-delegate-mode)                  |
| GPU support for WASI NN              | [WAMR_BUILD_WASI_NN_ENABLE_GPU](#enable-lib-wasi-nn-gpu-mode)                                            |
| LLAMA CPP for WASI NN                | WAMR_BUILD_WASI_NN_LLAMACPP                                                                              |
| ONNX for WASI NN                     | WAMR_BUILD_WASI_NN_ONNX                                                                                  |
| OpenVINO for WASI NN                 | WAMR_BUILD_WASI_NN_OPENVINO                                                                              |
| TFLite for WASI NN                   | WAMR_BUILD_WASI_NN_TFLITE                                                                                |
| WASM cache                           | WAMR_BUILD_WASM_CACHE                                                                                    |
| Configurable bounds checks           | [WAMR_CONFIGURABLE_BOUNDS_CHECKS](#configurable-memory-access-boundary-check)                            |
| Disable app entry                    | [WAMR_DISABLE_APP_ENTRY](#exclude-wamr-application-entry-functions)                                      |
| Disable hardware bound check         | [WAMR_DISABLE_HW_BOUND_CHECK](#disable-boundary-check-with-hardware-trap)                                |
| Disable stack hardware bound check   | [WAMR_DISABLE_STACK_HW_BOUND_CHECK](#disable-native-stack-boundary-check-with-hardware-trap)             |
| Disable wakeup blocking operation    | [WAMR_DISABLE_WAKEUP_BLOCKING_OP](#disable-async-wakeup-of-blocking-operation)                           |
| Disable write GS base                | [WAMR_DISABLE_WRITE_GS_BASE](#disable-writing-the-linear-memory-base-address-to-x86-gs-segment-register) |
| Test garbage collection              | WAMR_TEST_GC                                                                                             |

### **Privileged Features**

_Privileged Features_ are powerful options that can affect security. They can boost performance or add capabilities but lower security by compromising isolation. Use them with care and test thoroughly.

### **Configure platform and architecture**

- **WAMR_BUILD_PLATFORM**: set the target platform. Match the platform folder name under [core/shared/platform](../core/shared/platform).

- **WAMR_BUILD_TARGET**: set the target CPU architecture. Supported targets: X86_64, X86_32, AARCH64, ARM, THUMB, XTENSA, ARC, RISCV32, RISCV64, and MIPS.
  - For ARM and THUMB, use `<arch>[<sub-arch>][_VFP]`. `<sub-arch>` is the ARM sub-architecture. `_VFP` means arguments and returns use VFP coprocessor registers s0-s15 (d0-d7). Both are optional, for example ARMV7, ARMV7_VFP, THUMBV7, or THUMBV7_VFP.
  - For AARCH64, use `<arch>[<sub-arch>]`. VFP is on by default. `<sub-arch>` is optional, for example AARCH64, AARCH64V8, or AARCH64V8.1.
  - For RISCV64, use `<arch>[_abi]`. `_abi` is optional. Supported: RISCV64, RISCV64_LP64D, and RISCV64_LP64. RISCV64 and RISCV64_LP64D both use [LP64D](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) (LP64 with hardware floating-point for FLEN=64). RISCV64_LP64 uses [LP64](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) (integer calling convention only; no hardware floating-point calling convention).
  - For RISCV32, use `<arch>[_abi]`. `_abi` is optional. Supported: RISCV32, RISCV32_ILP32D, RISCV32_ILP32F, and RISCV32_ILP32. RISCV32 and RISCV32_ILP32D both use [ILP32D](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) (ILP32 with hardware floating-point for FLEN=64). RISCV32_ILP32F uses [ILP32F](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) (ILP32 with hardware floating-point for FLEN=32). RISCV32_ILP32 uses [ILP32](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#named-abis) (integer calling convention only).

```bash
cmake -DWAMR_BUILD_PLATFORM=linux -DWAMR_BUILD_TARGET=ARM
```

### **Configure interpreters**

- **WAMR_BUILD_INTERP**=1/0: turn the WASM interpreter on or off.

- **WAMR_BUILD_FAST_INTERP**=1/0: pick fast (default) or classic interpreter.

> [!NOTE]
> The fast interpreter runs about twice as fast as the classic one, and uses about twice the memory for the precompiled code.

### **Configure AOT**

- **WAMR_BUILD_AOT**=1/0: turn AOT on or off. Defaults to on.

### **Configure LLVM JIT**

Comparing with fast JIT, LLVM JIT covers more architectures and produces better optimized code, but takes longer on cold start.

- **WAMR_BUILD_JIT**=1/0: turn LLVM JIT on or off. Defaults to off.

### **Configure Fast JIT**

The fast JIT is a lightweight JIT that emits code quickly and tunes hot functions.

- **WAMR_BUILD_FAST_JIT**=1/0: turn Fast JIT on or off. Defaults to off.

> [!WARNING]
> It currently covers only a few architectures (x86_64).

### **Configure Multi-tier JIT**

Use fast jit as the first tier and LLVM JIT as the second tier.

- With **WAMR_BUILD_FAST_JIT**=1 and **WAMR_BUILD_JIT**=1, you get multi-tier JIT. Defaults to off.

> [!WARNING]
> It currently covers only a few architectures (x86_64).

### **Configure LIBC**

- **WAMR_BUILD_LIBC_BUILTIN**=1/0: build the built-in libc subset for WASM apps. Defaults to on.

- **WAMR_BUILD_LIBC_WASI**=1/0: build the [WASI](https://github.com/WebAssembly/WASI) libc subset for WASM apps. Defaults to on.

- **WAMR_BUILD_LIBC_UVWASI**=1/0 (Experiment): build the WASI libc subset for WASM apps using [uvwasi](https://github.com/nodejs/uvwasi). Defaults to off.

> [!WARNING]
> WAMR is not a secure sandbox on every platform. On platforms where **WAMR_BUILD_LIBC_WASI** is unsupported (for example Windows), you can try the uvwasi-based WASI via **WAMR_BUILD_LIBC_UVWASI**, but it is unsafe.

### **Enable Multi-Module feature**

- **WAMR_BUILD_MULTI_MODULE**=1/0, default to off.

> [!NOTE]
> See [Multiple Modules as Dependencies](./multi_module.md) for details.

> [!WARNING]
> The multi-module feature is not supported in fast-jit or llvm-jit modes.

### **Enable WASM mini loader**

- **WAMR_BUILD_MINI_LOADER**=1/0, default to off.

> [!NOTE]
> The mini loader skips integrity checks on the WASM binary. Make sure the file is valid yourself.

### **Enable shared memory feature**

- **WAMR_BUILD_SHARED_MEMORY**=1/0, default to off.

### **Enable bulk memory feature**

- **WAMR_BUILD_BULK_MEMORY**=1/0, default to off.

### **Enable memory64 feature**

- **WAMR_BUILD_MEMORY64**=1/0, default to off.

> [!WARNING]
> Supported only in classic interpreter mode and AOT mode.

### **Enable thread manager**

- **WAMR_BUILD_THREAD_MGR**=1/0, default to off.

### **Enable lib-pthread**

- **WAMR_BUILD_LIB_PTHREAD**=1/0, default to off.

> [!NOTE]
> When you enable lib pthread, required features such as `shared memory` and `thread manager` are enabled automatically. See [WAMR pthread library](./pthread_library.md) for details.

### **Enable lib-pthread-semaphore**

- **WAMR_BUILD_LIB_PTHREAD_SEMAPHORE**=1/0, default to off.

> [!NOTE]
> This depends on `lib-pthread` and turns it on automatically.

### **Enable lib wasi-threads**

- **WAMR_BUILD_LIB_WASI_THREADS**=1/0, default to off.

> [!NOTE]
> Enabling lib wasi-threads also enables its dependencies `shared memory` and `thread manager`. See [wasi-threads](./pthread_impls.md#wasi-threads-new) and [Introduction to WAMR WASI threads](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-wasi-threads) for details.

### **Enable lib wasi-nn**

- **WAMR_BUILD_WASI_NN**=1/0, default to off.

> [!NOTE]
> Using WAMR_BUILD_WASI_NN without WAMR_BUILD_WASI_EPHEMERAL_NN is deprecated and may be removed later. Please enable WAMR_BUILD_WASI_EPHEMERAL_NN too. See [WASI-NN](../core/iwasm/libraries/wasi-nn) for details.

### **Enable lib wasi-nn GPU mode**

- **WAMR_BUILD_WASI_NN_ENABLE_GPU**=1/0, default to off.

### **Enable lib wasi-nn external delegate mode**

- **WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE**=1/0, default to off.

- **WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH**=Path to the external delegate shared library (for example `libedgetpu.so.1.0` for Coral USB).

### **Enable lib wasi-nn with `wasi_ephemeral_nn` module support**

- **WAMR_BUILD_WASI_EPHEMERAL_NN**=1/0, default to on.

### **Disable boundary check with hardware trap**

- **WAMR_DISABLE_HW_BOUND_CHECK**=1/0, default to on if the platform supports it.

> [!NOTE]
> By default only [linux/darwin/android/windows/vxworks 64-bit](https://github.com/bytecodealliance/wasm-micro-runtime/blob/5fb5119239220b0803e7045ca49b0a29fe65e70e/core/shared/platform/linux/platform_internal.h#L81) platforms enable this hardware trap boundary check. On 32-bit platforms it is off even if the flag is 0. The wamrc tool omits boundary check instructions in AOT code for all 64-bit targets except SGX to improve speed. The boundary check covers linear memory access and native stack access unless `WAMR_DISABLE_STACK_HW_BOUND_CHECK` is set.

### **Disable native stack boundary check with hardware trap**

- **WAMR_DISABLE_STACK_HW_BOUND_CHECK**=1/0, default to on if the platform supports it; same rule as `WAMR_DISABLE_HW_BOUND_CHECK`.

> [!NOTE]
> If hardware trap boundary checks are off (or `WAMR_DISABLE_HW_BOUND_CHECK` is 1), native stack boundary checks are also off regardless of `WAMR_DISABLE_STACK_HW_BOUND_CHECK`. If hardware trap boundary checks are on, this setting decides whether the native stack check is on.

### **Disable async wakeup of blocking operation**

- **WAMR_DISABLE_WAKEUP_BLOCKING_OP**=1/0, default to on when the platform supports it.

> [!NOTE]
> This feature lets blocking threads terminate asynchronously. If you disable it, blocking threads may never finish when asked to exit.

### **Enable tail call feature**

- **WAMR_BUILD_TAIL_CALL**=1/0, default to off.

### **Enable 128-bit SIMD feature**

- **WAMR_BUILD_SIMD**=1/0, default to on.

> [!WARNING]
> Supported in AOT, JIT, and fast-interpreter modes with the SIMDe library.

### **Enable SIMDe library for SIMD in fast interpreter**

- **WAMR_BUILD_LIB_SIMDE**=1/0, default to off.

> [!NOTE]
> When enabled, SIMDe (SIMD Everywhere) implements SIMD operations in fast interpreter mode.

### **Enable Exception Handling**

- **WAMR_BUILD_EXCE_HANDLING**=1/0, default to off.

> [!WARNING]
> Exception handling currently works only in classic interpreter mode.

### **Enable Garbage Collection**

- **WAMR_BUILD_GC**=1/0, default to off.

> [!WARNING]
> Garbage collection is not supported in fast-jit mode.

### **Set the Garbage Collection heap size**

- **WAMR_BUILD_GC_HEAP_SIZE_DEFAULT**=n, default to 128 kB (131072).

### **Enable Multi Memory**

- **WAMR_BUIL_MULTI_MEMORY**=1/0, default to off.

> [!WARNING]
> Multi memory is supported only in classic interpreter mode.

### **Configure Debug**

- **WAMR_BUILD_CUSTOM_NAME_SECTION**=1/0: load function names from the custom name section. Default is off.

### **Enable AOT stack frame feature**

- **WAMR_BUILD_AOT_STACK_FRAME**=1/0, default to off.

> [!NOTE]
> When enabled, AOT or JIT stack frames (similar to classic interpreter frames but storing only what is needed) are built during calls. Add `--enable-dump-call-stack` to wamrc when compiling AOT modules.

### **Enable dump call stack feature**

- **WAMR_BUILD_DUMP_CALL_STACK**=1/0, default to off.

> [!NOTE]
> When enabled, the runtime dumps the call stack on exceptions.
>
> - In interpreter mode, names come first from the custom name section. If that section is absent or disabled, names come from import/export sections.
> - In AOT/JIT mode, names come from the import/export section. Export as many functions as possible (for `wasi-sdk` you can use `-Wl,--export-all`) when compiling the wasm module, and add `--enable-dump-call-stack --emit-custom-sections=name` to wamrc when compiling the AOT module.

### **Enable memory profiling (Experiment)**

- **WAMR_BUILD_MEMORY_PROFILING**=1/0, default to off.

> [!NOTE]
> When enabled, call `void wasm_runtime_dump_mem_consumption(wasm_exec_env_t exec_env)` to dump memory usage. Currently only module, module_instance, and exec_env memory are measured; other components such as `wasi-ctx`, `multi-module`, and `thread-manager` are not included. See [Memory usage estimation for a module](./memory_usage.md).

### **Enable performance profiling (Experiment)**

- **WAMR_BUILD_PERF_PROFILING**=1/0, default to off.

> [!NOTE]
> When enabled, call `void wasm_runtime_dump_perf_profiling(wasm_module_inst_t module_inst)` to dump per-function performance. Function name lookup follows the same order as the dump call stack feature. See [Tune the performance of running wasm/aot file](./perf_tune.md).

### **Enable the global heap**

- **WAMR_BUILD_GLOBAL_HEAP_POOL**=1/0, default to off for _iwasm_ apps except on Alios and Zephyr.

> [!NOTE] > **WAMR_BUILD_GLOBAL_HEAP_POOL** applies to _iwasm_ apps in `product-mini`. For your own host app, set `mem_alloc_type` to `Alloc_With_Pool` if you want to use a global heap. The global heap is described in [Memory model and memory usage tunning](memory_tune.md).

### **Set the global heap size**

- **WAMR_BUILD_GLOBAL_HEAP_SIZE**=n, default to 10 MB (10485760) for _iwasm_ apps, except Alios (256 kB), Riot (256 kB), and Zephyr (128 kB).

> [!NOTE] > **WAMR_BUILD_GLOBAL_HEAP_SIZE** applies to _iwasm_ apps in `product-mini`. For your host app, set `mem_alloc_option.pool` with the size you want for the global heap. The global heap is described in [Memory model and memory usage tunning](memory_tune.md).

### **Set maximum app thread stack size**

- **WAMR_APP_THREAD_STACK_SIZE_MAX**=n, default to 8 MB (8388608).

> [!NOTE]
> AOT boundary checks with hardware traps may use large stacks because the OS can grow stacks lazily when a guard page is hit. Use this setting to cap total stack use, for example `-DWAMR_APP_THREAD_STACK_SIZE_MAX=131072` (128 KB).

### **Set vprintf callback**

- **WAMR_BH_VPRINTF**=<vprintf_callback>, default to off.

> [!NOTE]
> If you provide `vprintf_callback`, `os_printf()` and `os_vprintf()` on Linux, Darwin, Windows, VxWorks, Android, and esp-idf, plus WASI libc output, call your callback instead of libc `vprintf()`. Example outside the runtime lib:
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
> Then run `cmake -DWAMR_BH_VPRINTF=my_vprintf ..`, or add the compiler macro `BH_VPRINTF=my_vprintf` (for example `add_definitions(-DBH_VPRINTF=my_vprintf)` in CMakeLists.txt). See [basic sample](../samples/basic/src/main.c) for an example.

### **WAMR_BH_LOG**=<log_callback>, default to off.

> [!NOTE]
> If you provide `log_callback`, WAMR logs go there. Example:
>
> ```C
> void my_log(uint32 log_level, const char *file, int line, const char *fmt, ...)
> {
>     /* Usage of custom logger */
> }
> ```
>
> See [basic sample](../samples/basic/src/main.c) for an example.

### **Enable reference types feature**

- **WAMR_BUILD_REF_TYPES**=1/0, default to on.

### **Exclude WAMR application entry functions**

- **WAMR_DISABLE_APP_ENTRY**=1/0, default to off.

> [!NOTE]
> The WAMR application entry (`core/iwasm/common/wasm_application.c`) wraps common steps to instantiate and run wasm functions and print results. These use platform APIs. Enable this flag to skip the file if your platform lacks those APIs. _Do not enable this flag when building `product-mini`._

### **Enable source debugging features**

- **WAMR_BUILD_DEBUG_INTERP**=1/0, default to 0.

> [!NOTE]
> Source debugging needs extra setup. See [source_debugging.md](./source_debugging.md) and [WAMR source debugging basic](https://bytecodealliance.github.io/wamr.dev/blog/wamr-source-debugging-basic).

### **Enable load wasm custom sections**

- **WAMR_BUILD_LOAD_CUSTOM_SECTION**=1/0, default to off.

> [!NOTE]
> By default, custom sections are ignored. Enable `WAMR_BUILD_LOAD_CUSTOM_SECTION` so the embedder can read them via `wasm_runtime_get_custom_section`. If `WAMR_BUILD_CUSTOM_NAME_SECTION` is on, the custom name section is consumed by the runtime and unavailable to the embedder. For AoT files, pass `--emit-custom-sections` to wamrc to keep the sections; otherwise they are dropped.

### **Stack guard size**

- **WAMR_BUILD_STACK_GUARD_SIZE**=n, default to N/A when not set.

> [!NOTE]
> By default, stack guard size is 1K (1024) or 24K when uvwasi is enabled.

### **Disable writing the linear memory base address to x86 GS segment register**

- **WAMR_DISABLE_WRITE_GS_BASE**=1/0, default to on if the platform supports it.

> [!NOTE]
> By default only [linux x86-64](https://github.com/bytecodealliance/wasm-micro-runtime/blob/5fb5119239220b0803e7045ca49b0a29fe65e70e/core/shared/platform/linux/platform_internal.h#L67) enables this. On 32-bit platforms it stays off even if set to 0. On linux x86-64, writing the linear memory base to the GS segment can speed up linear memory access for LLVM AOT/JIT when `--enable-segue=[<flags>]` is passed to `wamrc` or `iwasm`.
>
> See [Enable segue optimization for wamrc when generating the aot file](./perf_tune.md#3-enable-segue-optimization-for-wamrc-when-generating-the-aot-file) for details.

### **User defined linear memory allocator**

- **WAMR_BUILD_ALLOC_WITH_USAGE**=1/0, default to off.
- **WAMR_BUILD_ALLOC_WITH_USER_DATA**=1/0, default to off.

> [!NOTE]
> By default, the system allocates linear memory. With this on and `Alloc_With_Allocator` selected, you can provide your own allocator.

### **Enable running PGO(Profile-Guided Optimization) instrumented AOT file**

- **WAMR_BUILD_STATIC_PGO**=1/0, default to off.

> [!NOTE]
> See [Use the AOT static PGO method](./perf_tune.md#5-use-the-aot-static-pgo-method).

### **Enable linux perf support**

- **WAMR_BUILD_LINUX_PERF**=1/0: enable linux perf support to generate flamegraphs for wasm app performance. Default is off.

> [!NOTE]
> See [Use linux-perf](./perf_tune.md#7-use-linux-perf).

### **Enable module instance context APIs**

- **WAMR_BUILD_MODULE_INST_CONTEXT**=1/0: enable module instance context APIs so the embedder can set one or more contexts for a wasm module instance. Default is on.

```C
    wasm_runtime_create_context_key
    wasm_runtime_destroy_context_key
    wasm_runtime_set_context
    wasm_runtime_set_context_spread
    wasm_runtime_get_context
```

> [!NOTE]
> See [wasm_export.h](../core/iwasm/include/wasm_export.h) for details.

### **Enable quick AOT/JTI entries**

- **WAMR_BUILD_QUICK_AOT_ENTRY**=1/0: register quick call entries to speed up AOT/JIT function calls. Default is on.

> [!NOTE]
> See [Refine callings to AOT/JIT functions from host native](./perf_tune.md#83-refine-callings-to-aotjit-functions-from-host-native).

### **Enable AOT intrinsics**

- **WAMR_BUILD_AOT_INTRINSICS**=1/0: turn on AOT intrinsic functions. Default is on. AOT code can call these when wamrc uses `--disable-llvm-intrinsics` or `--enable-builtin-intrinsics=<intr1,intr2,...>`.

> [!NOTE]
> See [Tuning the XIP intrinsic functions](./xip.md#tuning-the-xip-intrinsic-functions).

### **Enable extended constant expression**

- **WAMR_BUILD_EXTENDED_CONST_EXPR**=1/0, default to off.

> [!NOTE]
> See [Extended Constant Expressions](https://github.com/WebAssembly/extended-const/blob/main/proposals/extended-const/Overview.md).

### **Enable bulk-memory-opt**

- **WAMR_BUILD_BULK_MEMORY_OPT**=1/0, default to off.

> [!NOTE]
> See [bulk-memory-opt](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#bulk-memory-opt).

### **Enable call-indirect-overlong**

- **WAMR_BUILD_CALL_INDIRECT_OVERLONG**=1/0, default to off.

> [!NOTE]
> See [call-indirect-overlong](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#call-indirect-overlong).

### **Enable Lime1 target**

- **WAMR_BUILD_LIME1**=1/0, default to off.

> [!NOTE]
> See [Lime1](https://github.com/WebAssembly/tool-conventions/blob/main/Lime.md#lime1).

### **Configurable memory access boundary check**

- **WAMR_CONFIGURABLE_BOUNDS_CHECKS**=1/0, default to off.

> [!WARNING]
> When enabled, you can run `iwasm --disable-bounds-checks` to turn off memory access boundary checks in interpreter mode. This is a [privileged feature](#privileged-features); use it carefully.

### **Module instance context APIs**

- **WAMR_BUILD_MODULE_INST_CONTEXT**=1/0, default to off.

> [!NOTE]
> When enabled, you can set contexts created by the embedder for a module instance through these APIs:
>
> ```C
>     wasm_runtime_create_context_key
>     wasm_runtime_destroy_context_key
>     wasm_runtime_set_context
>     wasm_runtime_set_context_spread
>     wasm_runtime_get_context
> ```

### **Shared heap among wasm apps and host native**

- **WAMR_BUILD_SHARED_HEAP**=1/0, default to off.

> [!NOTE]
> When enabled, you can create and attach shared heaps, and the following APIs become available:
>
> ```C
>    wasm_runtime_create_shared_heap
>    wasm_runtime_attach_shared_heap
>    wasm_runtime_detach_shared_heap
>    wasm_runtime_shared_heap_malloc
>    wasm_runtime_shared_heap_free
> ```
>
> A wasm app can call these to use the shared heap attached to its module instance:
>
> ```C
>    void *shared_heap_malloc();
>    void shared_heap_free(void *ptr);
> ```

> [!WARNING]
> The shared-heap feature is not supported in fast-jit mode.

### **Shrunk the memory usage**

- **WAMR_BUILD_SHRUNK_MEMORY**=1/0, default to on.

> [!NOTE]
> When enabled, this reduces memory by shrinking linear memory, especially when `memory.grow` is unused and memory needs are predictable.

## **Instruction metering**

- **WAMR_BUILD_INSTRUCTION_METERING**=1/0, default to off.

> [!NOTE]
> This limits the number of instructions a wasm module instance can run. Call `wasm_runtime_set_instruction_count_limit(...)` before `wasm_runtime_call_*(...)` to enforce the cap.

## **Combination of configurations:**

You can mix settings. For example, to disable the interpreter, enable AOT and WASI, run:

```Bash
cmake .. -DWAMR_BUILD_INTERP=0 -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_LIBC_WASI=1 -DWAMR_BUILD_PLATFORM=linux
```

To enable the interpreter, disable AOT and WASI, and target X86_32, run:

```Bash
cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_BUILD_TARGET=X86_32
```

When enabling SIMD for fast interpreter mode, turn on both SIMD and the SIMDe library:

```Bash

cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=1 -DWAMR_BUILD_SIMD=1 -DWAMR_BUILD_LIB_SIMDE=1
```

For Valgrind, start with these and add more as needed:

```Bash
  #...
  -DCMAKE_BUILD_TYPE=Debug \
  -DWAMR_DISABLE_HW_BOUND_CHECK=0 \
  -DWAMR_DISABLE_WRITE_GS_BASE=0
  #...
```

To enable the minimal Lime1 feature set, turn off features that are on by default such as bulk memory and reference types:

```Bash
cmake .. -DWAMR_BUILD_LIME1=1 -DWAMR_BUILD_BULK_MEMORY=0 -DWAMR_BUILD_REF_TYPES=0 -DDWAMR_BUILD_SIMD=0
```
