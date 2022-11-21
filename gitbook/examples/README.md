# More Examples

In this chapter, we provide some extra useful examples to demonstrate how you may want to use WAMR:

- [File Interaction Of WASI](../../samples/file/README.md): Demonstrating the supported file interaction API of WASI. This sample can also demonstrate the SGX IPFS (Intel Protected File System), enabling an enclave to seal and unseal data at rest.

- [GUI Examples](gui-examples/README.md): We provide two examples that both use [LVGL library](https://github.com/lvgl/lvgl)

- [Concurrent WASM Application](../../samples/spawn-thread): Demonstrating how to execute wasm functions of the same wasm application concurrently in threads created by host embedder or runtime, but not the wasm application itself.

- [Workload](../../samples/workload/README.md): Demonstrating how to build and run some complex workloads, e.g., tensorflow-lite, XNNPACK, wasm-av1, meshoptimizer, and bwa.
