# WAMR MULTI-MODUEL SAMPLE

**WAMR supports _multi-module_ in both _interpreter_ mode and _aot_ mode.**

Multi-modules will determine the running mode based on the type of the main module.

## Interpreter mode

```bash
$ cmake -S . -B build
$ cmake --build
$ cd build
$ ./multi_module mC.wasm
```

## Aot mode

```bash
$ cmake -S . -B build -DWAMR_BUILD_AOT=1
$ cmake --build
$ cd build
$ ./multi_module mC.aot
```
