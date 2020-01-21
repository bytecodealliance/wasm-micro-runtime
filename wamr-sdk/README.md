# SDK for Wasm Micro Runtime
This folder contains some tools to generate sdk for wamr runtime and wasm applications, the script and cmake files here are called by `make`, don't use them manually.

## Build the SDK
``` Bash
cd ${WAMR_ROOT}/wamr-sdk
make config
```
Following the prompt to finish the settings for your customized runtime and app sdk, then you will get `out` folder under `${WAMR_ROOT}`

The structure of the output folder is like bellow:
```
out
|--app-sdk/
|   |--sysroot/
|   |--wamr_toolchain.cmake
|
|--runtime-sdk/
    |--include
    |--lib
    |--wamr_config.cmake
```
### app-sdk usage
The `app-sdk` is used to develop wasm applications, if your project are built with cmake, then the `wamr_toolchain.cmake` file is what you need to compile your project into wasm bytecode.

### runtime-sdk usage
The `runtime-sdk` is used to help you embed WAMR runtime into your product easier. There are two method you can use the SDK:

1. Use the provided `runtime_lib.cmake` file:

    You can include `${WAMR_ROOT}/cmake/runtime_lib.cmake` in your project's `CMakeLists.txt` file:
    ``` cmake
    include (${WAMR_ROOT}/cmake/runtime_lib.cmake)
    add_library (vmlib ${WAMR_RUNTIME_LIB_SOURCE})
    # ......
    target_link_libraries (your_target vmlib -lm -ldl -lpthread)
    ```
2. Use the pre-built static library:

    You can link the pre-built library:
    ``` cmake
    link_directories(${SDK_DIR}/runtime-sdk/lib)
    include_directories(${SDK_DIR}/runtime-sdk/include)
    # ......
    target_link_libraries (your_target vmlib -lm -ldl -lpthread)
    ```

    This method can also be used when you don't use cmake

You can refer to this sample: [CMakeLists.txt](../samples/simple/CMakeLists.txt).

> NOTE: If you are familiar with how to configure WAMR by cmake and don't want to build the SDK, you can set the related settings on the top of your `CMakeLists.txt`, then the `runtime_lib.cmake` will not load settings from the SDK.