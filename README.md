WebAssembly Micro Runtime
=========================
WebAssembly Micro Runtime (WAMR) is a standalone WebAssembly (WASM) runtime designed for a small footprint. It includes:
- A WebAssembly VM core (namely iwasm)
- The supporting API's for the WASM applications
- A mechanism for dynamic management of the WASM application


Current Features of WAMR
=========================
- WASM interpreter (AOT is planned)
- Provides support for a subset of Libc.
- Supports "SIDE_MODULE=1" EMCC compilation option
- Provides API's for embedding runtime into production software
- Provides a mechanism for exporting native API's to WASM applications
- Supports the programming of firmware apps in a large range of languages (C/C++/Java/Rust/Go/TypeScript etc.)
- App sandbox execution environment on embedded OS
- The purely asynchronized programming model
- Menu configuration for easy platform integration
- Supports micro-service and pub-sub event inter-app communication models
- Easy to extend to support remote FW application management from host or cloud

Application framework architecture
===================================

By using the iwasm VM core, we are flexible to build different application frameworks for the specific domains. 

The WAMR has offered an comprehensive application framework for IoT device usages. The framework solves many common requirements for building a real project:
- Modular design for multiple runtimes support
- Inter application communication
- Remote application management
- WASM APP programming model and API extension mechanism 

<img src="./doc/pics/wamr-arch.JPG" width="80%" height="80%">



Build WAMR Core and run WASM applications
================================================

Please follow the instructions below to build the WAMR core (iwasm) on different platforms:
- Linux
- Zephyr
- Mac
- VxWorks
- AliOS-Things
- Docker

Then you can build some basic WASM applications and run it with the WAMR core. As the WAMR core doesn't include the extended application library, your WASM applications can only use the built-in APIs supported by WAMR.    

See the [doc/building.md](./doc/building.md) for the details.


Embed WAMR into software production
=====================================

WAMR can be built into a standalone executable which takes the WASM application file name as input, and then executes it. In some other situations, the WAMR source code is embedded the product code and built into the final product. 

WAMR provides a set of C API for embedding code to load the WASM module, instantiate the module and invoke a WASM  function from a native call. 

See the [doc/embed_wamr.md](./doc/embed_wamr.md) for the details.

WAMR application programming library
===================================

WAMR defined event driven programming model:
- Single thread per WASM app instance
- App must implement system callbacks: on_init, on_destrory


In general there are a few API classes for the WASM application programming:
- Built-in API: WAMR core provides a minimal libc API set for WASM APP
- 3rd party libraries: Programmers can download any 3rd party C/C++ source code and build it together with the WASM APP code
- WAMR application libraries: 
  - Timer
  - Micro service (Request/Response)
  - Pub/Sub
  - Sensor
  - Connection and data transmission
  - 2D graphic UI (based on littlevgl)
- User extended native API: extend the native API to the WASM applications

See the [doc/wamr_api.md](./doc/wamr_api.md) for the details.


Samples and demos
=================

The WAMR samples are located in folder [./samples](./samples). A sample usually contains the WAMR runtime build, WASM applications and test tools. The WARM provides following samples:
- [Simple](./samples/simple/README.md): The runtime integrated most of the WAMR APP libaries and multiple WASM applications are provided for using different WASM API set.
- [littlevgl](./samples/littlevgl/README.md): Demostrating the graphic user interface application usage on WAMR. The whole littlevgl 2D user graphic library and the UI application is built into WASM application.  
- [gui](./samples/gui/README.md): Moved the littlevgl library into the runtime and defined a WASM application interface by wrapping the littlevgl API.
- [IoT-APP-Store-Demo](./test-tools/IoT-APP-Store-Demo/README.md): A web site for demostrating a WASM APP store usage where we can remotely install and uninstall WASM application on remote devices.


The graphic user interface sample:

![WAMR samples diagram](./doc/pics/vgl_demo.png "WAMR samples diagram")




Releases, acknowledgment and roadmap
====================================


Submit issues and contact the maintainers
=========================================
[Click here to submit. Your feedback is always welcome!](https://github.com/intel/wasm-micro-runtime/issues/new)


Contact the maintainers: imrt-public@intel.com
