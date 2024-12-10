# File sample 
This sample demonstrates the use of WASI API to interact with the file system.

> 🛠️ **Work in progress:** The sample is functional but be aware that just a small part of WASI File System API was tested.
> Actual Zephyr APIs: 
> * directory creation = `fs_mkdir`
> * file opening/creation = `fs_open`
> * file write = `fs_write`
> * file offset = `fs_seek`
> * file read = `fs_read`
> * file close = `fs_close`
> * directory close = `fs_closedir`

## Run Command
* **Zephyr Build**
    1. **Build:** Replace `nucleo_h743zi` with your board name and the `WAMR_BUILD_TARGET` in `CMakeList.txt` with your target architecture.
        ```bash
        ZEPHYR_BASE=~/zephyrproject/zephyr \
        WAMR_ROOT_DIR=~/wasm-micro-runtime \
        WASI_SDK_PATH=~/wasi-sdk-21.0 \
        WAMR_APP_FRAMEWORK_DIR=~/wamr-app-framework \
        west build . -b nucleo_h563zi -p always 
        ```
        ⚠️ **Warning:** The flags `ZEPHYR_BASE`, `WAMR_ROOT_DIR`, `WASI_SDK_PATH`, and `WAMR_APP_FRAMEWORK_DIR` need to be set otherwise the build will fail.

    2. **Flash:** 
        ```bash
        ZEPHYR_BASE=~/zephyrproject/zephyr west flash
        ```

    3. **Monitor:** Use a serial link to monitor the output. Personally, I use minicom.
        ```bash
        minicom -D /dev/ttyACM0
        ```

    4. **Debug:** Curently investigating.

* **WebAssembly Module**

    ❗ **Important:** I used wasi-sdk 21 to compile the module. I still haven't tried the module with the new wasi-sdk 22.

    1. **Compile:** in the `wasm-apps` folder. 
        ```bash
        ~/wasi-sdk-21.0/bin/clang --sysroot=/home/user/wasi-sdk-21.0/share/wasi-sysroot -nodefaultlibs -lc -o file.wasm file.c -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--export=__heap_base -Wl,--export=__data_end
        ```
    2. **generate a C header:** Use `xxd` or other tool, I also put simple python script. At application root `simple-file/`.
        ```bash
        python3 to_c_header.py
        ```
        Be free to modify the script to fit your needs.

## Output
The output should be similar to the following:
```bash
*** Booting Zephyr OS build v3.6.0-4305-g2ec8f442a505 ***
Area 3 at 0x1f0000 on flash-controller@40022000 for 65536 bytes
[00:00:00.067,000] <inf> littlefs: LittleFS version 2.8, disk version 2.1
[00:00:00.074,000] <inf> littlefs: FS at flash-controller@40022000:0x1f0000 is 8 0x2000-byte blocks with 512 cycle
[00:00:00.085,000] <inf> littlefs: sizes: rd 16 ; pr 16 ; ca 64 ; la 32
[00:00:00.092,000] <err> littlefs: WEST_TOPDIR/modules/fs/littlefs/lfs.c:1351: Corrupted dir pair at {0x0, 0x1}
[00:00:00.103,000] <wrn> littlefs: can't mount (LFS -84); formatting
[00:00:00.114,000] <inf> littlefs: /lfs mounted
/lfs mount: 0
[00:00:00.120,000] <inf> main: stdin = 0
[00:00:00.124,000] <inf> main: stdout = 1
[00:00:00.128,000] <inf> main: stderr = 2
[00:00:00.133,000] <inf> main: global heap size: 131072
[00:00:00.142,000] <inf> main: Wasm file size: 34682
[00:00:00:000 - 2000AFE0]: WASI context initialization: START

[OS] os_rwlock_init
[OS] os_rwlock_init
[00:00:00:000 - 2000AFE0]: WASI context initialization: END

[00:00:00.190,000] <inf> main: main found
Hello WebAssembly Module !

mkdir returned 0
fopen Succeed
fwrite returned 13
fseek returned 0
fread returned 13
buffer read = Hello, World!

[00:00:00.225,000] <inf> main: main executed
[00:00:00.230,000] <inf> main: wasi exit code: 0
[00:00:00.239,000] <inf> main: elapsed: 178ms
[00:00:03.158,000] <inf> phy_mii: PHY (0) Link speed 100 Mb, full duplex

[00:00:00.051,000] <inf> phy_mii: PHY (0) ID 7C131
```