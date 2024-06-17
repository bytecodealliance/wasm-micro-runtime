## Test assembly file for new architecture
##### `tests/standalone/test_invoke_native` is a test case written by C  and invoke `wasm_runtime_invoke_native` function inside.

---

### RISCV
There are 4 `RISCV ABIs` support in WAMR: `lp64`, `lp64d`, `ilp32` and `ilp32`, and we test each ABI with different QEMU simulator using `test_invoke_native` test case.

```shell
> lp64 & ilp32 just use a0-a7[integer register], and all rest of args should be stored in to stack.
> lp64d & ilp32d use both a0-a7[int reg] and fa0-fa7[float reg].
```

- **lp64 & ilp32 -> zephyr qemu**

  1. clone zephyr project from [zephyr project](https://github.com/zephyrproject-rtos/zephyr.git)

  2. set up toolchain `zephyr-sdk` following [zephyr project guide](https://docs.zephyrproject.org/latest/getting_started/index.html#install-a-toolchain)

  3. copy the `test-invoke-native-zephyr`  to `zephyr-project` : 

     ```shell
     cp -r test-repo-root/tests/standalone/test-invoke-native-zephyr zephyr_prj_root/zephyr/samples/
     ```

  4. enter `test-invoke-native-zephyr`  and make soft link :

       ```shell
     cd zephyr_prj_root/zephyr/samples/test-invoke-native-zephyr
     ln -s wamr-root wamr
     ```

  5. build & run

     ```shell
     ./build_and_run.sh qemu_riscv32 #test riscv32_ilp32 target
     ./build_and_run.sh qemu_riscv64 #test riscv32_lp64 target
     ```



- **lp64d & ilp32d-> buildroot stable version**

  1.  clone `buildroot-stable-version` :

     ```shell
     wget https://buildroot.org/downloads/buildroot-2021.05.tar.gz
     tar xvf buildroot-2021.05.tar.gz
     cd buildroot-2021.05
     ```

  2.  set up `riscv32/64-default-config` :

     ```shell
     make qemu_riscv32_virt_defconfig #test ilp32d
     	(OR make qemu_riscv64_virt_defconfig #test lp64d)
     make menuconfig 
     	Filesystem images -> un-select `tar the root filesystem`
     make #build qemu image
     ```

     > After image building, the relevant cross-compile toolchain `riscv32/64-linux-gcc`, `riscv32/64-inux-ar` `riscv32/64-linux-ranlib`  will be generated under `out/host/bin`

  3.  export `GCC` toolchain path :

     ```shell
     export PATH:buildroot-2021.05/out/host/bin:$PATH
     ```

  4.  modify `test-invoke-native/CMakeLists.txt` :

     ```cmake
     set(CMAKE_C_COMPILER riscv32/64-linux-gcc)
     set(CMAKE_AR riscv32/64-linux-ar)
     set(CMAKE_RANLIB risc32/64-linux-ranlib)
     set(WAMR_BUILD_TARGET "RISCV32_ILP32D") #"RISCV64_LP64D" for riscv64_lp64
     
     set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wamr)
     ```

  5.  modify `build-scripts/config_common.cmake` :

     ```cmake
      #from line62 - 65. comment OR delete
      # else ()
      #   add_definitions (-m32)
      #   set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
      #   set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
     ```

  6.  build `test-invoke-native` program :

     ```shell
     cd test-invoke-native
     ln -s wamr_root wamr # make soft link for wamr
     mkdir build
     cd build && cmake ..
     make
     ```

  7.  copy the `test-invoke-native` program to `buildroot/out/target`:

     ```shell
     cp test-invoke-native/build/test_invoke_natve buildroot/out/target/root
     ```

  8.  re-make the image :

     ```shell
     cd buildroot && make
     ```

  9.  Boot the images :

     ```shell
      qemu-system-riscv32 \
        -M virt -nographic \
        -bios output/images/fw_jump.elf \
        -kernel output/images/Image \
        -append "root=/dev/vda ro" \
        -drive file=output/images/rootfs.ext2,format=raw,id=hd0 \
        -device virtio-blk-device,drive=hd0 \
        -netdev user,id=net0 -device virtio-net-device,netdev=net0
        
      login: root
      ./test_invoke_native #under /root
     ```

     

  

