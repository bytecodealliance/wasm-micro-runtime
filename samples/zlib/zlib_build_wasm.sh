#!/bin/bash

#cross compile zlib with wasi toolchain


COMPILER_BIN=/opt/wasi-sdk/bin
WASM_SYSROOT=/opt/wasi-sdk/share/wasi-sysroot
sudo mkdir -p /usr/local/zlib_wasm
export PATH=$PATH:$COMPILER_BIN

cd zlib

CC=${COMPILER_BIN}/clang \
AR=${COMPILER_BIN}/llvm-ar \
RANLIB=${COMPILER_BIN}/llvm-ranlib \
NM=${COMPILER_BIN}/llvm-nm \
LDFLAGS="-Wl,--export=deflateInit_ -Wl,--export=deflate -Wl,--export=deflateEnd -Wl,--export=inflateInit -Wl,--export=inflate  -Wl,--export=inflateEnd" \
CFLAGS="--target=wasm32-wasi -fno-trapping-math --sysroot=${WASM_SYSROOT} -fno-exceptions" ./configure --prefix=/usr/local/zlib_wasm 

make
sudo make install

