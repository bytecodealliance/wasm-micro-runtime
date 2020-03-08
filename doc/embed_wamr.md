Embedding WAMR guideline
=====================================


**Note**: All the embedding APIs supported by the runtime are defined under folder [core/iwasm/include](../core/iwasm/include). The API details are available in the header files.

## The initialization procedure



``` C
  static char global_heap_buf[512 * 1024];

  char *buffer, error_buf[128];
  wasm_module_t module;
  wasm_module_inst_t module_inst;
  wasm_function_inst_t func;
  wasm_exec_env_t exec_env;
  uint32 size, stack_size = 8092, heap_size = 8092;

  // all the WAMR heap and WASM applications are limited in this buffer
  bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf));

  wasm_runtime_init();

  // read WASM file into a memory buffer
  buffer = read_wasm_binary_to_buffer(…, &size);

  // parse the WASM file from buffer and create a WASM module
  module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));

  // create an instance of the WASM module (WASM linear memory is ready)
  module_inst = wasm_runtime_instantiate(module,
                                         stack_size,
                                         heap_size,
                                         error_buf,
                                         sizeof(error_buf));
```





## Native calls WASM functions and passes parameters

After a module is instantiated, the runtime native can lookup WASM functions by the names and call them.

```c
  unit32 argv[2];

  // lookup a WASM function by its name
  func = wasm_runtime_lookup_function(module_inst, "fib", NULL);

  // creat a excution environment which can be used by executing WASM functions
  exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);

  // arguments are always transferred in 32 bits element
  argv[0] = 8;

  // call the WASM function
  if (wasm_runtime_call_wasm(exec_env, func, 1, argv) ) {
    /* the return value is stored in argv[0] */
    printf("fib function return: %d\n", argv[0]);
  }
  else {
    printf("%s\n", wasm_runtime_get_exception(module_inst));
  }
```



The parameters are transferred in an array of 32 bits elements. For parameters that occupy 4 or fewer bytes, each parameter can be a single array element. For parameters in types like double or int64, each parameter will take two array elements. The function return value will be sent back in the first one or two elements of the array according to the value type. See the sample code below:

```c
  unit32 argv[6];
  char arg1 = 'a';
  int arg2 = 10;
  double arg3 = 1.0;
  int 64 arg4 = 100;
  double ret;

  argv[0] = arg1;
  argv[1] = arg2;

  // use memory copy for 8 bytes parameters rather than
  // *(double*)(&argv[2]) = arg3 here because some archs
  // like ARM, MIPS requires address is 8 aligned.
  // Or use the aligned malloc or compiler align attribute
  // to ensure the array address is 8 bytes aligned
  memcpy(&argv[2], &arg3, sizeof(arg3));
  memcpy(&argv[4], &arg4, sizeof(arg4));
  //
  // attention: the arg number is 6 here since both
  //            arg3 and arg4 each takes 2 elements
  //
  wasm_runtime_call_wasm(exec_env, func, 6, argv);
  
  // if the return value is type of 8 bytes, it takes
  // the first two array elements
  memcpy(&ret, &argv[0], sizeof(ret));

```



## Pass buffer to WASM function



If we need to transfer a buffer to WASM function, we can pass the buffer address through a parameter. **Attention**: The sandbox will forbid the WASM code to access outside memory, we must **allocate the buffer from WASM instance's own memory space and pass the buffer address in instance's space (not the runtime native address)**.



There are two runtime APIs available for this purpose.

```c
/*
* description: malloc a buffer from instance's private memory space.
*
* return: the buffer address in instance's memory space (pass to the WASM funciton)
* p_native_addr: return the native address of allocated memory
* size: the buffer size to allocate
*/
int32_t
wasm_runtime_module_malloc(wasm_module_inst_t module_inst,
           uint32_t size,
           void **p_native_addr);

/*
* description: malloc a buffer from instance's private memory space,
*              and copy the data from another native buffer to it.
* return: the buffer address in instance's memory space (pass to the WASM funciton)
* src: the native buffer address
* size: the size of buffer to be allocated and copy data
*/
int32
wasm_runtime_module_dup_data(WASMModuleInstanceCommon *module_inst,
                             const char *src,
                             uint32 size);
```



Usage sample:

```c
char * buffer = NULL;
int32_t buffer_for_wasm;

buffer_for_wasm = wasm_runtime_module_malloc(module_inst, 100, &buffer);
if(buffer_for_wasm != 0)
{
    unit32 argv[2];
    strncpy(buffer, "hello", 100);	// use native address for accessing in runtime
    argv[0] = buffer_for_wasm;  	// pass the buffer address for WASM space.
    argv[1] = 100;					// the size of buffer
    wasm_runtime_call_wasm(exec_env, func, 2, argv);
}

```



## Pass structured data to WASM function

We can't pass structure data or class objects through the pointer since the memory layout can different in two worlds. The way to do it is serialization. Refer to [export_native_api.md](./export_native_api.md) for the details.



## The deinitialization procedure

```
  wasm_runtime_destroy_exec_env(exec_env);
  wasm_runtime_deinstantiate(module_inst);
  wasm_runtime_unload(module);
  wasm_runtime_destroy();
  bh_memory_destroy();
```



## Native calling WASM function working flow

![WAMR embed diagram](./pics/embed.PNG "WAMR embed architecture diagram")
