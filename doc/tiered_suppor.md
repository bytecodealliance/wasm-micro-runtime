# Tiered Supported

**Tier definitions**

- **A — Production Ready:** fully tested and stable.
- **B — Almost Production Ready:** partially tested; close to production.
- **C — Experimental / Not Production Ready:** unfinished or volatile.

## Architecture Support

| Architecture | Tier  |
| ------------ | ----- |
| **x86-64**   | **A** |
| **x86-32**   | **A** |
| AArch64      | B     |
| ARC          | B[^1] |
| ARM          | B     |
| RISCV32      | B     |
| RISCV64      | B     |
| THUMB        | B     |
| MIPS         | C     |
| XTENSA       | C     |

[^1]: will upgrade to **A** after further testing and validation.

## OS / Platform Support

| Platform           | Tier  |
| ------------------ | ----- |
| **NuttX**          | **A** |
| **Ubuntu**         | **A** |
| Android            | B     |
| macOS              | B     |
| Windows            | B     |
| Zephyr             | B[^2] |
| AliOS-Things       | C     |
| Cosmopolitan       | C     |
| ESP-IDF (FreeRTOS) | C     |
| FreeBSD            | C     |
| iOS                | C     |
| RT-Thread          | C     |
| RIOT               | C     |
| VxWorks            | C     |

[^2]: will upgrade to **A** after further testing and validation.

## WebAssembly Proposal Support

> Defaults below reflect the sample build configuration you provided (e.g., `WAMR_BUILD_*` values), not necessarily the release bundle. “Always-on” items are part of WAMR’s baseline.

| WASM Proposal / Extension              | Tier        | Default |
| -------------------------------------- | ----------- | ------- |
| **Bulk Memory**                        | A           | **On**  |
| **Extended Constant Expressions**      | A           | Off     |
| **Import/Export of Mutable Globals**   | A           | **On**  |
| **Memory64**                           | A           | Off     |
| **Multi-value**                        | A           | **On**  |
| **Non-trapping float-to-int**          | A           | **On**  |
| **Reference Types**                    | A           | **On**  |
| **Shared Memory (Threads)**            | A           | Off     |
| **SIMD (128-bit)**                     | A           | **On**  |
| **Sign-extension Operators**           | A           | **On**  |
| GC (Garbage Collection)                | B           | Off     |
| Stringref                              | B           | Off     |
| Tail Calls                             | B           | Off     |
| Multi-memory                           | C           | Off     |
| Legacy Exception Handling              | C           | Off     |
| Branch Hinting                         | Unsupported |         |
| Custom Annotation Syntax (text format) | Unsupported |         |
| Exception Handling (new spec)          | Unsupported |         |
| JS String Builtins                     | Unsupported |         |
| Relaxed SIMD                           | Unsupported |         |

# WAMR-Specific Feature Support

> Defaults below mirror your sample build output (e.g., “enabled/disabled” lines) and common WAMR options.

| WAMR Feature                      | Tier | Default |
| --------------------------------- | ---- | ------- |
| **AoT (wamrc)**                   | A    | **On**  |
| **AOT intrinsics**                | A    | **On**  |
| **Fast Interpreter**              | A    | **Off** |
| **Interpreter (classic)**         | A    | **On**  |
| **Libc builtin**                  | A    | **On**  |
| **Libc WASI**                     | A    | **On**  |
| **Quick AOT/JIT entries**         | A    | **On**  |
| **Shrunk memory**                 | A    | **On**  |
| **Wakeup of blocking operations** | A    | **On**  |
| **WASM C API**                    | A    | **On**  |
| Fast JIT                          | B    | Off     |
| LLVM ORC JIT                      | B    | Off     |
| Memory profiling                  | B    | Off     |
| Module instance context[^7]       | B    | On      |
| Multi-module                      | B    | Off     |
| Perf profiling                    | B    | Off     |
| Pthread                           | B    | Off     |
| Shared heap                       | B    | Off     |
| WASI threads                      | B    | Off     |
| WASI-NN (neural network APIs)     | B    | Off     |
| Debug Interpreter                 | B    | Off     |
| Debug AOT                         | C    | Off     |
| Tier-up (Fast JIT → LLVM JIT)     | C    | Off     |

---

# Appendix: All compilation flags

| Compilation flags                           | Tiered | Default | on Ubuntu |
| ------------------------------------------- | ------ | ------- | --------- |
| WAMR_APP_THREAD_STACK_SIZE_MAX              | B      | ND[^3]  |           |
| WAMR_BH_LOG                                 | B      | ND      |           |
| WAMR_BH_VPRINTF                             | B      | ND      |           |
| WAMR_BUILD_ALLOC_WITH_USAGE                 | B      | ND      |           |
| WAMR_BUILD_ALLOC_WITH_USER_DATA             | B      | ND      |           |
| WAMR_BUILD_AOT                              | A      | ND      | 1         |
| WAMR_BUILD_AOT_INTRINSICS                   | A      | 1[^4]   |           |
| WAMR_BUILD_AOT_STACK_FRAME                  | A      | ND      |           |
| WAMR_BUILD_AOT_VALIDATOR                    | B      | ND      |           |
| WAMR_BUILD_BULK_MEMORY                      | A      | 1       |           |
| WAMR_BUILD_COPY_CALL_STACK                  | B      | ND      |           |
| WAMR_BUILD_CUSTOM_NAME_SECTION              | B      | ND      |           |
| WAMR_BUILD_DEBUG_AOT                        | C      | ND      |           |
| WAMR_BUILD_DEBUG_INTERP                     | B      | ND      |           |
| WAMR_BUILD_DUMP_CALL_STACK                  | B      | ND      |           |
| WAMR_BUILD_DYNAMIC_AOT_DEBUG                | C      | ND      |           |
| WAMR_BUILD_EXCE_HANDLING                    | C      | 0       |           |
| WAMR_BUILD_EXTENDED_CONST_EXPR              | A      | 0       |           |
| WAMR_BUILD_FAST_INTERP                      | A      | ND      | 1         |
| WAMR_BUILD_FAST_JIT                         | B      | ND      |           |
| WAMR_BUILD_FAST_JIT_DUMP                    | B      | ND      |           |
| WAMR_BUILD_GC                               | B      | 0       |           |
| WAMR_BUILD_GC_HEAP_VERIFY                   | B      | ND      |           |
| WAMR_BUILD_GLOBAL_HEAP_POOL                 | A      | ND      |           |
| WAMR_BUILD_GLOBAL_HEAP_SIZE                 | A      | ND      |           |
| WAMR_BUILD_INSTRUCTION_METERING             | C      | ND      |           |
| WAMR_BUILD_INTERP                           | A      | ND      | 1         |
| WAMR_BUILD_INVOKE_NATIVE_GENERAL            | B      | ND      |           |
| WAMR_BUILD_JIT                              | B      | ND      |           |
| WAMR_BUILD_LAZY_JIT                         | B      | 1[^5]   |           |
| WAMR_BUILD_LIBC_BUILTIN                     | A      | ND      | 1         |
| WAMR_BUILD_LIBC_EMCC                        | C      | ND      |           |
| WAMR_BUILD_LIBC_UVWASI                      | C      | ND      |           |
| WAMR_BUILD_LIBC_WASI                        | A      | ND      | 1         |
| WAMR_BUILD_LIB_PTHREAD                      | B      | ND      |           |
| WAMR_BUILD_LIB_PTHREAD_SEMAPHORE            | B      | ND      |           |
| WAMR_BUILD_LIB_RATS                         | C      | ND      |           |
| WAMR_BUILD_LIB_WASI_THREADS                 | B      | ND      |           |
| WAMR_BUILD_LINUX_PERF                       | B      | ND      |           |
| WAMR_BUILD_LOAD_CUSTOM_SECTION              | A      | ND      |           |
| WAMR_BUILD_MEMORY64                         | A      | 0       |           |
| WAMR_BUILD_MEMORY_PROFILING                 | B      | ND      |           |
| WAMR_BUILD_MINI_LOADER                      | B      | ND      |           |
| WAMR_BUILD_MODULE_INST_CONTEXT              | B      | ND      | 1         |
| WAMR_BUILD_MULTI_MEMORY                     | C      | 0       |           |
| WAMR_BUILD_MULTI_MODULE                     | B      | ND      |           |
| WAMR_BUILD_PERF_PROFILING                   | B      | ND      |           |
| WAMR_BUILD_PLATFORM                         | -      | ND      | linux     |
| WAMR_BUILD_QUICK_AOT_ENTRY                  | A      | 1[^6]   |           |
| WAMR_BUILD_REF_TYPES                        | A      | ND      | 1         |
| WAMR_BUILD_SANITIZER                        | B      | ND      |           |
| WAMR_BUILD_SGX_IPFS                         | C      | ND      |           |
| WAMR_BUILD_SHARED_HEAP                      | A      | ND      |           |
| WAMR_BUILD_SHARED_MEMORY                    | A      | 0       | 1         |
| WAMR_BUILD_SHRUNK_MEMORY                    | A      | ND      | 1         |
| WAMR_BUILD_SIMD                             | A      | ND      | 1         |
| WAMR_BUILD_SIMDE                            | A      | ND      | 1         |
| WAMR_BUILD_SPEC_TEST                        | A      | ND      |           |
| WAMR_BUILD_STACK_GUARD_SIZE                 | B      | ND      |           |
| WAMR_BUILD_STATIC_PGO                       | B      | ND      |           |
| WAMR_BUILD_STRINGREF                        | B      | 0       |           |
| WAMR_BUILD_TAIL_CALL                        | A      | 0       | 1         |
| WAMR_BUILD_TARGET                           | -      | ND      | X86-64    |
| WAMR_BUILD_THREAD_MGR                       | A      | ND      |           |
| WAMR_BUILD_WAMR_COMPILER                    | A      | ND      |           |
| WAMR_BUILD_WASI_EPHEMERAL_NN                | B      | ND      |           |
| WAMR_BUILD_WASI_NN                          | B      | ND      |           |
| WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE | B      | ND      |           |
| WAMR_BUILD_WASI_NN_ENABLE_GPU               | B      | ND      |           |
| WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH   | B      | ND      |           |
| WAMR_BUILD_WASI_NN_LLAMACPP                 | B      | ND      |           |
| WAMR_BUILD_WASI_NN_ONNX                     | B      | ND      |           |
| WAMR_BUILD_WASI_NN_OPENVINO                 | B      | ND      |           |
| WAMR_BUILD_WASI_NN_TFLITE                   | B      | ND      |           |
| WAMR_BUILD_WASI_TEST                        | B      | ND      |           |
| WAMR_BUILD_WASM_CACHE                       | B      | ND      |           |
| WAMR_CONFIGURABLE_BOUNDS_CHECKS             | C      | ND      |           |
| WAMR_DISABLE_APP_ENTRY                      | A      | ND      |           |
| WAMR_DISABLE_HW_BOUND_CHECK                 | A      | ND      |           |
| WAMR_DISABLE_STACK_HW_BOUND_CHECK           | A      | ND      |           |
| WAMR_DISABLE_WAKEUP_BLOCKING_OP             | B      | ND      |           |
| WAMR_DISABLE_WRITE_GS_BASE                  | B      | ND      |           |
| WAMR_TEST_GC                                | B      | ND      |           |

[^3]: _ND_ represents _not defined_
[^4]: active if `WAMR_BUILD_AOT` is 1
[^5]: active if `WAMR_BUILD_FAST_JIT` or `WAMR_BUILD_JIT1` is 1
[^6]: active if `WAMR_BUILD_AOT` or `WAMR_BUILD_JIT` is 1
[^7]: required by Libc WASI
