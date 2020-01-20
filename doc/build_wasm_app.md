

# Prepare WASM building environments

WASI-SDK version 7.0+ is the major tool supported by WAMR for building WASM applications. There are some other WASM compilers such as the standard clang compiler and Emscripten might also work [here](./other_wasm_compilers.md).



Install WASI SDK: Download the [wasi-sdk](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path /opt/wasi-sdk



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



To build the source file to WASM bytecode, input following command:

``` Bash
/path/to/wasi-sdk/bin/clang test.c -o test.wasm
```



# Build a project with cmake

If you have complex WASM application project which contains dozens of source files, you can consider  using cmake for project building. 

, you can cross compile your project by using the toolchain provided by WAMR, the compiler used by WAMR toolchain.

We can generate a `CMakeLists.txt` file for `test.c`:

```cmake
cmake_minimum_required (VERSION 3.5)
project(hello_world)
add_executable(hello_world test.c)
```

It is simple to build this project by cmake:

``` Bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$WAMR_ROOT/test-tools/toolchain/wamr_toolchain.cmake
make
```

You will get ```hello_world``` which is the WASM app binary.

For more details about wamr toolchain, please refer to [test-tools/toolchain](../test-tools/toolchain/README.md).

# Compile WASM to AoT module

Please ensure the warmc was already generated and available in your shell PATH. Then we can use wamrc to compile WASM app binary to WAMR AoT binary.

``` Bash
wamrc -o test.aot test.wasm
```

warmc supports a number of compilation options through the command line arguments:

``` Bash
wamrc --help

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
buf ptr: 0x400002b0
buf: 1234
```
If you would like to run the test app on Zephyr, we have embedded a test sample into its OS image. You will need to execute:
```
ninja run
```
