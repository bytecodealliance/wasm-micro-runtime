/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_EXPORT_H
#define _WASM_EXPORT_H

#include <inttypes.h>
#include <stdbool.h>
#include "lib_export.h"
#include "bh_memory.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Uninstantiated WASM module loaded from WASM binary file
   or AoT binary file*/
struct WASMModuleCommon;
typedef struct WASMModuleCommon *wasm_module_t;

/* Instantiated WASM module */
struct WASMModuleInstanceCommon;
typedef struct WASMModuleInstanceCommon *wasm_module_inst_t;

/* Function instance */
typedef void WASMFunctionInstanceCommon;
typedef WASMFunctionInstanceCommon *wasm_function_inst_t;

/* WASM section */
typedef struct wasm_section_t {
    struct wasm_section_t *next;
    /* section type */
    int section_type;
    /* section body, not include type and size */
    uint8_t *section_body;
    /* section body size */
    uint32_t section_body_size;
} wasm_section_t, aot_section_t, *wasm_section_list_t, *aot_section_list_t;

/* Execution environment, e.g. stack info */
struct WASMExecEnv;
typedef struct WASMExecEnv *wasm_exec_env_t;

/* Package Type */
typedef enum {
    Wasm_Module_Bytecode = 0,
    Wasm_Module_AoT,
    Package_Type_Unknown = 0xFFFF
} package_type_t;

/* Memory allocator type */
typedef enum {
    Alloc_With_Pool = 0,
    Alloc_With_Allocator
} mem_alloc_type_t;

/* WASM runtime initialize arguments */
typedef struct RuntimeInitArgs {
    mem_alloc_type_t mem_alloc_type;
    union {
        struct {
            void *heap_buf;
            uint32_t heap_size;
        } pool;
        struct {
            void *malloc_func;
            void *free_func;
        } allocator;
    } mem_alloc;

    const char *native_module_name;
    NativeSymbol *native_symbols;
    uint32_t n_native_symbols;
} RuntimeInitArgs;

/**
 * Initialize the WASM runtime environment.
 *
 * @return true if success, false otherwise
 */
bool
wasm_runtime_init();

/**
 * Destroy the WASM runtime environment.
 */
void
wasm_runtime_destroy();

/**
 * Initialize the WASM runtime environment, and also initialize
 * the memory allocator and register native symbols, which are specified
 * with init arguments
 *
 * @param init_args specifies the init arguments
 *
 * @return return true if success, false otherwise
 */
bool
wasm_runtime_full_init(RuntimeInitArgs *init_args);

/**
 * Destroy the wasm runtime environment, and also destroy
 * the memory allocator and registered native symbols
 */
void
wasm_runtime_full_destroy();

/**
 * Get the package type of a buffer.
 *
 * @param buf the package buffer
 * @param size the package buffer size
 *
 * @return the package type, return Package_Type_Unknown if the type is unknown
 */
package_type_t
get_package_type(const uint8_t *buf, uint32_t size);

/**
 * Load a WASM module from a specified byte buffer.
 *
 * @param buf the byte buffer which contains the WASM binary data
 * @param size the size of the buffer
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
wasm_module_t
wasm_runtime_load(const uint8_t *buf, uint32_t size,
                  char *error_buf, uint32_t error_buf_size);

/**
 * Load a WASM module from a specified WASM or AOT section list.
 *
 * @param section_list the section list which contains each section data
 * @param is_aot whether the section list is AOT section list
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
wasm_module_t
wasm_runtime_load_from_sections(wasm_section_list_t section_list, bool is_aot,
                                char *error_buf, uint32_t error_buf_size);

/**
 * Unload a WASM module.
 *
 * @param module the module to be unloaded
 */
void
wasm_runtime_unload(wasm_module_t module);

void
wasm_runtime_set_wasi_args(wasm_module_t module,
                           const char *dir_list[], uint32_t dir_count,
                           const char *map_dir_list[], uint32_t map_dir_count,
                           const char *env[], uint32_t env_count,
                           char *argv[], int argc);

/**
 * Instantiate a WASM module.
 *
 * @param module the WASM module to instantiate
 * @param stack_size the default stack size of the module instance, a stack
 *        will be created when function wasm_runtime_call_wasm() is called
 *        to run WASM function and the exec_env argument passed to
 *        wasm_runtime_call_wasm() is NULL. That means this parameter is
 *        ignored if exec_env is not NULL.
 * @param heap_size the default heap size of the module instance, a heap will
 *        be created besides the app memory space. Both wasm app and native
 *        function can allocate memory from the heap. If heap_size is 0, the
 *        default heap size will be used.
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return return the instantiated WASM module instance, NULL if failed
 */
wasm_module_inst_t
wasm_runtime_instantiate(const wasm_module_t module,
                         uint32_t stack_size, uint32_t heap_size,
                         char *error_buf, uint32_t error_buf_size);

/**
 * Deinstantiate a WASM module instance, destroy the resources.
 *
 * @param module_inst the WASM module instance to destroy
 */
void
wasm_runtime_deinstantiate(wasm_module_inst_t module_inst);

bool
wasm_runtime_is_wasi_mode(wasm_module_inst_t module_inst);

wasm_function_inst_t
wasm_runtime_lookup_wasi_start_function(wasm_module_inst_t module_inst);

/**
 * Lookup an exported function in the WASM module instance.
 *
 * @param module_inst the module instance
 * @param name the name of the function
 * @param signature the signature of the function, ignored currently
 *
 * @return the function instance found
 */
wasm_function_inst_t
wasm_runtime_lookup_function(wasm_module_inst_t const module_inst,
                             const char *name, const char *signature);

/**
 * Create execution environment for a WASM module instance.
 *
 * @param module_inst the module instance
 * @param stack_size the stack size to execute a WASM function
 *
 * @return the execution environment
 */
wasm_exec_env_t
wasm_runtime_create_exec_env(wasm_module_inst_t module_inst,
                             uint32_t stack_size);

/**
 * Destroy the execution environment.
 *
 * @param env the execution environment to destroy
 */
void
wasm_runtime_destroy_exec_env(wasm_exec_env_t exec_env);

/**
 * Get WASM module instance from execution environment
 *
 * @param exec_env the execution environment to retrieve
 *
 * @return the WASM module instance
 */
wasm_module_inst_t
wasm_runtime_get_module_inst(wasm_exec_env_t exec_env);

/**
 * Call the given WASM function of a WASM module instance with
 * arguments (bytecode and AoT).
 *
 * @param exec_env the execution environment to call the function
 *   which must be created from wasm_create_exec_env()
 * @param function the function to be called
 * @param argc the number of arguments
 * @param argv the arguments.  If the function method has return value,
 *   the first (or first two in case 64-bit return value) element of
 *   argv stores the return value of the called WASM function after this
 *   function returns.
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_runtime_call_wasm(wasm_exec_env_t exec_env,
                       wasm_function_inst_t function,
                       uint32_t argc, uint32_t argv[]);

/**
 * Find the unique main function from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main function is called, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_application_execute_main(wasm_module_inst_t module_inst,
                              int32_t argc, char *argv[]);

/**
 * Find the specified function in argv[0] from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param name the name of the function to execute
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the specified function is called, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_application_execute_func(wasm_module_inst_t module_inst,
                              const char *name, int32_t argc, char *argv[]);
/**
 * Get exception info of the WASM module instance.
 *
 * @param module_inst the WASM module instance
 *
 * @return the exception string
 */
const char *
wasm_runtime_get_exception(wasm_module_inst_t module_inst);

/**
 * Clear exception info of the WASM module instance.
 *
 * @param module_inst the WASM module instance
 */
void
wasm_runtime_clear_exception(wasm_module_inst_t module_inst);

/**
 * Set custom data to WASM module instance.
 *
 * @param module_inst the WASM module instance
 * @param custom_data the custom data to be set
 */
void
wasm_runtime_set_custom_data(wasm_module_inst_t module_inst,
                             void *custom_data);
/**
 * Get the custom data within a WASM module instance.
 *
 * @param module_inst the WASM module instance
 *
 * @return the custom data (NULL if not set yet)
 */
void *
wasm_runtime_get_custom_data(wasm_module_inst_t module_inst);

/**
 * Allocate memory from the heap of WASM module instance
 *
 * @param module_inst the WASM module instance which contains heap
 * @param size the size bytes to allocate
 * @param p_native_addr return native address of the allocated memory
 *        if it is not NULL, and return NULL if memory malloc failed
 *
 * @return the allocated memory address, which is a relative offset to the
 *         base address of the module instance's memory space, the value range
 *         is (-heap_size, 0). Note that it is not an absolute address.
 *         Return non-zero if success, zero if failed.
 */
int32_t
wasm_runtime_module_malloc(wasm_module_inst_t module_inst, uint32_t size,
                           void **p_native_addr);

/**
 * Free memory to the heap of WASM module instance
 *
 * @param module_inst the WASM module instance which contains heap
 * @param ptr the pointer to free
 */
void
wasm_runtime_module_free(wasm_module_inst_t module_inst, int32_t ptr);

/**
 * Allocate memory from the heap of WASM module instance and initialize
 * the memory with src
 *
 * @param module_inst the WASM module instance which contains heap
 * @param src the source data to copy
 * @param size the size of the source data
 *
 * @return the allocated memory address, which is a relative offset to the
 *         base address of the module instance's memory space, the value range
 *         is (-heap_size, 0). Note that it is not an absolute address.
 *         Return non-zero if success, zero if failed.
 */
int32_t
wasm_runtime_module_dup_data(wasm_module_inst_t module_inst,
                             const char *src, uint32_t size);

/**
 * Validate the app address, check whether it belongs to WASM module
 * instance's address space, or in its heap space or memory space.
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app address to validate, which is a relative address
 * @param size the size bytes of the app address
 *
 * @return true if success, false otherwise. If failed, an exception will
 *         be thrown.
 */
bool
wasm_runtime_validate_app_addr(wasm_module_inst_t module_inst,
                               int32_t app_offset, uint32_t size);

/**
 * Similar to wasm_runtime_validate_app_addr(), except that the size parameter
 * is not provided. This function validates the app string address, check whether it
 * belongs to WASM module instance's address space, or in its heap space or
 * memory space. Moreover, it checks whether it is the offset of a string that
 * is end with '\0'.
 * @param module_inst the WASM module instance
 * @param app_str_offset the app address of the string to validate, which is a
 *        relative address
 *
 * @return true if success, false otherwise. If failed, an exception will
 *         be thrown.
 */
bool
wasm_runtime_validate_app_str_addr(wasm_module_inst_t module_inst,
                                   int32_t app_str_offset);

/**
 * Validate the native address, check whether it belongs to WASM module
 * instance's address space, or in its heap space or memory space.
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address to validate, which is an absolute
 *        address
 * @param size the size bytes of the app address
 *
 * @return true if success, false otherwise. If failed, an exception will
 *         be thrown.
 */
bool
wasm_runtime_validate_native_addr(wasm_module_inst_t module_inst,
                                  void *native_ptr, uint32_t size);

/**
 * Convert app address(relative address) to native address(absolute address)
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app adress
 *
 * @return the native address converted
 */
void*
wasm_runtime_addr_app_to_native(wasm_module_inst_t module_inst,
                                int32_t app_offset);

/**
 * Convert native address(absolute address) to app address(relative address)
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address
 *
 * @return the app address converted
 */
int32_t
wasm_runtime_addr_native_to_app(wasm_module_inst_t module_inst,
                                void *native_ptr);

/**
 * Get the app address range (relative address) that a app address belongs to
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app address to retrieve
 * @param p_app_start_offset buffer to output the app start offset if not NULL
 * @param p_app_end_offset buffer to output the app end offset if not NULL
 *
 * @return true if success, false otherwise.
 */
bool
wasm_runtime_get_app_addr_range(wasm_module_inst_t module_inst,
                                int32_t app_offset,
                                int32_t *p_app_start_offset,
                                int32_t *p_app_end_offset);

/**
 * Get the native address range (absolute address) that a native address belongs to
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address to retrieve
 * @param p_native_start_addr buffer to output the native start address if not NULL
 * @param p_native_end_addr buffer to output the native end address if not NULL
 *
 * @return true if success, false otherwise.
 */
bool
wasm_runtime_get_native_addr_range(wasm_module_inst_t module_inst,
                                   uint8_t *native_ptr,
                                   uint8_t **p_native_start_addr,
                                   uint8_t **p_native_end_addr);

/**
  * Register native functions with same module name
  *
  * @param module_name the module name of the native functions
  * @param native_symbols specifies an array of NativeSymbol structures which
  *        contain the names, function pointers and signatures
  *        Note: WASM runtime will not allocate memory to clone the data, so
  *              user must ensure the array can be used forever
  *        Meanings of letters in function signature:
  *          'i': the parameter is i32 type
  *          'I': the parameter is i64 type
  *          'f': the parameter is f32 type
  *          'F': the parameter is f64 type
  *          '*': the parameter is a pointer (i32 in WASM), and runtime will
  *               auto check its boundary before calling the native function.
  *               If it is followed by '~', the checked length of the pointer
  *               is gotten from the following parameter, if not, the checked
  *               length of the pointer is 1.
  *          '~': the parameter is the pointer's length with i32 type, and must
  *               follow after '*'
  *          '$': the parameter is a string (i32 in WASM), and runtime will
  *               auto check its boundary before calling the native function
  * @param n_native_symbols specifies the number of native symbols in the array
  *
  * @return true if success, false otherwise
  */
bool wasm_runtime_register_natives(const char *module_name,
                                   NativeSymbol *native_symbols,
                                   uint32_t n_native_symbols);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_EXPORT_H */
