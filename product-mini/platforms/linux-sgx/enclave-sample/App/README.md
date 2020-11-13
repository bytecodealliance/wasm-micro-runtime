# Running WAMR as an [Enclave Runtime](https://github.com/alibaba/inclavare-containers/blob/master/docs/design/terminology.md#enclave-runtime) for [Inclavare Containers](https://github.com/alibaba/inclavare-containers)

In order to establish with `rune`, a novel OCI Runtime for spawning and running enclaves in containers, it is required to implement an [enclave runtime PAL](https://github.com/alibaba/inclavare-containers/blob/master/docs/design/terminology.md#enclave-runtime-pal) to make the communications with WAMR.

With the assist of `rune`, WAMR is brought to the cloud-native ecosystem beyond the basis. This is the so-called term "WAMR enclave runtime".

This guide will provide the information about the build, integration and deployment for WAMR enclave runtime. Eventually, the resulting docker image will be launched by `rune`, and the WARM application as the entrypoint of docker image will run in Intel SGX enclave with the hardware-enforced isolation and cryptographically data protection.

## Build WAMR vmcore (iwasm) and enclave image

Please follow [this guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md#build-wamr-vmcore-iwasm-for-linux-sgx) to build iwasm and enclave image as the prerequisite.

The generated enclave image enclave.signed.so will be consumed by WAMR enclave runtime mentioned below.

---

## Build and install the PAL of WAMR enclave runtime

```shell
g++ -shared -fPIC -o libwamr-pal.so App/*.o libvmlib_untrusted.a -L/opt/intel/sgxsdk/lib64 -lsgx_urts -lpthread -lssl -lcrypto
cp ./libwamr-pal.so /usr/lib/libwamr-pal.so
```

---

## Build WAMR application

As the prerequisite, please
- refer to [this step](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md#prepare-wasm-building-environments) to install wasi-sdk. Note that the binaries of wasi-sdk must be installed at `/opt/wasi-sdk/bin/`.
- refer to [this guide](https://github.com/bytecodealliance/wasm-micro-runtime#build-wamrc-aot-compiler) to generate wamrc AoT compiler.

The sample WAMR application test.c is provided in [this guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md#build-wasm-applications). Don't forget to compile the `.wasm` file to `.aot` file:
```shell
wamrc -sgx -o test.aot test.wasm
```

The generated test.aot is the WAMR application launched by WAMR enclave runtime.

---

## Build WAMR docker image

Under the `enclave-sample` directory, to create the WAMR docker images to load the `enclave.signed.so` and target application wasm files, please type the following commands to create a `Dockerfile`:

For CentOS:

```shell
cat >Dockerfile <<EOF
FROM centos:8.1.1911

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
COPY test.aot app
#COPY ${wasm_app.aot} .
ENTRYPOINT ["/run/rune/app"]
EOF
```

For ubuntu:

```shell
cat > Dockerfile <<EOF
FROM ubuntu:18.04

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
COPY test.aot app
#COPY ${wasm_app.aot} .
ENTRYPOINT ["/run/rune/app"]
EOF
```

where `${wasm_app.aot}` files are the extra WAMR applications you want to run.

Then build the WAMR docker image with the command:

```shell
docker build . -t wamr-sgx-app
```

---

## Deploy WAMR SGX Docker image

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

### Integrate OCI Runtime rune with Docker

Add the associated configuration for `rune` in dockerd config file, e.g, `/etc/docker/daemon.json`, on your system.

```json
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

### Run WAMR SGX docker image

You need to specify a set of parameters to `docker run` to run:

```shell
docker run -it --rm --runtime=rune \
  -e ENCLAVE_TYPE=intelSgx \
  -e ENCLAVE_RUNTIME_PATH=/usr/lib/libwamr-pal.so \
  -e ENCLAVE_RUNTIME_ARGS=debug \
  wamr-sgx-app
```

where:

- @ENCLAVE_TYPE: specify the type of enclave hardware to use, such as `intelSgx`.
- @ENCLAVE_RUNTIME_PATH: specify the path to enclave runtime to launch. For an WAMR application, you need to specify the path to `libwamr-pal.so`.
- @ENCLAVE_RUNTIME_ARGS: specify the specific arguments to enclave runtime, separated by the comma.

---

## Develop and debug WAMR enclave runtime with rune

Please refer to [this guide](https://github.com/leyao-daily/wasm-micro-runtime/blob/main/product-mini/platforms/linux-sgx/enclave-sample/App/wamr-bundle.md). This is optional, and suits for the developer in most cases.
