# WARM API

* **Notice**: The python package `wamr.wamrapi.wamr` need python >= `3.10`.

## Setup

### Pre-requisites

Install requirements,

```shell
pip install -r requirements.txt
```

### Build native lib and update bindings

The following command builds the iwasm library and generates the Python bindings,

```sh
# In WAMR root directory
bash language-bindings/python/utils/create_lib.sh
```

This will build and copy libiwasm into the package.

## Samples

- **[basic](./samples/basic)**: Demonstrating how to use basic python bindings.
- **[native-symbol](./samples/native-symbol)**: Desmostrate how to call WASM from Python and how to export Python functions into WASM.
