# WAMR as an Enclave Runtime for Rune

## Build WAMR vmcore (iwasm) for Linux-SGX

Please follow [this guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md#build-wamr-vmcore-iwasm-for-linux-sgx) to build iwasm as the prerequisite.

Then build enclave image and sign it:

```shell
cd enclave-sample
make
/opt/intel/sgxsdk/bin/x64/sgx_sign sign -key Enclave/Enclave_private.pem -enclave enclave.so -out enclave.signed.so -config Enclave/Enclave.config.xml
```

The generated enclave.signed.so is required by PAL.

---

## Build PAL dynamically linked shared object

To build WAMR as an Enclave Runtime for [Inclavare Containers](https://github.com/alibaba/inclavare-containers), we should implement the [PAL API v2](https://github.com/alibaba/inclavare-containers/blob/master/rune/libenclave/internal/runtime/pal/spec_v2.md) in WAMR for rune to call the PAL to create the enclave with WAMR and run applications.

```shell
g++ -shared -fPIC -o libwamr-pal.so App/*.o libvmlib_untrusted.a -L/opt/intel/sgxsdk/lib64 -lsgx_urts -lpthread -lssl -lcrypto
cp ./libwamr-pal.so /usr/lib/libwamr-pal.so
```

Note: Assuming `/opt/intel/sgxsdk/` is where you installed the SGX SDK.

---

## Build WAMR application

To Build a WAMR application, please refer to [this guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md#build-wasm-applications)

To run a WAMR application with Intel SGX enclave by `rune`, please refer to [this guide](https://github.com/bytecodealliance/wasm-micro-runtime#build-wamrc-aot-compiler) to generate wamrc AoT compiler, and then refer to [this guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md#compile-wasm-to-aot-module) to compile the `.wasm` file to `.aot` file.

Note: the AoT file must be generated using --size-level=1 to set a bigger code size, e.g,

```shell
wamrc --size-level=1 -o test.aot test.wasm
```

---

## Build WAMR docker image

Under the `enclave-sample` directory, to create the WAMR docker images to load the `enclave.signed.so` and target application wasm files, please type the following commands to create a `Dockerfile`:

For centos:

```shell
cat >Dockerfile <<EOF
FROM centos:8.1.1911

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
COPY ${wasm_app.aot} .
#COPY ${wasm_app2.aot} .
#...
EOF
```

For ubuntu:

```shell
cat > Dockerfile <<EOF
FROM ubuntu:18.04

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
COPY ${wasm_app.aot} .
#COPY ${wasm_app.aot} .
#...
EOF
```

where `${wasm_app.aot}` files are the other applications you want to run in WAMR.

Then build the WAMR docker image with the command:

```shell
docker build . -t wamr-app
```

---

## Run WAMR SGX with Docker and OCI Runtime rune

The following guide provides the steps to run WAMR with Docker and OCI Runtime `rune`.

[rune](https://github.com/alibaba/inclavare-containers/tree/master/rune) is a novel OCI Runtime used to run trusted applications in containers with the hardware-assisted enclave technology.

### Requirements

- Ensure that you have one of the following required operating systems to build a WAMR docker image:

  - CentOS 8.1
  - Ubuntu 18.04-server

- Please follow [Intel SGX Installation Guide](https://download.01.org/intel-sgx/sgx-linux/2.11/docs/Intel_SGX_Installation_Guide_Linux_2.11_Open_Source.pdf) to install Intel SGX driver, Intel SGX SDK & PSW for Linux.

  - For CentOS 8.1, UAE service libraries are needed but may not installed if SGX PSW installer is used. Please manually install it:

    ```shell
    rpm -i libsgx-uae-service-2.11.100.2-1.el8.x86_64.rpm
    ```

- The simplest way to install `rune` is to download a pre-built binary from [Inclavare Containers release page](https://github.com/alibaba/inclavare-containers/releases).

### Configuring OCI Runtime rune for Docker

Add the assocated configuration for `rune` in dockerd config file, e.g, `/etc/docker/daemon.json`, on your system.

```
{
	"runtimes": {
		"rune": {
			"path": "/usr/local/bin/rune",
			"runtimeArgs": []
		}
	}
}
```

then restart dockerd on your system.

You can check whether `rune` is correctly enabled or not with:

```
docker info | grep rune
```

The expected result would be:

```
Runtimes: rune runc
```

### Run WAMR docker image

You need to specify a set of parameters to `docker run` to run:

```shell
docker run -it --rm --runtime=rune \
  -e ENCLAVE_TYPE=intelSgx \
  -e ENCLAVE_RUNTIME_PATH=/usr/lib/libwamr-pal.so \
  -e ENCLAVE_RUNTIME_ARGS=debug \
  wamr-app /run/rune/${wasm_app.aot}
```

where:

- @ENCLAVE_TYPE: specify the type of enclave hardware to use, such as `intelSgx`.
- @ENCLAVE_RUNTIME_PATH: specify the path to enclave runtime to launch. For an WAMR application, you need to specify the path to `libwamr-pal.so`.
- @ENCLAVE_RUNTIME_ARGS: specify the specific arguments to enclave runtime, separated by the comma.

---

## (Optional) Run WAMR bundle for rune

Please refer to [this guide](https://github.com/leyao-daily/wasm-micro-runtime/blob/main/product-mini/platforms/linux-sgx/enclave-sample/App/wamr-bundle.md). This is optional, and suits for the developer in most cases.
