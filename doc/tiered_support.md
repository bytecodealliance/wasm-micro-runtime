# Tiered Support

Tier Definitions

## Tier A

This tier is the highest level of support. Features and targets in this tier are fully supported, actively maintained, and regularly tested. Users can expect prompt assistance and comprehensive documentation for any issues or questions related to these features. Users can rely on Tier A features for production environments. Targets in this tier usually have been used in products.

## Tier B

This tier represents a moderate level of support. Features and targets in this tier are generally supported and maintained, but may not receive the same level of attention as Tier A. While efforts are made to ensure stability, users may encounter occasional issues that are not immediately addressed. Documentation may be less comprehensive compared to Tier A. Users are encouraged to report any issues they encounter, but response times may vary.

## Tier C

This tier indicates a basic level of support. Features and targets in this tier are considered experimental or less stable. They may not be actively maintained, and users should be prepared for potential issues or limitations. Documentation may be minimal or outdated. Users opting to use Tier C features do so at their own risk and should be prepared to troubleshoot issues independently. These features are typically not recommended for production use.

> [!NOTE]
>
> - _tested_ mentioned above specifically refers to whether there are enough tests in CI.
>
> - _actively maintained_ means that the code is regularly updated to fix bugs, improve performance, and ensure compatibility with other components.
>
> - _fully supported_ means that users can expect timely assistance, comprehensive documentation, and regular updates for any issues or questions related to these features.
>
> _Runtime Extensions_ are features that extend the runtime capabilities of the system beyond the core WebAssembly specification. These extensions may include optimizations, additional APIs, or other enhancements that improve performance, usability, or functionality.

## TierA

| Description                        | Compilation Flags                                                                                           | Labels             |
| ---------------------------------- | ----------------------------------------------------------------------------------------------------------- | ------------------ |
| x86_64-pc-linux-gnu                | N/A                                                                                                         | Target             |
| x86_64-apple-darwin                | N/A                                                                                                         | Target             |
| x86_64-none-linux-gnu              | N/A                                                                                                         | Target             |
| i386-pc-linux-gnu                  | N/A                                                                                                         | Target             |
| aarch64-unknown-nuttx-eabi         | N/A                                                                                                         | Target             |
| Linux Compatibility                | N/A                                                                                                         | Runtime Extensions |
| Bulk Memory                        | [WAMR_BUILD_BULK_MEMORY](./build_wamr.md#enable-bulk-memory-feature)                                        | Wasm Proposal      |
| Custom sections                    | [WAMR_BUILD_LOAD_CUSTOM_SECTION](./build_wamr.md#enable-load-wasm-custom-sections)                          | Wasm Proposal      |
| Extended Constant Expressions      | [WAMR_BUILD_EXTENDED_CONST_EXPR](./build_wamr.md#configure-interpreters)                                    | Wasm Proposal      |
| Import/Export of Mutable Globals   | ALWAYS ON. Can not be disabled                                                                              | Wasm Proposal      |
| Memory64                           | [WAMR_BUILD_MEMORY64](./build_wamr.md#enable-memory64-feature)                                              | Wasm Proposal      |
| Multi-value                        | ALWAYS ON. Can not be disabled                                                                              | Wasm Proposal      |
| Name section                       | WAMR_BUILD_CUSTOM_NAME_SECTION                                                                              | Wasm Proposal      |
| Non-trapping float-to-int          | ALWAYS ON . Can not be disabled                                                                             | Wasm Proposal      |
| Reference Types                    | [WAMR_BUILD_REF_TYPES](./build_wamr.md#configure-interpreters)                                              | Wasm Proposal      |
| Threads                            | [WAMR_BUILD_SHARED_MEMORY](./build_wamr.md#enable-shared-memory-feature)                                    | Wasm Proposal      |
| SIMD (128-bit)                     | [WAMR_BUILD_SIMD](./build_wamr.md#enable-128-bit-simd-feature)                                              | Wasm Proposal      |
| WASI LIBC                          | [WAMR_BUILD_LIBC_WASI](./build_wamr.md#configure-libc)                                                      | Wasm Proposal      |
| AoT compilation (wamrc)            | [WAMR_BUILD_WAMR_COMPILER](./build_wamr.md#configure-aot-and-jits)                                          | Runtime Extensions |
| AoT runtime                        | [WAMR_BUILD_AOT](./build_wamr.md#configure-aot)                                                             | Runtime Extensions |
| AOT intrinsics                     | [WAMR_BUILD_AOT_INTRINSICS](./build_wamr.md#configure-aot-and-jits)                                         | Runtime Extensions |
| AoT stack frame                    | [WAMR_BUILD_AOT_STACK_FRAME](./build_wamr.md#enable-aot-stack-frame-feature)                                | Runtime Extensions |
| Fast Interpreter                   | [WAMR_BUILD_FAST_INTERP](./build_wamr.md#configure-interpreters)                                            | Runtime Extensions |
| Interpreter (classic)              | [WAMR_BUILD_INTERP](./build_wamr.md#configure-interpreters)                                                 | Runtime Extensions |
| Libc builtin                       | [WAMR_BUILD_LIBC_BUILTIN](./build_wamr.md#configure-libc)                                                   | Runtime Extensions |
| Quick AOT/JIT entries              | [WAMR_BUILD_QUICK_AOT_ENTRY](./build_wamr.md#configure-aot-and-jits)                                        | Runtime Extensions |
| Shrunk memory                      | [WAMR_BUILD_SHRUNK_MEMORY](./build_wamr.md#enable-shared-memory-feature)                                    | Runtime Extensions |
| Wakeup blocking operation          | [WAMR_DISABLE_WAKEUP_BLOCKING_OP](./build_wamr.md#disable-async-wakeup-of-blocking-operation)               | Runtime Extensions |
| Disable app entry                  | [WAMR_DISABLE_APP_ENTRY](./build_wamr.md#exclude-wamr-application-entry-functions)                          | Runtime Extensions |
| Disable hardware bound check       | [WAMR_DISABLE_HW_BOUND_CHECK](./build_wamr.md#disable-boundary-check-with-hardware-trap)                    | Runtime Extensions |
| Disable stack hardware bound check | [WAMR_DISABLE_STACK_HW_BOUND_CHECK](./build_wamr.md#disable-native-stack-boundary-check-with-hardware-trap) | Runtime Extensions |
| Global heap pool                   | [WAMR_BUILD_GLOBAL_HEAP_POOL](./build_wamr.md#enable-the-global-heap)                                       | Runtime Extensions |
| Global heap size                   | [WAMR_BUILD_GLOBAL_HEAP_SIZE](./build_wamr.md#set-the-global-heap-size)                                     | Runtime Extensions |
| Thread manager                     | [WAMR_BUILD_THREAD_MGR](./build_wamr.md#enable-thread-manager)                                              | Runtime Extensions |
| WASI threads                       | [WAMR_BUILD_LIB_WASI_THREADS](./build_wamr.md#enable-lib-wasi-threads)                                      | Runtime Extensions |

## TierB

> [!TODO]
>
> - [ ] targets on nuttx
> - [ ] targets on arm, riscv
> - [ ] targets on esp32
> - [ ] targets on zephyr

| Description                        | Compilation Flags                                                                                                       | Labels             |
| ---------------------------------- | ----------------------------------------------------------------------------------------------------------------------- | ------------------ |
| arc-unknown-none-elf               | N/A                                                                                                                     | Target             |
| x86_64-pc-windows-msvc             | N/A                                                                                                                     | Target             |
| mips-unknown-elf                   | N/A                                                                                                                     | Target             |
| mips64-unknown-elf                 | N/A                                                                                                                     | Target             |
| Darwin Compatibility               | N/A                                                                                                                     | Runtime Extensions |
| ESP-IDF Compatibility              | N/A                                                                                                                     | Runtime Extensions |
| Nuttx Compatibility                | N/A                                                                                                                     | Runtime Extensions |
| SGX Compatibility                  | N/A                                                                                                                     | Runtime Extensions |
| Zephyr Compatibility               | N/A                                                                                                                     | Runtime Extensions |
| GC (Garbage Collection)            | [WAMR_BUILD_GC](./build_wamr.md#enable-garbage-collection)                                                              | Wasm Proposal      |
| Stringref                          | [WAMR_BUILD_STRINGREF](./build_wamr.md#configure-debug)                                                                 | Wasm Proposal      |
| Tail Calls                         | [WAMR_BUILD_TAIL_CALL](./build_wamr.md#enable-tail-call-feature)                                                        | Wasm Proposal      |
| Allocation with usage tracking     | [WAMR_BUILD_ALLOC_WITH_USAGE](./build_wamr.md#user-defined-linear-memory-allocator)                                     | Runtime Extensions |
| Allocation with user data          | [WAMR_BUILD_ALLOC_WITH_USER_DATA](./build_wamr.md#user-defined-linear-memory-allocator)                                 | Runtime Extensions |
| Copy Call Stack                    | WAMR_BUILD_COPY_CALL_STACK                                                                                              | Runtime Extensions |
| Debug Interpreter                  | [WAMR_BUILD_DEBUG_INTERP](./build_wamr.md#configure-debug)                                                              | Runtime Extensions |
| Dump call stack                    | [WAMR_BUILD_DUMP_CALL_STACK](./build_wamr.md#enable-dump-call-stack-feature)                                            | Runtime Extensions |
| External Delegate for WASI NN      | WAMR_BUILD_WASI_NN_ENABLE_EXTERNAL_DELEGATE                                                                             | Runtime Extensions |
| External Delegate Path for WASI NN | WAMR_BUILD_WASI_NN_EXTERNAL_DELEGATE_PATH                                                                               | Runtime Extensions |
| Garbage Collection Heap Verify     | WAMR_BUILD_GC_HEAP_VERIFY                                                                                               | Runtime Extensions |
| GPU Support for WASI NN            | WAMR_BUILD_WASI_NN_ENABLE_GPU                                                                                           | Runtime Extensions |
| Host defined logging               | [WAMR_BH_LOG](./build_wamr.md#wamr_bh_log)                                                                              | Runtime Extensions |
| Host defined vprintf               | [WAMR_BH_vprintf](./build_wamr.md#wamr_bh_vprintf)                                                                      | Runtime Extensions |
| Lazy JIT Compilation               | WAMR_BUILD_LAZY_JIT                                                                                                     | Runtime Extensions |
| Linux Performance Counters         | WAMR_BUILD_LINUX_PERF                                                                                                   | Runtime Extensions |
| LLAMA CPP for WASI NN              | WAMR_BUILD_WASI_NN_LLAMACPP                                                                                             | Runtime Extensions |
| Maximum stack size for app threads | [WAMR_APP_THREAD_STACK_SIZE_MAX](./build_wamr.md#set-maximum-app-thread-stack-size)                                     | Runtime Extensions |
| Memory profiling                   | [WAMR_BUILD_MEMORY_PROFILING](./build_wamr.md#enable-memory-profiling-experiment)                                       | Runtime Extensions |
| Module instance context            | [WAMR_BUILD_MODULE_INST_CONTEXT](./build_wamr.md#enable-multi-module-feature)                                           | Runtime Extensions |
| Multi-module                       | [WAMR_BUILD_MULTI_MODULE](./build_wamr.md#enable-multi-module-feature)                                                  | Runtime Extensions |
| Native General Invocation          | WAMR_BUILD_INVOKE_NATIVE_GENERAL                                                                                        | Runtime Extensions |
| ONNX for WASI NN                   | WAMR_BUILD_WASI_NN_ONNX                                                                                                 | Runtime Extensions |
| OpenVINO for WASI NN               | WAMR_BUILD_WASI_NN_OPENVINO                                                                                             | Runtime Extensions |
| Perf profiling                     | [WAMR_BUILD_PERF_PROFILING](./build_wamr.md#enable-performance-profiling-experiment)                                    | Runtime Extensions |
| Pthread                            | [WAMR_BUILD_LIB_PTHREAD](./build_wamr.md#enable-lib-pthread)                                                            | Runtime Extensions |
| Pthread Semaphore Support          | WAMR_BUILD_LIB_PTHREAD_SEMAPHORE                                                                                        | Runtime Extensions |
| Shared heap                        | [WAMR_BUILD_SHARED_HEAP](./build_wamr.md#enable-shared-memory-feature)                                                  | Runtime Extensions |
| Stack Guard Size                   | WAMR_BUILD_STACK_GUARD_SIZE                                                                                             | Runtime Extensions |
| TFLite for WASI NN                 | WAMR_BUILD_WASI_NN_TFLITE                                                                                               | Runtime Extensions |
| WASI Ephemeral NN                  | WAMR_BUILD_WASI_EPHEMERAL_NN                                                                                            | Runtime Extensions |
| WASI-NN (neural network APIs)      | [WAMR_BUILD_WASI_NN](./build_wamr.md#enable-lib-wasi-nn)                                                                | Runtime Extensions |
| Configurable bounds checks         | [WAMR_CONFIGURABLE_BOUNDS_CHECKS](./build_wamr.md#configurable-memory-access-boundary-check)                            | Runtime Extensions |
| Bulk-memory-opt                    | [WAMR_BUILD_BULK_MEMORY_OPT](./build_wamr.md#bulk-memory-opt)                                                           | Runtime Extensions |
| Call-indirect-overlong             | [WAMR_BUILD_CALL_INDIRECT_OVERLONG](./build_wamr.md#call-indirect-overlong)                                             | Runtime Extensions |
| Lime1 runtime                      | [WAMR_BUILD_LIME1](./build_wamr.md#enable-lime1-target)                                                                 | Runtime Extensions |
| Test garbage collection            | WAMR_TEST_GC                                                                                                            | Runtime Extensions |
| Write GS base                      | [WAMR_DISABLE_WRITE_GS_BASE](./build_wamr.md#disable-writing-the-linear-memory-base-address-to-x86-gs-segment-register) | Runtime Extensions |
| Per Instance running mode          | ALWAYS ON. Can not be disabled                                                                                          | Runtime Extensions |
| Shared heap                        | [WAMR_BUILD_SHARED_HEAP](./build_wamr.md#shared-heap-among-wasm-apps-and-host-native)                                   | Runtime Extensions |

## TierC

| Description                   | Compilation Flags                                                                                            | Labels             |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------ | ------------------ |
| x86_64-linux-cosmo            | N/A                                                                                                          | Target             |
| riscv32-unknown-elf           | N/A                                                                                                          | Target             |
| riscv64-unknown-elf           | N/A                                                                                                          | Target             |
| xtensa-esp32-elf              | N/A                                                                                                          | Target             |
| riscv32-esp-elf               | N/A                                                                                                          | Target             |
| x86_64-unknown-freebsd        | N/A                                                                                                          | Target             |
| x86_64-linux-android          | N/A                                                                                                          | Target             |
| aarch64-apple-ios             | N/A/                                                                                                         | Target             |
| arm-none-eabi                 | N/A                                                                                                          | Target             |
| i386-unknown-elf              | N/A                                                                                                          | Target             |
| x86_64-wrs-vxworks            | N/A                                                                                                          | Target             |
| i386-wrs-vxworks              | N/A                                                                                                          | Target             |
| AliOS compatibility           | N/A                                                                                                          | Runtime Extensions |
| Android Compatibility         | N/A                                                                                                          | Runtime Extensions |
| Cosmo Compatibility           | N/A                                                                                                          | Runtime Extensions |
| FreeBSD Compatibility         | N/A                                                                                                          | Runtime Extensions |
| iOS Compatibility             | N/A                                                                                                          | Runtime Extensions |
| RIOT OS Compatibility         | N/A                                                                                                          | Runtime Extensions |
| RT-Thread Compatibility       | N/A                                                                                                          | Runtime Extensions |
| VxWorks Compatibility         | N/A                                                                                                          | Runtime Extensions |
| Windows Compatibility         | N/A                                                                                                          | Runtime Extensions |
| Legacy Exception Handling     | [WAMR_BUILD_EXCE_HANDLING](./build_wamr.md#enable-exception-handling)                                        | Wasm Proposal      |
| Multi-memory                  | [WAMR_BUILD_MULTI_MEMORY](./build_wamr.md#enable-multi-memory)                                               | Wasm Proposal      |
| Extended constant expressions | [WAMR_BUILD_EXTENDED_CONST_EXPR](./build_wamr.md#enable-extended-constant-expression)                        | Wasm Proposal      |
| Debug AOT                     | [WAMR_BUILD_DEBUG_AOT](./build_wamr.md#configure-debug)                                                      | Runtime Extensions |
| Fast JIT                      | [WAMR_BUILD_FAST_JIT](./build_wamr.md#configure-aot-and-jits)                                                | Runtime Extensions |
| Instruction Metering          | [WAMR_BUILD_INSTRUCTION_METERING](./build_wamr.md#enable-instruction-metering)                               | Runtime Extensions |
| Libc EMCC Compatibility       | [WAMR_BUILD_LIBC_EMCC](./build_wamr.md#enable-libc-emcc)                                                     | Runtime Extensions |
| Libc UVWASI Compatibility     | [WAMR_BUILD_LIBC_UVWASI](./build_wamr.md#enable-libc-uvwasi)                                                 | Runtime Extensions |
| Multi-tier JIT                |                                                                                                              | Runtime Extensions |
| RATS Library                  | WAMR_BUILD_LIB_RATS                                                                                          | Runtime Extensions |
| SGX IPFS Support              | WAMR_BUILD_SGX_IPFS                                                                                          | Runtime Extensions |
| Tier-up (Fast JIT → LLVM JIT) | [WAMR_BUILD_DYNAMIC_AOT_DEBUG](./build_wamr.md#configure-debug)                                              | Runtime Extensions |
| Dynamic AoT debugging         | WAMR_BUILD_DYNAMIC_AOT_DEBUG                                                                                 | Runtime Extensions |
| Bulk-memory-opt               | [WAMR_BUILD_BULK_MEMORY_OPT](./build_wamr.md#bulk-memory-opt)                                                | Runtime Extensions |
| Call-indirect-overlong        | [WAMR_BUILD_CALL_INDIRECT_OVERLONG](./build_wamr.md#call-indirect-overlong)                                  | Runtime Extensions |
| WASM cache                    | WAMR_BUILD_WASM_CACHE                                                                                        | Runtime Extensions |
| Mini Loader                   | WAMR_BUILD_MINI_LOADER                                                                                       | Runtime Extensions |
| AoT Validator                 | WAMR_BUILD_AOT_VALIDATOR                                                                                     | Runtime Extensions |
| Fast JIT Dump                 | WAMR_BUILD_FAST_JIT_DUMP                                                                                     | Runtime Extensions |
| Static PGO                    | [WAMR_BUILD_STATIC_PGO](./build_wamr.md#enable-running-pgoprofile-guided-optimization-instrumented-aot-file) | Runtime Extensions |
