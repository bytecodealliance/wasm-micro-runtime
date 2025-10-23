---
description: "This page is under construction/refinement. p.s. wanna hear a construction joke? we are still working on it"
---
# build tutorial

In this chapter, we provide a detailed tutorial on how to build [iwasm vmcore](../../../doc/build_wamr.md) and [wamrc](build_wamrc.md).

## Quick build matrix

Our powerful **iwasm vmcore** provide various running mode you could choose using the compile CMake option. Here is the matrix for different running mode and their attributes:

| Running mode | CMake build options | Pros and Cons  |
| -----------  | -----------         | ---------      |
|  AOT         | none(default)       |                |
|  Classic Interpreter | -DWAMR_BUILD_FAST_INTERP=0 |         |
|  Fast Interpreter | none(default)  |                |
|  LLVM JIT         | -DWAMR_BUILD_JIT=1  |                |
|  Fast JIT    | -DWAMR_BUILD_FAST_JIT=1 |            |

## Supported architectures and platforms

Here is a list of architectures and platforms WAMR support. You could click on the link for quick reference.

The iwasm supports the following architectures:

- X86-64, X86-32
- ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
- AArch64 (Cortex-A57 and Cortex-A53 are tested)
- RISCV64, RISCV32 (RISC-V LP64 and RISC-V LP64D are tested)
- XTENSA, MIPS, ARC

The following platforms are supported. Click each link below for how to build iwasm on that platform. Refer to [WAMR porting guide](../../../doc/port_wamr.md) for how to port WAMR to a new platform.

- [Linux](../../../doc/build_wamr.md#linux), [Linux SGX (Intel Software Guard Extension)](../../../doc/linux_sgx.md), [MacOS](../../../doc/build_wamr.md#macos), [Android](../../../doc/build_wamr.md#android), [Windows](../../../doc/build_wamr.md#windows), [Windows (MinGW)](../../../doc/build_wamr.md#mingw)

- [Zephyr](../../../doc/build_wamr.md#zephyr), [AliOS-Things](../../../doc/build_wamr.md#alios-things), [VxWorks](../../../doc/build_wamr.md#vxworks), [NuttX](../../../doc/build_wamr.md#nuttx), [RT-Thread](../../../doc/build_wamr.md#RT-Thread), [ESP-IDF](../../../doc/build_wamr.md#esp-idf)
