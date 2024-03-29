#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "wasm_c_api.h"

#define own

// A function to be called from Wasm code.
own wasm_trap_t* hello_callback(
  const wasm_val_vec_t* args, wasm_val_vec_t* results
) {
  printf("Calling back...\n");
  printf("> Hello World!\n");
  return NULL;
}


int main(int argc, const char* argv[]) {
  // Initialize.
  printf("Initializing...\n");
  wasm_engine_t* engine = wasm_engine_new();
  wasm_store_t* store = wasm_store_new(engine);

  // Load binary.
  printf("Loading binary...\n");
#if WASM_ENABLE_AOT != 0 && WASM_ENABLE_INTERP == 0
  FILE* file = fopen("hello.aot", "rb");
#else
  FILE* file = fopen("hello.wasm", "rb");
#endif
  if (!file) {
    printf("> Error loading module!\n");
    return 1;
  }

  int ret = fseek(file, 0L, SEEK_END);
  if (ret == -1) {
    printf("> Error loading module!\n");
    fclose(file);
    return 1;
  }

  long file_size = ftell(file);
  if (file_size == -1) {
    printf("> Error loading module!\n");
    fclose(file);
    return 1;
  }

  ret = fseek(file, 0L, SEEK_SET);
  if (ret == -1) {
    printf("> Error loading module!\n");
    fclose(file);
    return 1;
  }

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, file_size);
  if (fread(binary.data, file_size, 1, file) != 1) {
    printf("> Error loading module!\n");
    fclose(file);
    return 1;
  }
  fclose(file);

  if (!wasm_module_validate(store, &binary)) {
    printf("> Error validate module!\n");
    wasm_byte_vec_delete(&binary);
    return 1;
  }

  // Compile.
  printf("Compiling module...\n");
  own wasm_module_t* module = wasm_module_new(store, &binary);
  if (!module) {
    printf("> Error compiling module!\n");
    return 1;
  }

  wasm_byte_vec_delete(&binary);

  assert(wasm_module_set_name(module, "hello"));

  // Create external print functions.
  printf("Creating callback...\n");
  own wasm_functype_t* hello_type = wasm_functype_new_0_0();
  own wasm_func_t* hello_func =
    wasm_func_new(store, hello_type, hello_callback);

  wasm_functype_delete(hello_type);

  // Instantiate.
  printf("Instantiating module...\n");
  wasm_extern_t* externs[] = { wasm_func_as_extern(hello_func) };
  wasm_extern_vec_t imports = WASM_ARRAY_VEC(externs);
  own wasm_instance_t* instance =
    wasm_instance_new(store, module, &imports, NULL);
  if (!instance) {
    printf("> Error instantiating module!\n");
    return 1;
  }

  wasm_func_delete(hello_func);

  // Extract export.
  printf("Extracting export...\n");
  own wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  if (exports.size == 0) {
    printf("> Error accessing exports!\n");
    return 1;
  }

  const wasm_func_t* run_func = wasm_extern_as_func(exports.data[0]);
  if (run_func == NULL) {
    printf("> Error accessing export!\n");
    return 1;
  }

  {
    const char* name = wasm_module_get_name(module);
    assert(strncmp(name, "hello", 5) == 0);
    printf("> removing module %s \n", name);
  }

  wasm_module_delete(module);
  wasm_instance_delete(instance);

  // Call.
  printf("Calling export...\n");
  wasm_val_vec_t args = WASM_EMPTY_VEC;
  wasm_val_vec_t results = WASM_EMPTY_VEC;
  wasm_trap_t *trap = wasm_func_call(run_func, &args, &results);
  if (trap) {
      printf("> Error calling function!\n");
      wasm_trap_delete(trap);
      return 1;
  }

  wasm_extern_vec_delete(&exports);

  // Shut down.
  printf("Shutting down...\n");
  wasm_store_delete(store);
  wasm_engine_delete(engine);

  // All done.
  printf("Done.\n");
  return 0;
}
