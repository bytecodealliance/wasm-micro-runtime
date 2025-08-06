# Socket sample 
this sample demonstrates the use of WASI API to interact with sockets.

> ❗ **Important:** This sample was ported/adapted from the http_get zephyr sample. The original sample can be found [here]( https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/net/sockets/http_get/src/http_get.c).

> 🛠️ **Work in progress:** The sample is functional but be aware that just a small part of WASI socket API was tested.
> Actual Zephyr APIs: 
> * socket creation = `zsock_socket`
> * socket connection = `zsock_connect`
> * socket emission = `zsock_sendto`
> * socket reception = `zsock_recvfrom`
> * socket destruction = `zsock_close`
>
> With the sockets most API are in fact provided by the runtime instead of WASI because of the lack of socket support in WASI preview1.

## Setup
1. Connect a network cable to the board ethernet port.
2. Configure the network interface on the host machine
    ```
    Internet Protocol Version 4 (TCP/IPv4) Properties:
        IP Address:         192.0.2.10
        Subnet Mask:        255.255.255.0
        Default Gateway:    192.0.2.2
    ```
3. Start a simple HTTP server on the host machine.
    ```bash
    python3 -m http.server --bind 0.0.0.0
    ```
4. Disable any firewall that may block the connection.

## Configuration
To configure the server side IP address and port modify the following lines in the `http_get.c` file.

1. The `HTTP_HOST` and `HTTP_PORT` macros define the server IP address and port.
    ```c
    /* HTTP server to connect to */
    #define HTTP_HOST "192.0.2.10"
    /* Port to connect to, as string */
    #define HTTP_PORT "8000"
    /* HTTP path to request */
    #define HTTP_PATH "/"

    // ...

    #define REQUEST "GET " HTTP_PATH " HTTP/1.0\r\nHost: " HTTP_HOST "\r\n\r\n"
    ```
    > 📄 **Notes:** These macros are used to build the request string, but they are not used to instantiate the address structure. Because at one point we didn't want to use `inet_pton` to convert the string to an address and it remained like this.

2. The `addr` structure is used to store the server address.
    ```c
    addr.sin_port = htons(8000);
        addr.sin_addr.s_addr =
            htonl(0xC000020A); // hard coded IP address for 192.0.2.10
    ```

To configure the authorized IP address(es) modify the following lines in the `main.c` file. WAMR will only allow the IP addresses in the pool to connect to the server.
```c
#define ADDRESS_POOL_SIZE 1
    const char *addr_pool[ADDRESS_POOL_SIZE] = {
        "192.0.2.10/24",
    };
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

    0. **Compile a static lib:** in the `wasm-apps` folder. 
        * **Compile the an object:**
        ```bash
        ~/wasi-sdk-21.0/bin/clang --sysroot=/home/user/wasi-sdk-21.0/share/wasi-sysroot -Iinc/ -c inc/wasi_socket_ext.c -o inc/wasi_socket_ext.o
        ```
        * **Create a static lib:**
        ```bash
        ~/wasi-sdk-21.0/bin/llvm-ar rcs inc/libwasi_socket_ext.a inc/wasi_socket_ext.o
        ```
    1. **Compile:** in the `wasm-apps` folder. 
        ```bash
        ~/wasi-sdk-21.0/bin/clang --sysroot=/home/user/wasi-sdk-21.0/share/wasi-sysroot -Iinc/ -nodefaultlibs -o http_get.wasm http_get.c -lc -Linc/ -lwasi_socket_ext -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--export=__heap_base -Wl,--export=__data_end  -Wl,--allow-undefined
        ```
    2. **generate a C header:** Use `xxd` or other tool, I also put simple python script. At application root `simple-http/`.
        ```bash
        python3 to_c_header.py
        ```
        Be free to modify the script to fit your needs.

## Output
The output should be similar to the following:
```bash
*** Booting Zephyr OS build v3.6.0-4305-g2ec8f442a505 ***
[00:00:00.061,000] <inf> net_config: Initializing network
[00:00:00.067,000] <inf> net_config: Waiting interface 1 (0x2000a910) to be up...
[00:00:03.158,000] <inf> phy_mii: PHY (0) Link speed 100 Mb, full duplex

[00:00:03.288,000] <inf> net_config: Interface 1 (0x2000a910) coming up
[00:00:03.295,000] <inf> net_config: IPv4 address: 192.0.2.1
global heap size: 131072
Wasm file size: 36351
main found
[wasm-mod] Preparing HTTP GET request for http://192.0.2.10:8000/
[wasm-mod] sock = 3
[wasm-mod] connect rc = 0
[wasm-mod] send rc = 36
[wasm-mod] Response:

HTTP/1.0 200 OK
Server: SimpleHTTP/0.6 Python/3.10.10
Date: Fri, 14 Jun 2024 07:26:56 GMT
Content-type: text/html; charset=utf-8
Content-Length: 2821

# Skip the HTML content

[wasm-mod] len = 0 break

[wasm-mod] Connection closed
main executed
wasi exit code: 0
elapsed: 405ms
```