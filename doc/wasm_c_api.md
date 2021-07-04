All samples come from the commit 340fd9528cc3b26d22fe30ee1628c8c3f2b8c53b
of [wasm-c-api][https://github.com/WebAssembly/wasm-c-api].

Every user should be familiar with *APIs* listed in
[wasm.h][https://github.com/WebAssembly/wasm-c-api/blob/master/include/wasm.h].

all [examples][https://github.com/WebAssembly/wasm-c-api/tree/master/example] are
very helpful for learning.

Currently, we support partial of APIs and are going to support the rest of
them in next releases.

a summary of unsupported APIs

- Configuration

``` c
WASM_API_EXTERN own wasm_config_t* wasm_config_new(void);
```

- References

``` c
WASM_API_EXTERN bool wasm_##name##_same(const wasm_##name##_t*, const wasm_##name##_t*); \
WASM_API_EXTERN void* wasm_##name##_get_host_info(const wasm_##name##_t*); \
WASM_API_EXTERN void wasm_##name##_set_host_info(wasm_##name##_t*, void*); \
WASM_API_EXTERN void wasm_##name##_set_host_info_with_finalizer( \
WASM_API_EXTERN wasm_ref_t* wasm_##name##_as_ref(wasm_##name##_t*); \
WASM_API_EXTERN wasm_##name##_t* wasm_ref_as_##name(wasm_ref_t*); \
WASM_API_EXTERN const wasm_ref_t* wasm_##name##_as_ref_const(const wasm_##name##_t*); \
WASM_API_EXTERN const wasm_##name##_t* wasm_ref_as_##name##_const(const wasm_ref_t*);
WASM_API_EXTERN own wasm_shared_##name##_t* wasm_##name##_share(const wasm_##name##_t*); \
WASM_API_EXTERN own wasm_##name##_t* wasm_##name##_obtain(wasm_store_t*, const wasm_shared_##name##_t*);
```

- Frames

``` c
WASM_API_EXTERN own wasm_frame_t* wasm_frame_copy(const wasm_frame_t*);
WASM_API_EXTERN struct wasm_instance_t* wasm_frame_instance(const wasm_frame_t*);
WASM_API_EXTERN uint32_t wasm_frame_func_index(const wasm_frame_t*);
WASM_API_EXTERN size_t wasm_frame_func_offset(const wasm_frame_t*);
WASM_API_EXTERN size_t wasm_frame_module_offset(const wasm_frame_t*);
WASM_API_EXTERN own wasm_frame_t* wasm_trap_origin(const wasm_trap_t*);
WASM_API_EXTERN void wasm_trap_trace(const wasm_trap_t*, own wasm_frame_vec_t* out);
```

 Foreign Objects

``` c
WASM_API_EXTERN own wasm_foreign_t* wasm_foreign_new(wasm_store_t*);
```

- Several Module APIs

``` c
WASM_API_EXTERN bool wasm_module_validate(wasm_store_t*, const wasm_byte_vec_t* binary);
WASM_API_EXTERN void wasm_module_serialize(const wasm_module_t*, own wasm_byte_vec_t* out);
WASM_API_EXTERN void wasm_module_serialize(const wasm_module_t*, own wasm_byte_vec_t* out);
```

- Table Operations APIs

``` c
WASM_API_EXTERN own wasm_ref_t* wasm_table_get(const wasm_table_t*, wasm_table_size_t index);
WASM_API_EXTERN bool wasm_table_set(wasm_table_t*, wasm_table_size_t index, wasm_ref_t*);
WASM_API_EXTERN wasm_table_size_t wasm_table_size(const wasm_table_t*);
WASM_API_EXTERN bool wasm_table_grow(wasm_table_t*, wasm_table_size_t delta, wasm_ref_t* init);
```

- Memory Grow APIs

``` c
WASM_API_EXTERN bool wasm_memory_grow(wasm_memory_t*, wasm_memory_pages_t delta);
```