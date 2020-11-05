All workloads have similar a requirment of software dependencies. It includes
**wasi-sdk**, **clang-11**, **emsdk**, **wabt** and **binaryen**

> It might slightly different when using MacOS, and other linux distro than Ubuntu. This document only target
Ubuntu 18.04 as an example.

## Installation instructions

- **wasi-sdk**. Install
  [latest release](https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-11/wasi-sdk-11.0-linux.tar.gz)
  in */opt/wasi-sdk* or */opt/wasi-sdk-11*

- **wabt**. Install
  [latest release](https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-1.0.19-ubuntu.tar.gz)
  in */opt/wabt* or */opt/wabt-1.0.19*

- **clang-11**. Refer to [the guide](https://apt.llvm.org/).

- **emsdk**. Refer to [the guide](https://emscripten.org/docs/getting_started/downloads.html). Don't forget to activate
  emsdk and set up environment variables. Verify it with `echo ${EMSDK}`.

- **libclang_rt.builtins-wasm32.a**. *wasi* has its private rt library. Put it under clang search path

``` shell
# copy it
$ cp -r /opt/wasi-sdk-11.0/lib/clang/10.0.0/lib/wasi /usr/lib/llvm-11/lib/clang/11.0.0/lib/

# or just link it
$ ln -sf /opt/wasi-sdk-11.0/lib/clang/10.0.0/lib/wasi/ /usr/lib/llvm-11/lib/clang/11.0.0/lib/
```

- **binaryen**. Install
  [latest release](https://github.com/WebAssembly/binaryen/releases/download/version_97/binaryen-version_97-x86_64-linux.tar.gz)
  in */opt/binaryen* or */opt/binaryen-version_97*
