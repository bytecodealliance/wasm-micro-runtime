# Using docker

Now that we have set up docker, we could run the following command directly in VS Code terminal(or the bash of your  if you prefer ssh docker container directly).

Similarly, build iwasm vmcore.

```sh
cd product-mini/platforms/linux
mkdir build && cd build
cmake ..
make 
```

Then you are ready to go to the directory that contains hello world program and copy our iwasm vmcore

```sh
cp iwasm ../../app-samples/hello-world
cd ../..//app-samples/hello-world
./build.sh
```

Now you could execute your first wasm program!

```sh
iwasm test.wasm
```
