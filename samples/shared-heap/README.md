# Shared heap Sample introduction

This is a sample to show how to use the shared heap feature in WAMR. The shared heap feature allows multiple WASM instances to share the same memory space. This feature is useful when you want to run multiple WASM instances in the same process and share data between them. The sandbox nature of WASM is still maintained in the shared heap by WAMR. But the data management and correct data synchronization in shared heap is relied on the user's implementation.

> Note: The shared heap feature is experimental feature, it should be used with caution. It's optional and only available when building WAMR with the CMake cache variable `WAMR_BUILD_SHARED_HEAP` set to 1.

## Build and run the sample

For the simple shared heap sample, it demonstrates how to create a shared heap and use it shares data between two WASM instances, which would satisfy most of the use cases. Use the following commands to build the sample:

```bash
cmake -S . -B build
cmake --build build
```

For the shared heap chain sample. It chains a pre-allocated heap and a normal shared heap to one chain as a whole and attaches/detaches all together, and pass the WASM address directly bewteen two WASM instances. Use the following commands to build the sample:

```bash
cmake -S . -DUSE_SHARED_HEAP_CHAIN=1 -B build
cmake --build build
```

> PS: This cache variable `USE_SHARED_HEAP_CHAIN` is only used in this sample to demonstrate how to use the shared heap chain feature, it's not a compile flag in WAMR.
