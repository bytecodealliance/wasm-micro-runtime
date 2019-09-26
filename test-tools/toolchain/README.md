# Cross Compile Toolchain for Wasm Micro Runtime
This folder contains sysroot and toolchain files for building wasm application by using cmake.

## Build a project
To build a C project into wasm, you may use the toolchain file provided here as `wamr_toolchain.cmake`:
```Bash
cmake /path/to/CMakeLists.txt -DCMAKE_TOOLCHAIN_FILE=/path/to/wamr_toolchain.cmake
make
```

## Generate a toolchain for your runtime
If you extend more APIs of wasm runtime by using `EXPORT_WASM_API` API, we also provide a tool which allow you to generate the toolchain for your runtime:
```Bash
./generate_toolchain.py -o out_dir -f api_list_file
```
A toolchain which enables your extended APIs should be generated in the path you specified.