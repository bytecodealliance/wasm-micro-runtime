# Simple http 
This is a simple http client that make a GET request to a server.

## Setup
1. Connect the USB cable to the Nucleo board.
2. Optional: Connect a network cable to the board ethernet port.

## Run Command

1. **Build:** Replace `nucleo_h743zi` with your board name and `THUMBV8` with your target architecture.
    ```bash
    ZEPHYR_BASE=~/zephyrproject/zephyr \
    WAMR_ROOT_DIR=~/wasm-micro-runtime \
    WASI_SDK_PATH=~/wasi-sdk-21.0 \
    WAMR_APP_FRAMEWORK_DIR=~/wamr-app-framework \
    west build . -b nucleo_h563zi -p always -- -DWAMR_BUILD_TARGET=THUMBV8
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

## Work Done
1. Removed the previous introduced definition `-DWAMR_PLATFORM_ZEPHYR_FORCE_NO_ERROR`.

2. Implemented missing `os_` abstraction needed to compile.
    * **Increase Support:**
        * **Socket API**: nearly alls, Zephyr socket API is POXIX. Unable to test.
    * **Fix compilation errors:** mostly due to sandboxed primitives.
        * **File System API:**
            1. `os_convert_<stdin | stoud | stderr>_handle`: called in `wasm_runtime_common` by `wasm_runtime_init_wasi`.
            2. `os_file_get_access_mode`: called in `posix.c` by `fd_table_insert_existing` and `wasi_ssp_sock_accept`.
            3. `os_fstat`: called by in posix.x by `fd_determine_type_rights`.
        * **Thread API:**
            1. `os_rwlock_wrlock`: 
            2. `os_rwlock_init`: called by `posix.c` by `fd_table_init`.
            3. `os_rwlock_unlock`:
        * **New Functions:**
            1. `os_ioctl`: because `ioctl` was called in `posix.c` and caused compilation error.
            2. `os_poll`: because `poll` was called in `blocking_op.c`.
        * **New types:**
            1. `os_poll_file_handle`: because `pollfd` was used in `blocking_op.c` and `posix.c` 
            2. `os_nfds_t`: because `nfds_t` was used in `blocking_op.c` (could pass `unsigned int` instead of abstracting).
            3. `os_timespec`: because `timespec` was used in `posix.c`.
        * **Define:** To not throw errors Posix flag were defined with Zephyr equivalent or similar to Zephyr POSIX implemtation layer (eg: `POLLIN`, `POLLOUT`, `CLOCK_REALTIME`, ...).
        * **ifdef:** Some `#if defined(BH_PLATFORM_ZEPHYR)` were added to make the code compile.
        * **ssp_config:** The `ssp_config` file was modified to add support for Zephyr like for freeRTOS.

3. Now that the code compile, I changed build policy.
    * **previously:** I tried to build the `http_get.wasm` each time the simple-http sample is compiled.
    * **now:** I compile the `http_get.wasm` module standalone and include it in the `wasm-apps` folder. I still let the source code and dependencies in the present.
    * Then I still use the python script to make an c byte array, it's quite heavy. I'm investigationg how to pass the module without it. 

4. Change in `main.c`:
    * No longer use a thread to launch the runtime.
    * Added call to set WASI context.
    * Simplified the code just one function `main`.

⚠️ **Warning:** No other OS will compile because of the new functions and types.

### Outputs
1. **Serial Monitor Output:**
    ```bash
    *** Booting Zephyr OS build v3.6.0-3137-g1ad4b5c61703 ***
    [00:00:00.006,000] <inf> net_config: Initializing network
    [00:00:00.011,000] <inf> net_config: Waiting interface 1 (0x200098fc) to be up...
    [00:00:01.501,000] <inf> net_config: Interface 1 (0x200098fc) coming up
    [00:00:01.508,000] <inf> net_config: IPv4 address: 192.0.2.1
    global heap size: 153600
    Wasm file size: 32478
    Creating exec_env
    Calling main function
    Failed to call main function
    Exception: wasi proc exit
    wasi exit code: 1
    elapsed: 56
    ```
2. **Error Code:**
    * `wasi proc exit`: From what I read it is similar to a POSIX `exit()`.
    * `wasi errno`: The returned status code is 1 so `_WASI_E2BIG`.

3. **Try to solve:**
    * I suspected a lack of memory so I increased both heap and stack.
        ```
        Runtime stack                = 8KB
        Runtime global heap pool     = 150KB
        App stack                    = 8KB
        App heap                     = 40KB
        ```
    * I tried to build `iwasm` (samples/socket-api) on Linux with these parameters:
        ```cmake
        set(WAMR_BUILD_GLOBAL_HEAP_SIZE  153600)
        set(DEFAULT_WASM_STACK_SIZE 8192)
        ```
        It worked, and executed the `http_get.wasm` module without problem.
    * I also saw that in the samples/socket-api we link `libm` and `libdl`:
        ```cmake
        target_link_libraries(iwasm vmlib -lpthread -lm -ldl)
        ```
        Not sure how should I handle this because here I'm building my executable with zephyr toolchain and it doesn't contain the `libdl`. May be I should link with WASI toolchain `libdl` and `libm`.


## Expected Output
### Host
```bash
python3 -m http.server --bind 0.0.0.0
Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
127.0.0.1 - - [12/Apr/2024 09:54:24] "GET / HTTP/1.0" 200 -
```

### Target
```bash
Preparing HTTP GET request for http://127.0.0.1:8000/
sock = 5
Response:

HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.10.12
Date: Thu, 11 Apr 2024 13:41:11 GMT
Content-type: text/html
Content-Length: 14054
Last-Modified: Thu, 30 Mar 2023 09:11:09 GMT

# Skip HTML content
```