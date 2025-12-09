# Tiered Support

Tier Definitions

## Tier A

This tier is the highest level of support. Features and targets in this tier are fully supported, actively maintained, and regularly tested. Users can expect prompt assistance and comprehensive documentation for any issues or questions related to these features. Users can rely on Tier A features for production environments. Targets in this tier usually have been landed in products.

## Tier B

This tier represents a moderate level of support. Features and targets in this tier are generally supported and maintained, but may not receive the same level of attention as Tier A. While efforts are made to ensure stability, users may encounter occasional issues that are not immediately addressed. Documentation may be less comprehensive compared to Tier A. Users are encouraged to report any issues they encounter, but response times may vary.

## Tier C

This tier indicates a basic level of support. Features and targets in this tier are considered experimental or less stable. They may not be actively maintained, and users should be prepared for potential issues or limitations. Documentation may be minimal or outdated. Users opting to use Tier C features do so at their own risk and should be prepared to troubleshoot issues independently. These features are typically not recommended for production use.

- _tested_ mentioned above specifically refers to whether there are enough tests in CI.
- _actively maintained_ means that the code is regularly updated to fix bugs, improve performance, and ensure compatibility with other components.
- _fully supported_ means that users can expect timely assistance, comprehensive documentation, and regular updates for any issues or questions related to these features.

There are some

# Tiered Features and Targets

_Runtime Extensions_ are features that extend the runtime capabilities of the system beyond the core WebAssembly specification. These extensions may include optimizations, additional APIs, or other enhancements that improve performance, usability, or functionality.

_Privileged Features_ are features that require users' awareness of potential security implications. But brings significant benefits on performance, functionality, or other aspects. These features may introduce risks or challenges that users should consider before enabling them in their applications. The use of privileged features requires additional configuration, testing, or monitoring to ensure safe and effective operation.

## Privileged Features

## TierA

| Description                      | Compilation Flags                                                        | Labels             |
| -------------------------------- | ------------------------------------------------------------------------ | ------------------ |
| x86_64-pc-linux-gnu              | N/A                                                                      | Target             |
| i386-pc-linux-gnu                | N/A                                                                      | Target             |
| aarch64-none-?                   | N/A                                                                      | Target             |
| x86_64-none-linux-gnu            | N/A                                                                      | Target             |
| Bulk Memory                      | [WAMR_BUILD_BULK_MEMORY](./build_wamr.md#enable-bulk-memory-feature)     | Wasm Proposal      |
| Extended Constant Expressions    | [WAMR_BUILD_EXTENDED_CONST_EXPR](./build_wamr.md#configure-interpreters) | Wasm Proposal      |
| Import/Export of Mutable Globals | ALWAYS ON                                                                | Wasm Proposal      |
| Memory64                         | [WAMR_BUILD_MEMORY64](./build_wamr.md#enable-memory64-feature)           | Wasm Proposal      |
| Multi-value                      | ALWAYS ON                                                                | Wasm Proposal      |
| Non-trapping float-to-int        | ALWAYS ON                                                                | Wasm Proposal      |
| Reference Types                  | [WAMR_BUILD_REF_TYPES](./build_wamr.md#configure-interpreters)           | Wasm Proposal      |
| Shared Memory (Threads)          | [WAMR_BUILD_SHARED_MEMORY](./build_wamr.md#enable-shared-memory-feature) | Wasm Proposal      |
| SIMD (128-bit)                   | [WAMR_BUILD_SIMD](./build_wamr.md#enable-128-bit-simd-feature)           | Wasm Proposal      |
| Sign-extension Operators         | ALWAYS ON                                                                | Wasm Proposal      |
| Wasm C API                       | ALWAYS ON                                                                | Wasm Proposal      |
| WASI Libc                        | [WAMR_BUILD_LIBC_WASI](./build_wamr.md#configure-libc)                   | Wasm Proposal      |
| AoT (wamrc)                      | [WAMR_BUILD_AOT](./build_wamr.md#configure-aot-and-jits)                 | Runtime Extensions |
| AOT intrinsics                   | [WAMR_BUILD_AOT_INTRINSICS](./build_wamr.md#configure-aot-and-jits)      | Runtime Extensions |
| Fast Interpreter                 | [WAMR_BUILD_FAST_INTERP](./build_wamr.md#configure-interpreters)         | Runtime Extensions |
| Interpreter (classic)            | [WAMR_BUILD_INTERP](./build_wamr.md#configure-interpreters)              | Runtime Extensions |
| Libc builtin                     | [WAMR_BUILD_LIBC_BUILTIN](./build_wamr.md#configure-libc)                | Runtime Extensions |
| Quick AOT/JIT entries            | [WAMR_BUILD_QUICK_AOT_ENTRY](./build_wamr.md#configure-aot-and-jits)     | Runtime Extensions |
| Shrunk memory                    | [WAMR_BUILD_SHRUNK_MEMORY](./build_wamr.md#enable-shared-memory-feature) | Runtime Extensions |
| Wakeup of blocking operations    | N/A                                                                      | Runtime Extensions |

## TierB

| Description                   | Compilation Flags                                                                    | Labels             |
| ----------------------------- | ------------------------------------------------------------------------------------ | ------------------ |
| ARC                           | N/A                                                                                  | Target             |
| ARM                           | N/A                                                                                  | Target             |
| RISCV32                       | N/A                                                                                  | Target             |
| RISCV64                       | N/A                                                                                  | Target             |
| THUMB                         | N/A                                                                                  | Target             |
| XTENSA                        | N/A                                                                                  | Target             |
| Android                       | N/A                                                                                  | OS                 |
| macOS                         | N/A                                                                                  | OS                 |
| Windows                       | N/A                                                                                  | OS                 |
| Zephyr                        | N/A                                                                                  | OS                 |
| GC (Garbage Collection)       | [WAMR_BUILD_GC](./build_wamr.md#enable-garbage-collection)                           | Wasm Proposal      |
| Stringref                     | [WAMR_BUILD_STRINGREF](./build_wamr.md#configure-debug)                              | Wasm Proposal      |
| Tail Calls                    | [WAMR_BUILD_TAIL_CALL](./build_wamr.md#enable-tail-call-feature)                     | Wasm Proposal      |
| Fast JIT                      | [WAMR_BUILD_FAST_JIT](./build_wamr.md#configure-aot-and-jits)                        | Runtime Extensions |
| LLVM JIT                      | [WAMR_BUILD_JIT](./build_wamr.md#configure-aot-and-jits)                             | Runtime Extensions |
| Memory profiling              | [WAMR_BUILD_MEMORY_PROFILING](./build_wamr.md#enable-memory-profiling-experiment)    | Runtime Extensions |
| Module instance context       | [WAMR_BUILD_MODULE_INST_CONTEXT](./build_wamr.md#enable-multi-module-feature)        | Runtime Extensions |
| Multi-module                  | [WAMR_BUILD_MULTI_MODULE](./build_wamr.md#enable-multi-module-feature)               | Runtime Extensions |
| Perf profiling                | [WAMR_BUILD_PERF_PROFILING](./build_wamr.md#enable-performance-profiling-experiment) | Runtime Extensions |
| Pthread                       | [WAMR_BUILD_LIB_PTHREAD](./build_wamr.md#enable-lib-pthread)                         | Runtime Extensions |
| Shared heap                   | [WAMR_BUILD_SHARED_HEAP](./build_wamr.md#enable-shared-memory-feature)               | Runtime Extensions |
| WASI threads                  | [WAMR_BUILD_LIB_WASI_THREADS](./build_wamr.md#enable-lib-wasi-threads)               | Runtime Extensions |
| WASI-NN (neural network APIs) | [WAMR_BUILD_WASI_NN](./build_wamr.md#enable-lib-wasi-nn)                             | Runtime Extensions |
| Debug Interpreter             | [WAMR_BUILD_DEBUG_INTERP](./build_wamr.md#configure-debug)                           | Runtime Extensions |

## TierC

| Description                   | Compilation Flags                                                     | Labels             |
| ----------------------------- | --------------------------------------------------------------------- | ------------------ |
| MIPS                          | N/A                                                                   | Target             |
| AliOS-Things                  | N/A                                                                   | OS                 |
| Cosmopolitan                  | N/A                                                                   | OS                 |
| ESP-IDF (FreeRTOS)            | N/A                                                                   | OS                 |
| FreeBSD                       | N/A                                                                   | OS                 |
| iOS                           | N/A                                                                   | OS                 |
| RT-Thread                     | N/A                                                                   | OS                 |
| RIOT                          | N/A                                                                   | OS                 |
| VxWorks                       | N/A                                                                   | OS                 |
| Multi-memory                  | [WAMR_BUILD_MULTI_MEMORY](./build_wamr.md#enable-multi-memory)        | Wasm Proposal      |
| Legacy Exception Handling     | [WAMR_BUILD_EXCE_HANDLING](./build_wamr.md#enable-exception-handling) | Wasm Proposal      |
| Debug AOT                     | [WAMR_BUILD_DEBUG_AOT](./build_wamr.md#configure-debug)               | Runtime Extensions |
| Tier-up (Fast JIT → LLVM JIT) | [WAMR_BUILD_DYNAMIC_AOT_DEBUG](./build_wamr.md#configure-debug)       | Runtime Extensions |

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
