# Introducing the Shared-Heap Feature in WAMR

In the world of WebAssembly, flexibility and performance are key. The WebAssembly Micro Runtime (WAMR) has introduced a powerful feature known as the shared heap, designed to enhance performance by allowing data sharing between multiple WebAssembly (WASM) instances, or between WASM and its host, without incurring the overhead of data copying. Let's delve into what this feature offers and how it can be effectively utilized.

## What is the Shared Heap?

The shared heap is an innovative extension of the WebAssembly linear memory. Unlike traditional memory, which requires data copying between WASM instances or to the host, the shared heap allows direct access to the same memory space. This can significantly improve performance in scenarios where multiple WASM instances need to interact or share data with the host.

## Key Benefits

- Expanded Memory Space: The shared-heap feature effectively expands the linear memory space by introducing hosted memory address spaces. This new linear memory space can be seen as a virtual space, encompassing multiple real spaces.
- Toolchain Workaround: The shared heap acts as a workaround for the current lack of toolchain support for the multi-memory proposal. This provides a practical solution for developers needing enhanced memory capabilities.
- Boundary Checks and Sandbox Protection: The shared heap maintains boundary checks and extends sandbox protection to portions of the real memory space, ensuring secure and reliable memory access across shared heaps.
- Ongoing Evaluation: The shared-heap feature is still under evaluation, and the team is actively seeking better solutions to further improve the functionality and performance.

## How Does It Work?

While the concept of a shared heap might seem straightforward, implementing it correctly requires careful attention. It is a runtime feature, not part of the standard WebAssembly specification, nor an ongoing proposal. Here’s how you can leverage the shared heap in your applications:

### Creating a Shared Heap

1. WAMR Managed Shared Heap: Use the `wasm_runtime_create_shared_heap(SharedHeapInitArgs \*init_args)` API to create a shared heap. If only `init_args.size` is specified with `init_args.pre_allocated_addr` set to NULL, WAMR will allocate and manage the shared heap. This allows dynamic memory management through `wasm_runtime_shared_heap_malloc()` and `wasm_runtime_shared_heap_free()`. Memory allocation from this heap is valid and can be shared, with automatic cleanup when the runtime is destroyed.

2. Preallocated Shared Heap: Alternatively, you can use pre-allocated memory, either from the system heap or a static global buffer. This requires you to handle its accessibility, size, and management. Specify `init_args.pre_allocated_addr` along with `init_args.size` to create this type of shared heap, which acts as a single large chunk for direct data sharing.

### Creating and Attaching Shared Heap Chains

To form a unified memory space, you can chain multiple shared heaps using the `wasm_runtime_chain_shared_heaps(wasm_shared_heap_t head, wasm_shared_heap_t body)` API. This creates a continuous memory region from the perspective of the WASM app, even though it might consist of separate regions in the native environment.

Once chained, attach the shared heap to WASM apps using `wasm_runtime_attach_shared_heaps(wasm_module_inst_t module_inst, wasm_shared_heap_t shared_heaps)`. This ensures no overlap with the existing linear memory of the WASM app instance, preventing accidental overwrites.

### Resource Allocation and Data Transfer

After attaching the shared heap, both host and WASM code can allocate resources directly from it. Host code can use `wasm_runtime_shared_heap_malloc()`, while WASM code can utilize `shared_heap_malloc()`. This allows one side to allocate memory and pass the address or index to the other side, facilitating efficient data transfer. The original boundary checks for loading and storing in linear memory naturally extend to the shared-heap area, as it is part of the linear memory. This integration ensures that memory operations remain secure and consistent.

## Conclusion

The shared heap feature is an exciting advancement in WASM performance optimization. By enabling direct memory sharing, it reduces overhead and boosts efficiency in applications requiring high interactivity. While it offers great benefits, remember it heavily relies on correct implementation to manage shared data effectively. As the feature is still under evaluation, let's work together on a better solution. We are collecting every potential usage and unique feature, looking for the shared common ground that will drive future innovations in WebAssembly applications.
