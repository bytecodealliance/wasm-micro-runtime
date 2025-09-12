---
description: "This page is under construction/refinement. p.s. wanna hear a construction joke? we are still working on it"
---
# Features And Examples

<!-- TODO: each sample should have a link/path to the source code involved -->

In this chapter, you can see the complete list of features that WAMR support. And for each feature, we have an example followed demonstrating the usage of such a feature.

## IWASM features

### Key features

- Full compliant to the W3C WASM MVP
- Small runtime binary size (~85K for interpreter and ~50K for AOT) and low memory usage
- Near to native speed by AOT and JIT
- Self-implemented AOT module loader to enable AOT work on Linux, Windows, MacOS, Android, SGX, and MCU systems
- Choices of WASM application libc support: the built-in libc subset for the embedded environment or [WASI](https://github.com/WebAssembly/WASI) for the standard libc
- [The simple C APIs to embed WAMR into host environment](../../doc/embed_wamr.md), see [how to integrate WAMR](../../doc/embed_wamr.md) and the [API list](../../core/iwasm/include/wasm_export.h)
- [The mechanism to export native APIs to WASM applications](../../doc/export_native_api.md), see [how to register native APIs](../../doc/export_native_api.md)
- [Multiple modules as dependencies](../../doc/multi_module.md), ref to [document](../../doc/multi_module.md) and [sample](../../samples/multi-module)
- [Multi-thread, pthread APIs and thread management](../../doc/pthread_library.md), ref to [document](../../doc/pthread_library.md) and [sample](../../samples/multi-thread)
- [Linux SGX (Intel Software Guard Extension) support](../../doc/linux_sgx.md), ref to [document](../../doc/linux_sgx.md)
- [Source debugging support](../../doc/source_debugging.md), ref to [document](../../doc/source_debugging.md)
- [WAMR-IDE (Experimental)](../../test-tools/wamr-ide) to develop WebAssembly applications with build, run and debug support, ref to [document](../../test-tools/wamr-ide)
- [XIP (Execution In Place) support](../../doc/xip.md), ref to [document](../../doc/xip.md)
- [Berkeley/Posix Socket support](../../doc/socket_api.md), ref to [document](../../doc/socket_api.md) and [sample](../../samples/socket-api)
- Language bindings: [Go](../../language-bindings/go/README.md), [Python](../../language-bindings/python/README.md)

### WASM post-MVP features

There are many post-MVP features for WASM. We support some of them. You can see the details in [this section](demo-examples/README.md)

- [wasm-c-api](https://github.com/WebAssembly/wasm-c-api)
- [128-bit SIMD](https://github.com/WebAssembly/simd)
- [Reference Types](https://github.com/WebAssembly/reference-types)
- [Non-trapping float-to-int conversions](https://github.com/WebAssembly/nontrapping-float-to-int-conversions)
- [Sign-extension operators](https://github.com/WebAssembly/sign-extension-ops), [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations)
- [Multi-value](https://github.com/WebAssembly/multi-value), [Tail-call](https://github.com/WebAssembly/tail-call), [Shared memory](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md#shared-linear-memory)
