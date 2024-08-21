# WASI-NN

## How to use

### Host

Enable WASI-NN in the WAMR by spefiying it in the cmake building configuration as follows,

```cmake
set (WAMR_BUILD_WASI_NN  1)
```

or in command line

```bash
$ cmake -DWAMR_BUILD_WASI_NN=1 <other options> ...
```

> ![Caution]
> If enable `WAMR_BUID_WASI_NN`, iwasm will link a shared WAMR library instead of a static one. Wasi-nn backends will be loaded dynamically at runtime. Users shall specify the path of the backend library and register it to the iwasm runtime with `--native-lib=<path of backend library>`. All shared libraries should be placed in the `LD_LIBRARY_PATH`.

#### Compilation options

- `WAMR_BUILD_WASI_NN`. enable wasi-nn support. can't work alone. need to identify a backend. Match legacy wasi-nn spec naming convention. use `wasi_nn` as import module names.
- `WAMR_BUILD_WASI_EPHEMERAL_NN`. Match latest wasi-nn spec naming convention. use `wasi_ephemeral_nn` as import module names.
- `WAMR_BUILD_WASI_NN_TFLITE`. identify the backend as TensorFlow Lite.
- `WAMR_BUILD_WASI_NN_OPENVINO`. identify the backend as OpenVINO.

### Wasm

The definition of functions provided by WASI-NN (Wasm imports) is in the header file [wasi_nn.h](_core/iwasm/libraries/wasi-nn/wasi_nn.h_). By only including this file in a WASM application you will bind WASI-NN into your module.

For some historical reasons, there are two sets of functions in the header file. The first set is the original one, and the second set is the new one. The new set is recommended to use. In code, `WASM_ENABLE_WASI_EPHEMERAL_NN` is used to control which set of functions to use. If `WASM_ENABLE_WASI_EPHEMERAL_NN` is defined, the new set of functions will be used. Otherwise, the original set of functions will be used.

There is a big difference between the two sets of functions, `tensor_type`.

```c
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
typedef enum { fp16 = 0, fp32, fp64, bf16, u8, i32, i64 } tensor_type;
#else
typedef enum { fp16 = 0, fp32, up8, ip32 } tensor_type;
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
```

It is required to recompile the Wasm application if you want to switch between the two sets of functions.

#### Openvino

If you're planning to use OpenVINO backends, the first step is to install OpenVINO on your computer. To do this correctly, please follow the official installation guide which you can find at this link: https://docs.openvino.ai/2024/get-started/install-openvino/install-openvino-archive-linux.html.

After you've installed OpenVINO, you'll need to let cmake system know where to find it. You can do this by setting an environment variable named `OpenVINO_DIR`. This variable should point to the place on your computer where OpenVINO is installed. By setting this variable, your system will be able to locate and use OpenVINO when needed. You can find installation path by running the following command if using APT `$dpkg -L openvino`. The path should be _/opt/intel/openvino/_ or _/usr/lib/openvino_.

## Tests

To run the tests we assume that the current directory is the root of the repository.

### Build the runtime

Build the runtime image for your execution target type.

`EXECUTION_TYPE` can be:

- `cpu`
- `nvidia-gpu`
- `vx-delegate`
- `tpu`

```bash
$ pwd
<somewhere>/wasm-micro-runtime

$ EXECUTION_TYPE=cpu docker build -t wasi-nn-${EXECUTION_TYPE} -f core/iwasm/libraries/wasi-nn/test/Dockerfile.${EXECUTION_TYPE} .
```

### Build wasm app

```
docker build -t wasi-nn-compile -f core/iwasm/libraries/wasi-nn/test/Dockerfile.compile .
```

```
docker run -v $PWD/core/iwasm/libraries/wasi-nn:/wasi-nn wasi-nn-compile
```

### Run wasm app

If all the tests have run properly you will the the following message in the terminal,

```
Tests: passed!
```

> [!TIP]
> Use _libwasi-nn-tflite.so_ as an example. You shall use whatever you have built.

- CPU

```bash
docker run \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets \
    -v $PWD/core/iwasm/libraries/wasi-nn/test/models:/models \
    wasi-nn-cpu \
    --dir=/ \
    --env="TARGET=cpu" \
    --native-lib=/lib/libwasi-nn-tflite.so \
    /assets/test_tensorflow.wasm
```

- (NVIDIA) GPU
  - Requirements:
    - [NVIDIA docker](https://github.com/NVIDIA/nvidia-docker).

```bash
docker run \
    --runtime=nvidia \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets \
    -v $PWD/core/iwasm/libraries/wasi-nn/test/models:/models \
    wasi-nn-nvidia-gpu \
    --dir=/ \
    --env="TARGET=gpu" \
    --native-lib=/lib/libwasi-nn-tflite.so \
    /assets/test_tensorflow.wasm
```

- vx-delegate for NPU (x86 simulator)

```bash
docker run \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets \
    wasi-nn-vx-delegate \
    --dir=/ \
    --env="TARGET=gpu" \
    --native-lib=/lib/libwasi-nn-tflite.so \
    /assets/test_tensorflow_quantized.wasm
```

- (Coral) TPU
  - Requirements:
    - [Coral USB](https://coral.ai/products/accelerator/).

```bash
docker run \
    --privileged \
    --device=/dev/bus/usb:/dev/bus/usb \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets \
    wasi-nn-tpu \
    --dir=/ \
    --env="TARGET=tpu" \
    --native-lib=/lib/libwasi-nn-tflite.so \
    /assets/test_tensorflow_quantized.wasm
```

## What is missing

Supported:

- Graph encoding: `tensorflowlite`.
- Execution target: `cpu`, `gpu` and `tpu`.
- Tensor type: `fp32`.

## Smoke test

### Testing with WasmEdge-WASINN Examples

To ensure everything is set up correctly, use the examples from [WasmEdge-WASINN-examples](https://github.com/second-state/WasmEdge-WASINN-examples/tree/master). These examples help verify that WASI-NN support in WAMR is functioning as expected.

> Note: The repository contains two types of examples. Some use the [standard wasi-nn](https://github.com/WebAssembly/wasi-nn), while others use [WasmEdge's version of wasi-nn](https://github.com/second-state/wasmedge-wasi-nn), which is enhanced to meet specific customer needs.

The examples test the following machine learning backends:

- OpenVINO
- PyTorch
- TensorFlow Lite

Due to the different requirements of each backend, we'll use a Docker container for a hassle-free testing environment.

#### Prepare the execution environment

```bash
$ pwd
/workspaces/wasm-micro-runtime/

$ docker build -t wasi-nn-smoke:v1.0 -f ./core/iwasm/libraries/wasi-nn/test/Dockerfile.wasi-nn-smoke .
```

#### Execute

```bash
$ docker run --rm wasi-nn-smoke:v1.0
```

### Testing with bytecodealliance wasi-nn

For another example, check out [classification-example](https://github.com/bytecodealliance/wasi-nn/tree/main/rust/examples/classification-example), which focuses on OpenVINO. You can run it using the same Docker container mentioned above.
