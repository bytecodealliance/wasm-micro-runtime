"tensorflow" sample introduction
==============

This sample demonstrates how to build [tensorflow](https://github.com/tensorflow/tensorflow) into WebAssembly with emsdk toolchain and run it with iwasm.

First, need to prepare a iwasm which is enable emscripten libraries support

``` bash
$ cd <wamr_dir>
$ cd product-mini/platforms/linux
> [!TODO]
> lib-pthread
$ cmake -S . -B build -DWAMR_BUILD_LIBC_EMCC=1 <other cmake options>
$ cmake --build build --target iwasm
```

Then, build [benchmark_model](./tensorflow/tensorflow/lite/tools/benchmark/BUILD) of *tflite* in .wasm

```bash
$ cd <wamr_dir>
$ cd samples/workload/tensorflow/
$ cmake -S . -B build
$ cmake --build build
```

Finally, run the *benchmark_model* with iwasm

```bash
$ cd build
$ <iwasm> --dir=. --heap-size=10475860 ./benchmark_model.aot --graph=./mobilenet_v1_1.0_224_quant.tflite --max_secs=300
```

Btw,
- `--heap-size=10475860` mostly depends on the model size. iwasm needs enough memory to load .tflie file and run calculation
- `--max-secs 300` means the max training time cost is 5 minutes, you can adjust it by yourself

You will see something like below:

```
INFO: STARTING!
INFO: Log parameter values verbosely: [0]
INFO: Max runs duration (seconds): [300]
INFO: Graph: [./mobilenet_v1_1.0_224_quant.tflite]
INFO: Loaded model ./mobilenet_v1_1.0_224_quant.tflite
INFO: Created TensorFlow Lite XNNPACK delegate for CPU.
INFO: The input model file size (MB): 4.27635
INFO: Initialized session in 7.003ms.
INFO: Running benchmark for at least 1 iterations and at least 0.5 seconds but terminate if exceeding 300 seconds.
INFO: count=9 first=56903 curr=56276 min=56251 max=56903 avg=56415.2 std=245

INFO: Running benchmark for at least 50 iterations and at least 1 seconds but terminate if exceeding 300 seconds.
INFO: count=50 first=56282 curr=56234 min=56227 max=56707 avg=56283.9 std=79

INFO: Inference timings in us: Init: 7003, First inference: 56903, Warmup (avg): 56415.2, Inference (avg): 56283.9
```

---
> [!TODO]
> - `--threads`
> - `--sgx`