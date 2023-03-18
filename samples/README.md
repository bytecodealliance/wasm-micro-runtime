
# Samples
- [**basic**](./samples/basic): Demonstrating how to use runtime exposed API's to call WASM functions, how to register native functions and call them, and how to call WASM function from native function.
- **[simple](./samples/simple/README.md)**: The runtime is integrated with most of the WAMR APP libraries, and a few WASM applications are provided for testing the WAMR APP API set. It uses **built-in libc** and executes apps in **interpreter** mode by default.
- **[file](./samples/file/README.md)**: Demonstrating the supported file interaction API of WASI. This sample can also demonstrate the SGX IPFS (Intel Protected File System), enabling an enclave to seal and unseal data at rest.
- **[littlevgl](./samples/littlevgl/README.md)**: Demonstrating the graphic user interface application usage on WAMR. The whole [LVGL](https://github.com/lvgl/lvgl) 2D user graphic library and the UI application are built into WASM application.  It uses **WASI libc** and executes apps in **AOT mode** by default.
- **[gui](./samples/gui/README.md)**: Move the [LVGL](https://github.com/lvgl/lvgl) library into the runtime and define a WASM application interface by wrapping the littlevgl API. It uses **WASI libc** and executes apps in **interpreter** mode by default.
- **[multi-thread](./samples/multi-thread/)**: Demonstrating how to run wasm application which creates multiple threads to execute wasm functions concurrently, and uses mutex/cond by calling pthread related API's.
- **[spawn-thread](./samples/spawn-thread)**: Demonstrating how to execute wasm functions of the same wasm application concurrently, in threads created by host embedder or runtime, but not the wasm application itself.
- **[multi-module](./samples/multi-module)**: Demonstrating the [multiple modules as dependencies](./doc/multi_module.md) feature which implements the [load-time dynamic linking](https://webassembly.org/docs/dynamic-linking/).
- **[ref-types](./samples/ref-types)**: Demonstrating how to call wasm functions with argument of externref type introduced by [reference types proposal](https://github.com/WebAssembly/reference-types).
- **[wasm-c-api](./samples/wasm-c-api/README.md)**: Demonstrating how to run some samples from [wasm-c-api proposal](https://github.com/WebAssembly/wasm-c-api) and showing the supported API's.
- **[socket-api](./samples/socket-api/README.md)**: Demonstrating how to run wasm tcp server and tcp client applications, and how they communicate with each other.
- **[workload](./samples/workload/README.md)**: Demonstrating how to build and run some complex workloads, e.g. tensorflow-lite, XNNPACK, wasm-av1, meshoptimizer and bwa.
- **[sgx-ra](./samples/sgx-ra/README.md)**: Demonstrating how to execute Remote Attestation on SGX with [librats](https://github.com/inclavare-containers/librats), which enables mutual attestation with other runtimes or other entities that support librats to ensure that each is running within the TEE.
