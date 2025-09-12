# Compile, build and test hello world on the host

Now we have our host set up, we can build our hello world program and run it using WAMR.

First, build iwasm vmcore on your platform.

```sh
cd ${WAMR-dir}/product-mini/platforms/${your platform}
mkdir build && cd build
cmake ..
make 
```

Then you are ready to go to the directory that contains the hello world program and copy our iwasm vmcore

```sh
cp iwasm ../../../app-samples/hello-world
cd ${WAMR-dir}/product-mini/app-samples/hello-world
./build.sh
```

Now you could execute your first wasm program!

```sh
./iwasm test.wasm
```
