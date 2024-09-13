# Dynamic AOT Module Debugging

This guide outlines the process for debugging WAMR AOT module compiled modules using dynamic AOT debugging features. Follow the steps below to set up and run your debugging environment.

## How to use

### 1. Enable Debug Configuration

To enable dynamic AOT debugging, set the following
compile macro switch:

```
cmake -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_DYNAMIC_AOT_DEBUG=1
```

### 2. Build AOT and Object Files

#### 2.1 Build wamrc Compiler

Ensure that wamrc is built with the WAMR_BUILD_DEBUG_AOT flag enabled. Please refer to the first two steps in [doc/source_debugging_aot.md](../../doc/source_debugging_aot.md).

#### 2.2 Compile Source Code to WebAssembly (WASM)

Compile test.c to test.wasm using the following command:

```
/opt/wasi-sdk/bin/clang -O0 -nostdlib -z stack-size=8192 -Wl,--initial-memory=65536
-g -gdwarf-2 -o test.wasm test.c -Wl,--export=main -Wl,--export=__main_argc_argv
-Wl,--export=__heap_base -Wl,--export=__data_end -Wl,--no-entry -Wl,--allow-undefined
```

Adjust the compiler options for different architectures and instruction sets. For example, to target ARMv7:

#### 2.3 Compile WebAssembly to AOT File

Generate the test.aot file using wamrc with the following command. Ensure WAMR_BUILD_DEBUG_AOT is not enabled here:

```
./wamrc --opt-level=0 --target=thumbv7 --target-abi=gnueabihf --cpu=cortex-a7 
--cpu-features=-neon -o test.aot test.wasm
```

#### 2.4 Compile WebAssembly to Object File

Generate the test object file using wamrc with WAMR_BUILD_DEBUG_AOT enabled:

```
./wamrc --opt-level=0 --format=object --target=thumbv7 --target-abi=gnueabihf 
--cpu=cortex-a7 --cpu-features=-neon -o test test.wasm
```

### 3. Start Emulator and NuttX

#### 3.1 Start Emulator

In Terminal 1, start the emulator in debug mode and launch the GDB server:

```
# start emulator on debug mode, and will start gdb server, set port as 1234
$ ./emulator.sh vela -qemu -S -s
ap> iwasm test.aot
```

#### 3.2 Start NuttX Using GDB

In Terminal 2, set the path to your object file and start NuttX with GDB:

```
# You can save test obj file in this path
export OBJ_PATH=~/work/data/
gdb-multiarch ./nuttx/nuttx -ex "tar remote:1234" -ex "source dynamic_aot_debug.py"
```

In the GDB prompt:

```
$(gdb) c
$(gdb) b main
```

### 4. Workflow

Refer to the workflow diagram (wasm-micro-runtime/test-tools/dynamic-aot-debug) for an overview of the debugging process.
