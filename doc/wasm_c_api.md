All samples come from the commit 340fd9528cc3b26d22fe30ee1628c8c3f2b8c53b
of [wasm-c-api][https://github.com/WebAssembly/wasm-c-api].

Every user should be familiar with *APIs* listed in
[wasm.h][https://github.com/WebAssembly/wasm-c-api/blob/master/include/wasm.h].

all [examples][https://github.com/WebAssembly/wasm-c-api/tree/master/example] are
very helpful for learning.

Currently, we support partial of APIs and are going to support the rest of
them in next releases.

a summary of unsupported APIs

- References

``` c
WASM_API_EXTERN own wasm_shared_##name##_t* wasm_##name##_share(const wasm_##name##_t*);
WASM_API_EXTERN own wasm_##name##_t* wasm_##name##_obtain(wasm_store_t*, const wasm_shared_##name##_t*);
```

- Several Module APIs

``` c
WASM_API_EXTERN void wasm_module_serialize(const wasm_module_t*, own wasm_byte_vec_t* out);
WASM_API_EXTERN own wasm_module_t* wasm_module_deserialize(wasm_store_t*, const wasm_byte_vec_t*);
```

we tend to grow a table or a memory by opcode only and not support growing both
by host-side function callings.

- Table Grow APIs

``` c
WASM_API_EXTERN bool wasm_table_grow(wasm_table_t*, wasm_table_size_t delta, wasm_ref_t* init);
```

- Memory Grow APIs

``` c
WASM_API_EXTERN bool wasm_memory_grow(wasm_memory_t*, wasm_memory_pages_t delta);
```
