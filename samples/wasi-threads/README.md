# "WASI threads" sample introduction

Currently, since the `wasi-sdk` does not have thread support in the latest release, make sure to have [wasi-libc](https://github.com/WebAssembly/wasi-libc) installed. After cloning, build the branch `wasi-sdk-17` with threads enabled, e.g.

```shell
make \
    AR=/opt/wasi-sdk/bin/llvm-ar \
    NM=/opt/wasi-sdk/bin/llvm-nm \
    CC=/opt/wasi-sdk/bin/clang \
    THREAD_MODEL=posix
```
A successful build outputs the path to the wasi-libc sysroot. Use the path to build and run the samples, as shown below.

### Build and run the samples

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
