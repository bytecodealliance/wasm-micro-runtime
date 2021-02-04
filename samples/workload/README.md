All workloads have similar requirment of software dependencies, including
**wasi-sdk**, **emsdk**, **wabt** and **binaryen**

> There might be slight differences when using MacOS and other Linux distro than Ubuntu. This document only target
Ubuntu 18.04 as example.

## Installation instructions

use [preparation.sh](./preparation.sh) to install all dependencies before compiling any workload.

for details, the script includes below steps:

- **wasi-sdk**. Install
  [latest release](https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-linux.tar.gz)
  to */opt/wasi-sdk*

- **wabt**. Install
  [latest release](https://github.com/WebAssembly/wabt/releases/download/1.0.20/wabt-1.0.20-ubuntu.tar.gz)
  to */opt/wabt* or */opt/wabt-1.0.20*

- **emsdk**. Refer to [the guide](https://emscripten.org/docs/getting_started/downloads.html). Don't forget to activate
  emsdk and set up environment variables. Verify it with `echo ${EMSDK}`.

- **binaryen**. Install
  [latest release](https://github.com/WebAssembly/binaryen/releases/download/version_97/binaryen-version_97-x86_64-linux.tar.gz)
  to */opt/binaryen* or */opt/binaryen-version_97*
