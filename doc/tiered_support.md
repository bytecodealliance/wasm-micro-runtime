# Tiered Supported

**Tier definitions**

- **A — Production Ready:** fully tested and stable.
- **B — Almost Production Ready:** partially tested; close to production.
- **C — Experimental / Not Production Ready:** unfinished or volatile.

The condition _tested_ mentioned above specifically refers to whether there are enough tests in CI.

## Architecture Support

| Architecture | Tier  |
| ------------ | ----- |
| **x86-64**   | **A** |
| **x86-32**   | **A** |
| AArch64      | B     |
| ARC          | B     |
| ARM          | B     |
| RISCV32      | B     |
| RISCV64      | B     |
| THUMB        | B     |
| XTENSA       | B     |
| MIPS         | C     |

## OS / Platform Support

| Platform           | Tier  |
| ------------------ | ----- |
| **NuttX**          | **A** |
| **Ubuntu**         | **A** |
| Android            | B     |
| macOS              | B     |
| Windows            | B     |
| Zephyr             | B     |
| AliOS-Things       | C     |
| Cosmopolitan       | C     |
| ESP-IDF (FreeRTOS) | C     |
| FreeBSD            | C     |
| iOS                | C     |
| RT-Thread          | C     |
| RIOT               | C     |
| VxWorks            | C     |

## WebAssembly Proposal Support

> During configuration, It is able to disable or enable the following features by setting the corresponding flags (see Appendix). It is also possible to check features status in the configuration output.

| WASM Proposal / Extension              | Tier        |
| -------------------------------------- | ----------- |
| **Bulk Memory**                        | A           |
| **Extended Constant Expressions**      | A           |
| **Import/Export of Mutable Globals**   | A           |
| **Memory64**                           | A           |
| **Multi-value**                        | A           |
| **Non-trapping float-to-int**          | A           |
| **Reference Types**                    | A           |
| **Shared Memory (Threads)**            | A           |
| **SIMD (128-bit)**                     | A           |
| **Sign-extension Operators**           | A           |
| GC (Garbage Collection)                | B           |
| Stringref                              | B           |
| Tail Calls                             | B           |
| Multi-memory                           | C           |
| Legacy Exception Handling              | C           |
| Branch Hinting                         | Unsupported |
| Custom Annotation Syntax (text format) | Unsupported |
| Exception Handling (new spec)          | Unsupported |
| JS String Builtins                     | Unsupported |
| Relaxed SIMD                           | Unsupported |

# WAMR-Specific Feature Support

> During configuration, It is able to disable or enable the following features by setting the corresponding flags (see Appendix). It is also possible to check features status in the configuration output.

| WAMR Feature                      | Tier |
| --------------------------------- | ---- |
| **AoT (wamrc)**                   | A    |
| **AOT intrinsics**                | A    |
| **Fast Interpreter**              | A    |
| **Interpreter (classic)**         | A    |
| **Libc builtin**                  | A    |
| **Libc WASI**                     | A    |
| **Quick AOT/JIT entries**         | A    |
| **Shrunk memory**                 | A    |
| **Wakeup of blocking operations** | A    |
| **WASM C API**                    | A    |
| Fast JIT                          | B    |
| LLVM ORC JIT                      | B    |
| Memory profiling                  | B    |
| Module instance context[^7]       | B    |
| Multi-module                      | B    |
| Perf profiling                    | B    |
| Pthread                           | B    |
| Shared heap                       | B    |
| WASI threads                      | B    |
| WASI-NN (neural network APIs)     | B    |
| Debug Interpreter                 | B    |
| Debug AOT                         | C    |
| Tier-up (Fast JIT → LLVM JIT)     | C    |

---

# Appendix: All compilation flags

| Compilation flags                           | Tiered | Default | on Ubuntu |
| ------------------------------------------- | ------ | ------- | --------- |
| WAMR_APP_THREAD_STACK_SIZE_MAX              | B      | ND[^1]  |           |
| WAMR_BH_LOG                                 | B      | ND      |           |
| WAMR_BH_VPRINTF                             | B      | ND      |           |
| WAMR_BUILD_ALLOC_WITH_USAGE                 | B      | ND      |           |
| WAMR_BUILD_ALLOC_WITH_USER_DATA             | B      | ND      |           |
| WAMR_BUILD_AOT                              | A      | ND      | 1         |
| WAMR_BUILD_AOT_INTRINSICS                   | A      | 1[^2]   |           |
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
| WAMR_BUILD_LAZY_JIT                         | B      | 1[^3]   |           |
| WAMR_BUILD_LIBC_BUILTIN                     | A      | ND      | 1         |
| WAMR_BUILD_LIBC_EMCC                        | C      | ND      |           |
| WAMR_BUILD_LIBC_UVWASI                      | C      | ND      |           |
| WAMR_BUILD_LIBC_WASI                        | A      | ND      | 1         |
| WAMR_BUILD_LIB_PTHREAD                      | B      | ND      |           |
| WAMR_BUILD_LIB_PTHREAD_SEMAPHORE            | B      | ND      |           |
| WAMR_BUILD_LIB_RATS                         | C      | ND      |           |
| WAMR_BUILD_LIB_WASI_THREADS                 | B      | ND      |           |
| WAMR_BUILD_LINUX_PERF                       | B      | ND      |           |
| WAMR_BUILD_LIME1                            | A      | NO      |           |
| WAMR_BUILD_LOAD_CUSTOM_SECTION              | A      | ND      |           |
| WAMR_BUILD_MEMORY64                         | A      | 0       |           |
| WAMR_BUILD_MEMORY_PROFILING                 | B      | ND      |           |
| WAMR_BUILD_MINI_LOADER                      | B      | ND      |           |
| WAMR_BUILD_MODULE_INST_CONTEXT              | B      | ND      | 1         |
| WAMR_BUILD_MULTI_MEMORY                     | C      | 0       |           |
| WAMR_BUILD_MULTI_MODULE                     | B      | ND      |           |
| WAMR_BUILD_PERF_PROFILING                   | B      | ND      |           |
| WAMR_BUILD_PLATFORM                         | -      | ND      | linux     |
| WAMR_BUILD_QUICK_AOT_ENTRY                  | A      | 1[^4]   |           |
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

[^1]: _ND_ represents _not defined_
[^2]: active if `WAMR_BUILD_AOT` is 1
[^3]: active if `WAMR_BUILD_FAST_JIT` or `WAMR_BUILD_JIT1` is 1
[^4]: active if `WAMR_BUILD_AOT` or `WAMR_BUILD_JIT` is 1
