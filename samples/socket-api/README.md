"socket-api" sample introduction
================================

This sample demonstrates how to use WAMR socket-api to develop wasm network applications.
Two wasm applications are provided: tcp-server and tcp-client, and this sample demonstrates
how they communicate with each other.

## Preparation

Please install WASI SDK, download the [wasi-sdk release](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path `/opt/wasi-sdk`.
And install wabt, download the [wabt release](https://github.com/WebAssembly/wabt/releases) and extract the archive to default path `/opt/wabt`

## Build the sample

```bash
mkdir build
cd build
cmake ..
make
```

The file `tcp_server.wasm`, `tcp_client.wasm` and `iwasm` will be created.
And also file `tcp_server` and `tcp_client` of native version are created.

Note that iwasm is built with libc-wasi and lib-pthread enabled.

## Run workload

Start the tcp server, which opens port 1234 and waits for clients to connect.
```bash
cd build
./iwasm --addr-pool=0.0.0.0/15 tcp_server.wasm
```

Start the tcp client, which connects the server and receives message.
```bash
cd build
./iwasm --addr-pool=127.0.0.1/15 tcp_client.wasm
```

The output of client is like:
```bash
[Client] Create socket
[Client] Connect socket
[Client] Client receive
[Client] 115 bytes received:
Buffer recieved:
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server
Say Hi from the Server

[Client] BYE
```

Refer to [socket api document](../../doc/socket_api.md) for more details.
