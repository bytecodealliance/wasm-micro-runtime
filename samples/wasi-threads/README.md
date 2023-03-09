# "WASI threads" sample introduction

Currently, since the `wasi-sdk` does not have thread support in the latest release, make sure to have [wasi-libc](https://github.com/WebAssembly/wasi-libc) installed. Build it with threads enabled, e.g.

```shell
make \
    AR=/opt/wasi-sdk/bin/llvm-ar \
    NM=/opt/wasi-sdk/bin/llvm-nm \
    CC=/opt/wasi-sdk/bin/clang \
    THREAD_MODEL=posix
```

## Build and run the samples

```shell
$ mkdir build
$ cd build
$ cmake -DWASI_SYSROOT=/path/to/wasi-libc/sysroot ..
$ make
...
$ ./iwasm wasm-apps/no_pthread.wasm
...
$ ./iwasm wasm-apps/exception_propagation.wasm
```

## Run samples in AOT mode
```shell
$ ../../../wamr-compiler/build/wamrc \
    --enable-multi-thread \
    -o wasm-apps/no_pthread.aot wasm-apps/no_pthread.wasm
$ ./iwasm wasm-apps/no_pthread.aot
```
