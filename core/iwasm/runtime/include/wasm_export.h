/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WASM_EXPORT_H
#define _WASM_EXPORT_H

#include <inttypes.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Uninstantiated WASM module loaded from WASM binary file */
struct WASMModule;
typedef struct WASMModule *wasm_module_t;

/* Instantiated WASM module */
struct WASMModuleInstance;
typedef struct WASMModuleInstance *wasm_module_inst_t;

/* Function instance */
struct WASMFunctionInstance;
typedef struct WASMFunctionInstance *wasm_function_inst_t;

/* WASM section */
typedef struct wasm_section {
    struct wasm_section *next;
    /* section type */
    int section_type;
    /* section body, not include type and size */
    uint8_t *section_body;
    /* section body size */
    uint32_t section_body_size;
} wasm_section_t, *wasm_section_list_t;

/* Execution environment, e.g. stack info */
typedef struct WASMExecEnv {
    uint8_t *stack;
    uint32_t stack_size;
} *wasm_exec_env_t;

/* Package Type */
typedef enum {
    Wasm_Module_Bytecode = 0,
    Wasm_Module_AoT,
    Package_Type_Unknown = 0xFFFF
} package_type_t;

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
 * Load a WASM module from a specified WASM section list.
 *
 * @param section_list the section list which contains each section data
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
wasm_module_t
wasm_runtime_load_from_sections(wasm_section_list_t section_list,
                                char *error_buf, uint32_t error_buf_size);

/**
 * Unload a WASM module.
 *
 * @param module the module to be unloaded
 */
void
wasm_runtime_unload(wasm_module_t module);

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

#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
bool
wasm_runtime_set_ext_memory(wasm_module_inst_t module_inst,
                            uint8_t *ext_mem_data, uint32_t ext_mem_size,
                            char *error_buf, uint32_t error_buf_size);
#endif

/**
 * Load WASM module instance from AOT file.
 *
 * @param aot_file the AOT file of a WASM module
 * @param aot_file_size the AOT file size
 * @param heap_size the default heap size of the module instance, a heap will
 *        be created besides the app memory space. Both wasm app and native
 *        function can allocate memory from the heap. If heap_size is 0, the
 *        default heap size will be used.
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return the instantiated WASM module instance, NULL if failed
 */
wasm_module_inst_t
wasm_runtime_load_aot(uint8_t *aot_file, uint32_t aot_file_size,
                      uint32_t heap_size,
                      char *error_buf, uint32_t error_buf_size);

/**
 * Lookup an exported function in the WASM module instance.
 *
 * @param module_inst the module instance
 * @param name the name of the function
 * @param signature the signature of the function, use "i32"/"i64"/"f32"/"f64"
 *        to represent the type of i32/i64/f32/f64, e.g. "(i32i64)" "(i32)f32"
 *
 * @return the function instance found, if the module instance is loaded from
 *         the AOT file, the return value is the function pointer
 */
wasm_function_inst_t
wasm_runtime_lookup_function(const wasm_module_inst_t module_inst,
                             const char *name, const char *signature);

/**
 * Create execution environment.
 *
 * @param stack_size the stack size to execute a WASM function
 *
 * @return the execution environment
 */
wasm_exec_env_t
wasm_runtime_create_exec_env(uint32_t stack_size);

/**
 * Destroy the execution environment.
 *
 * @param env the execution environment to destroy
 */
void
wasm_runtime_destroy_exec_env(wasm_exec_env_t env);

/**
 * Call the given WASM function of a WASM module instance with
 * arguments (bytecode and AoT).
 *
 * @param module_inst the WASM module instance which the function belongs to
 * @param exec_env the execution environment to call the function. If the module
 *        instance is created by AoT mode, it is ignored and just set it to NULL.
 *        If the module instance is created by bytecode mode and it is NULL,
 *        a temporary env object will be created
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
wasm_runtime_call_wasm(wasm_module_inst_t module_inst,
                       wasm_exec_env_t exec_env,
                       wasm_function_inst_t function,
                       uint32_t argc, uint32_t argv[]);

/**
 * Get exception info of the WASM module instance.
 *
 * @param module_inst the WASM module instance
 *
 * @return the exception string
 */
const char*
wasm_runtime_get_exception(wasm_module_inst_t module_inst);

/**
 * Clear exception info of the WASM module instance.
 *
 * @param module_inst the WASM module instance
 */
void
wasm_runtime_clear_exception(wasm_module_inst_t module_inst);

/**
 * Attach the current native thread to a WASM module instance.
 * A native thread cannot be attached simultaneously to two WASM module
 * instances. The WASM module instance will be attached to the native
 * thread which it is instantiated in by default.
 *
 * @param module_inst the WASM module instance to attach
 * @param thread_data the thread data that current native thread requires
 *        the WASM module instance to store
 *
 * @return true if SUCCESS, false otherwise
 */
bool
wasm_runtime_attach_current_thread(wasm_module_inst_t module_inst,
                                   void *thread_data);

/**
 * Detach the current native thread from a WASM module instance.
 *
 * @param module_inst the WASM module instance to detach
 */
void
wasm_runtime_detach_current_thread(wasm_module_inst_t module_inst);

/**
 * Get the thread data that the current native thread requires the WASM
 * module instance to store when attaching.
 *
 * @return the thread data stored when attaching
 */
void*
wasm_runtime_get_current_thread_data();

/**
 * Get current WASM module instance of the current native thread
 *
 * @return current WASM module instance of the current native thread, NULL
 *         if not found
 */
wasm_module_inst_t
wasm_runtime_get_current_module_inst();

/**
 * Allocate memory from the heap of WASM module instance
 *
 * @param module_inst the WASM module instance which contains heap
 * @param size the size bytes to allocate
 *
 * @return the allocated memory address, which is a relative offset to the
 *         base address of the module instance's memory space, the value range
 *         is (-heap_size, 0). Note that it is not an absolute address.
 *         Return non-zero if success, zero if failed.
 */
int32_t
wasm_runtime_module_malloc(wasm_module_inst_t module_inst, uint32_t size);

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
void *
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

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_EXPORT_H */
