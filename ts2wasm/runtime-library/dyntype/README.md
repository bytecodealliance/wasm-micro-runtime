# Dynamic type library

Dynamic type library is an independent library which provide APIs to create/operate the dynamic values, it's part of the ts2wasm compiler.

## Building

This document helps to configure dyntype library.

#### step 1

```shell
cd ts2wasm/runtime-library/dyntype/
```

then run command below to initiate quickjs library.

```shell
git pull --recurse-submodules
git submodule update --remote --recursive
```

#### step 2

run commands below to build it.

``` shell
mkdir build && cd build
cmake ../
make
```

## Unit test

``` bash
cd test
mkdir build && cd build
cmake ..
make
make test # run the tests
```

### view coverage report

``` bash
# After executing make test
make cov-test
cd result
python3 -m http.server
# And then open the browser and navigate to http://localhost:8000
```

### debug tests

``` bash
# After building
gdb ./object_property_test  # or any other targets
```

### enable sanitizer

Sanitizer helps to detect invalid memory access, memory leak, undefined behavior and so on. Currently it is disabled by default, to enable it, pass `-DUNITTEST_USE_SANITIZER=1` during cmake configuration

``` bash
cmake .. -DUNITTEST_USE_SANITIZER=1
```

