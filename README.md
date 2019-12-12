WebAssembly Micro Runtime
=========================

**A [Bytecode Alliance][BA] project**

[BA]: https://bytecodealliance.org/

WebAssembly Micro Runtime (WAMR) is a standalone WebAssembly (WASM) runtime with small footprint. It includes a few parts as below:
- A WebAssembly VM core (namely iwasm)
- The supporting API's for the WASM applications
- A mechanism for dynamic management of the WASM application


Current Features of WAMR
=========================
- WASM interpreter (AOT is planned)
- Provides embedding C API
- Provides a mechanism for exporting native API's to WASM applications
- Supports libc for WASM applications in two modes: the built-in libc subset for embedded environment and [WASI](https://github.com/WebAssembly/WASI) for standard libc
- The WASM application framework and asynchronized app programming model
- Supports for micro-service and pub-sub event inter-app communication models
- Supports remote WASM application management from either host or cloud
- Menu configuration for easy integration of application libraries

Application framework architecture
===================================

By using the iwasm VM core, we are flexible to build different application frameworks for the specific domains.

The WAMR has offered a comprehensive application framework for device and IoT usages. The framework solves many common requirements for building a real project:
- Modular design for more language runtimes support
- Inter application communication
- Remote application management
- WASM APP programming model and API extension mechanism

<img src="./doc/pics/wamr-arch.JPG" width="80%">



Build WAMR
==========

## Build WAMR VM core


WAMR VM core (iwasm) can support building for different target platforms:
- Linux
- Zephyr
- MacOS
- VxWorks
- AliOS-Things
- Intel Software Guard Extention (SGX)

See [Build WAMR VM core](./doc/build_wamr.md) for the detailed instructions.

## Libc building options

WAMR supports WASI for standard libc library as well as a [built-in libc subset](./doc/wamr_api.md) for tiny footprint.

WASI is supported for following platforms and enabled by default building:
- Linux



## Embed WAMR VM core

WAMR can be built into a standalone executable which takes the WASM application file name as input, and then executes it. In some other situations, the WAMR source code is embedded the product code and built into the final product.

WAMR provides a set of C API for loading the WASM module, instantiating the module and invoking a WASM function from a native call.  See [Embed WAMR VM core](./doc/embed_wamr.md) for the details.

The WAMR application framework supports dynamically installing WASM application remotely by embedding the WAMR VM core. It can be used as reference for how to use the embedding API's.


## Integrate  WAMR application library

The WAMR provides an application framework which supports event driven programming model as below:

- Single thread per WASM app instance
- App must implement system callbacks: on_init, on_destroy

Application programming API sets are available as below:

- Timer
- Micro service (Request/Response) and Pub/Sub inter-app communication
- Sensor
- Connectivity and data transmission
- 2D graphic UI (based on littlevgl)

See [WAMR application library](./doc/wamr_api.md) for the details.

One WAMR build can select a subset from the WAMR application library. Refer to the sample "simple" for how to integrate API sets into WAMR building.



## Build WAMR with customized application library

When WAMR is integrated into a specific project, it probably includes additional project specific applications APIs which can be either expansion or modification to the standard WAMR application library.

The extended application library should be created in the folder [core/iwasm/lib/app-libs](./core/iwasm/lib/app-libs/). See the [doc/export_native_api.md](./doc/export_native_api.md) for the details.



# Create WASM application SDK

When a customized WAMR runtime is shipped with the products, an associated WASM application SDK should be distributed to the application developers in order to develop WASM applications for the product. At the most time, the WASM application SDK should match the version of the runtime distribution.


Typically a WASM APP SDK package contains following components:

* **WASI-SDK**: only needed when WASI is enabled in the runtime. It can be a link to the WASI-SDK GitHub or the full offline copy.
* **sysroot** folder: only needed when WASI is not enabled in the runtime. copied from [test-tools/toolchain/sysroot](./test-tools/toolchain/sysroot)
* **app-lib** folder: copied from [core/iwasm/lib/app-libs](./core/iwasm/lib/app-libs/)
* **cmake toolchain** file: copied from [test-tools/toolchain/wamr_toolchain.cmake](./test-tools/toolchain/wamr_toolchain.cmake)
* optionally with some guide documents and samples



Build WASM applications
===================================

WebAssembly as a new binary instruction can be viewed as a virtual architecture. If the WASM application is developed in C/C++ language,  developers can use conventional cross-compilation procedure to build the WASM application.  cmake is the recommended building tool and Clang is the preferred compiler. While emcc may still work but it is not guaranteed.

Refer to [Build WASM applications](doc/build_wasm_app.md) for details.


Samples and demos
=================

The WAMR samples are located in folder [./samples](./samples). A sample usually contains the WAMR runtime build, WASM applications and test tools. The WARM provides following samples:
- [Simple](./samples/simple/README.md): The runtime is integrated with most of the WAMR APP libaries and multiple WASM applications are provided for using different WASM API set.
- [littlevgl](./samples/littlevgl/README.md): Demostrating the graphic user interface application usage on WAMR. The whole [LittlevGL](https://github.com/littlevgl/) 2D user graphic library and the UI application is built into WASM application.
- [gui](./samples/gui/README.md): Moved the [LittlevGL](https://github.com/littlevgl/) library into the runtime and defined a WASM application interface by wrapping the littlevgl API.
- [IoT-APP-Store-Demo](./test-tools/IoT-APP-Store-Demo/README.md): A web site for demostrating a WASM APP store usage where we can remotely install and uninstall WASM application on remote devices.


The graphic user interface demo photo:

![WAMR samples diagram](./doc/pics/vgl_demo.png "WAMR samples diagram")




Releases and acknowledgments
============================

WAMR is a community efforts. Since Intel Corp contributed the first release of this open source project, this project has received many good contributions from the community.

See the [major features releasing history and contributor names](./doc/release_ack.md)


Roadmap
=======

See the [roadmap](./doc/roadmap.md) to understand what major features are planned or under development.

Please submit issues for any new feature request, or your plan for contributing new features.


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
