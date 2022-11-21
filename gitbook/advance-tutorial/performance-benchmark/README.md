# Performance Test

Like word on the street said(no way it's just us saying!) or you may saw in previous chapters(maybe multiple times), WAMR is a **lightweight** standalone WebAssembly (WASM) runtime with **small footprint**, **high performance** and highly configurable features.

Well, you don't have to take our word for it. You could run the [Benchmarks in our repo](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/tests/benchmarks) and decide whether the performance is good enough.

We provide multiple benchmarks that you could try:

- [PolyBench](../../../tests/benchmarks/polybench/README.md)
- [CoreMark](../../../tests/benchmarks/coremark/README.md)
- [Sightglass](../../../tests/benchmarks/sightglass/README.md)
- [JetStream2](../../../tests/benchmarks/jetstream/README.md)

For the memory footprint, you can refer to the links below.

- [Performance and footprint data](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance): checkout [here](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance) for the performance and footprint data

And in the next section, we provide tutorials on memory usage tuning. You can [profile memory usage](../../../doc/build_wamr.md#enable-memory-profiling-experiment) and [tunning memory usage](../../../doc/memory_tune.md)
