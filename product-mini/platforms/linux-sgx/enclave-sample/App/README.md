# WAMR as an Enclave Runtime for Rune

## Build WAMR vmcore (iwasm) for Linux-SGX

### SIM Mode

The default SGX mode in WAMR is the SIM mode. Build the source code and enclave example, please refer to [this guild](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md#build-wamr-vmcore-iwasm-for-linux-sgx).

### HW Mode

Please do the following changes before execute [this guild](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md#build-wamr-vmcore-iwasm-for-linux-sgx).

```shell
diff --git a/product-mini/platforms/linux-sgx/enclave-sample/Makefile b/product-mini/platforms/linux-sgx/enclave-sample/Makefile
index f06b5b8..f247f3e 100644
--- a/product-mini/platforms/linux-sgx/enclave-sample/Makefile
+++ b/product-mini/platforms/linux-sgx/enclave-sample/Makefile
@@ -4,7 +4,7 @@
 ######## SGX SDK Settings ########

 SGX_SDK ?= /opt/intel/sgxsdk
-SGX_MODE ?= SIM
+SGX_MODE ?= HW
 SGX_ARCH ?= x64
 SGX_DEBUG ?= 0
 SPEC_TEST ?= 0
```

```shell
diff --git a/product-mini/platforms/linux-sgx/enclave-sample/Makefile_minimal b/product-mini/platforms/linux-sgx/enclave-sample/Makefile_minimal
index a64d577..747d995 100644
--- a/product-mini/platforms/linux-sgx/enclave-sample/Makefile_minimal
+++ b/product-mini/platforms/linux-sgx/enclave-sample/Makefile_minimal
@@ -4,7 +4,7 @@
 ######## SGX SDK Settings ########

 SGX_SDK ?= /opt/intel/sgxsdk
-SGX_MODE ?= SIM
+SGX_MODE ?= HW
 SGX_ARCH ?= x64
 SGX_DEBUG ?= 0
 SPEC_TEST ?= 0

```

After building, please sign enclave.so to generate enclave.signed.so which is needed in PAL

```shell
/opt/intel/sgxsdk/bin/x64/sgx_sign sign -key Enclave/Enclave_private.pem -enclave enclave.so -out enclave.signed.so -config Enclave/Enclave.config.xml
```

---

## Build PAL dynamically linked shared object

To build WAMR as an Enclave Runtime for [Inclavare Containers](https://github.com/alibaba/inclavare-containers), we should implement the [PAL interface](https://github.com/alibaba/inclavare-containers/blob/master/rune/libenclave/internal/runtime/pal/spec_v2.md) in WAMR for rune to call the PAL to create the enclave with WAMR and run applications.

```shell
g++ -shared -fPIC -o libwamr.so App/*.o libvmlib_untrusted.a -L/opt/intel/sgxsdk/lib64 -lsgx_urts -lpthread -lssl -lcrypto
```

Note: /opt/intel/sgxsdk/ is where you installed the SGX SDK

---

## Build WAMR container image

Under the `enclave-sample` directory, to create the WAMR docker images to load the `enclave.signed.so` and target application wasm files, please type the following commands to create a `Dockerfile`:

For centos:

```shell
cat >Dockerfile <<EOF
FROM centos:8.1.1911

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
EOF
```

 For ubuntu:

```shell
cat > Dockerfile <<EOF
FROM ubuntu:18.04

RUN mkdir -p /run/rune
WORKDIR /run/rune

COPY enclave.signed.so .
EOF
```

Then build the WAMR container image with the command:

```shell
docker build . -t wamr-app
```

---

## Create WAMR Application bundle

In order to use `rune` you must have your container image in the format of an OCI bundle. If you have Docker installed you can use its `export` method to acquire a root filesystem from an existing WAMR application container image.

```shell
# create the top most bundle directory
mkdir -p "$HOME/rune_workdir"
cd "$HOME/rune_workdir"
mkdir rune-container
cd rune-container

# create the rootfs directory
mkdir rootfs

# export wamr application image via Docker into the rootfs directory
docker export $(docker create ${wamr_application_image}) | sudo tar -C rootfs -xvf -
```

After a root filesystem is populated you just generate a spec in the format of a config.json file inside your bundle. `rune` provides a spec command which is similar to `runc` to generate a template file that you are then able to edit.

```shell
rune spec
```

To find features and documentation for fields in the spec please refer to the [specs](https://github.com/opencontainers/runtime-spec) repository.

In order to run the hello world demo program in WAMR with `rune`, you need to change the entrypoint from `sh` to `/bin/hello_world`

```yaml
  "process": {
      "args": [
          "/bin/hello_world"
      ],
  }
```

and then configure enclave runtime as following:

```yaml
  "annotations": {
      "enclave.type": "intelSgx",
      "enclave.runtime.path": "/usr/lib/wamr-pal.so",
      "enclave.runtime.args": "./"
  }
```

where:

- @enclave.type: specify the type of enclave hardware to use, such as `intelSgx`.
- @enclave.runtime.path: specify the path to enclave runtime to launch. For an WAMR application, you need to specify the path to `libwamr-pal.so`.
- @enclave.runtime.args: specify the specific arguments to enclave runtime, separated by the comma.

---

## Run WAMR Application

Assuming you have an OCI bundle from the previous step you can execute the container in this way.

```shell
cd "$HOME/rune_workdir/rune-container"
sudo rune run ${wamr_application_container_name}
```

