

Build WASM applications
=========================
You can write a simple ```test.c``` as the first sample.

``` C
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *buf;

    printf("Hello world!\n");

    buf = malloc(1024);
    if (!buf) {
        printf("malloc buf failed\n");
        return -1;
    }

    printf("buf ptr: %p\n", buf);

    sprintf(buf, "%s", "1234\n");
    printf("buf: %s", buf);

    free(buf);
    return 0;
}
```

There are several methods to build a WASM binary, including the clang compiler, cmake, wasi-sdk, Docker, Emscripten and so on.

And after building the WASM binary, we can use the WAMR AoT compiler tool (namely wamrc) to compile the WASM binary into the WAMR AoT binary. And we can use iwasm to run both the WASM binary and the WAMR AoT binary.

## Use clang compiler

The recommended method to build a WASM binary is to use clang compiler ```clang-8```. You can refer to [apt.llvm.org](https://apt.llvm.org) for the detailed instructions. Here are referenced steps to install clang-8 in Ubuntu 16.04 and Ubuntu 18.04.

(1) Add source to your system source list from llvm website

For Ubuntu 16.04, add the following lines to /etc/apt/sources.list:

``` Bash
deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial main
deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial main
# 8
deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-8 main
deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial-8 main
# 9
deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-9 main
deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial-9 main
```

For Ubuntu 18.04, add the following lines to /etc/apt/sources.list:

``` Bash
# i386 not available
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic main
# 8
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main
# 9
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main
```

(2) Download and install clang-8 tool-chain using following commands:

``` Bash
sudo wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
# Fingerprint: 6084 F3CF 814B 57C1 CF12 EFD5 15CF 4D18 AF4F 7421
sudo apt-get update
sudo apt-get install llvm-8 lld-8 clang-8
```

(3) Create a soft link under /usr/bin:

``` Bash
cd /usr/bin
sudo ln -s wasm-ld-8 wasm-ld
```

(4) Use the clang-8 command below to build the WASM C source code into the WASM binary.

``` Bash
clang-8 --target=wasm32 -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined,--export=main \
        -Wl,--strip-all,--no-entry -nostdlib \
        -o test.wasm test.c
```

You will get ```test.wasm``` which is the WASM app binary.

## Use WAMR AoT compiler (wamrc)

Firstly we should build the WAMR AoT compiler:
``` Bash
cd <wamr_root_dir>/test-tools/aot-compiler
./build_llvm.sh     (The llvm source code is cloned under <wamr_root_dir>/core/iwasm/lib/3rdparty/llvm and auto built)
mkdir build
cd build
cmake ..
make
```
The binary file wamrc will be generated under build folder.

Then we can use wamrc to compile WASM app binary to WAMR AoT binary, e.g.
``` Bash
wamrc -o test.aot test.wasm
```
To specify the build target, please use --target=<arch-name> option, e.g.
``` Bash
wamrc --target=i386 test.aot test.wasm
```

## Use cmake

If you have a cmake project, you can cross compile your project by using the toolchain provided by WAMR, the compiler used by WAMR toolchain is `clang-8`.

We can generate a `CMakeLists.txt` file for `test.c`:
```cmake
cmake_minimum_required (VERSION 3.5)
project(hello_world)
add_executable(hello_world test.c)
```
It is quite simple to build this project by cmake:
``` Bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$WAMR_ROOT/test-tools/toolchain/wamr_toolchain.cmake
make
```
You will get ```hello_world``` which is the WASM app binary.

For more details about wamr toolchain, please refer to [test-tools/toolchain](../test-tools/toolchain/README.md).

## Use wasi-sdk

To build a wasm application with wasi support, wasi-sdk is required. Download the [wasi-sdk](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive, then you can use it to build your application:
``` Bash
/path/to/wasi-sdk/bin/clang test.c -o test.wasm
```

You will get ```test.wasm``` which is the WASM app binary.

## Using Docker

Another method availble is using [Docker](https://www.docker.com/). We assume you've already configured Docker (see Platform section above) and have a running interactive shell. Currently the Dockerfile only supports compiling apps with clang, with Emscripten planned for the future.

Use the clang-8 command below to build the WASM C source code into the WASM binary.

``` Bash
clang-8 --target=wasm32 -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined,--export=main \
        -Wl,--strip-all,--no-entry -nostdlib \
        -o test.wasm test.c
```

You will get ```test.wasm``` which is the WASM app binary.

##  Use Emscripten tool

The last method to build a WASM binary is to use Emscripten tool ```emcc```.
Assuming you are using Linux, you may install emcc from Emscripten EMSDK following the steps below:

```
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest-fastcomp
./emsdk activate latest-fastcomp
```
The Emscripten website provides other installation methods beyond Linux.

Use the emcc command below to build the WASM C source code into the WASM binary.
``` Bash
cd emsdk
source emsdk_env.sh     (or add it to ~/.bashrc if you don't want to run it each time)
cd <dir of test.c>
EMCC_ONLY_FORCED_STDLIBS=1 emcc -g -O3 -s WASM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
          -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
          -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
          -s "EXPORTED_FUNCTIONS=['_main']" -o test.wasm test.c
```
You will get ```test.wasm``` which is the WASM app binary.

Run WASM app
========================

Assume you are using Linux, the command to run the test.wasm or test.aot is:
``` Bash
cd iwasm/products/linux/build
./iwasm test.wasm   or
./iwasm test.aot
```
You will get the following output:
```
Hello world!
buf ptr: 0x400002b0
buf: 1234
```
If you would like to run the test app on Zephyr, we have embedded a test sample into its OS image. You will need to execute:
```
ninja run
```
