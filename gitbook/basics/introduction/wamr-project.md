# WAMR project

 In this section, we will introduce the basics of project WAMR to you. In each brief introduction section, you are more than welcome to jump to details of that section triggering your interest.

## What is it?

WebAssembly Micro Runtime (WAMR) is a [Bytecode Alliance](https://bytecodealliance.org/) project. A lightweight standalone WebAssembly (WASM) runtime with small footprint, high performance and highly configurable features for applications cross from embedded, IoT, edge to Trusted Execution Environment (TEE), smart contract, cloud native and so on.

## Why you may want to use it

<!-- TODO: link  -->

Like what we explained in previous section, WebAssembly is great for code reuse in server-side with the help of runtime like our Project WAMR. So the most straight forward way is to use WAMR to run your WASM program.

It's not limited to simply being a command line application that run your wasm program. You could also use it as library, integrated in your application to run any wasm program inside your application. Although the most user case is embeded WAMR in their C/C++ program, we do support a several of [language-binding](../../tutorial/language_embedding/README.md) so that you could use WAMR in some language you prefer.

## Component of WAMR

<!-- TODO: link -->

There are four parts of WAMR, two main parts of WAMR are:

1. The "iwasm" VM core to run WASM applications. It has many features and achieve several functionality, here are some example may interest you:

   - Flexibility: It supports multiple running mode to provide the ideal responsive time and performance on your demand. The running mode includes interpreter mode, AOT mode (Ahead-of-Time compilation) and JIT modes (Just-in-Time compilation, LLVM JIT and Fast JIT are supported). Details on how to build and use each mode properly and where you may want to use it can be found in [Tutorial](../../tutorial/README.md)

   - Portability: It supports many architectures and platforms.

     The architectures it support:

     - X86-64, X86-32
     - ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
     - AArch64 (Cortex-A57 and Cortex-A53 are tested)
     - RISCV64, RISCV32 (RISC-V LP64 and RISC-V LP64D are tested)
     - XTENSA, MIPS, ARC

     The  platforms it supports:
     - Linux, Linux SGX (Intel Software Guard Extension), MacOS, Android, Windows, Windows (MinGW)
     - Zephyr, AliOS-Things, VxWorks, NuttX, RT-Thread, ESP-IDF

     It enable true cross-platform development experience. You can even port WAMR to a new platform follow [this tutorial](../../../doc/port_wamr.md). Though it's unlikely since we support so many platforms, it's comforting to have such features.

   - Security: It has Linux SGX (Intel Software Guard Extension) support, through this unique application isolation technology, your application data is as safe as it could be.

   Features listed above is just the tip of iceberg, a simple teaser for complete lists. The complete lists of feature and examples demonstrating it could be found in [Features](../../features/README.md)

2. The "wamrc" AOT compiler to compile WASM file into AOT file for best performance and smaller runtime footprint, which is run by "iwasm" VM Core

   Both wasm binary file and AOT file are supported by iwasm. The wamrc AOT compiler is to compile wasm binary file to AOT file which can also be run by iwasm. The speed by AOT and JIT are near to native.

The other 2 parts are:

1. Application framework:

   The WAMR application manager supports remote application management from the host environment or the cloud through any physical communications such as TCP, UPD, UART, BLE, etc. Its modular design makes it able to support application management for different managed runtimes.

2. Application manager:

   By using the iwasm VM core, we are flexible to build different application frameworks for the specific domains, although it would take quite some effort.

   The WAMR has offered a comprehensive framework for programming WASM applications for device and IoT usages. The framework supports running multiple applications, that are based on the event driven programming model. Here are the supporting API sets by the WAMR application framework library :

   - Timer, Inter-app communication (request/response and pub/sub), Sensor, Connectivity and data transmission, 2D graphic UI
