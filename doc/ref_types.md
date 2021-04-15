# WAMR reference-types introduction

WebAssembly [reference-types](https://github.com/WebAssembly/reference-types) proposal introduces the new type `externref` and makes it easier and more efficient to interoperate with host environment, allowing host references to be represented directly by type externref. And WASM modules can talk about host references directly, rather than requiring external glue code running in the host.

WAMR implements the reference-types proposal, allowing developer to pass the host object to WASM application and then restore and access the host object in native lib. In WAMR internal, the external host object is represented as externref index with `uint32` type, developer must firstly map the host object of `void *` type to the externref index, and then pass the index to the function to called as the function's externref argument.

Currently WAMR provides APIs as below:
```C
bool
wasm_externref_obj2ref(wasm_module_inst_t module_inst,
                       void *extern_obj, uint32_t *p_externref_idx);

WASM_RUNTIME_API_EXTERN bool
wasm_externref_ref2obj(uint32_t externref_idx, void **p_extern_obj);

WASM_RUNTIME_API_EXTERN bool
wasm_externref_retain(uint32 externref_idx);
```

The `wasm_externref_obj2ref()` API is used to map the host object to the externref index, and the `wasm_externref_ref2obj()` API is used to retrieve the original host object mapped. The `wasm_externref_retain()` API is to retain the host object if we don't want the object to be cleaned when it isn't used during externref object reclaim.

Please ref to the [sample](../samples/ref-types) for more details.
