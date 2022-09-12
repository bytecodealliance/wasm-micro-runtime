# "socket-api" sample introduction

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

`iwasm` and three Wasm modules, `tcp_server.wasm`, `tcp_client.wasm`, `send_recv.wasm`
will be generated. And their corresponding native version, `tcp_server`,
`tcp_client`, `send_recv` are generated too.

> Note that iwasm is built with libc-wasi and lib-pthread enabled.

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

`send_recv.wasm` contains a thread as a server and a thread as a client. They
send and receive data via 127.0.0.1:1234.

```bash
$ ./iwasm --addr-pool=127.0.0.1/0  ./send_recv.wasm
```

The output is:

```bash
Server is online ...
Client is running...
Start receiving.
Start sending.
Send 106 bytes successfully!
Receive 106 bytes successlly!
Data:
  The stars shine down
  It brings us light
  Light comes down
  To make us paths
  It watches us
  And mourns for us
```

`socket_opts.wasm` shows an example of getting and setting various supported socket options
```bash
$ ./iwasm ./socket_opts.wasm
```

The output is:
```bash
[Client] Create TCP socket
[Client] Create UDP socket
[Client] Create UDP IPv6 socket
SO_RCVTIMEO tv_sec is expected
SO_RCVTIMEO tv_usec is expected
SO_SNDTIMEO tv_sec is expected
SO_SNDTIMEO tv_usec is expected
SO_SNDBUF buf_len is expected
SO_RCVBUF buf_len is expected
SO_KEEPALIVE enabled is expected
SO_KEEPALIVE disabled is expected
SO_REUSEADDR enabled is expected
SO_REUSEADDR disabled is expected
SO_REUSEPORT enabled is expected
SO_REUSEPORT disabled is expected
SO_LINGER l_onoff is expected
SO_LINGER l_linger is expected
SO_BROADCAST enabled is expected
SO_BROADCAST disabled is expected
TCP_KEEPIDLE is expected
TCP_KEEPINTVL is expected
TCP_FASTOPEN_CONNECT enabled is expected
TCP_FASTOPEN_CONNECT disabled is expected
TCP_NODELAY enabled is expected
TCP_NODELAY disabled is expected
TCP_QUICKACK enabled is expected
TCP_QUICKACK disabled is expected
IP_TTL is expected
IPV6_V6ONLY enabled is expected
IPV6_V6ONLY disabled is expected
IP_MULTICAST_LOOP enabled is expected
IP_MULTICAST_LOOP disabled is expected
IP_ADD_MEMBERSHIP is expected
IP_DROP_MEMBERSHIP is expected
IP_MULTICAST_TTL is expected
IPV6_MULTICAST_LOOP enabled is expected
IPV6_MULTICAST_LOOP disabled is expected
[Client] Close sockets
```

Refer to [socket api document](../../doc/socket_api.md) for more details.
