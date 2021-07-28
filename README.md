WebAssembly Micro Runtime
=========================
[Build WAMR VM core](./doc/build_wamr.md) | [Embed WAMR](./doc/embed_wamr.md) | [Export native function](./doc/export_native_api.md) | [Build WASM applications](./doc/build_wasm_app.md) | [Samples](https://github.com/bytecodealliance/wasm-micro-runtime#samples)

**A [Bytecode Alliance][BA] project**

[BA]: https://bytecodealliance.org/

WebAssembly Micro Runtime (WAMR) is a standalone WebAssembly (WASM) runtime with a small footprint. It includes a few parts as below:
- The **"iwasm" VM core**, supporting WebAssembly interpreter, ahead of time compilation (AoT) and Just-in-Time compilation (JIT)

- The **application framework** and the supporting API's for the WASM applications

- The **dynamic management** of the WASM applications

iwasm VM core
=========================

### key features

- 100% compliant to the W3C WASM MVP
- Small runtime binary size (85K for interpreter and 50K for AoT) and low memory usage
- Near to native speed by AoT
- Self-implemented module loader enables AoT working cross Linux, SGX and MCU systems
- Choices of WASM application libc support: the built-in libc subset for the embedded environment or [WASI](https://github.com/WebAssembly/WASI) for standard libc
- [Embeddable with the supporting C API's](./doc/embed_wamr.md)
- [The mechanism for exporting native API's to WASM applications](./doc/export_native_api.md)
- [Multiple modules as dependencies](./doc/multi_module.md), ref to [sample](samples/multi-module)
- [Thread management and pthread library](./doc/pthread_library.md), ref to [sample](samples/multi-thread)

### post-MVP features
- [Non-trapping float-to-int conversions](https://github.com/WebAssembly/nontrapping-float-to-int-conversions)
- [Sign-extension operators](https://github.com/WebAssembly/sign-extension-ops)
- [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations)
- [Shared memory](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md#shared-linear-memory)
- [Multi-value](https://github.com/WebAssembly/multi-value)
- [wasm-c-api](https://github.com/WebAssembly/wasm-c-api), ref to [document](doc/wasm_c_api.md) and [sample](samples/wasm-c-api)
- [Tail-call](https://github.com/WebAssembly/tail-call)
- [128-bit SIMD](https://github.com/WebAssembly/simd), ref to [samples/workload](samples/workload)
- [Reference Types](https://github.com/WebAssembly/reference-types), ref to [document](doc/ref_types.md) and [sample](samples/ref-types)

### Supported architectures and platforms

The iwasm supports the following architectures:

- X86-64, X86-32
- ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
- AArch64 (Cortex-A57 and Cortex-A53 are tested)
- MIPS
- XTENSA
- RISCV64, RISCV32 (RISC-V LP64 and RISC-V LP64D are tested)

Following platforms are supported. Refer to [WAMR porting guide](./doc/port_wamr.md) for how to port WAMR to a new platform.

- [Linux](./doc/build_wamr.md#linux),  [Linux SGX (Intel Software Guard Extension)](./doc/linux_sgx.md),  [MacOS](./doc/build_wamr.md#macos),  [Android](./doc/build_wamr.md#android), [Windows](./doc/build_wamr.md#windows)
- [Zephyr](./doc/build_wamr.md#zephyr),  [AliOS-Things](./doc/build_wamr.md#alios-things),  [VxWorks](./doc/build_wamr.md#vxworks), [NuttX](./doc/build_wamr.md#nuttx), [RT-Thread](./doc/build_wamr.md#RT-Thread)

### Build iwasm VM core (mini product)

WAMR supports building the iwasm VM core only (no app framework) to the mini product. The WAMR mini product takes the WASM application file name or AoT file name as input and then executes it. For the detailed procedure, please see **[build WAMR VM core](./doc/build_wamr.md)** and **[build and run WASM application](./doc/build_wasm_app.md)**. Also we can click the link of each platform above to see how to build iwasm on it.

### Build wamrc AoT compiler

Both wasm binary file and AoT file are supported by iwasm. The wamrc AoT compiler is to compile wasm binary file to AoT file which can also be run by iwasm. Execute following commands to build **wamrc** compiler for Linux:

```shell
cd wamr-compiler
./build_llvm.sh (or "./build_llvm_xtensa.sh" to support xtensa target)
mkdir build && cd build
cmake .. (or "cmake .. -DWAMR_BUILD_TARGET=darwin" for MacOS)
make
# wamrc is generated under current directory
```

For **Windows**：
```shell
cd wamr-compiler
python build_llvm.py
open LLVM.sln in wasm-micro-runtime\core\deps\llvm\win32build with Visual Studio
build LLVM.sln Release
mkdir build && cd build
cmake ..
cmake --build . --config Release
# wamrc.exe is generated under .\Release directory
```

Application framework
===================================

By using the iwasm VM core, we are flexible to build different application frameworks for the specific domains, although it would take quite some effort.

The WAMR has offered a comprehensive framework for programming WASM applications for device and IoT usages. The framework supports running multiple applications, that are based on the event driven programming model. Here are the supporting API sets by the [WAMR application framework library](./doc/wamr_api.md) :

- Timer,  Inter-app communication (request/response and pub/sub), Sensor, Connectivity and data transmission, 2D graphic UI

Browse the folder  [core/app-framework](./core/app-framework) for how to extend the application framework.

# Remote application management

The WAMR application manager supports [remote application management](./core/app-mgr) from the host environment or the cloud through any physical communications such as TCP, UPD, UART, BLE, etc. Its modular design makes it able to support application management for different managed runtimes.

The tool [host_tool](./test-tools/host-tool) communicates to the WAMR app manager for installing/uninstalling the WASM applications on companion chip from the host system. And the [IoT App Store Demo](./test-tools/IoT-APP-Store-Demo/) shows the conception of remotely managing the device applications from the cloud.


WAMR SDK
==========

Usually there are two tasks for integrating the WAMR into a particular project:

- Select what WAMR components (vmcore, libc, app-mgr, app-framework components) to be integrated, and get the associated source files added into the project building configuration
- Generate the APP SDK for developing the WASM apps on the selected libc and framework components

The **[WAMR SDK](./wamr-sdk)** tools is helpful to finish the two tasks quickly. It supports menu configuration for selecting WAMR components and builds the WAMR to a SDK package that includes **runtime SDK** and **APP SDK**. The runtime SDK is used for building the native application and the APP SDK should be shipped to WASM application developers.


Samples
=================

The WAMR [samples](./samples) integrate the iwasm VM core, application manager and selected application framework components.

- [**basic**](./samples/basic): Demonstrating how to use runtime exposed API's to call WASM functions, how to register native functions and call them, and how to call WASM function from native function.
- **[simple](./samples/simple/README.md)**: The runtime is integrated with most of the WAMR APP libraries, and a few WASM applications are provided for testing the WAMR APP API set. It uses **built-in libc** and executes apps in **interpreter** mode by default.
- **[littlevgl](./samples/littlevgl/README.md)**: Demonstrating the graphic user interface application usage on WAMR. The whole [LittleVGL](https://github.com/lvgl/) 2D user graphic library and the UI application are built into WASM application.  It uses **WASI libc** and executes apps in **AoT mode** by default.
- **[gui](./samples/gui/README.md)**: Move the [LittleVGL](https://github.com/lvgl/) library into the runtime and define a WASM application interface by wrapping the littlevgl API. It uses **WASI libc** and executes apps in **interpreter** mode by default.
- **[multi-thread](./samples/multi-thread/)**: Demonstrating how to run wasm application which creates multiple threads to execute wasm functions concurrently, and uses mutex/cond by calling pthread related API's.
- **[spawn-thread](./samples/spawn-thread)**: Demonstrating how to execute wasm functions of the same wasm application concurrently, in threads created by host embedder or runtime, but not the wasm application itself.
- **[multi-module](./samples/multi-module)**: Demonstrating the [multiple modules as dependencies](./doc/multi_module.md) feature which implements the [load-time dynamic linking](https://webassembly.org/docs/dynamic-linking/).
- **[ref-types](./samples/ref-types)**: Demonstrating how to call wasm functions with argument of externref type introduced by [reference types proposal](https://github.com/WebAssembly/reference-types).
- **[wasm-c-api](./samples/wasm-c-api/README.md)**: Demonstrating how to run some samples from [wasm-c-api proposal](https://github.com/WebAssembly/wasm-c-api) and showing the supported API's.
- **[workload](./samples/workload/README.md)**: Demonstrating how to build and run some complex workloads, e.g. tensorflow-lite, XNNPACK, wasm-av1, meshoptimizer and bwa.


Project Technical Steering Committee
====================================
The [WAMR PTSC Charter](./TSC_Charter.md) governs the operations of the project TSC.
The current TSC members:
- [lum1n0us](https://github.com/lum1n0us) - **Liang He**， <liang.he@intel.com>
- [qinxk-inter](https://github.com/qinxk-inter) - **Xiaokang Qin**， <xiaokang.qxk@antgroup.com>
- [wei-tang](https://github.com/wei-tang) - **Wei Tang**， <tangwei.tang@antgroup.com>
- [wenyongh](https://github.com/wenyongh) - **Wenyong Huang**， <wenyong.huang@intel.com>
- [xujuntwt95329](https://github.com/xujuntwt95329) - **Jun Xu**， <Jun1.Xu@intel.com>
- [xwang98](https://github.com/xwang98) - **Xin Wang**， <xin.wang@intel.com>


License
=======
WAMR uses the same license as LLVM: the `Apache 2.0 license` with the LLVM
exception. See the LICENSE file for details. This license allows you to freely
use, modify, distribute and sell your own products based on WAMR.
Any contributions you make will be under the same license.



# More resources

Check out the [Wiki documents ](https://github.com/bytecodealliance/wasm-micro-runtime/wiki) for more resources:

- [Performance and footprint data](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance)
- [Community news and events](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Events)
- [Roadmap](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Roadmap)
- [WAMR TSC meetings](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/TSC-meeting)
- Technical documents

