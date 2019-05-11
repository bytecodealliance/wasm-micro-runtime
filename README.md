WebAssembly Micro Runtime
=========================
WebAssembly Micro Runtime (WAMR) is standalone WebAssembly (WASM) runtime with a small footprint. It includes a few components:
- WebAssembly VM core
- WASM application programming API (code available, but compilation depends on the app manager component)
- Dynamic WASM application management (Not available on Github yet. It will be released soon)

Why should you use a WASM runtime out of your browser? There are a few points where this might be meaningful:
1.	WASM is already a LLVM official backend target. That means WASM can run any programming languages which can be compiled to LLVM IR. It is a huge advantage compared to language bound runtimes like JS or Lua.
2.	WASM is an open standard and it is fast becoming supported by the whole web ecosystem.
3.	WASM is designed to be very friendly for compiling to native binaries and gaining the native speed.
4.	It can potentially change the development practices. Imagine we can do both the WASM application development and validation in a browser, then just download the WASM binary code onto the target device.
5.	WASM can work without garbage collection. It is designed to support execution determinics for the time sensitive requirement.


Features
=========================
- WASM interpreter (AOT is planned)
- Provides built-in Libc subset, supports "side_module=1" EMCC compilation option
- Provides API's for embedding runtime into production software
- Provides a mechanism for exporting native API's to WASM applications
- Supports the programming of firmware apps in a large range of languages (C/C++/Java/Rust/Go/TypeScript etc.)
- App sandbox execution environment on embedded OS
- Purely asynchronized programming model
- Menu configuration for easy platform integration
- Supports micro-service and pub-sub event inter-app communication models
- Easy to extend to support remote FW application management from host or cloud

Architecture
=========================
The application manager component handles the packets that the platform receives from external sources through any communication buses such as socket, serial port or PSI. A packet type can be either a request, response or event. The app manager will serve the requests with URI "/applet" and call the runtime glue layer interfaces for installing/uninstalling the application. For other URI's, it will filter the resource registration table and route the request to the internal queue of the responsible application.

The WebAssembly runtime is the execution environment for WASM applications. 

The messaging layer can support the API for WASM applications to communicate to each other and also the host environment.

When Ahead of Time compilation is enabled, the WASM application can be either bytecode or a compiled native binary. 

<img src="./doc/pics/architecture.PNG" width="80%" height="80%">

  

Build WAMR Core
=========================
Please follow the instructions below to build the WAMR core on different platforms.

Linux
-------------------------
First of all please install library dependencies of lib gcc.
Use installation commands below for Ubuntu Linux: 
``` Bash
sudo apt install lib32gcc-5-dev
sudo apt-get install g++-multilib
```
After installing dependencies, build the source code:
``` Bash
cd core/iwasm/products/linux/
mkdir build
cd build
cmake ..
make
```
Zephyr
-------------------------
You need to download the Zephyr source code first and embedded WAMR into it.
``` Bash
git clone https://github.com/zephyrproject-rtos/zephyr.git
cd zephyr/samples/
cp -a <iwasm_dir>/products/zephyr/simple .
cd simple
ln -s <iwam_dir> iwasm
ln -s <shared_lib_dir> shared-lib
mkdir build && cd build
source ../../../zephyr-env.sh
cmake -GNinja -DBOARD=qemu_x86 ..
ninja
```

Build WASM app
=========================
A popular method to build out WASM binary is to use ```emcc```. 
Assuming you are using Linux, please install emcc from Emscripten EMSDK following the steps below:
```
git clone https://github.com/emscripten-core/emsdk.git
emsdk install latest
emsdk activate latest
```
add ```./emsdk_env.sh``` into the path to ease future use, or source it everytime.
The Emscripten website provides other installation methods beyond Linux.

todo: user should copy the app-libs folder into project and include and build.

You can write a simple ```test.c``` as the first sample.
``` C
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  char *buf;

  printf("Hello world!\n");

  buf = malloc(1024);
  if (!buf) {
    printf("malloc buf failed\n");
    return -1;
  }

  printf("buf ptr: %p\n", buf);

  sprintf(buf, "%s", "1234\n");
  printf("buf: %s", buf);

  free(buf);
  return 0;
}
```
Use the emcc command below to build the WASM C source code into the WASM binary.
``` Bash
emcc -g -O3 *.c -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
                -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 -o test.wasm
```
You will get ```test.wasm``` which is the WASM app binary.

Run WASM app
========================
Assume you are using Linux, the command to run the test.wasm is:
``` Bash
cd iwasm/products/linux/bin
./iwasm test.wasm
```
You will get the following output:
```
Hello world!
buf ptr: 0x000101ac
buf: 1234
```
If you would like to run the test app on Zephyr, we have embedded a test sample into its OS image. You will need to execute:
```
ninja run
```

Embed WAMR into software production
=====================================
WAMR can be built into a standalone executable which takes WASM application file name as input, and then execute it. To use it in the embedded environment, you should embed WAMR into your own software product. WASM provides a set of API's for embedded code to load WASM module, instantiate the module and invoke the WASM function from a native call.

<img src="./doc/pics/embed.PNG" width="60%" height="60%">


A typical WAMR API usage is as described below:
``` C
  wasm_module_t module;
  wasm_module_inst_t inst;
  wasm_function_inst_t func;
  wasm_exec_env_t env;
  wasm_runtime_init();
  module = wasm_runtime_load(buffer, size, err, err_size);
  inst = wasm_runtime_instantiate(module, 0, err, err_size);
  func = wasm_runtime_lookup_function(inst, "fib", "(i32i32");
  env = wasm_runtime_create_exec_env(stack_size);

  if (!wasm_runtime_call_wasm(inst, env, func, 1, argv_buf) ) {
          wasm_runtime_clear_exception(inst);
    }

  wasm_runtime_destory_exec_env(env);
  wasm_runtime_deinstantiate(inst);
  wasm_runtime_unload(module);
  wasm_runtime_destroy();
```


WASM application library 
========================
In general, there are 3 kinds of API's for programming the WASM application:
- Built-in API's: WAMR has already provided a minimal API set for developers. 
- 3rd party API's: Programmer can download and include any 3rd party C source code, and add it into their own WASM app source tree.
- Platform native API's: WAMR provides a mechanism to export the native API to the WASM application.


Built-in application library
---------------
Built-in API's include Libc APIs, Base library and Extension library reference.

**Libc APIs**<br/>
This is the minimal Libc APIs like memory allocation and string copy etc.
The header file is ```lib/app-libs/libc/lib-base.h```. The API set is listed as below:
``` C
void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void free(void *ptr);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int putchar(int c);
int snprintf(char *str, size_t size, const char *format, ...);
int sprintf(char *str, const char *format, ...);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *s);
int strncmp(const char * str1, const char * str2, size_t n);
char *strncpy(char *dest, const char *src, unsigned long n);
```

**Base library**<br/>
The basic support for communication, timers etc is already available. You can refer to the header file ```lib/app-libs/base/wasm-app.h``` which contains the definitions for request and response API's, event pub/sub APIs and timer APIs. Please note that these API's require the native implementations.
The API set is listed as below:
``` C
typedef void(*request_handler_f)(request_t *) ;
typedef void(*response_handler_f)(response_t *, void *) ;

// Request APIs
bool api_register_resource_handler(const char *url, request_handler_f);
void api_send_request(request_t * request, response_handler_f response_handler, void * user_data);
void api_response_send(response_t *response);

// event AP
bool api_publish_event(const char *url,  int fmt, void *payload,  int payload_len);
bool api_subscribe_event(const char * url, request_handler_f handler);

struct user_timer;
typedef struct user_timer * user_timer_t;

// Timer APIs
user_timer_t api_timer_create(int interval, bool is_period, bool auto_start, void(*on_user_timer_update)(user_timer_t
));
void api_timer_cancel(user_timer_t timer);
void api_timer_restart(user_timer_t timer, int interval);
```

**Library extension reference**<br/>
Currently we provide the sensor API's as one library extension sample. In the header file ```lib/app-libs/extension/sensor/sensor.h```, the API set is defined as below:
``` C
sensor_t sensor_open(const char* name, int index,
                                     void(*on_sensor_event)(sensor_t, attr_container_t *, void *),
                                     void *user_data);
bool sensor_config(sensor_t sensor, int interval, int bit_cfg, int delay);
bool sensor_config_with_attr_container(sensor_t sensor, attr_container_t *cfg);
bool sensor_close(sensor_t sensor);
```

The mechanism of exporting Native API to WASM application
=======================================================

The basic working flow for WASM application calling into the native API is described in the following diagram.
<img src="./doc/pics/extend_library.PNG" width="60%" height="60%">


WAMR provides the macro `EXPORT_WASM_API` to enable users to export native API to a WASM application. WAMR implemented a base API for the timer and messaging by using `EXPORT_WASM_API`. They can be a reference point for extending your own library.
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


![#f03c15](https://placehold.it/15/f03c15/000000?text=+) **Security attention:** The WebAssembly application is supposed to access its own memory space, the integrator should carefully design the native function to ensure that the memory is safe. The native API to be exported to the WASM application must follow these rules:
- Only use 32 bits number for parameters
- Don't pass data to the structure pointer (do data serialization instead)
- Do the pointer address conversion in the native API
- Don’t pass function pointer as callback

Below is a sample of a library extension. All code invoked across WASM and native world must be serialized and de-serialized, and the native world must do a boundary check for every incoming address from the WASM world.

<img src="./doc/pics/safe.PNG" width="100%" height="100%">

Exporting native API steps
==========================

WAMR implemented a framework for developers to export API's. Below is the procedure to expose the platform APIs in three steps:

**Step 1. Create a header file**<br/>
Declare the API's for your WASM application source project to include.

**Step 2. Create a source file**<br/>
Export the platform API's, for example in ``` products/linux/ext-lib-export.c ```
``` C
#include "lib-export.h"

static NativeSymbol extended_native_symbol_defs[] =
{
};

#include "ext-lib-export.h"
```

**Step 3. Register new APIs**<br/>
Use the macro `EXPORT_WASM_API` and `EXPORT_WASM_API2` to add exported API's into the array of ```extended_native_symbol_defs```.
The pre-defined MACRO `EXPORT_WASM_API` should be used to declare a function export:
``` c
#define EXPORT_WASM_API(symbol)  {#symbol, symbol}
```

Below code example shows how to extend the library to support `customized()`:
``` C
//lib-export-impl.c
void customized()
{
   // your code
}


// lib-export-dec.h
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


// ext-lib-export.c
#include "lib-export.h"
#include "lib-export-dec.h"

static NativeSymbol extended_native_symbol_defs[] =
{
  EXPORT_WASM_API(customized)
};

#include "ext-lib-export.h"
```
Use extended library
------------------------
In the application source project, it will include the WAMR built-in APIs header file and platform extension header files.
This is assuming the board vendor extends the library which added an API called customized(). The WASM application would be like this:
``` C
#include <stdio.h>
#include "lib-export-dec.h" // provided by the platform vendor

int main(int argc, char **argv)
{
  int I;
  char *buf = “abcd”;
  customized();                   // customized API provided by the platform vendor
  return i;
}
```

Coming soon...
========================
We are preparing the open source code for the application manager and related code samples like inter-application communication, application life cycle management, 2D graphic demo and more. This will get updated soon.

Submit issues and request
=========================
[Click here to submit. Your feedback is always welcome!](https://github.com/intel/wasm-micro-runtime/issues/new)

