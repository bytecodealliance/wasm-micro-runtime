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

### Wasm

The definition of functions provided by WASI-NN (Wasm imports) is in the header file [wasi_nn.h](_core/iwasm/libraries/wasi-nn/wasi_nn.h_). By only including this file in a WASM application you will bind WASI-NN into your module.

For some historical reasons, there are two sets of functions in the header file. The first set is the original one, and the second set is the new one. The new set is recommended to use. In code, `WASM_ENABLE_WASI_EPHEMERAL_NN` is used to control which set of functions to use. If `WASM_ENABLE_WASI_EPHEMERAL_NN` is defined, the new set of functions will be used. Otherwise, the original set of functions will be used.

There is a big difference between the two sets of functions, `tensor_type`.

``` c
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
typedef enum { fp16 = 0, fp32, fp64, bf16, u8, i32, i64 } tensor_type;
#else
typedef enum { fp16 = 0, fp32, up8, ip32 } tensor_type;
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
```

It is required to recompile the Wasm application if you want to switch between the two sets of functions.

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

Use [classification-example](https://github.com/bytecodealliance/wasi-nn/tree/main/rust/examples/classification-example) as a smoke test case to make sure the wasi-nn support in WAMR is working properly.

> [!Important]
> It requires openvino.

### Prepare the model and the wasm

```bash
$ pwd
/workspaces/wasm-micro-runtime/core/iwasm/libraries/wasi-nn/test

$ docker build -t wasi-nn-example:v1.0 -f Dockerfile.wasi-nn-example .
```

There are model files(\*mobilenet\**) and wasm files(*wasi-nn-example.wasm*) in the directory */workspaces/wasi-nn/rust/examples/classification-example/build\* in the image of wasi-nn-example:v1.0.

### build iwasm and test

_TODO: May need alternative steps to build the iwasm and test in the container of wasi-nn-example:v1.0_

```bash
$ pwd
/workspaces/wasm-micro-runtime

$ docker run --rm -it -v $(pwd):/workspaces/wasm-micro-runtime wasi-nn-example:v1.0 /bin/bash
```

> [!Caution]
> The following steps are executed in the container of wasi-nn-example:v1.0.

```bash
$ cd /workspaces/wasm-micro-runtime/product-mini/platforms/linux
$ cmake -S . -B build -DWAMR_BUILD_WASI_NN=1 -DWAMR_BUILD_WASI_EPHEMERAL_NN=1
$ cmake --build build
$ ./build/iwasm -v=5 --map-dir=/workspaces/wasi-nn/rust/examples/classification-example/build/::fixture /workspaces/wasi-nn/rust/examples/classification-example/build/wasi-nn-example.wasm
```
