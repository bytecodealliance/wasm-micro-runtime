# Tiered Support

## Tier A

This tier is the highest level of support. Features and targets in this tier are fully supported, actively maintained, and regularly tested. Users can expect prompt assistance and comprehensive documentation for any issues or questions related to these features. Users can rely on Tier A features for production environments. Targets in this tier usually have been used in products.

## Tier B

This tier represents a moderate level of support. Features and targets in this tier are generally supported and maintained, but may not receive the same level of attention as Tier A. While efforts are made to ensure stability, users may encounter occasional issues that are not immediately addressed. Documentation may be less comprehensive compared to Tier A. Users are encouraged to report any issues they encounter, but response times may vary.

## Tier C

This tier indicates experimental features with foundational support levels. These implementations are typically optimized for specific platforms, running modes, or use cases, and may not receive active maintenance. Documentation tends to be minimal or require updates. Production deployment requires specialized expertise and careful evaluation, including establishing appropriate threat models and ensuring comprehensive understanding of the implementation scope. Users should be prepared to troubleshoot issues and handle ongoing maintenance independently, accepting full responsibility for any limitations or challenges that may arise.

> [!NOTE]
>
> - **actively maintained** and **fully supported**. users can expect timely assistance, comprehensive documentation, and regular updates for any issues or questions related to these features.
> - **regularly tested**. means there are automated tests in CI that run on a regular basis to ensure the stability and functionality of these features.

## Labels

**Target** refers to the specific hardware architecture or operating system that the runtime can be compiled for and run on.

**Runtime Extensions** are features that extend the runtime capabilities of the system beyond the core WebAssembly specification. These extensions may include optimizations, additional APIs, or other enhancements that improve performance, usability, or functionality.

**Portability** indicates the ability of the runtime to operate across different platforms or environments without requiring significant modifications. This includes compatibility with various operating systems, hardware architectures, and development frameworks.

# TierA

## Targets

| Description                |
| -------------------------- |
| aarch64-unknown-nuttx-eabi |
| i386-pc-linux-gnu          |
| x86_64-pc-linux-gnu        |
| x86_64-apple-darwin        |
| x86_64-none-linux-gnu      |

## Features

| Description                      | Compilation Flags                                                                                           | Labels             |
| -------------------------------- | ----------------------------------------------------------------------------------------------------------- | ------------------ |
| Linux Compatibility              | WAMR_BUILD_PLATFORM=linux                                                                                   | Portability        |
| AoT runtime                      | [WAMR_BUILD_AOT](./build_wamr.md#configure-aot)                                                             | Running mode       |
| Fast Interpreter                 | [WAMR_BUILD_FAST_INTERP](./build_wamr.md#configure-interpreters)                                            | Running mode       |
| Classic Interpreter              | [WAMR_BUILD_INTERP](./build_wamr.md#configure-interpreters)                                                 | Running mode       |
| AoT compilation (wamrc)          | [WAMR_BUILD_WAMR_COMPILER](./build_wamr.md#configure-aot)                                                   | Compiler           |
| Bulk Memory                      | [WAMR_BUILD_BULK_MEMORY](./build_wamr.md#bulk-memory-feature)                                               | Wasm Proposal      |
| Name section                     | [WAMR_BUILD_CUSTOM_NAME_SECTION](./build_wamr.md#name-section)                                              | Wasm Proposal      |
| Extended Constant Expressions    | [WAMR_BUILD_EXTENDED_CONST_EXPR](./build_wamr.md#extended-constant-expression)                              | Wasm Proposal      |
| Non-trapping float-to-int        | ALWAYS ON. Can not be disabled                                                                              | Wasm Proposal      |
| Import/Export of Mutable Globals | ALWAYS ON. Can not be disabled                                                                              | Wasm Proposal      |
| Multi-value                      | ALWAYS ON. Can not be disabled                                                                              | Wasm Proposal      |
| WASI LIBC                        | [WAMR_BUILD_LIBC_WASI](./build_wamr.md#configure-libc)                                                      | Wasm Proposal      |
| WASI threads                     | [WAMR_BUILD_LIB_WASI_THREADS](./build_wamr.md#lib-wasi-threads)                                             | Wasm Proposal      |
| Custom sections                  | [WAMR_BUILD_LOAD_CUSTOM_SECTION](./build_wamr.md#load-wasm-custom-sections)                                 | Wasm Proposal      |
| Memory64                         | [WAMR_BUILD_MEMORY64](./build_wamr.md#memory64-feature)                                                     | Wasm Proposal      |
| Reference Types                  | [WAMR_BUILD_REF_TYPES](./build_wamr.md#reference-types-feature)                                             | Wasm Proposal      |
| Threads                          | [WAMR_BUILD_SHARED_MEMORY](./build_wamr.md#shared-memory-feature)                                           | Wasm Proposal      |
| SIMD (128-bit)                   | [WAMR_BUILD_SIMD](./build_wamr.md#128-bit-simd-feature)                                                     | Wasm Proposal      |
| AOT intrinsics                   | [WAMR_BUILD_AOT_INTRINSICS](./build_wamr.md#aot-intrinsics)                                                 | Runtime Extensions |
| AoT stack frame                  | [WAMR_BUILD_AOT_STACK_FRAME](./build_wamr.md#aot-stack-frame-feature)                                       | Runtime Extensions |
| Global heap pool                 | [WAMR_BUILD_GLOBAL_HEAP_POOL](./build_wamr.md#a-pre-allocation-for-runtime-and-wasm-apps)                   | Runtime Extensions |
| Global heap size                 | [WAMR_BUILD_GLOBAL_HEAP_SIZE](./build_wamr.md#a-pre-allocation-for-runtime-and-wasm-apps)                   | Runtime Extensions |
| Libc builtin                     | [WAMR_BUILD_LIBC_BUILTIN](./build_wamr.md#configure-libc)                                                   | Runtime Extensions |
| Module instance context          | [WAMR_BUILD_MODULE_INST_CONTEXT](./build_wamr.md#module-instance-context-apis)                              | Runtime Extensions |
| Quick AOT/JIT entries            | [WAMR_BUILD_QUICK_AOT_ENTRY](./build_wamr.md#configure-aot)                                                 | Runtime Extensions |
| Shrunk memory                    | [WAMR_BUILD_SHRUNK_MEMORY](./build_wamr.md#shrunk-the-memory-usage)                                         | Runtime Extensions |
| Thread manager                   | [WAMR_BUILD_THREAD_MGR](./build_wamr.md#thread-manager)                                                     | Runtime Extensions |
| App entry                        | [WAMR_DISABLE_APP_ENTRY](./build_wamr.md#exclude-wamr-application-entry-functions)                          | Runtime Extensions |
| hardware bound check             | [WAMR_DISABLE_HW_BOUND_CHECK](./build_wamr.md#disable-boundary-check-with-hardware-trap)                    | Runtime Extensions |
| stack hardware bound check       | [WAMR_DISABLE_STACK_HW_BOUND_CHECK](./build_wamr.md#disable-native-stack-boundary-check-with-hardware-trap) | Runtime Extensions |
| Wakeup blocking operation        | [WAMR_DISABLE_WAKEUP_BLOCKING_OP](./build_wamr.md#disable-async-wakeup-of-blocking-operation)               | Runtime Extensions |

# TierB

## Targets

| Description            |
| ---------------------- |
| arc-unknown-none-elf   |
| x86_64-pc-windows-msvc |
| mips-unknown-elf       |
| mips64-unknown-elf     |

## Features

| Description                        | Compilation Flags                                                                                                       | Labels             |
| ---------------------------------- | ----------------------------------------------------------------------------------------------------------------------- | ------------------ |
| Darwin Compatibility               | WAMR_BUILD_PLATFORM=darwin                                                                                              | Portability        |
| ESP-IDF Compatibility              | WAMR_BUILD_PALTFORM=esp-idf                                                                                             | Portability        |
| Nuttx Compatibility                | WAMR_BUILD_PALTFORM=nuttx                                                                                               | Portability        |
| SGX Compatibility                  | WAMR_BUILD_PALTFORM=linux-sgx                                                                                           | Portability        |
| Zephyr Compatibility               | WAMR_BUILD_PALTFORM=zephyr                                                                                              | Portability        |
| Stringref                          | [WAMR_BUILD_STRINGREF](./build_wamr.md#garbage-collection)                                                              | Wasm Proposal      |
| Tail Calls                         | [WAMR_BUILD_TAIL_CALL](./build_wamr.md#tail-call-feature)                                                               | Wasm Proposal      |
| LLVM JIT                           | [WAMR_BUILD_JIT](./build_wamr.md#configure-llvm-jit)                                                                    | Running mode       |
| Per Instance running mode          | ALWAYS ON. Can not be disabled                                                                                          | Runtime Extensions |
| Maximum stack size for app threads | [WAMR_APP_THREAD_STACK_SIZE_MAX](./build_wamr.md#set-maximum-app-thread-stack-size)                                     | Runtime Extensions |
| Host defined logging               | [WAMR_BH_LOG](./build_wamr.md#host-defined-log)                                                                         | Runtime Extensions |
| Host defined vprintf               | [WAMR_BH_vprintf](./build_wamr.md#host-defined-vprintf)                                                                 | Runtime Extensions |
| Allocation with usage tracking     | [WAMR_BUILD_ALLOC_WITH_USAGE](./build_wamr.md#user-defined-linear-memory-allocator)                                     | Runtime Extensions |
| Allocation with user data          | [WAMR_BUILD_ALLOC_WITH_USER_DATA](./build_wamr.md#user-defined-linear-memory-allocator)                                 | Runtime Extensions |
| Bulk-memory-opt                    | [WAMR_BUILD_BULK_MEMORY_OPT](./build_wamr.md#bulk-memory-opt)                                                           | Runtime Extensions |
| Call-indirect-overlong             | [WAMR_BUILD_CALL_INDIRECT_OVERLONG](./build_wamr.md#call-indirect-overlong)                                             | Runtime Extensions |
| Copy Call Stack                    | [WAMR_BUILD_COPY_CALL_STACK](./build_wamr.md#copy-call-stack)                                                           | Runtime Extensions |
| Debug Interpreter                  | [WAMR_BUILD_DEBUG_INTERP](./build_wamr.md#configure-debug)                                                              | Runtime Extensions |
| Dump call stack                    | [WAMR_BUILD_DUMP_CALL_STACK](./build_wamr.md#dump-call-stack-feature)                                                   | Runtime Extensions |
| Native General Invocation          | [WAMR_BUILD_INVOKE_NATIVE_GENERAL](./build_wamr.md#invoke-general-ffi)                                                  | Runtime Extensions |
| Lazy JIT Compilation               | [WAMR_BUILD_LAZY_JIT](./build_wamr.md#configure-llvm-jit)                                                               | Runtime Extensions |
| Pthread                            | [WAMR_BUILD_LIB_PTHREAD](./build_wamr.md#lib-pthread)                                                                   | Runtime Extensions |
| Pthread Semaphore Support          | [WAMR_BUILD_LIB_PTHREAD_SEMAPHORE](./build_wamr.md#lib-pthread-semaphore)                                               | Runtime Extensions |
| Lime1 runtime                      | [WAMR_BUILD_LIME1](./build_wamr.md#lime1-target)                                                                        | Runtime Extensions |
| Linux Performance Counters         | [WAMR_BUILD_LINUX_PERF](./build_wamr.md#linux-perf-support)                                                             | Runtime Extensions |
| Memory profiling                   | [WAMR_BUILD_MEMORY_PROFILING](./build_wamr.md#memory-profiling-experiment)                                              | Runtime Extensions |
| Multi-module                       | [WAMR_BUILD_MULTI_MODULE](./build_wamr.md#multi-module-feature)                                                         | Runtime Extensions |
| Perf profiling                     | [WAMR_BUILD_PERF_PROFILING](./build_wamr.md#performance-profiling-experiment)                                           | Runtime Extensions |
| Shared heap                        | [WAMR_BUILD_SHARED_HEAP](./build_wamr.md#shared-heap-among-wasm-apps-and-host-native)                                   | Runtime Extensions |
| Stack Guard Size                   | [WAMR_BUILD_STACK_GUARD_SIZE](./build_wamr.md#stack-guard-size)                                                         | Runtime Extensions |
| WASI Ephemeral NN                  | [WAMR_BUILD_WASI_EPHEMERAL_NN](./build_wamr.md#lib-wasi-nn-with-wasi_ephemeral_nn-module-support)                       | Runtime Extensions |
| WASI-NN (neural network APIs)      | [WAMR_BUILD_WASI_NN](./build_wamr.md#lib-wasi-nn)                                                                       | Runtime Extensions |
| External Delegate for WASI NN      | [WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE](./build_wamr.md#lib-wasi-nn-external-delegate-mode)                       | Runtime Extensions |
| GPU Support for WASI NN            | [WAMR_BUILD_WASI_NN_ENABLE_GPU](./build_wamr.md#lib-wasi-nn-gpu-mode)                                                   | Runtime Extensions |
| External Delegate Path for WASI NN | [WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH](./build_wamr.md#lib-wasi-nn-external-delegate-mode)                         | Runtime Extensions |
| LLAMA CPP for WASI NN              | [WAMR_BUILD_WASI_NN_LLAMACPP](./build_wamr.md#lib-wasi-nn)                                                              | Runtime Extensions |
| ONNX for WASI NN                   | [WAMR_BUILD_WASI_NN_ONNX](./build_wamr.md#lib-wasi-nn)                                                                  | Runtime Extensions |
| OpenVINO for WASI NN               | [WAMR_BUILD_WASI_NN_OPENVINO](./build_wamr.md#lib-wasi-nn)                                                              | Runtime Extensions |
| TFLite for WASI NN                 | [WAMR_BUILD_WASI_NN_TFLITE](./build_wamr.md#lib-wasi-nn)                                                                | Runtime Extensions |
| Configurable bounds checks         | [WAMR_CONFIGURABLE_BOUNDS_CHECKS](./build_wamr.md#configurable-memory-access-boundary-check)                            | Runtime Extensions |
| Write GS base                      | [WAMR_DISABLE_WRITE_GS_BASE](./build_wamr.md#disable-writing-the-linear-memory-base-address-to-x86-gs-segment-register) | Runtime Extensions |

# TierC

## Targets

| Description            |
| ---------------------- |
| aarch64-apple-ios      |
| arm-none-eabi          |
| i386-unknown-elf       |
| i386-wrs-vxworks       |
| riscv32-esp-elf        |
| riscv32-unknown-elf    |
| riscv64-unknown-elf    |
| x86_64-linux-android   |
| x86_64-linux-cosmo     |
| x86_64-unknown-freebsd |
| x86_64-wrs-vxworks     |
| xtensa-esp32-elf       |

## Features

| Description                    | Compilation Flags                                                                                     | Labels             |
| ------------------------------ | ----------------------------------------------------------------------------------------------------- | ------------------ |
| AliOS compatibility            | WAMR_BUILD_PLATFORM=alios-things                                                                      | Portability        |
| Android Compatibility          | WAMR_BUILD_PLATFORM=android                                                                           | Portability        |
| Cosmo Compatibility            | WAMR_BUILD_PLATFORM=cosmopolitan                                                                      | Portability        |
| FreeBSD Compatibility          | WAMR_BUILD_PLATFORM=freebsd                                                                           | Portability        |
| iOS Compatibility              | WAMR_BUILD_PLATFORM=darwin                                                                            | Portability        |
| RIOT OS Compatibility          | WAMR_BUILD_PLATFORM=riot                                                                              | Portability        |
| RT-Thread Compatibility        | WAMR_BUILD_PLATFORM=rt-thread                                                                         | Portability        |
| VxWorks Compatibility          | WAMR_BUILD_PLATFORM=vxworks                                                                           | Portability        |
| Windows Compatibility          | WAMR_BUILD_PLATFORM=windows                                                                           | Portability        |
| GC (Garbage Collection)        | [WAMR_BUILD_GC](./build_wamr.md#garbage-collection)                                                   | Wasm Proposal      |
| Legacy Exception Handling      | [WAMR_BUILD_EXCE_HANDLING](./build_wamr.md#exception-handling)                                        | Wasm Proposal      |
| Multi-memory                   | [WAMR_BUILD_MULTI_MEMORY](./build_wamr.md#multi-memory)                                               | Wasm Proposal      |
| Fast JIT                       | [WAMR_BUILD_FAST_JIT](./build_wamr.md#configure-fast-jit)                                             | Running mode       |
| Multi-tier JIT                 | [Combination of flags](./build_wamr.md#configure-multi-tier-jit)                                      | Running mode       |
| AoT Validator                  | [WAMR_BUILD_AOT_VALIDATOR](./build_wamr.md#aot-validator)                                             | Runtime Extensions |
| Debug AOT                      | [WAMR_BUILD_DEBUG_AOT](./build_wamr.md#configure-debug)                                               | Runtime Extensions |
| Dynamic AoT debugging          | [WAMR_BUILD_DYNAMIC_AOT_DEBUG](./build_wamr.md#configure-debug)                                       | Runtime Extensions |
| Fast JIT Dump                  | [WAMR_BUILD_FAST_JIT_DUMP](./build_wamr.md#configure-fast-jit)                                        | Runtime Extensions |
| Garbage Collection Heap Verify | [WAMR_BUILD_GC_HEAP_VERIFY](./build_wamr.md#garbage-collection)                                       | Runtime Extensions |
| Instruction Metering           | [WAMR_BUILD_INSTRUCTION_METERING](./build_wamr.md#instruction-metering)                               | Runtime Extensions |
| Libc EMCC Compatibility        | [WAMR_BUILD_LIBC_EMCC](./build_wamr.md#libc-emcc)                                                     | Runtime Extensions |
| Libc UVWASI Compatibility      | [WAMR_BUILD_LIBC_UVWASI](./build_wamr.md#libc-uvwasi)                                                 | Runtime Extensions |
| RATS Library                   | [WAMR_BUILD_LIB_RATS](./build_wamr.md#librats)                                                        | Runtime Extensions |
| Mini Loader                    | [WAMR_BUILD_MINI_LOADER](./build_wamr.md#wasm-mini-loader)                                            | Runtime Extensions |
| SGX IPFS Support               | [WAMR_BUILD_SGX_IPFS](./build_wamr.md#intel-protected-file-system)                                    | Runtime Extensions |
| Static PGO                     | [WAMR_BUILD_STATIC_PGO](./build_wamr.md#running-pgoprofile-guided-optimization-instrumented-aot-file) | Runtime Extensions |
| WASM cache                     | [WAMR_BUILD_WASM_CACHE](./build_wamr.md#wasm-cache)                                                   | Runtime Extensions |
| Test garbage collection        | [WAMR_TEST_GC](./build_wamr.md#test-garbage-collection)                                               | Runtime Extensions |
