WebAssembly Micro Runtime
=========================
WebAssembly Micro Runtime (WAMR) is a standalone WebAssembly (WASM) runtime designed for a small footprint. It includes:
- A WebAssembly (WASM) VM core
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

Architecture
=========================
The application manager component handles the packets that the platform receives from external sources through any communication buses such as a socket, serial port or SPI. A packet type can be either a request, a response or an event. The application manager will serve the requests with URI "/applet" and call the runtime glue layer interfaces for installing/uninstalling the application. For other URI's, it will filter the resource registration table and route the request to the internal queue of the responsible application.

- The WebAssembly runtime provides the execution environment for WASM applications.

- The messaging layer can support the API for WASM applications to communicate with each other and also the host environment.

- When ahead of time (AOT) compilation is enabled (TODO), the WASM application could be either WASM or a compiled native binary.

<img src="./doc/pics/architecture.PNG" width="80%" height="80%">



Build WAMR Core and run basic WASM applications
================================================

Please follow the instructions below to build the WAMR core on different platforms.
-Linux
-Zephyr
-Mac
-VxWorks
-AliOS-Things
-Docker


Embed WAMR into software production
=====================================



WAMR application programming library
===================================



Samples and demos
=================


Releases, acknowledgment and roadmap
====================================


Submit issues and contact the maintainers
=========================================
[Click here to submit. Your feedback is always welcome!](https://github.com/intel/wasm-micro-runtime/issues/new)


Contact the maintainers: imrt-public@intel.com
