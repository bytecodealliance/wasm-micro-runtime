All samples come from the commit 340fd9528cc3b26d22fe30ee1628c8c3f2b8c53b
of [wasm-c-api][https://github.com/WebAssembly/wasm-c-api].

Every user should be familiar with *APIs* listed in
[wasm.h][https://github.com/WebAssembly/wasm-c-api/blob/master/include/wasm.h].

all [examples][https://github.com/WebAssembly/wasm-c-api/tree/master/example] are
very helpful for learning.

Currently, we support partial of *APIs* and are going to support the rest of
them in next releases.

Supported APIs:

``` c
/* wasm_bytevec_t APIs ... */

wasm_engine_t *wasm_engine_new();
wasm_engine_t *wasm_engine_new_with_args(mem_alloc_type_t, const MemAllocOption*, runtime_mode_e);
void wasm_engine_delete(wasm_engine_t *);

wasm_store_t *wasm_store_new(wasm_engine_t *);
void wasm_store_delete(wasm_store_t *);

/* wasm_valtype_t APIs ... */
/* wasm_valtype_vec_t APIs ... */
/* wasm_functype_vec_t APIs ... */
/* wasm_globaltype_vec_t APIs ... */
/* wasm_val_t APIs ... */
/* wasm_trap_t partial APIs ... */

wasm_module_t *wasm_module_new(wasm_store_t *, const wasm_byte_vec_t *);
void wasm_module_delete(wasm_module_t *);

wasm_func_t *wasm_func_new(wasm_store_t *, const wasm_functype_t *, wasm_func_callback_t);
wasm_func_t *wasm_func_new_with_env(wasm_store_t *store, const wasm_functype_t *, wasm_func_callback_with_env_t, void *env, void (*finalizer)(void *));
void wasm_func_delete(wasm_func_t *);
wasm_fucn_t *wasm_func_copy(const wasm_func_t *);
wasm_functype_t *wasm_func_type(const wasm_func_t *);
wasm_trap_t * wasm_func_call(const wasm_func_t *, const wasm_val_t params[], wasm_val_t results[]);
size_t wasm_func_param_arity(const wasm_func_t *);
size_t wasm_func_result_arity(const wasm_func_t *);

wasm_global_t *wasm_global_new(wasm_store_t *, const wasm_globaltype_t *, const wasm_val_t *);
wasm_global_t * wasm_global_copy(const wasm_global_t *);
void wasm_global_delete(wasm_global_t *);
bool wasm_global_same(const wasm_global_t *, const wasm_global_t *);
void wasm_global_set(wasm_global_t *, const wasm_val_t *);
void wasm_global_get(const wasm_global_t *, wasm_val_t *out);
wasm_globaltype_t * wasm_global_type(const wasm_global_t *);

wasm_instance_t *wasm_instance_new(wasm_store_t *, const wasm_module_t *, const wasm_extern_t *const imports[], wasm_trap_t **traps);
void wasm_instance_delete(wasm_instance_t *);
void wasm_instance_exports(const wasm_instance_t *, wasm_extern_vec_t *out);

/* wasm_extern_t APIs */
```

Unsupported APIs:

``` c
/* wasm_tabletype_t APIs */
/* wasm_memorytype_t APIs */
/* wasm_externtype_t APIs */
/* wasm_importtype_t APIs */
/* wasm_exporttype_t APIs */
/* wasm_ref_t APIs */
/* wasm_shared_##name##_t APIs */

WASM_API_EXTERN bool wasm_module_validate(wasm_store_t*, const wasm_byte_vec_t* binary);
WASM_API_EXTERN void wasm_module_imports(const wasm_module_t*, own wasm_importtype_vec_t* out);
WASM_API_EXTERN void wasm_module_serialize(const wasm_module_t*, own wasm_byte_vec_t* out);
WASM_API_EXTERN own wasm_module_t* wasm_module_deserialize(wasm_store_t*, const wasm_byte_vec_t*);

/* wasm_table_t APIs */
/* wasm_memory_t APIs */
```
