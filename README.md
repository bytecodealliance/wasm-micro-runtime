WebAssembly Micro Runtime
=========================
[Build WAMR VM core](./doc/build_wamr.md) | [Embed WAMR](./doc/embed_wamr.md) | [Export native function](./doc/export_native_api.md) | [Build WASM applications](./doc/build_wasm_app.md) | [Samples](https://github.com/bytecodealliance/wasm-micro-runtime#samples-and-demos)

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

### Performance and memory usage
The WAMR performance, footprint and memory usage data are available at the [performance](../../wiki/Performance) wiki page.

### Supported architectures and platforms

The iwasm supports the following architectures:

- X86-64, X86-32
- ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
- AArch64 (Cortex-A57 and Cortex-A53 are tested)
- MIPS
- XTENSA

Following platforms are supported. Refer to [WAMR porting guide](./doc/port_wamr.md) for how to port WAMR to a new platform.

- [Linux](./doc/build_wamr.md#linux), [Zephyr](./doc/build_wamr.md#zephyr), [MacOS](./doc/build_wamr.md#macos), [VxWorks](./doc/build_wamr.md#vxworks), [AliOS-Things](./doc/build_wamr.md#alios-things), [Intel Software Guard Extention (Linux)](./doc/build_wamr.md#linux-sgx-intel-software-guard-extention), [Android](./doc/build_wamr.md#android)



### Build wamrc AoT compiler

Execute following commands to build **wamrc** compiler:

```shell
cd wamr-compiler
./build_llvm.sh
mkdir build && cd build
cmake ..
make
ln -s {current path}/wamrc /usr/bin/wamrc
```

### Build the mini product

WAMR supports building the iwasm VM core only (no app framework) to the mini product.  The WAMR mini product takes the WASM application file name as input and then executes it. For the detailed procedure, see **[build WAMR VM core](./doc/build_wamr.md)** and **[build and run WASM application](./doc/build_wasm_app.md)**.



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

- [**Basic**](./samples/basic): Demonstrating how host runtime calls WASM function as well as WASM function calls native function.
- **[Simple](./samples/simple/README.md)**: The runtime is integrated with most of the WAMR APP libraries, and a few WASM applications are provided for testing the WAMR APP API set. It uses **built-in libc** and executes apps in **interpreter** mode by default.
- **[littlevgl](./samples/littlevgl/README.md)**: Demonstrating the graphic user interface application usage on WAMR. The whole [LittlevGL](https://github.com/littlevgl/) 2D user graphic library and the UI application is built into WASM application.  It uses **WASI libc** and executes apps in **AoT mode** by default.
- **[gui](./samples/gui/README.md)**: Moved the [LittlevGL](https://github.com/littlevgl/) library into the runtime and defined a WASM application interface by wrapping the littlevgl API. It uses **WASI libc** and executes apps in **interpreter** mode by default.




Releases and acknowledgments
============================

WAMR is a community effort. Since Intel Corp contributed the first release of this open source project, this project has received many good contributions from the community.

See the [major features releasing history and contributor names](./doc/release_ack.md)


Roadmap
=======

See the [roadmap](./doc/roadmap.md) to understand what major features are planned or under development.

Please submit issues for any new feature request or your plan for contributing new features.


License
=======
WAMR uses the same license as LLVM: the `Apache 2.0 license` with the LLVM
exception. See the LICENSE file for details. This license allows you to freely
use, modify, distribute and sell your own products based on WAMR.
Any contributions you make will be under the same license.


Submit issues and contact the maintainers
=========================================
[Click here to submit. Your feedback is always welcome!](https://github.com/intel/wasm-micro-runtime/issues/new)


Contact the maintainers: imrt-public@intel.com
