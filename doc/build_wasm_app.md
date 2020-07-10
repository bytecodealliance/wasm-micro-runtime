

# Prepare WASM building environments

WASI-SDK version 8.0+ is the major tool supported by WAMR to build WASM applications. There are some other WASM compilers such as the standard clang compiler and Emscripten might also work [here](./other_wasm_compilers.md).

Install WASI SDK: Download the [wasi-sdk](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path `/opt/wasi-sdk`


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

To build the source file to WASM bytecode, we can input the following command:

``` Bash
/opt/wasi-sdk/bin/clang -O3 -o test.wasm test.c
```

There are some useful options which can be specified to build the source code:

- **-nostdlib** Do not use the standard system startup files or libraries when linking. In this mode, the libc-builtin library of WAMR must be built to run the wasm app, otherwise, the libc-wasi library must be built. You can specify **-DWAMR_BUILD_LIBC_BUILTIN** or **-DWAMR_BUILD_LIBC_WASI** for cmake to build WAMR with libc-builtin support or libc-wasi support.

- **-Wl,--no-entry** Do not output any entry point

- **-Wl,--export=<value>** Force a symbol to be exported, e.g. **-Wl,--export=main** to export main function

- **-Wl,--export-all** Export all symbols (normally combined with --no-gc-sections)

- **-Wl,--initial-memory=<value>** Initial size of the linear memory, which must be a multiple of 65536

- **-Wl,--max-memory=<value>** Maximum size of the linear memory, which must be a multiple of 65536

- **-z stack-size=<vlaue>** The auxiliary stack size, which is an area of linear memory, and must be smaller than initial memory size.

- **-Wl,--strip-all** Strip all symbols

- **-Wl,--shared-memory** Use shared linear memory

- **-Wl,--threads** or **-Wl,--no-threads** Run or do not run the linker multi-threaded

- **-Wl,--allow-undefined** Allow undefined symbols in linked binary

- **-Wl,--allow-undefined-file=<value>** Allow symbols listed in <file> to be undefined in linked binary

For example, we can build the wasm app with command:
``` Bash
/opt/wasi-sdk/bin/clang -O3 -nostdlib \
    -z stack-size=8192 -Wl,--initial-memory=65536 \
    -Wl,--export=main -o test.wasm test.c \
    -Wl,--export=__heap_base,--export=__data_end \
    -Wl,--no-entry -Wl,--strip-all -Wl,--allow-undefined
```
to generate a wasm binary with small footprint.

# Build a project with cmake

If you have complex WASM application project which contains dozens of source files, you can consider using cmake for project building.

You can cross compile your project by using the toolchain provided by WAMR.

We can generate a `CMakeLists.txt` file for `test.c`:

``` cmake
cmake_minimum_required (VERSION 3.5)
project(hello_world)

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS},--export=main")
add_executable(hello_world test.c)
```

It is simple to build this project by cmake:

``` Bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$WAMR_ROOT/wamr-sdk/app/wamr_toolchain.cmake
make
```

You will get ```hello_world``` which is the WASM app binary.

> Note: If you have already built a SDK profile, then the **DCMAKE_TOOLCHAIN_FILE** should be changed into `$WAMR_ROOT/wamr-sdk/out/${PROFILE}/app-sdk/wamr_toolchain.cmake`


# Compile WASM to AoT module

Please ensure the wamrc was already generated and available in your shell PATH. Then we can use wamrc to compile WASM app binary to WAMR AoT binary.

``` Bash
wamrc -o test.aot test.wasm
```

wamrc supports a number of compilation options through the command line arguments:

``` Bash
wamrc --help
Usage: wamrc [options] -o output_file wasm_file
  --target=<arch-name>      Set the target arch, which has the general format: <arch><sub>
                            <arch> = x86_64, i386, aarch64, arm, thumb, xtensa, mips.
                              Default is host arch, e.g. x86_64
                            <sub> = for ex. on arm or thumb: v5, v6m, v7a, v7m, etc.
                            Use --target=help to list supported targets
  --target-abi=<abi>        Set the target ABI, e.g. gnu, eabi, gnueabihf, etc. (default: gnu)
                            Use --target-abi=help to list all the ABI supported
  --cpu=<cpu>               Set the target CPU (default: host CPU, e.g. skylake)
                            Use --cpu=help to list all the CPU supported
  --cpu-features=<features> Enable or disable the CPU features
                            Use +feature to enable a feature, or -feature to disable it
                            For example, --cpu-features=+feature1,-feature2
                            Use --cpu-features=+help to list all the features supported
  --opt-level=n             Set the optimization level (0 to 3, default: 3, which is fastest)
  --size-level=n            Set the code size level (0 to 3, default: 3, which is smallest)
  -sgx                      Generate code for SGX platform (Intel Software Guard Extention)
  --format=<format>         Specifies the format of the output file
                            The format supported:
                              aot (default)  AoT file
                              object         Native object file
                              llvmir-unopt   Unoptimized LLVM IR
                              llvmir-opt     Optimized LLVM IR
Examples: wamrc -o test.aot test.wasm
          wamrc --target=i386 -o test.aot test.wasm
          wamrc --target=i386 --format=object -o test.o test.wasm

```


Run WASM app in WAMR mini product build
========================

Run the test.wasm or test.aot with WAMR mini product build:
``` Bash
./iwasm test.wasm   or
./iwasm test.aot
```
You will get the following output:
```
Hello world!
buf ptr: 0xffffc2c8
buf: 1234
```
If you would like to run the test app on Zephyr, we have embedded a test sample into its OS image. You will need to execute:
```
ninja run
```
