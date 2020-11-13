# Run WAMR enclave runtime with bundle

## Create WAMR Application bundle

`rune` can directly launch an OCI bundle converted from docker image. If you have Docker installed you can use its `export` sub-command to acquire a root filesystem from an existing WAMR application docker image.

```shell
# create the top most bundle directory
mkdir -p "$HOME/rune_workdir"
cd "$HOME/rune_workdir"
mkdir wamr-sgx-bundle
cd warmr-sgx-bundle

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

In order to run the target application in WAMR with `rune`, you need to change the entrypoint from `sh` to the target application, and in order to run multi-applications in one runtime with enclave, change it to `/run/rune/${wasm_app1.aot}`, `/run/rune/${wasm_app2.aot}` ... 

```json
  "process": {
      "args": [
          "/run/rune/${wasm_app}"
      ],
  }
```

and then configure enclave runtime as following:

```json
  "annotations": {
      "enclave.type": "intelSgx",
      "enclave.runtime.path": "/usr/lib/libwamr-pal.so",
      "enclave.runtime.args": "debug"
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
cd "$HOME/rune_workdir/wamr-sgx-bundle"
sudo rune run wamr-sgx-app
```
