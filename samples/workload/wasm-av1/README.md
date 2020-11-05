"wasm-av1" sample introduction
==============
This sample demonstrates how to build [wasm-av1](https://github.com/GoogleChromeLabs/wasm-av1) into WebAssembly with emcc toolchain and run it with iwasm. Please first install [emsdk](https://github.com/emscripten-core/emsdk):
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
```
And set up ensdk environment:
```bash
source emsdk_env.sh
```
Then run
```bash
./build.sh
```
to build wasm-av1 and run it with iwasm, which basically contains the following steps:
- hack emcc to delete some objects in libc.a
- patch wasm-av1 and build it with emcc compiler
- build iwasm with simd and libc-emcc support
- run testav1.aot with iwasm
