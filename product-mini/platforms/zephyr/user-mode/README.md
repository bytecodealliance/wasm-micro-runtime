# How to use WAMR with Zephyr in user mode

This example demonstrates how to build and run a WebAssembly application in user mode on Zephyr.

> Note: The user mode is not supported on all Zephyr boards. Please refer to the Zephyr documentation for more information.

## Setup

Please refer to the [this README.md](../simple/README.md) for general Zephyr setup instructions.

### Example Targets

x86_64 QEMU (x86_64) is a 64-bit x86 target for emulating the x86_64 platform.

```shell
west build -b qemu_x86_tiny . -p always -- -DWAMR_BUILD_TARGET=X86_32
```

Use qemu to run the image.

```shell
qemu-system-i386 -m 32 -cpu qemu32,+nx,+pae -machine pc -device isa-debug-exit,iobase=0xf4,iosize=0x04 -no-reboot -nographic -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on -serial chardev:con -mon chardev=con,mode=readline -icount shift=5,align=off,sleep=off -rtc clock=vm -kernel ./build/zephyr/zephyr.elf
```
