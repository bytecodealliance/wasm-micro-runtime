
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

In wasm world:
``` C
void api_send_request(request_t * request, response_handler_f response_handler,
        void * user_data)
{
    int size;
    char *buffer;
    transaction_t *trans;

    if ((trans = (transaction_t *) malloc(sizeof(transaction_t))) == NULL) {
        printf(
                "send request: allocate memory for request transaction failed!\n");
        return;
    }

    memset(trans, 0, sizeof(transaction_t));
    trans->handler = response_handler;
    trans->mid = request->mid;
    trans->time = wasm_get_sys_tick_ms();
    trans->user_data = user_data;

    // pack request
    if ((buffer = pack_request(request, &size)) == NULL) {
        printf("send request: pack request failed!\n");
        free(trans);
        return;
    }

    transaction_add(trans);

    /* if the trans is the 1st one, start the timer */
    if (trans == g_transactions) {
        /* assert(g_trans_timer == NULL); */
        if (g_trans_timer == NULL) {
            g_trans_timer = api_timer_create(TRANSACTION_TIMEOUT_MS,
            false,
            true, transaction_timeout_handler);
        }
    }

    // call native API
    wasm_post_request(buffer, size);

    free_req_resp_packet(buffer);
}
```

In native world:
``` C
void
wasm_post_request(wasm_exec_env_t exec_env,
                  int32 buffer_offset, int size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *buffer = NULL;

    // do boundary check
    if (!validate_app_addr(buffer_offset, size))
        return;

    // do address conversion
    buffer = addr_app_to_native(buffer_offset);

    if (buffer != NULL) {
        request_t req[1];

        // De-serialize data
        if (!unpack_request(buffer, size, req))
            return;

        // set sender to help dispatch the response to the sender app later
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App,
                                                        module_inst);
        bh_assert(mod_id != ID_NONE);
        req->sender = mod_id;

        if (req->action == COAP_EVENT) {
            am_publish_event(req);
            return;
        }

        am_dispatch_request(req);
    }
}
```





Steps for exporting native API
==========================

WAMR implemented a framework for developers to export API's. Below is the procedure to expose the platform API's in three steps:


## Step 1: Define the native API for exporting

Define the function **example_native_func** in your source file, namely `example.c` here:
``` C
int example_native_func(wasm_exec_env_t exec_env,
                        int arg1, int arg2)
{
    // Your implementation here
}
```
The first function argument must be defined using type **wasm_exec_env_t** which is the WAMR calling convention for native API exporting. 

The function prototype should also be declared in a header file so the wasm application can include it.
``` C
#ifndef _EXAMPLE_H_
#define _EXAMPLE_H_
#ifdef __cplusplus
extern "C" {
#endif

void example_native_func(int arg1, int arg2);

#ifdef __cplusplus
}
#endif
#endif
```

## Step 2: Declare the native API exporting

Declare the function **example_native_func** with macro **EXPORT_WASM_API** in your **.inl** file, namely `example.inl` in this sample.
``` C
EXPORT_WASM_API(example_native_func),
```

Then include the file **example.inl** in definition of array **extended_native_symbol_defs** in the `ext_lib_export.c`.
``` C
static NativeSymbol extended_native_symbol_defs[] = {
   #include "example.inl"
};

#include "ext_lib_export.h"
```


## Step 3: Compile the runtime product
Add the source file **example.c** and **ext_lib_export.c** into the CMakeList.txt for building runtime with the exported API's:
``` cmake
set (EXT_API_SOURCE example.c)

add_executable (sample
    # other source files
    # ......
    ${EXT_API_SOURCE}
    ext_lib_export.c
)
```

# Use exported API in wasm application

We can call the exported native API **example_native_func** in wasm application like this:
``` C
#include <stdio.h>
#include "example.h"

int main(int argc, char **argv)
{
    int a = 0, b = 1;

    example_native_func(a, b);
    return 0;
}
```