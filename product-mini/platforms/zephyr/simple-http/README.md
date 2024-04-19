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

## Different approach

* **Soft:** Trying to implement missing API, and enhancing the existing abstraction API.

* **Hard:** Making the code compile and run with the existing API. 

By default, we are following the **Soft** approach. If you want to follow the **Hard** approach, you need to uncomment the following line in the `shared_plateform.cmake` file.
```cmake
# add_definitions (-DWAMR_PLATFORM_ZEPHYR_FORCE_NO_ERROR)
```

### Outputs

* **Soft:** Unable to compile the code.

* **Hard:** The code will compile but cause a stack overflow error.
    ```bash
    [00:00:00.001,000] <err> os: ***** USAGE FAULT *****
    [00:00:00.007,000] <err> os:   Stack overflow (context area not valid)
    [00:00:00.014,000] <err> os: r0/a1:  0xf0f0f0f0  r1/a2:  0x693b613b  r2/a3:  0x0807be46
    [00:00:00.022,000] <err> os: r3/a4:  0x09000000 r12/ip:  0x2002fb58 r14/lr:  0x0804f0b1
    [00:00:00.031,000] <err> os:  xpsr:  0x08080c00
    [00:00:00.036,000] <err> os: Faulting instruction address (r15/pc): 0x2a1b681b
    [00:00:00.044,000] <err> os: >>> ZEPHYR FATAL ERROR 2: Stack overflow on CPU 0
    [00:00:00.052,000] <err> os: Current thread: 0x20002ef8 (unknown)
    [00:00:00.059,000] <err> os: Halting system
    ```

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