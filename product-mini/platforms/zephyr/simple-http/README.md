# Simple http 
This is a simple http client that make a GET request to a server.

## Setup
TODO

## Run Command
Replace `nucleo_h743zi` with your board name.

```bash
ZEPHYR_BASE=~/zephyrproject/zephyr \
WAMR_ROOT_DIR=~/wasm-micro-runtime \
WASI_SDK_PATH=~/wasi-sdk-21.0 \
WAMR_APP_FRAMEWORK_DIR=~/wamr-app-framework \
west build . -b nucleo_h563zi -p always -- -DWAMR_BUILD_TARGET=THUMBV8 -DCONFIG_LOG_MODE_IMMEDIATE=y
```