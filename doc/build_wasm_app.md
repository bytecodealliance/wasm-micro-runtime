

# Prepare WASM building environments

For C and C++, WASI-SDK version 12.0+ is the major tool supported by WAMR to build WASM applications. Also we can use [Emscripten SDK (EMSDK)](https://github.com/emscripten-core/emsdk), but it is not recommended. And there are some other compilers such as the standard clang compiler, which might also work [here](./other_wasm_compilers.md).

To install WASI SDK, please download the [wasi-sdk release](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path `/opt/wasi-sdk`.

For [AssemblyScript](https://github.com/AssemblyScript/assemblyscript), please refer to [AssemblyScript quick start](https://www.assemblyscript.org/quick-start.html) and [AssemblyScript compiler](https://www.assemblyscript.org/compiler.html#command-line-options) for how to install `asc` compiler and build WASM applications.

For Rust, please firstly ref to [Install Rust and Cargo](https://doc.rust-lang.org/cargo/getting-started/installation.html) to install cargo, rustc and rustup, by default they are installed under ~/.cargo/bin, and then run `rustup target add wasm32-wasi` to install wasm32-wasi target for Rust toolchain. To build WASM applications, we can run `cargo build --target wasm32-wasi`, the output files are under `target/wasm32-wasi`.


Build WASM applications with wasi-sdk
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

## 1. wasi-sdk options

There are some useful options which can be specified to build the source code (for more link options, please run `/opt/wasi-sdk/bin/wasm-ld --help`):

- **-nostdlib** Do not use the standard system startup files or libraries when linking. In this mode, the **libc-builtin** library of WAMR must be built to run the wasm app, otherwise, the **libc-wasi** library must be built. You can specify **-DWAMR_BUILD_LIBC_BUILTIN=1** or **-DWAMR_BUILD_LIBC_WASI=1** for cmake to build WAMR with libc-builtin support or libc-wasi support.

- **-Wl,--no-entry** Do not output any entry point

- **-Wl,--export=\<value\>** Force a symbol to be exported, e.g. **-Wl,--export=foo** to export foo function

- **-Wl,--export-all** Export all symbols (normally combined with --no-gc-sections)

- **-Wl,--initial-memory=\<value\>** Initial size of the linear memory, which must be a multiple of 65536

- **-Wl,--max-memory=\<value\>** Maximum size of the linear memory, which must be a multiple of 65536

- **-z stack-size=\<vlaue\>** The auxiliary stack size, which is an area of linear memory, and must be smaller than initial memory size.

- **-Wl,--strip-all** Strip all symbols

- **-Wl,--shared-memory** Use shared linear memory

- **-Wl,--allow-undefined** Allow undefined symbols in linked binary

- **-Wl,--allow-undefined-file=\<value\>** Allow symbols listed in \<file\> to be undefined in linked binary

- **-pthread** Support POSIX threads in generated code

For example, we can build the wasm app with command:

``` Bash
/opt/wasi-sdk/bin/clang -O3 -nostdlib \
    -z stack-size=8192 -Wl,--initial-memory=65536 \
    -o test.wasm test.c \
    -Wl,--export=main -Wl,--export=__main_argc_argv \
    -Wl,--export=__heap_base -Wl,--export=__data_end \
    -Wl,--no-entry -Wl,--strip-all -Wl,--allow-undefined
```
to generate a wasm binary with nostdlib mode, auxiliary stack size is 8192 bytes, initial memory size is 64 KB,  main function, heap base global and data end global are exported, no entry function is generated (no _start function is exported), and all symbols are stripped. Note that it is nostdlib mode, so libc-builtin should be enabled by runtime embedder or iwasm (with cmake -DWAMR_BUILD_LIBC_BUILT=1, enabled by iwasm in Linux by default).

If we want to build the wasm app with wasi mode, we may build the wasm app with command:

```bash
/opt/wasi-sdk/bin/clang -O3 \
    -z stack-size=8192 -Wl,--initial-memory=65536 \
    -o test.wasm test.c \
    -Wl,--export=__heap_base -Wl,--export=__data_end \
    -Wl,--strip-all
```

to generate a wasm binary with wasi mode, auxiliary stack size is 8192 bytes, initial memory size is 64 KB,  heap base global and data end global are exported, wasi entry function exported (_start function), and all symbols are stripped. Note that it is wasi mode, so libc-wasi should be enabled by runtime embedder or iwasm (with cmake -DWAMR_BUILD_LIBC_WASI=1, enabled by iwasm in Linux by default), and normally no need to export main function, by default _start function is executed by iwasm.

## 2. How to reduce the footprint?

Firstly if libc-builtin (-nostdlib) mode meets the requirements, e.g. there are no file io operations in wasm app, we should build the wasm app with -nostdlib option as possible as we can, since the compiler doesn't build the libc source code into wasm bytecodes, which greatly reduces the binary size.

### (1) Methods to reduce the libc-builtin (-nostdlib) mode footprint

- export \_\_heap_base global and \_\_data_end global
  ```bash
  -Wl,--export=__heap_base -Wl,--export=__data_end
  ```
  If the two globals are exported, and there are no memory.grow and memory.size opcodes (normally nostdlib mode doesn't introduce these opcodes since the libc malloc function isn't built into wasm bytecode), WAMR runtime will truncate the linear memory at the place of \__heap_base and append app heap to the end, so we don't need to allocate the memory specified by `-Wl,--initial-memory=n` which must be at least 64 KB. This is helpful for some embedded devices whose memory resource might be limited.

- reduce auxiliary stack size

  The auxiliary stack is an area of linear memory, normally the size is 64 KB by default which might be a little large for embedded devices and actually partly used, we can use `-z stack-size=n` to set its size.

- use -O3 and -Wl,--strip-all

- reduce app heap size when running iwasm

  We can pass `--heap-size=n` option to set the maximum app heap size for iwasm, by default it is 16 KB. For the runtime embedder, we can set the `uint32_t heap_size` argument when calling API ` wasm_runtime_instantiate`.

- reduce wasm operand stack size when running iwasm

  WebAssembly is a binary instruction format for a stack-based virtual machine, which requires a stack to execute the bytecodes. We can pass `--stack-size=n` option to set the maximum stack size for iwasm, by default it is 16 KB. For the runtime embedder, we can set the `uint32_t stack_size` argument when calling API ` wasm_runtime_instantiate` and `wasm_runtime_create_exec_env`.

- decrease block_addr_cache size for classic interpreter

  The block_addr_cache is an hash cache to store the else/end addresses for WebAssembly blocks (BLOCK/IF/LOOP) to speed up address lookup. This is only available in classic interpreter. We can set it by define macro `-DBLOCK_ADDR_CACHE_SIZE=n`, e.g. add `add_defintion (-DBLOCK_ADDR_CACHE_SIZE=n)` in CMakeLists.txt, by default it is 64, and total block_addr_cache size is 3072 bytes in 64-bit platform and 1536 bytes in 32-bit platform.

### (2) Methods to reduce the libc-wasi (without -nostdlib) mode footprint

Most of the above methods are also available for libc-wasi mode, besides them, we can export malloc and free functions with `-Wl,--export=malloc -Wl,--export=free` option, so WAMR runtime will disable its app heap and call the malloc/free function exported to allocate/free the memory from/to the heap space managed by libc.

## 3. Build wasm app with pthread support

Please ref to [pthread library](./pthread_library.md) for more details.

## 4. Build wasm app with SIMD support

Normally we should install emsdk and use its SSE header files, please ref to workload samples, e.g. [bwa CMakeLists.txt](../samples/workload/bwa/CMakeLists.txt) and [wasm-av1 CMakeLists.txt](../samples/workload/wasm-av1/CMakeLists.txt) for more details.

# Build WASM applications with emsdk

## 1. Install emsdk

Assuming you are using Linux, you may install emcc and em++ from Emscripten EMSDK following the steps below:

```
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
# And then source the emsdk_env.sh script before build wasm app
source emsdk_env.sh    (or add it to ~/.bashrc if you don't want to run it each time)
```

The Emscripten website provides other installation methods beyond Linux.

## 2. emsdk options

To build the wasm C source code into wasm binary, we can use the following command:

```bash
EMCC_ONLY_FORCED_STDLIBS=1 emcc -O3 -s STANDALONE_WASM=1 \
    -o test.wasm test.c \
    -s TOTAL_STACK=4096 -s TOTAL_MEMORY=65536 \
    -s "EXPORTED_FUNCTIONS=['_main']" \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0
```

There are some useful options:

- **EMCC_ONLY_FORCED_STDLIBS=1** whether to link libc library into the output binary or not, similar to `-nostdlib` option of wasi-sdk clang. If specified, then no libc library is linked and the **libc-builtin** library of WAMR must be built to run the wasm app, otherwise, the **libc-wasi** library must be built. You can specify **-DWAMR_BUILD_LIBC_BUILTIN=1** or **-DWAMR_BUILD_LIBC_WASI=1** for cmake to build WAMR with libc-builtin support or libc-wasi support.

  The emsdk's wasi implementation is incomplete, e.g. open a file might just return fail, so it is strongly not recommended to use this mode, especially when there are file io operations in wasm app, please use wasi-sdk instead.

- **-s STANDALONE_WASM=1** build wasm app in standalone mode (non-web mode), if the output file has suffix ".wasm", then only wasm file is generated (without html file and JavaScript file).

- **-s TOTAL_STACK=\<value\>** the auxiliary stack size, same as `-z stack-size=\<value\>` of wasi-sdk

- **-s TOTAL_MEMORY=\<value\>**  or **-s INITIAL_MEORY=\<value\>** the initial linear memory size

- **-s MAXIMUM_MEMORY=\<value\>** the maximum linear memory size, only take effect if **-s ALLOW_MEMORY_GROWTH=1** is set

- **-s ALLOW_MEMORY_GROWTH=1/0** whether the linear memory is allowed to grow or not

- **-s "EXPORTED_FUNCTIONS=['func name1', 'func name2']"** to export functions

- **-s ERROR_ON_UNDEFINED_SYMBOLS=0** disable the errors when there are undefined symbols

For more options, please ref to <EMSDK_DIR>/upstream/emscripten/src/settings.js, or [Emscripten document](https://emscripten.org/docs/compiling/Building-Projects.html).

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
  --opt-level=n             Set the optimization level (0 to 3, default is 3)
  --size-level=n            Set the code size level (0 to 3, default is 3)
  -sgx                      Generate code for SGX platform (Intel Software Guard Extention)
  --bounds-checks=1/0       Enable or disable the bounds checks for memory access:
                              by default it is disabled in all 64-bit platforms except SGX and
                              in these platforms runtime does bounds checks with hardware trap,
                              and by default it is enabled in all 32-bit platforms
  --format=<format>         Specifies the format of the output file
                            The format supported:
                              aot (default)  AoT file
                              object         Native object file
                              llvmir-unopt   Unoptimized LLVM IR
                              llvmir-opt     Optimized LLVM IR
  --enable-bulk-memory      Enable the post-MVP bulk memory feature
  --enable-multi-thread     Enable multi-thread feature, the dependent features bulk-memory and
  --enable-tail-call        Enable the post-MVP tail call feature
                            thread-mgr will be enabled automatically
  --enable-simd             Enable the post-MVP 128-bit SIMD feature
  --enable-dump-call-stack  Enable stack trace feature
  -v=n                      Set log verbose level (0 to 5, default is 2), larger with more log
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
