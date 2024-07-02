# Simple http 
This is a simple http client that make a GET request to a server.

> ⚠️ **Warning:** The code is functionnal on stm32 `nucleo_h743zi` board, but:
> * The code is likely breaking the other OS WASI implem
>   * incrementing pointer in SSP.
> * The code contains lot of hardcoded values
>   * Mainly in the `http_get.c` file.
>   * Also in the `zephyr_file.c` file.
> * The WASI module does not use libc (wasi).   
>   * When linked with libc, the code does not work, it silently fails after connecting to the server.
>   * The `printf` function is not working, so it's hard to debug.
>
> It's a first step to support WASI on Zephyr 🎉

## Setup
1. Connect the USB cable to the Nucleo board.
2. Connect a network cable to the board ethernet port.
3. Configure the network interface on the host machine
    ```
    Internet Protocol Version 4 (TCP/IPv4) Properties:
        IP Address:         192.0.2.10
        Subnet Mask:        255.255.255.0
        Default Gateway:    192.0.2.2
    ```
4. Start a simple HTTP server on the host machine.
    ```bash
    python3 -m http.server --bind 0.0.0.0
    ```

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

    0. **Update Path:** change the path in the `Makefile` to your environment, and particularly the `WAMR_ROOT_DIR`, `CC` and `AR`.
    1. **Compile:** in the `wasm-apps` folder. 
        ```bash
        make all
        ```
    2. **generate a C header:** Use `xxd` or other tool, I also put simple python script. At application root `simple-http/`.
        ```bash
        python3 to_c_header.py
        ```
        Be free to modify the script to fit your needs.

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
        * **Define:** To not throw errors Posix flag were defined with Zephyr equivalent or similar to Zephyr POSIX implementation layer (eg: `POLLIN`, `POLLOUT`, `CLOCK_REALTIME`, ...).
        * **ifdef:** Some `#if defined(BH_PLATFORM_ZEPHYR)` were added to make the code compile.
        * **ssp_config:** The `ssp_config` file was modified to add support for Zephyr like for freeRTOS.
        * **posix.c:** The `posix.c` file in SSP was modified to pass correct address.
            * pointer incrementation on `__wasi_addr_t *`
                ```
                Before incrementation:
                +---+---+---+---+---+---+---+---+
                | 0 | 0 | 0 | 0 | 0 | 0 |10 | 2 |
                +---+---+---+---+---+---+---+---+
                                  ^ 
                                  |
                            __wasi_addr_t *

                After incrementation:
                +---+---+---+---+---+---+---+---+---+---+---+---+
                | 0 | 0 | 0 | 0 | 0 | 0 |10 | 2 | 0 |192| x | x |
                +---+---+---+---+---+---+---+---+---+---+---+---+
                                          ^ 
                                          |
                                    __wasi_addr_t *
                ```
            * In some functions we need to copy the address to a buffer to avoid write that could change the address.
                ```
                Without copy: __wasi_addr_t * --> 10.2.0.192
                Without copy: wasi_addr_to_string
                Without copy: __wasi_addr_t * --> 2.2.0.192
                # wasi_addr_to_string doesn't modify anything but the address is changed.
                ```
            > 📄 **Notes:** The reason for this offset will be investigated.

3. Now that the code compile, I changed build policy.
    * **previously:** I tried to build the `http_get.wasm` each time the simple-http sample is compiled.
    * **now:** I compile the `http_get.wasm` module standalone with `nostdlib` and include it in the `wasm-apps` folder. 
        ```bash
        wasm-apps/
        ├── Makefile                       # Indicative need to change path
        ├── http_get.c                     # Source code hardcoded IP / PORT
        ├── http_get.wasm                  # Compiled module
        └── inc
            ├── wasi_socket_ext.c          # Socket API external lib (modified)
            └── wasi_socket_ext.h          # Socket API external lib
        ```
    * **http_get.c:** changes functions to send and receive because it look like we can't use (function declared in sysroot):
	    * `__wasi_sock_recv`: wrapped by `recv` in `wasi_socket_ext.c`
	    * `__wasi_sock_send`: wrapped by `sendmsg` in `wasi_socket_ext.c`
	    * `__wasi_sock_shutdown`: not tested but likely to be the same.
        * `__wasi_sock_accept`: not tested but likely to be the same.

4. Change in `main.c`:
    * No longer use a thread to launch the runtime.
    * Added call to set WASI context.
    * Simplified the code just one function `main`.

> ⚠️ **Warning:** No other OS will compile because of the new functions and types.

> 📄 **Notes:** The abstraction of the `pollfd` was to use `poll()` but remain untested. 

### Outputs
1. **Serial Monitor Output:**
```bash
*** Booting Zephyr OS build v3.6.0-3137-g1ad4b5c61703 ***
[00:00:00.006,000] <inf> net_config: Initializing network
[00:00:00.011,000] <inf> net_config: Waiting interface 1 (0x20002584) to be up...
[00:00:01.896,000] <inf> net_config: Interface 1 (0x20002584) coming up
[00:00:01.903,000] <inf> net_config: IPv4 address: 192.0.2.1
global heap size: 131072
Wasm file size: 2918
[00:00:01:000 - 20002AB0]: warning: a module with WASI apis should be either a command or a reactor
[00:00:01:000 - 20002AB0]: warning: failed to link import function (env, close)
[00:00:01:000 - 20002AB0]: warning: failed to link import function (env, __assert_fail)
[00:00:01:000 - 20002AB0]: warning: failed to link import function (env, recv)
[OS-Layer] inet network
[OS-Layer] address:  192.0.2.10 To representation: = 3221225994
main found
[wasm-mod] Preparing HTTP GET request for http://192.0.2.10:8000/
[OS-Layer] Creating socket...
[OS-Layer] fstat
[OS-Layer] handle: 0
[OS-Layer] Dumbly return that fd is a socket
[wasm-mod] sock = 1
[sock-lib] wasi addr: 10.2.0.192
[SSP] sock connect
[SSP] wasi addr moved: 10.2.0.192
[SSP] raw addr port 8000
[SSP] addr pool search
[OS-Layer] inet network
[OS-Layer] address:  192.0.2.10 To representation: = 3221225994
[SSP] compare address
[SSP] compare address succeed
[SSP] addr pool search success: target = 167903424 | buf = 192.0.2.10
[SSP] fd_object_get_locked
[SSP] blocking op sock connect
[OS-Layer] About to connect to 192.0.2.10
[OS-Layer] Port: 8000
[wasm-mod] connect rc = 0
[SSP] wasi addr moved: 10.2.0.192
[SSP] raw addr port 8000
[SSP] addr pool search
[OS-Layer] inet network
[OS-Layer] address:  192.0.2.2 To representation: = 3221225986
[SSP] compare address
[SSP] compare address succeed
[SSP] addr pool search success: target = 33685696 | buf = 192.0.2.2
[SSP] fd_object_get_locked
[wasm-mod] send rc = 36
[wasm-mod] Response:

[SSP] wasi addr moved: 10.2.0.192
[SSP] raw addr port 8000
[SSP] fd_object_get_locked
[OS-Layer] Receiving from socket 0
[OS-Layer] Received 157 bytes
[SSP] message buffer size: 157
[wasm-lib] recv from ret code = 0
[wasm-lib] wasi to addr = 0
HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.10.10
Date: Thu, 16 May 2024 07:51:17 GMT
Content-type: text/html; charset=utf-8
Content-Length: 2821

# Skip rest of logs and response.

main executed
Exception: failed to call unlinked import function (env, close)
wasi exit code: 0
elapsed: 597ms
```