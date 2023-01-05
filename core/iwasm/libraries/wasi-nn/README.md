# WASI-NN

## How to use

Enable WASI-NN in the WAMR by spefiying it in the cmake building configuration as follows,

```
set (WAMR_BUILD_WASI_NN  1)
```

The definition of the functions provided by WASI-NN is in the header file `core/iwasm/libraries/wasi-nn/wasi_nn.h`.

By only including this file in your WASM application you will bind WASI-NN into your module.

## Tests

To run the tests we assume that the current directory is the root of the repository.


1. Build the runtime

```
docker build -t wasi-nn-base -f core/iwasm/libraries/wasi-nn/test/Dockerfile.base .
```

```
EXECUTION_TYPE=cpu
docker build -t wasi-nn-${EXECUTION_TYPE} -f core/iwasm/libraries/wasi-nn/test/Dockerfile.${EXECUTION_TYPE} .
```

where `EXECUTION_TYPE` can be `cpu` or `gpu`.


2. Build wasm app

```
docker build -t wasi-nn-compile -f core/iwasm/libraries/wasi-nn/test/Dockerfile.compile .
```

```
docker run -v $PWD/core/iwasm/libraries/wasi-nn:/wasi-nn wasi-nn-compile
```


3. Run wasm app

```
docker run \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets wasi-nn-${EXECUTION_TYPE} \
    --dir=/assets \
    /assets/test_tensorflow.wasm
```

or

```
docker run \
    --runtime=nvidia \
    -v $PWD/core/iwasm/libraries/wasi-nn/test:/assets wasi-nn-${EXECUTION_TYPE} \
    --dir=/assets \
    /assets/test_tensorflow.wasm
```

if using NVIDIA GPU.

If all the tests have run properly you will the the following message in the terminal,

```
Tests: passed!
```

## What is missing

Supported:

* Only 1 WASM app at a time.
* Only 1 model at a time.
    * `graph` and `graph-execution-context` are ignored.
* Graph encoding: `tensorflowlite`.
* Execution target: `cpu`.
* Tensor type: `fp32`.
