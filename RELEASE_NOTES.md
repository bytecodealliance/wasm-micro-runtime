## WAMR-X.Y.Z

### Breaking Changes

### New Features

### Bug Fixes

### Enhancements

### Others

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
