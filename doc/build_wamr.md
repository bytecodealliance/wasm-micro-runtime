
Build WAMR core (iwasm)
=========================
Please follow the instructions below to build the WAMR VM core on different platforms.

Linux
-------------------------
First of all please install the dependent packages.
Run command below in Ubuntu-18.04:
``` Bash
sudo apt install build-essential cmake g++-multilib libgcc-8-dev lib32gcc-8-dev
```
Or in Ubuntu-16.04:
``` Bash
sudo apt install build-essential cmake g++-multilib libgcc-5-dev lib32gcc-5-dev
```
Or in Fedora:
``` Bash
sudo dnf install glibc-devel.i686
```

After installing dependencies, build the source code:
``` Bash
cd product-mini/platforms/linux/
mkdir build
cd build
cmake ..
make
```
The binary file iwasm will be generated under build folder.

Note:
WAMR provides some features which can be easily configured by passing options to cmake:
``` Bash
cmake -DWAMR_BUILD_INTERP=1/0 to enable or disable WASM intepreter
cmake -DWAMR_BUILD_AOT=1/0 to enable or disable WASM AOT
cmake -DWAMR_BUILD_JIT=1/0 to enable or disable WASM JIT
cmake -DWAMR_BUILD_LIBC_BUILTIN=1/0 enable or disable Libc builtin API's
cmake -DWAMR_BUILD_LIBC_WASI=1/0 enable or disable Libc WASI API's
cmake -DWAMR_BUILD_TARGET=<arch><sub> to set the building target, including:
    X86_64, X86_32, ARM, THUMB, XTENSA and MIPS
    for ARM and THUMB, we can specify the <sub> info, e.g. ARMV4, ARMV4T, ARMV5, ARMV5T, THUMBV4T, THUMBV5T and so on.
```

For example, if we want to disable interpreter, enable AOT and WASI, we can:
``` Bash
cmake .. -DWAMR_BUILD_INTERP=0 -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_LIBC_WASI=0
```
Or if we want to enable inerpreter, disable AOT and WASI, and build as X86_32, we can:
``` Bash
cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_BUILD_TARGET=X86_32
```

By default in Linux, the interpreter, AOT and WASI are enabled, and JIT is disabled. And the build target is
set to X86_64 or X86_32 depending on the platform's bitwidth.

To enable WASM JIT, firstly we should build LLVM:
``` Bash
cd product-mini/platforms/linux/
./build_llvm.sh     (The llvm source code is cloned under <wamr_root_dir>/core/deps/llvm and auto built)
```
Then pass option -DWAMR_BUILD_JIT=1 to cmake to enable WASM JIT:
``` Bash
mkdir build
cd build
cmake .. -DWAMR_BUILD_JIT=1
make
```

Linux SGX (Intel Software Guard Extention)
-------------------------
First of all please install the [Intel SGX SDK](https://software.intel.com/en-us/sgx/sdk).

After installing dependencies, build the source code:
``` Bash
source <SGX_SDK dir>/environment
cd product-mini/platforms/linux-sgx/
mkdir build
cd build
cmake ..
make
```
This builds the libraries used by SGX enclave sample, the generated file libvmlib.a and libextlib.a will be copied to enclave-sample folder.

Then build the enclave sample:
``` Bash
source <SGX_SDK dir>/environment
cd enclave-sample
make
```
The binary file app will be generated.

To run the sample:
``` Bash
source <SGX_SDK dir>/environment
./app
```

MacOS
-------------------------
Make sure to install Xcode from App Store firstly, and install cmake.

If you use Homebrew, install cmake from the command line:
``` Bash
brew install cmake
```

Then build the source codes:
```
cd product-mini/platforms/darwin/
mkdir build
cd build
cmake ..
make
```
Note:
WAMR provides some features which can be easily configured by passing options to cmake, please see [Linux platform](./build_wamr.md#linux) for details. Currently in MacOS, interpreter, AoT, and builtin libc are enabled by default.

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
cd product-mini/platforms/vxworks/
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

Note:
WAMR provides some features which can be easily configured by passing options to cmake, please see [Linux platform](./build_wamr.md#linux) for details. Currently in VxWorks, interpreter and builtin libc are enabled by default.

Zephyr
-------------------------
You need to download the Zephyr source code first and embed WAMR into it.
``` Bash
git clone https://github.com/zephyrproject-rtos/zephyr.git
cd zephyr/samples/
cp -a <wamr_root_dir>/product-mini/platforms/zephyr/simple .
cd simple
ln -s <wamr_root_dir> wamr
mkdir build && cd build
source ../../../zephyr-env.sh
cmake -GNinja -DBOARD=qemu_x86_nommu ..
ninja
```
Note:
WAMR provides some features which can be easily configured by passing options to cmake, please see [Linux platform](./build_wamr.md#linux) for details. Currently in Zephyr, interpreter, AoT and builtin libc are enabled by default.


AliOS-Things
-------------------------
1. a developerkit board id needed for testing
2. download the AliOS-Things code
   ``` Bash
   git clone https://github.com/alibaba/AliOS-Things.git
   ```
3. copy <wamr_root_dir>/product-mini/platforms/alios-things directory to AliOS-Things/middleware, and rename it as iwasm
   ``` Bash
   cp -a <wamr_root_dir>/product-mini/platforms/alios-things middleware/iwasm
   ```
4. create a link to <wamr_root_dir> in middleware/iwasm/ and rename it to wamr
   ``` Bash
   ln -s <wamr_root_dir> middleware/iwasm/wamr
   ```
5. modify file app/example/helloworld/helloworld.c, patch as:
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
6. modify file app/example/helloworld/aos.mk
   ``` C
      $(NAME)_COMPONENTS := osal_aos iwasm
   ```
7. build source code and run
   For linuxhost:
   ``` Bash
   aos make helloworld@linuxhost -c config
   aos make
   ./out/helloworld@linuxhost/binary/helloworld@linuxhost.elf
   ```

   For developerkit:
   Modify file middleware/iwasm/aos.mk, patch as:
   ``` C
   WAMR_BUILD_TARGET := THUMBV7M
   ```

   ``` Bash
   aos make helloworld@developerkit -c config
   aos make
   ```
   download the binary to developerkit board, check the output from serial port

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

