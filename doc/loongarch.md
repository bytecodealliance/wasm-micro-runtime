# Introduction

This document describes how to cross build `iwasm` on x86-64 Ubuntu 24.04 and run it with `qemu-loongarch64` without depending on an real LoongArch hardware.

## Download LoongArch cross toolchain

```bash
wget https://github.com/loongson/build-tools/releases/download/untagged-afda1c2ad38028517e0e/x86_64-cross-tools-loongarch64-binutils_2.42-gcc_14.1.0-glibc_2.39.tar.xz
# extract it to anywhere
```

## Build iwasm

```bash
cd product-mini/platforms/linux
mkdir buildla && cd buildla
cmake .. \
  -DWAMR_BUILD_TARGET=LOONGARCH64 \
  -DCMAKE_C_COMPILER=/path/to/your/cross-tools/bin/loongarch64-unknown-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=/path/to/your/cross-tools/bin/loongarch64-unknown-linux-gnu-g++
make
```

## Install qemu-loongarch64

```bash
sudo apt install qemu-user
```

## Run

```bash
qemu-loongarch64 -L /path/to/your/cross-tools/target/usr -E LD_LIBRARY_PATH=/path/to/your/cross-tools/target/usr/lib64 ./iwasm /path/to/wasm_or_aot_file
```
