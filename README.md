# WebAssembly Micro Runtime


**A [Bytecode Alliance][BA] project**

[BA]: https://bytecodealliance.org/

**[Guide](https://wamr.gitbook.io/)**&emsp;&emsp;**[Website](https://bytecodealliance.github.io/wamr.dev)**&emsp;&emsp;**[Chat](https://bytecodealliance.zulipchat.com/#narrow/stream/290350-wamr)**

[Build WAMR](./doc/build_wamr.md) | [Build AOT Compiler](./README.md#build-wamrc-aot-compiler) | [Embed WAMR](./doc/embed_wamr.md) | [Export Native API](./doc/export_native_api.md) | [Build Wasm Apps](./doc/build_wasm_app.md) | [Samples](./README.md#samples)

WebAssembly Micro Runtime (WAMR) is a lightweight standalone WebAssembly (Wasm) runtime with small footprint, high performance and highly configurable features for applications cross from embedded, IoT, edge to Trusted Execution Environment (TEE), smart contract, cloud native and so on. It includes a few parts as below:
- The [**WAMR VM core**](./README.md#iwasm-vm-core) is a set of runtime libraries for loading and running Wasm modules. It supportes many execution modes including interpreter, Ahead-of-Time compilation(AoT)) and Just-in-Time compilation (JIT). The WAMR supports two JIT tiers - Fast JIT, LLVM JIT, and dynamic tier-up from Fast JIT to LLVM JIT.
- The [**iwasm**] is executable binary built with WAMR VMcore that supports WASI and command line interface.
- The [**"wamrc" AOT compiler**](./README.md#build-wamrc-aot-compiler) to compile Wasm file into AOT file for best performance and smaller runtime footprint
- Additional components for quickly building a real solution with WAMR vmcore:
  - The [**application framework**](./README.md#application-framework) and the supporting APIs for the Wasm applications
  - The [**dynamic management**](./README.md#remote-application-management) of the Wasm applications
  - The [WAMR-IDE (Experimental)](./test-tools/wamr-ide) is VSCode extension for developping WebAssembly applications with C/C++


### Key features
- Full compliant to the W3C Wasm MVP
- Small runtime binary size (~85K for interpreter and ~50K for AOT) and low memory usage
- Near to native speed by AOT and JIT
- Self-implemented AOT module loader to enable AOT working on Linux, Windows, MacOS, Android, SGX and MCU systems
- Choices of Wasm application libc support: the built-in libc subset for the embedded environment or [WASI](https://github.com/WebAssembly/WASI) for the standard libc
- [The simple C APIs to embed WAMR into host environment](./doc/embed_wamr.md), see [how to integrate WAMR](./doc/embed_wamr.md) and the [API list](./core/iwasm/include/wasm_export.h)
- [The mechanism to export native APIs to Wasm applications](./doc/export_native_api.md), see [how to register native APIs](./doc/export_native_api.md)
- [Multiple modules as dependencies](./doc/multi_module.md), ref to [document](./doc/multi_module.md) and [sample](samples/multi-module)
- [Multi-thread, pthread APIs and thread management](./doc/pthread_library.md), ref to [document](./doc/pthread_library.md) and [sample](samples/multi-thread)
- [Linux SGX (Intel Software Guard Extension) support](./doc/linux_sgx.md), ref to [document](./doc/linux_sgx.md)
- [Source debugging support](./doc/source_debugging.md), ref to [document](./doc/source_debugging.md)
- [XIP (Execution In Place) support](./doc/xip.md), ref to [document](./doc/xip.md)
- [Berkeley/Posix Socket support](./doc/socket_api.md), ref to [document](./doc/socket_api.md) and [sample](./samples/socket-api)
- Language bindings: [Go](./language-bindings/go/README.md), [Python](./language-bindings/python/README.md)

### Wasm post-MVP features
- [wasm-c-api](https://github.com/WebAssembly/wasm-c-api), ref to [document](doc/wasm_c_api.md) and [sample](samples/wasm-c-api)
- [128-bit SIMD](https://github.com/WebAssembly/simd), ref to [samples/workload](samples/workload)
- [Reference Types](https://github.com/WebAssembly/reference-types), ref to [document](doc/ref_types.md) and [sample](samples/ref-types)
- [Non-trapping float-to-int conversions](https://github.com/WebAssembly/nontrapping-float-to-int-conversions)
- [Sign-extension operators](https://github.com/WebAssembly/sign-extension-ops), [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations)
- [Multi-value](https://github.com/WebAssembly/multi-value), [Tail-call](https://github.com/WebAssembly/tail-call), [Shared memory](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md#shared-linear-memory)

### Supported architectures and platforms
The iwasm supports the following architectures:  
- X86-64, X86-32
- ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
- AArch64 (Cortex-A57 and Cortex-A53 are tested)
- RISCV64, RISCV32 (RISC-V LP64 and RISC-V LP64D are tested)
- XTENSA, MIPS, ARC

The following platforms are supported, click each link below for how to build iwasm on that platform. Refer to [WAMR porting guide](./doc/port_wamr.md) for how to port WAMR to a new platform.  
- [Linux](./doc/build_wamr.md#linux),  [Linux SGX (Intel Software Guard Extension)](./doc/linux_sgx.md),  [MacOS](./doc/build_wamr.md#macos),  [Android](./doc/build_wamr.md#android), [Windows](./doc/build_wamr.md#windows), [Windows (MinGW)](./doc/build_wamr.md#mingw)
- [Zephyr](./doc/build_wamr.md#zephyr),  [AliOS-Things](./doc/build_wamr.md#alios-things),  [VxWorks](./doc/build_wamr.md#vxworks), [NuttX](./doc/build_wamr.md#nuttx), [RT-Thread](./doc/build_wamr.md#RT-Thread), [ESP-IDF](./doc/build_wamr.md#esp-idf)


### Performance and Footprint
- [Performance and footprint data](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance): checkout [here](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance) for the performance and footprint data
- [Memory usage tunning](./doc/memory_tune.md): checkout [here](./doc/memory_tune.md) for the memory model and how to tune the memory usage
- [Memory usage profiling](./doc/build_wamr.md#enable-memory-profiling-experiment): checkout [here](./doc/build_wamr.md#enable-memory-profiling-experiment) for how to profile the memory usage
- [Benchmarks](./tests/benchmarks): checkout these links for how to run the benchmarks: [PolyBench](./tests/benchmarks/polybench), [CoreMark](./tests/benchmarks/coremark), [Sightglass](./tests/benchmarks/sightglass), [JetStream2](./tests/benchmarks/jetstream)


## Getting started
- [Build iwasm (mini product)](./product-mini/README.md)
- [Build iwasm VM core](./doc/build_wamr.md) on [Linux](./doc/build_wamr.md#linux), [SGX](./doc/linux_sgx.md), [MacOS](./doc/build_wamr.md#macos) and [Windows](./doc/build_wamr.md#windows), and [Build wamrc AOT compiler](./README.md#build-wamrc-aot-compiler)
- [Embed WAMR into host applications](./doc/embed_wamr.md)
- [Embed into C/C++](./doc/embed_wamr.md), [Embed into Python](./language-bindings/python), [Embed into Go](./language-bindings/go)
- [Build wamrc AOT compiler](./wamr-compiler/README.md)
- [Register native APIs for Wasm applications](./doc/export_native_api.md)
- [Build Wasm applications](./doc/build_wasm_app.md)
- [Port WAMR to a new platform](./doc/port_wamr.md)
- [Benchmarks](./tests/benchmarks) and [Samples](./samples)
- [VS Code development container](./doc/devcontainer.md)

Project Technical Steering Committee
====================================
The [WAMR PTSC Charter](./TSC_Charter.md) governs the operations of the project TSC.
The current TSC members:
- [dongsheng28849455](https://github.com/dongsheng28849455) - **Dongsheng Yan**, <dongsheng.yan@sony.com>
- [loganek](https://github.com/loganek) - **Marcin Kolny**, <mkolny@amazon.co.uk>
- [lum1n0us](https://github.com/lum1n0us) - **Liang He**， <liang.he@intel.com>
- [no1wudi](https://github.com/no1wudi) **Qi Huang**, <huangqi3@xiaomi.com>
- [qinxk-inter](https://github.com/qinxk-inter) - **Xiaokang Qin**， <xiaokang.qxk@antgroup.com>
- [wei-tang](https://github.com/wei-tang) - **Wei Tang**， <tangwei.tang@antgroup.com>
- [wenyongh](https://github.com/wenyongh) - **Wenyong Huang**， <wenyong.huang@intel.com>
- [xujuntwt95329](https://github.com/xujuntwt95329) - **Jun Xu**， <Jun1.Xu@intel.com>
- [xwang98](https://github.com/xwang98) - **Xin Wang**， <xin.wang@intel.com> (chair)
- [yamt](https://github.com/yamt) - **Takashi Yamamoto**, <yamamoto@midokura.com>


License
=======
WAMR uses the same license as LLVM: the `Apache 2.0 license` with the LLVM
exception. See the LICENSE file for details. This license allows you to freely
use, modify, distribute and sell your own products based on WAMR.
Any contributions you make will be under the same license.

# More resources

- [WAMR Blogs](https://bytecodealliance.github.io/wamr.dev/blog/)
- [Community news and events](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Events)
- [Roadmap](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Roadmap)
- [WAMR TSC meetings](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/TSC-meeting-notes)

