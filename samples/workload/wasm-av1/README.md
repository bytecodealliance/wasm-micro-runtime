"wasm-av1" sample introduction
==============

This sample demonstrates how to build [wasm-av1](https://github.com/GoogleChromeLabs/wasm-av1) into
WebAssembly with simd support and run it with iwasm.

## Preparation

please refer to [installation instructions](../README.md).

## Build with EMSDK

just run the convenience script:

```bash
./build.sh
```

it is going to build wasm-av1 and run it with iwasm, which basically contains the following steps:
- hack emcc to delete some objects in libc.a
- patch wasm-av1 and build it with emcc compiler
- build iwasm with simd and libc-emcc support
- run testav1.aot with iwasm

## Or build with clang-11 and wasi-sdk

``` shell
$ mkdir build && cd build
$ cmake ..
$ make
# to verify
$ ls testavx.wasm
```

### Run workload

Firstly please build iwasm with simd support:

``` shell
$ cd <wamr dir>/product-mini/platforms/linux/
$ mkdir build && cd build
$ cmake .. -DWAMR_BUILD_SIMD=1
$ make
```

Then compile wasm file to aot file and run:

``` shell
$ cd <wamr dir>/wamr-compiler/build
$ ./wamrc --enable-simd -o testavx.aot testavx.wasm
$ cd <wamr dir>/product-mini/platforms/linux/
$ # copy sample data like <wamr dir>/samples/workload/wasm-av1/av1/third_party/samples/elephants_dream_480p24.ivf
$ # copy testavx.aot
$ # make sure you declare the access priority of the directory in which the sample data is
$ ./iwasm --dir=. ./testavx.aot ./elephants_dream_480p24.ivf
```