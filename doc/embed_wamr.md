Embed WAMR into software production
=====================================

![WAMR embed diagram](./pics/embed.PNG "WAMR embed architecture diagram")


A typical WAMR API usage is shown below (some return value checks are ignored):
``` C
  static char global_heap_buf[512 * 1024];

  char *buffer;
  wasm_module_t module;
  wasm_module_inst_t inst;
  wasm_function_inst_t func;
  wasm_exec_env_t env;
  uint32 argv[2];

  bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf));
  wasm_runtime_init();

  buffer = read_wasm_binary_to_buffer(…);
  module = wasm_runtime_load(buffer, size, err, err_size);
  inst = wasm_runtime_instantiate(module, 0, 0, err, err_size);
  func = wasm_runtime_lookup_function(inst, "fib", "(i32)i32");
  env = wasm_runtime_create_exec_env(stack_size);

  argv[0] = 8;
  if (!wasm_runtime_call_wasm(inst, env, func, 1, argv_buf) ) {
      wasm_runtime_clear_exception(inst);
  }
  /* the return value is stored in argv[0] */
  printf("fib function return: %d\n", argv[0]);

  wasm_runtime_destroy_exec_env(env);
  wasm_runtime_deinstantiate(inst);
  wasm_runtime_unload(module);
  wasm_runtime_destroy();
  bh_memory_destroy();
```

