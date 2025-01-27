# Shared heap Sample introduction

This is a sample to show how to use the shared heap feature in WAMR. The shared heap feature allows multiple WASM instances to share the same memory space. This feature is useful when you want to run multiple WASM instances in the same process and share data between them.

> Note: The shared heap feature is experimental feature, optional and only available when building WAMR with the CMake cache variable `WAMR_BUILD_SHARED_HEAP` set to 1. The sandbox nature of WASM is still maintained in the shared heap by WAMR. But the data management and correct data synchronization in shared heap is relied on the user's implementation.

## Build and run the sample

For shared heap sample

```bash
cmake -S . -B build
cmake --build build
```

For shared heap chain sample. It chains a pre-allocated heap and a normal shared heap to one chain as a whole and attaches/detaches all together. This cache variable `USE_SHARED_HEAP_CHAIN` is only used in this sample to demonstrate how to use the shared heap chain feature, it's not a compile flag in WAMR.

```bash
cmake -S . -DUSE_SHARED_HEAP_CHAIN=1 -B build
cmake --build build
```
