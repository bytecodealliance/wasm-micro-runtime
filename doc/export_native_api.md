
The mechanism of exporting native API to WASM application
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

![#f03c15](https://placehold.it/15/f03c15/000000?text=+) **Security attention:** A WebAssembly application should only have access to its own memory space. As a result, the integrator should carefully design the native function to ensure that the memory accesses are safe. The native API to be exported to the WASM application must:
- Only use 32 bits number for parameters
- Should not pass data to the structure pointer (do data serialization instead)
- Should do the pointer address conversion in the native API
- Should not pass function pointer as callback

Below is a sample of a library extension. All code invoked across WASM and native world must be serialized and de-serialized, and the native world must do a boundary check for every incoming address from the WASM world.


<img src="./pics/safe.PNG" width="90%">


Steps for exporting native API
==========================

WAMR implemented a framework for developers to export API's. Below is the procedure to expose the platform API's in three steps:

**Step 1. Create a header file**<br/>
Declare the API's for your WASM application source project to include.

**Step 2. Create a source file**<br/>
Export the platform API's, for example in ``` products/linux/ext_lib_export.c ```
``` C
#include "lib_export.h"

static NativeSymbol extended_native_symbol_defs[] =
{
};

#include "ext_lib_export.h"
```

**Step 3. Register new API's**<br/>
Use the macro `EXPORT_WASM_API` and `EXPORT_WASM_API2` to add exported API's into the array of ```extended_native_symbol_defs```.
The pre-defined MACRO `EXPORT_WASM_API` should be used to declare a function export:
``` c
#define EXPORT_WASM_API(symbol)  {#symbol, symbol}
```

Below code example shows how to extend the library to support `customized()`:

``` 
//lib_export_impl.c
void customized()
{
   // your code
}


// lib_export_dec.h
#ifndef _LIB_EXPORT_DEC_H_
#define _LIB_EXPORT_DEC_H_
#ifdef __cplusplus
extern "C" {
#endif

void customized();

#ifdef __cplusplus
}
#endif
#endif


// ext_lib_export.c
#include "lib_export.h"
#include "lib_export_dec.h"

static NativeSymbol extended_native_symbol_defs[] =
{
    EXPORT_WASM_API(customized)
};

#include "ext_lib_export.h"
```

Use extended library
------------------------
In the application source project, it will include the WAMR built-in API's header file and platform extension header files. Assuming the board vendor extends the library which added an API called customized(), the WASM application would be like this:
``` C
#include <stdio.h>
#include "lib_export_dec.h" // provided by the platform vendor

int main(int argc, char **argv)
{
    int I;
    char *buf = “abcd”;
    customized();                   // customized API provided by the platform vendor
    return i;
}
```


