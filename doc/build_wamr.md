
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

And then install the [Intel SGX SDK](https://software.intel.com/en-us/sgx/sdk).

After installing dependencies, build the source code:
``` Bash
source <SGX_SDK dir>/environment
cd core/iwasm/products/linux-sgx/
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
On Linux, WASI is enabled by default. To build iwasm without wasi support, pass an option when you run cmake:
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

