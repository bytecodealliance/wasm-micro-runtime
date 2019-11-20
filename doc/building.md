
Build WAMR Core
=========================
Please follow the instructions below to build the WAMR core on different platforms.

Linux
-------------------------
First of all please install library dependencies of lib gcc.
Use installation commands below for Ubuntu Linux:
``` Bash
sudo apt install lib32gcc-5-dev g++-multilib
```
Or in Fedora:
``` Bash
sudo dnf install glibc-devel.i686
```

After installing dependencies, build the source code:
``` Bash
cd core/iwasm/products/linux/
mkdir build
cd build
cmake ..
make
```
Note:
The WASI feature is enabled by default, if we want to disable it, please run:
``` Bash
cmake .. -DWASM_ENALBE_WASI=0
```

Linux SGX (Intel Software Guard Extention)
-------------------------
First of all please install library dependencies of lib gcc.
Use installation commands below for Ubuntu Linux:
``` Bash
sudo apt install lib32gcc-5-dev g++-multilib
```
Or in Fedora:
``` Bash
sudo dnf install glibc-devel.i686
```

And then install the [Intel SGX SDK](https://software.intel.com/en-us/sgx/sdk).

After installing dependencies, build the source code:
``` Bash
cd core/iwasm/products/linux-sgx/
mkdir build
cd build
cmake ..
make
```
This builds the libraries used by SGX enclave sample, the generated file libvmlib.a and libextlib.a will be copied to enclave-sample folder.

Then build the enclave sample:
``` Bash
cd enclave-sample
make
```
The binary file app will be generated.

To run the sample:
``` Bash
source <SGX_SDK dir>/environment
./app
```

Mac
-------------------------
Make sure to install Xcode from App Store firstly, and install cmake.

If you use Homebrew, install cmake from the command line:
``` Bash
brew install cmake
```

Then build the source codes:
```
cd core/iwasm/products/darwin/
mkdir build
cd build
cmake ..
make
```

VxWorks
-------------------------
VxWorks 7 SR0620 release is validated.

First you need to build a VSB. Make sure *UTILS_UNIX* layer is added in the VSB.
After the VSB is built, export the VxWorks toolchain path by:
```
export <vsb_dir_path>/host/vx-compiler/bin:$PATH
```
Now switch to iwasm source tree to build the source code:
```
cd core/iwasm/products/vxworks/
mkdir build
cd build
cmake ..
make
```
Create a VIP based on the VSB. Make sure the following components are added:
* INCLUDE_POSIX_PTHREADS
* INCLUDE_POSIX_PTHREAD_SCHEDULER
* INCLUDE_SHARED_DATA
* INCLUDE_SHL

Copy the generated iwasm executable, the test WASM binary as well as the needed
shared libraries (libc.so.1, libllvm.so.1 or libgnu.so.1 depending on the VSB,
libunix.so.1) to a supported file system (eg: romfs).

WASI
-------------------------
On Linux / Mac / VxWorks platforms, WASI is enabled by default. To build iwasm without wasi support, pass an option when you run cmake:
```
cmake .. -DWASM_ENABLE_WASI=0
make
```

Zephyr
-------------------------
You need to download the Zephyr source code first and embed WAMR into it.
``` Bash
git clone https://github.com/zephyrproject-rtos/zephyr.git
cd zephyr/samples/
cp -a <iwasm_dir>/products/zephyr/simple .
cd simple
ln -s <iwam_dir> iwasm
ln -s <shared_lib_dir> shared-lib
mkdir build && cd build
source ../../../zephyr-env.sh
cmake -GNinja -DBOARD=qemu_x86 ..
ninja
```

AliOS-Things
-------------------------
1. a developerkit board id needed for testing
2. download the AliOS-Things code
   ``` Bash
   git clone https://github.com/alibaba/AliOS-Things.git
   ```
3. copy <iwasm_root_dir>/products/alios-things directory to AliOS-Things/middleware, and rename it as iwasm
   ``` Bash
   cp -a <iwasm_root_dir>/products/alios-things middleware/iwasm
   ```
4. create a link to <iwasm_root_dir> in middleware/iwasm/ and rename it to iwasm
   ``` Bash
   ln -s <iwasm_root_dir> middleware/iwasm/iwasm
   ```
5. create a link to <shared-lib_root_dir> in middleware/iwasm/ and rename it to shared-lib
   ``` Bash
   ln -s <shared-lib_root_dir> middle/iwasm/shared-lib
   ```
6. modify file app/example/helloworld/helloworld.c, patch as:
   ``` C
   #include <stdbool.h>
   #include <aos/kernel.h>
   extern bool iwasm_init();
   int application_start(int argc, char *argv[])
   {
        int count = 0;
        iwasm_init();
       ...
   }
   ```
7. modify file app/example/helloworld/aos.mk
   ``` C
      $(NAME)_COMPONENTS := osal_aos iwasm
   ```
8. build source code
   ``` Bash
   aos make helloworld@developerkit -c config
   aos make
   ```
9. download the binary to developerkit board, check the output from serial port

Docker
-------------------------
[Docker](https://www.docker.com/) will download all the dependencies and build WAMR Core on your behalf.

Make sure you have Docker installed on your machine: [macOS](https://docs.docker.com/docker-for-mac/install/), [Windows](https://docs.docker.com/docker-for-windows/install/) or [Linux](https://docs.docker.com/install/linux/docker-ce/ubuntu/).

Build the Docker image:

``` Bash
docker build --rm -f "Dockerfile" -t wamr:latest .
```
Run the image in interactive mode:
``` Bash
docker run --rm -it wamr:latest
```
You'll now enter the container at `/root`.


Build WASM app
=========================
You can write a simple ```test.c``` as the first sample.

```C
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

There are several methods to build a WASM binary. They are the clang compiler, Docker, Emscripten and so on.

## Use clang compiler

The recommended method to build a WASM binary is to use clang compiler ```clang-8```. You can refer to [apt.llvm.org](https://apt.llvm.org) for the detailed instructions. Here are referenced steps to install clang-8 in Ubuntu 16.04 and Ubuntu 18.04.

(1) Add source to your system source list from llvm website

For Ubuntu 16.04, add the following lines to /etc/apt/sources.list:

```Bash
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

```Bash
# i386 not available
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic main
# 8
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main
# 9
deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main
deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main

(2) Download and install clang-8 tool-chain using following commands:

```Bash
sudo wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
# Fingerprint: 6084 F3CF 814B 57C1 CF12 EFD5 15CF 4D18 AF4F 7421
sudo apt-get update
sudo apt-get install llvm-8 lld-8 clang-8
```

(3) Create a soft link under /usr/bin:

```Bash
cd /usr/bin
sudo ln -s wasm-ld-8 wasm-ld
```

(4) Use the clang-8 command below to build the WASM C source code into the WASM binary.

```Bash
clang-8 --target=wasm32 -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined,--export=main \
        -Wl,--strip-all,--no-entry -nostdlib \
        -o test.wasm test.c
```

You will get ```test.wasm``` which is the WASM app binary.

## Use cmake

If you have a cmake project, you can cross compile your project by using the toolchain provided by WAMR, the compiler used by WAMR toolchain is `clang-8`.

We can generate a `CMakeLists.txt` file for `test.c`:
```cmake
cmake_minimum_required (VERSION 3.5)
project(hello_world)
add_executable(hello_world test.c)
```
It is quite simple to build this project by cmake:
```Bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$WAMR_ROOT/test-tools/toolchain/wamr_toolchain.cmake
make
```
You will get ```hello_world``` which is the WASM app binary.

For more details about wamr toolchain, please refer to [test-tools/toolchain](../test-tools/toolchain/README.md).

## Use wasi-sdk

To build a wasm application with wasi support, wasi-sdk is required. Download the [wasi-sdk](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive, then you can use it to build your application:
```Bash
/path/to/wasi-sdk/bin/clang test.c -o test.wasm
```

You will get ```test.wasm``` which is the WASM app binary.

## Using Docker

Another method availble is using [Docker](https://www.docker.com/). We assume you've already configured Docker (see Platform section above) and have a running interactive shell. Currently the Dockerfile only supports compiling apps with clang, with Emscripten planned for the future.

Use the clang-8 command below to build the WASM C source code into the WASM binary.

```Bash
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

Assume you are using Linux, the command to run the test.wasm is:
``` Bash
cd iwasm/products/linux/build
./iwasm test.wasm
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
