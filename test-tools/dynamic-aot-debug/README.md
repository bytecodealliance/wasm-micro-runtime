# Dynamic AOT Module Debugging

### How to use

#### 1. Enable config

```
WASM_ENABLE_AOT=1
WASM_ENABLE_DYNAMIC_AOT_DEBUG=1
```

#### 2. Build aot and obj file

- build [wamrc](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/wamr-compiler) (enable WAMR_BUILD_DEBUG_AOT)
- build test.c to test.wasm

```
/opt/wasi-sdk/bin/clang -O0 -nostdlib -z stack-size=8192 -Wl,--initial-memory=65536 -g -gdwarf-2   -o test.wasm test.c -Wl,--export=main -Wl,--export=__main_argc_argv  -Wl,--export=__heap_base -Wl,--export=__data_end -Wl,--no-entry -Wl,--allow-undefined
```
You can specify other architectures and instruction sets to compile, here we take armv7 as an example：

- build test.wasm to test.aot

Compile and generate the test.aot file using wamrc without WAMR_BUILD_DEBUG_AOT.

```
./wamrc --opt-level=0 --target=thumbv7 --target-abi=gnueabihf --cpu=cortex-a7 --cpu-features=-neon -o test.aot test.wasm
```

- build test.wasm to test obj file

Use wamrc that needs to turn on WAMR_BUILD_DEBUG_AOT to compile and generate the test obj file.

```
./wamrc --opt-level=0 --format=object --target=thumbv7 --target-abi=gnueabihf --cpu=cortex-a7 --cpu-features=-neon -o test test.wasm
```

#### 3. Start emulator and nuttx

- Start emulator in terminal 1

```
/* start emulator on debug mode, and will start gdb server, set port as 1234 */
./emulator.sh vela -qemu -S -s
$ iwasm test.aot
```

- Start nuttx by GDB in terminal 2

```
/* You can save test obj file in this path */
export OBJ_PATH=~/work/data/
gdb-multiarch ./nuttx/nuttx -ex "tar remote:1234" -ex "source dynamic_aot_debug.py"
$(gdb) c
$(gdb) b main
```

#### 4. Workflow

image:: dynamic_aot_debug_workflow.png
