
Build WAMR vmcore (iwasm)
=========================
It is recommended to use the [WAMR SDK](../wamr-sdk) tools to build a project that integrates the WAMR. This document introduces how to build the WAMR minimal product which is vmcore only (no app-framework and app-mgr) for multiple platforms.

## WAMR vmcore cmake building configurations

By including the script `runtime_lib.cmake` under folder [build-scripts](../build-scripts) in CMakeList.txt, it is easy to build minimal product with cmake.

```cmake
# add this into your CMakeList.txt
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
```

The script `runtime_lib.cmake` defines a number of variables for configuring the WAMR runtime features. You can set these variables in your CMakeList.txt or pass the configurations from cmake command line.

#### **Configure platform and architecture**

- **WAMR_BUILD_PLATFORM**:  set the target platform. It can be set to any platform name (folder name) under folder [core/shared/platform](../core/shared/platform).

- **WAMR_BUILD_TARGET**: set the target CPU architecture. Current supported targets are:  X86_64, X86_32, AArch64, ARM, THUMB, XTENSA and MIPS. For AArch64, ARM and THUMB, the format is \<arch>\[\<sub-arch>]\[_VFP] where \<sub-arch> is the ARM sub-architecture and the "_VFP" suffix means VFP coprocessor registers s0-s15 (d0-d7) are used for passing arguments or returning results in standard procedure-call. Both \<sub-arch> and "_VFP" are optional, e.g. AARCH64, AARCH64V8, AARCHV8.1, ARMV7, ARMV7_VFP, THUMBV7, THUMBV7_VFP and so on.

```bash
cmake -DWAMR_BUILD_PLATFORM=linux -DWAMR_BUILD_TARGET=ARM
```

#### **Configure interpreter**

- **WAMR_BUILD_INTERP**=1/0:  enable or disable WASM interpreter

- **WAMR_BUILD_FAST_INTERP**=1/0：build fast (default) or classic WASM interpreter.

  NOTE: the fast interpreter runs ~2X faster than classic interpreter, but consumes about 2X memory to hold the WASM bytecode code.

#### **Configure AoT and JIT**

- **WAMR_BUILD_AOT**=1/0, default to enable if not set
- **WAMR_BUILD_JIT**=1/0 , default to disable if not set

#### **Configure LIBC**

- **WAMR_BUILD_LIBC_BUILTIN**=1/0,  default to enable if not set

- **WAMR_BUILD_LIBC_WASI**=1/0, default to enable if not set

#### **Configure Debug**

- **WAMR_BUILD_CUSTOM_NAME_SECTION**=1/0,  load the function name from custom name section, default to disable if not set

> Note: if it is enabled, the call stack will be dumped when exception occurs.

#### **Enable Multi-Module feature**

- **WAMR_BUILD_MULTI_MODULE**=1/0, default to disable if not set

#### **Enable WASM mini loader**

- **WAMR_BUILD_MINI_LOADER**=1/0, default to disable if not set

> Note: the mini loader doesn't check the integrity of the WASM binary file, developer must ensure that the WASM file is well-formed.

#### **Enable shared memory feature**
- **WAMR_BUILD_SHARED_MEMORY**=1/0, default to disable if not set

#### **Enable thread manager**
- **WAMR_BUILD_THREAD_MGR**=1/0, default to disable if not set

#### **Enable lib-pthread**
- **WAMR_BUILD_LIB_PTHREAD**=1/0, default to disable if not set
> Note: The dependent feature of lib pthread such as the `shared memory` and `thread manager` will be enabled automatically.

#### **Disable boundary check with hardware trap in AOT or JIT mode**
- **WAMR_DISABLE_HW_BOUND_CHECK**=1/0, default to enable if not set and supported by platform
> Note: by default only platform linux/darwin/android/vxworks 64-bit will enable boundary check with hardware trap in AOT or JIT mode, and the wamrc tool will generate AOT code without boundary check instructions in all 64-bit targets except SGX to improve performance.

#### **Enable memory profiling (Experiment)**
- **WAMR_BUILD_MEMORY_PROFILING**=1/0, default to disable if not set
> Note: if it is enabled, developer can use API `void wasm_runtime_dump_mem_consumption(wasm_exec_env_t exec_env)` to dump the memory consumption info.
Currently we only profile the memory consumption of module, module_instance and exec_env, the memory consumed by other components such as `wasi-ctx`, `multi-module` and `thread-manager` are not included.

#### **Set maximum app thread stack size**
- **WAMR_APP_THREAD_STACK_SIZE_MAX**=n, default to 8 MB (8388608) if not set
> Note: the AOT boundary check with hardware trap mechanism might consume large stack since the OS may lazily grow the stack mapping as a guard page is hit, we may use this configuration to reduce the total stack usage, e.g. -DWAMR_APP_THREAD_STACK_SIZE_MAX=131072 (128 KB).

#### **Enable tail call feature**
- **WAMR_BUILD_TAIL_CALL**=1/0, default to disable if not set

**Combination of configurations:**

We can combine the configurations. For example, if we want to disable interpreter, enable AOT and WASI, we can run command:

``` Bash
cmake .. -DWAMR_BUILD_INTERP=0 -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_BUILD_PLATFORM=linux
```

Or if we want to enable interpreter, disable AOT and WASI, and build as X86_32, we can run command:

``` Bash
cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_BUILD_TARGET=X86_32
```

## Cross compilation

If you are building for ARM architecture on a X86 development machine, you can use the `CMAKE_TOOLCHAIN_FILE`  to set the toolchain file for cross compling.

```
cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOL_CHAIN_FILE  \
         -DWAMR_BUILD_PLATFORM=linux    \
         -DWAMR_BUILD_TARGET=ARM
```

Refer to toolchain sample file [`samples/simple/profiles/arm-interp/toolchain.cmake`](../samples/simple/profiles/arm-interp/toolchain.cmake) for how to build mini product for ARM target architecture.

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


By default in Linux, the interpreter, AOT and WASI are enabled, and JIT is disabled. And the build target is
set to X86_64 or X86_32 depending on the platform's bitwidth.

To enable WASM JIT, firstly we should build LLVM:

``` Bash
cd product-mini/platforms/linux/
./build_llvm.sh     (The llvm source code is cloned under <wamr_root_dir>/core/deps/llvm and auto built)
```

Then pass argument `-DWAMR_BUILD_JIT=1` to cmake to enable WASM JIT:

``` Bash
mkdir build
cd build
cmake .. -DWAMR_BUILD_JIT=1
make
```

Linux SGX (Intel Software Guard Extension)
-------------------------

Please see [Build and Port WAMR vmcore for Linux SGX](./linux_sgx.md) for the details.

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
WAMR provides some features which can be easily configured by passing options to cmake, please see [WAMR vmcore cmake building configurations](./build_wamr.md#wamr-vmcore-cmake-building-configurations) for details. Currently in MacOS, interpreter, AoT, and builtin libc are enabled by default.

VxWorks
-------------------------
VxWorks 7 SR0620 release is validated.

First you need to build a VSB. Make sure *UTILS_UNIX* layer is added in the VSB.
After the VSB is built, export the VxWorks toolchain path by:
```bash
export <vsb_dir_path>/host/vx-compiler/bin:$PATH
```
Now switch to iwasm source tree to build the source code:
```bash
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
WAMR provides some features which can be easily configured by passing options to cmake, please see [WAMR vmcore cmake building configurations](./build_wamr.md#wamr-vmcore-cmake-building-configurations) for details. Currently in VxWorks, interpreter and builtin libc are enabled by default.

Zephyr
-------------------------
You need to download the Zephyr source code first and embed WAMR into it.
``` Bash
git clone https://github.com/zephyrproject-rtos/zephyr.git
cd zephyr/samples/
cp -a <wamr_root_dir>/product-mini/platforms/zephyr/simple .
cd simple
ln -s <wamr_root_dir> wamr
source ../../zephyr-env.sh
# Execute the ./build_and_run.sh script with board name as parameter. Here take x86 as example:
./build_and_run.sh x86

```

Note:
WAMR provides some features which can be easily configured by passing options to cmake, please see [WAMR vmcore cmake building configurations](./build_wamr.md#wamr-vmcore-cmake-building-configurations) for details. Currently in Zephyr, interpreter, AoT and builtin libc are enabled by default.


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
   For linux host:

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

Android
-------------------------
able to generate a shared library support Android platform.
- need an [android SDK](https://developer.android.com/studio). Go and get the "Command line tools only"
- look for a command named *sdkmanager* and download below components. version numbers might need to check and pick others
   - "build-tools;29.0.3"
   - "cmake;3.10.2.4988404"
   - "ndk;21.0.6113669"
   - "patcher;v4"
   - "platform-tools"
   - "platforms;android-29"
- add bin/ of the downloaded cmake to $PATH
- export ANDROID_SDK_HOME=/the/path/of/downloaded/sdk/
- export ANDROID_NDK_HOME=/the/path/of/downloaded/sdk/ndk/
- ready to go

Use such commands, you are able to compile with default configurations. Any compiling requirement should be satisfied by modifying product-mini/platforms/android/CMakeList.txt. For example, chaning ${WAMR_BUILD_TARGET} in CMakeList could get different libraries support different ABIs.

``` shell
$ cd product-mini/platforms/android/
$ mkdir build
$ cd build
$ cmake ..
$ make
$ # check output in distribution/wasm
$ # include/ includes all necesary head files
$ # lib includes libiwasm.so
```

NuttX
-------------------------
WAMR is intergrated with NuttX, just enable the WAMR in Kconfig option (Application Configuration/Interpreters).

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

