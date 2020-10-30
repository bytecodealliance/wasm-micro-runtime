# Run WAMR bundle for Rune

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

In order to run the target applications in WAMR with `rune`, you need to change the entrypoint from `sh` to `/run/rune/${wasm_app1.wasm}`, and in order to run multi-applications in one runtime with enclave, change it to `/run/rune/${wasm_app1.aot}`, `/run/rune/${wasm_app2.aot}` ... 

```yaml
  "process": {
      "args": [
          "/run/rune/demo.aot"
      ],
  }
```

and then configure enclave runtime as following:

```yaml
  "annotations": {
      "enclave.type": "intelSgx",
      "enclave.runtime.path": "/usr/lib/libwamr-pal.so",
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

