# Remote application management

The WAMR application manager supports **remote application management**(check out local directory {WAMR-DIR}/core/app-mgr or [same directory on GitHub](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/core/app-mgr) for more) from the host environment or the cloud through any physical communications such as TCP, UPD, UART, BLE, etc. Its modular design makes it able to support application management for different managed runtimes.

The tool **host_tool** (check out local directory {WAMR-DIR}/test-tools/host-tool or [same directory on GitHub](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/test-tools/host-tool) for more) communicates to the WAMR app manager for installing/uninstalling the wasm applications on the companion chip from the host system.

We have two example demos of the use of **host_tool**. One is the [simple example](../../../samples/simple/README.md) using the tool "host_tool" to remotely install/uninstall wasm applications from the WAMR runtime over either TCP socket or UART cable; the other is the [IoT App Store Demo](../../../test-tools/IoT-APP-Store-Demo/README.md) showing the concept of remotely managing the device applications from the cloud.
