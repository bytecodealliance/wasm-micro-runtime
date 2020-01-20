
Export native API to WASM application
=======================================================

The basic working flow for WASM application calling into the native API is shown in the following diagram:

![WAMR WASM API ext diagram](./pics/extend_library.PNG "WAMR WASM API ext architecture diagram")


WAMR provides the macro `EXPORT_WASM_API` to enable users to export a native API to a WASM application. WAMR has implemented a base API for the timer and messaging by using `EXPORT_WASM_API`. This can be a point of reference for extending your own library.
``` C
static NativeSymbol extended_native_symbol_defs[] = {
    EXPORT_WASM_API(wasm_register_resource),
    EXPORT_WASM_API(wasm_response_send),
    EXPORT_WASM_API(wasm_post_request),
    EXPORT_WASM_API(wasm_sub_event),
    EXPORT_WASM_API(wasm_create_timer),
    EXPORT_WASM_API(wasm_timer_set_interval),
    EXPORT_WASM_API(wasm_timer_cancel),
    EXPORT_WASM_API(wasm_timer_restart)
};
```

**Security attention:** A WebAssembly application should only have access to its own memory space. As a result, the integrator should carefully design the native function to ensure that the memory accesses are safe. The native API to be exported to the WASM application must:

- Only use 32 bits number for parameters
- Should not pass data to the structure pointer (do data serialization instead)
- Should do the pointer address conversion in the native API
- Should not pass function pointer as callback



Below is a sample of a library extension. All code invoked across WASM and native world must be serialized and de-serialized, and the native world must do a boundary check for every incoming address from the WASM world.

```
insert sample code
```





Steps for exporting native API
==========================

WAMR implemented a framework for developers to export API's. Below is the procedure to expose the platform API's in three steps:


