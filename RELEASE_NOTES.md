## WAMR-X.Y.Z

### Breaking Changes

### New Features

### Bug Fixes

### Enhancements

### Others

---

## WAMR-1.1.2

### Breaking Changes
Remove the LLVM MCJIT mode, replace it with LLVM ORC JIT eager mode
Add option to pass user data to the allocator functions of RuntimeInitArgs
Change how iwasm returns:
  return 1 if an exception was thrown, else
  return the wasi exit code if the wasm app is a wasi app, else
  keep the same behavior as before
Enable bulk memory by default

### New Features
Add control for the native stack check with hardware trap
Add memory watchpoint support to debugger
Add wasm_module_obtain() to clone wasm_module_t
Implement Fast JIT dump call stack and perf profiling
esp-idf: Add socket support for esp-idf platform

### Bug Fixes
Fix XIP issue caused by rem_s on RISC-V
Fix XIP issues of fp to int cast and int rem/div
Fix missing float cmp for XIP
Correct the arch name for armv7a on NuttX
Fix issue of restoring wasm operand stack
Fix issue of thumb relocation R_ARM_THM_MOVT_ABS
Fix fast jit issue of translating opcode i32.rem_s/i64.rem_s
Fix interp/fast-jit float min/max issues
Fix missing intrinsics for risc-v which were reported by spec test
wasm-c-api: Fix init/destroy thread env multiple times issue
Fix wasm-c-api import func link issue in wasm_instance_new
Fix sample ref-types/wasm-c-api build error with wat2wasm low version
Fix zephyr sample build errors
Fix source debugger error handling: continue executing when detached
Fix scenario where the timeout for atomic wait is set to negative number

### Enhancements
Refactor the layout of interpreter and AOT module instance
Refactor LLVM JIT: remove mcjit and legacy pass manager, upgrade to ORCv2 JIT
Refine Fast JIT call indirect and call native process
Refine Fast JIT accessing memory/table instance and global data
Refine AOT exception check when function return
Enable source debugger reconnection
Add wasm_runtime_get_wasi_exit_code
linux-sgx: Use non-destructive modes for opening files using SGX IPFS
Add wasm_runtime_unregister_natives
Implement invokeNative asm code for MinGW
Add wamr Blog link and Gitbook link to readme
Remove unnecessary app heap memory clean operations to reduce process RSS
Normalize how the global heap pool is configured across iwasm apps
Refine the stack frame size check in interpreter
Enlarge the default wasm operand stack size to 64KB
Use cmake POSITION_INDEPENDENT_CODE instead of hardcoding -pie -fPIE
Implement R_ARM_THM_MOVT_[ABS|REPL] for thumb
Suppress the warnings when building with GCC11
samples/native-lib: Add a bit more complicated example
Add mutex initializer for wasm-c-api engine operations
XIP adaptation for xtensa platform
Update libuv version number
Remove an improper assumption when creating wasm_trap
Avoid initialize LLVM repeatedly
linux-sgx: Improve the remote attestation
linux-sgx: Improve the documentation of SGX-RA sample
linux-sgx: Allow to open files with arbitrary paths in the sandbox using IPFS
Avoid raising exception when debugging with VSCode
wamr-test-suites: Update runtest.py to support python3
Enable Nuttx spec test option and register aot symbols
Use wabt binary instead of building from source in spec test
nuttx: Enable ref types by Kconfig
Update xtensa LLVM version to 15.x
Add bh_print_proc_mem() to dump memory info of current process
Create trap for error message when wasm_instance_new fails
wamr-test-suites: Add support for ARM/RISCV by QEMU
Enable to compile WAMR on platforms that don't support IPV6
Fix warnings in the posix socket implementation
Update document for MacOS compilation
Install patched LLDB on vscode extension activation
Add ARM aeabi memcpy/memmove/memset symbols for AOT bulk memory ops

### Others
Add CIs to release new version and publish binary files
Add more compilation groups of fast jit into CI
Enable spec test on nuttx and daily run it

---

## WAMR-1.1.1

- Implement Linux SGX socket API getpeername, recvfrom and sendto
- Implement Linux SGX POSIX calls based on getsockname and set/getbool
- Integrate WASI-NN into WAMR: support TensorFlow/CPU/F32 in the first stage
- Add timeout send/recv and multicast client/server socket examples
- Support cross building and linking LLVM shared libs for wamrc
- Add darwin support for app_framework
- Add ios support for product-mini
- Update export_native_api.md: Relax the "ground rule"
- wasm_export.h: Add comments on wasm_runtime_register_natives
- Remove unused wasm_runtime_is_module_registered
- samples/multi-module: Examine module registration a bit
- samples/native-lib: Fix exec_env type
- Fix Linux SGX directional OCALL parameter for getsockname
- Fix threads issue to enable running threads spec proposal test cases
- Fix the "register native with iwasm" stuff for macOS
- Fix issues in assemblyscript lib
- Wrap wasi_socket_ext api with extern "C" to fix link failure with cxx project
- Fix invalid size of memory allocated in wasi init
- posix_thread.c: Avoid sem_getvalue deprecation warning on macOS

---

## WAMR-1.1.0

- Extend support for Socket API:
  - Implement IPv6 (along with IPv4) for all the socket-related operations
  - Enable resolving host name IP address by adding a host call to WASI
  - Implement a security feature for controlling what domains are allowed to be resolved
  - Allow configuring socket options by adding host calls to WASI for setting and reading the options
  - Enable connection-less communication between hosts by adding host calls to WASI for sending
  - data directly to a given address and receiving messages from a specific address
  - Fix verification of the address in the address pool
  - Add more samples and update the documents
  - Implement SGX IPFS as POSIX backend for file interaction for linux-sgx
- Integrates the Intel SGX feature called Intel Protection File System Library (IPFS) into the runtime
  to create, operate and delete files inside the enclave, while guaranteeing the confidentiality and
  integrity of the data persisted
- Make libc-builtin buffered printf be a common feature
- Enable passing through arguments for build_llvm.sh
- Update \_\_wasi_sock_accept signature to match wasi_snapshot_preview1
- Enable build wasi_socket_ext.c with both clang and clang++
- Add check for code section size, fix interpreter float operations
- Prevent an already detached thread from being detached again for thread manager
- Fix several issues related to AOT debug and update source_debugging.md
- Fix Windows/MSVC build issues and compile warnings
- Fix wasm loader: function sub local count can be 0
- Fix crash in dumping call stack when the AOT file doesn't contain custom name section
- Fix Dockerfile lint errors and suppress hadolint warnings for pinning versions part
- Fix Fast JIT issues reported by instrument test
- Fix link error for ESP-IDF 4.4.2
- Fix syntax errors and undefined names in Python code
- Fix issues reported by Coverity
- Fix Go binding build error
- Fix a wrongly named parameter and enhance the docs in bh_hashmap.h

---

## WAMR-1.0.0

- Implement Python language binding
- Implement Go language binding
- Implement Fast JIT engine
- Implement hw bound check for interpreter and Fast JIT
- Enable the semantic version mechanism for WAMR
- Implement POSIX semaphore support for linux platform
- Implement SGX getrandom/getentropy without ocall
- Enable remote attestation by librats in SGX mode
- Upgrade WAMR-IDE and source debugging
- Support print exception info in source debugger
- Support emit specified custom sections into AoT file
- Refactor spec test script and CI workflows
- Support integrate 3rd-party toolchains into wamrc
- Enable dump call stack to a buffer
- Enable aot compiler with llvm-14/15
- Don't suppress prev signal handler in hw bound check
- Remove unnecessary memset after mmap
- Refine wasm\*runtime_call_wasm_a/v
- Enable app management and thread support for esp32 arch
- Enable libc-wasi support for esp-idf arch
- Implement xtensa XIP
- Enable memory leak check
- Introduce basic CI for nuttx
- Update documents
- Fix module_realloc with NULL ptr issue
- Fix a typo of macro in wasm_application.c
- nuttx: add CONFIG_INTERPRETERS_WAMR_PERF_PROFILING
- aot_reloc_xtensa.c: define \_\_packed if not available
- Fix bh_vector extend_vector not locked issue
- Enable build libc-wasi for nuttx
- Fix typo in embed_wamr.md
- Fix drop opcode issue in fast interpreter
- Fix typos in wasm_mini_loader.c
- Fix issues reported by Coverity and Klocwork
- Add missing aot relocation symbols for xtensa target
- Add arc compiler-rt functions and reloc type for mwdt
- Fix get invokeNative float ret value issue with clang compiler
- Make robust on choosing target assumption for X86_32 support
- Fix an issue of wasm_cluster_spread_custom_data when called before exec
- Fix socket api verification of addresses in the address pool
- Add API wasm_runtime_set_module_inst
- Set noexecstack CXX link flags for wamrc
- Add import subtyping validation
- Fix libc-wasi/uvwasi poll/environ_get issues
- Add missing symbol for aot_reloc_arc.c
- Add a dev docker container for WAMR repo
- Fix dump call stack issue in interpreter
- Fix windows thread data issue and enhance windows os_mmap
- Support custom stack guard size
- Implement i64.div and i64.rem intrinsics
- Let iwasm return non-zero value when running failed
- Reserve one pointer size for fast-interp code_compiled_size
- Enable libc-wasi support for esp-idf
- Expose wasm_runtime_get_exec_env_singleton to the API users
- Normalize wasm types to refine interpreter call_indirect
- Remove unused wasm_runtime_create_exec_env_and_call_wasm
- Fix linear memory page count issues
- debug: Retire wasm_debug\*(get|set)\_engine_active mechanism
- wasm_application.c: Do not start debug instance automatically
- Fix typo in simd_conversions.c
- nuttx: Add CONFIG_INTERPRETERS_WAMR_DEBUG_INTERP
- Add a new API to get free memory in memory pool
- Fix multi-module and some other issues
- Fix build issue of the meshoptimizer workload
- Fix build error on alios platform

---
