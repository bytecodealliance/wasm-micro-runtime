/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_EXPORT_2_H
#define _WASM_EXPORT_2_H

#include "wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime structure */
struct WASMRuntime;
typedef struct WASMRuntime WASMRuntime;

/**
 * Initialize the WASM runtime environment, and also initialize
 * the memory allocator with system allocator, which calls os_malloc
 * to allocate memory
 *
 * @param runtime the runtime context
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_init2(WASMRuntime *runtime);

/**
 * Initialize the WASM runtime environment, WASM running mode,
 * and also initialize the memory allocator and register native symbols,
 * which are specified with init arguments
 *
 * @param runtime the runtime context
 * @param init_args specifies the init arguments
 *
 * @return return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_full_init2(WASMRuntime *runtime, RuntimeInitArgs *init_args);

/**
 * Set the log level. To be called after the runtime is initialized.
 *
 * @param runtime the runtime context
 * @param level the log level to set
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_log_level2(WASMRuntime *runtime, log_level_t level);

/**
 * Query whether a certain running mode is supported for the runtime
 *
 * @param runtime the runtime context
 * @param running_mode the running mode to query
 *
 * @return true if this running mode is supported, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_is_running_mode_supported2(WASMRuntime *runtime,
                                        RunningMode running_mode);

/**
 * Set the default running mode for the runtime. It is inherited
 * to set the running mode of a module instance when it is instantiated,
 * and can be changed by calling wasm_runtime_set_running_mode
 *
 * @param runtime the runtime context
 * @param running_mode the running mode to set
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_set_default_running_mode2(WASMRuntime *runtime,
                                       RunningMode running_mode);

/**
 * Destroy the WASM runtime environment.
 *
 * @param runtime the runtime context
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_destroy2(WASMRuntime *runtime);

/**
 * Allocate memory from runtime memory environment.
 *
 * @param runtime the runtime context
 * @param size bytes need to allocate
 *
 * @return the pointer to memory allocated
 */
WASM_RUNTIME_API_EXTERN void *
wasm_runtime_malloc2(WASMRuntime *runtime, unsigned int size);

/**
 * Reallocate memory from runtime memory environment
 *
 * @param runtime the runtime context
 * @param ptr the original memory
 * @param size bytes need to reallocate
 *
 * @return the pointer to memory reallocated
 */
WASM_RUNTIME_API_EXTERN void *
wasm_runtime_realloc2(WASMRuntime *runtime, void *ptr, unsigned int size);

/**
 * Free memory to runtime memory environment.
 *
 * @param runtime the runtime context
 * @param ptr the pointer to memory
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_free2(WASMRuntime *runtime, void *ptr);

/**
 * Get memory info, only pool mode is supported now.
 *
 * @param runtime the runtime context
 * @param mem_alloc_info the memory allocation info
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_get_mem_alloc_info2(WASMRuntime *runtime,
                                 mem_alloc_info_t *mem_alloc_info);

/**
 * Get the package type of a buffer.
 *
 * @param runtime the runtime context
 * @param buf the package buffer
 * @param size the package buffer size
 *
 * @return the package type, return Package_Type_Unknown if the type is unknown
 */
WASM_RUNTIME_API_EXTERN package_type_t
get_package_type2(WASMRuntime *runtime, const uint8_t *buf, uint32_t size);

/**
 * Get the package type of a buffer (same as get_package_type).
 *
 * @param runtime the runtime context
 * @param buf the package buffer
 * @param size the package buffer size
 *
 * @return the package type, return Package_Type_Unknown if the type is unknown
 */
WASM_RUNTIME_API_EXTERN package_type_t
wasm_runtime_get_file_package_type2(WASMRuntime *runtime, const uint8_t *buf,
                                    uint32_t size);

/**
 * Get the package type of a module.
 *
 * @param runtime the runtime context
 * @param module the module
 *
 * @return the package type, return Package_Type_Unknown if the type is unknown
 */
WASM_RUNTIME_API_EXTERN package_type_t
wasm_runtime_get_module_package_type2(WASMRuntime *runtime,
                                      const wasm_module_t module);

/**
 * Get the package version of a buffer.
 *
 * @param runtime the runtime context
 * @param buf the package buffer
 * @param size the package buffer size
 *
 * @return the package version, return zero if the version is unknown
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_get_file_package_version2(WASMRuntime *runtime, const uint8_t *buf,
                                       uint32_t size);

/**
 * Get the package version of a module
 *
 * @param runtime the runtime context
 * @param module the module
 *
 * @return the package version, or zero if version is unknown
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_get_module_package_version2(WASMRuntime *runtime,
                                         const wasm_module_t module);

/**
 * Get the currently supported version of the package type
 *
 * @param runtime the runtime context
 * @param package_type the package type
 *
 * @return the currently supported version, or zero if package type is unknown
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_get_current_package_version2(WASMRuntime *runtime,
                                          package_type_t package_type);

/**
 * Check whether a file is an AOT XIP (Execution In Place) file
 *
 * @param runtime the runtime context
 * @param buf the package buffer
 * @param size the package buffer size
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_is_xip_file2(WASMRuntime *runtime, const uint8_t *buf,
                          uint32_t size);

/**
 * Setup callbacks for reading and releasing a buffer about a module file
 *
 * @param runtime the runtime context
 * @param reader a callback to read a module file into a buffer
 * @param destroyer a callback to release above buffer
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_module_reader2(WASMRuntime *runtime,
                                const module_reader reader,
                                const module_destroyer destroyer);

/**
 * Give the "module" a name "module_name".
 * Can not assign a new name to a module if it already has a name
 *
 * @param runtime the runtime context
 * @param module_name indicate a name
 * @param module the target module
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return true means success, false means failed
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_register_module2(WASMRuntime *runtime, const char *module_name,
                              wasm_module_t module, char *error_buf,
                              uint32_t error_buf_size);

/**
 * Check if there is already a loaded module named module_name in the
 * runtime. Repeatedly loading a module with the same name is not allowed.
 *
 * @param runtime the runtime context
 * @param module_name indicate a name
 *
 * @return return WASM module loaded, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_t
wasm_runtime_find_module_registered2(WASMRuntime *runtime,
                                     const char *module_name);

/**
 * Load a WASM module from a specified byte buffer. The byte buffer can be
 * WASM binary data when interpreter or JIT is enabled, or AOT binary data
 * when AOT is enabled. If it is AOT binary data, it must be 4-byte aligned.
 *
 * @param runtime the runtime context
 * @param buf the byte buffer which contains the WASM/AOT binary data
 * @param size the size of the buffer
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_t
wasm_runtime_load2(WASMRuntime *runtime, uint8_t *buf, uint32_t size,
                   char *error_buf, uint32_t error_buf_size);

/**
 * Load a WASM module with specified load argument.
 *
 * @param runtime the runtime context
 * @param buf the byte buffer which contains the WASM/AOT binary data
 * @param size the size of the buffer
 * @param args the loading arguments
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_t
wasm_runtime_load_ex2(WASMRuntime *runtime, uint8_t *buf, uint32_t size,
                      const LoadArgs *args, char *error_buf,
                      uint32_t error_buf_size);

/**
 * Resolve symbols for a previously loaded WASM module. Only useful when the
 * module was loaded with LoadArgs::no_resolve set to true
 *
 * @param runtime the runtime context
 * @param module the WASM module
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_resolve_symbols2(WASMRuntime *runtime, wasm_module_t module);

/**
 * Load a WASM module from a specified WASM or AOT section list.
 *
 * @param runtime the runtime context
 * @param section_list the section list which contains each section data
 * @param is_aot whether the section list is AOT section list
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_t
wasm_runtime_load_from_sections2(WASMRuntime *runtime,
                                 wasm_section_list_t section_list, bool is_aot,
                                 char *error_buf, uint32_t error_buf_size);

/**
 * Unload a WASM module.
 *
 * @param runtime the runtime context
 * @param module the module to be unloaded
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_unload2(WASMRuntime *runtime, wasm_module_t module);

/**
 * Get the module hash of a WASM module, currently only available on
 * linux-sgx platform when the remote attestation feature is enabled
 *
 * @param runtime the runtime context
 * @param module the WASM module to retrieve
 *
 * @return the module hash of the WASM module
 */
WASM_RUNTIME_API_EXTERN char *
wasm_runtime_get_module_hash2(WASMRuntime *runtime, wasm_module_t module);

/**
 * Set WASI parameters.
 *
 * While this API operates on a module, these parameters will be used
 * only when the module is instantiated. That is, you can consider these
 * as extra parameters for wasm_runtime_instantiate().
 *
 * @param runtime the runtime context
 * @param module        The module to set WASI parameters.
 * @param dir_list      The list of directories to preopen. (real path)
 * @param dir_count     The number of elements in dir_list.
 * @param map_dir_list  The list of directories to preopen. (mapped path)
 *                      Format for each map entry: <guest-path>::<host-path>
 * @param map_dir_count The number of elements in map_dir_list.
 * @param env           The list of environment variables.
 * @param env_count     The number of elements in env.
 * @param argv          The list of command line arguments.
 * @param argc          The number of elements in argv.
 * @param stdin_handle  The raw host handle to back WASI STDIN_FILENO.
 * @param stdoutfd      The raw host handle to back WASI STDOUT_FILENO.
 * @param stderrfd      The raw host handle to back WASI STDERR_FILENO.
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_wasi_args_ex2(WASMRuntime *runtime, wasm_module_t module,
                               const char *dir_list[], uint32_t dir_count,
                               const char *map_dir_list[],
                               uint32_t map_dir_count, const char *env[],
                               uint32_t env_count, char *argv[], int argc,
                               int64_t stdinfd, int64_t stdoutfd,
                               int64_t stderrfd);

/**
 * Set WASI parameters (with default stdio).
 *
 * Same as wasm_runtime_set_wasi_args_ex2 but with default stdio handles
 *
 * @param runtime the runtime context
 * @param module        The module to set WASI parameters.
 * @param dir_list      The list of directories to preopen. (real path)
 * @param dir_count     The number of elements in dir_list.
 * @param map_dir_list  The list of directories to preopen. (mapped path)
 *                      Format for each map entry: <guest-path>::<host-path>
 * @param map_dir_count The number of elements in map_dir_list.
 * @param env           The list of environment variables.
 * @param env_count     The number of elements in env.
 * @param argv          The list of command line arguments.
 * @param argc          The number of elements in argv.
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_wasi_args2(WASMRuntime *runtime, wasm_module_t module,
                            const char *dir_list[], uint32_t dir_count,
                            const char *map_dir_list[], uint32_t map_dir_count,
                            const char *env[], uint32_t env_count, char *argv[],
                            int argc);

/**
 * Set WASI address pool.
 *
 * @param runtime the runtime context
 * @param module the module to set WASI address pool
 * @param addr_pool the address pool
 * @param addr_pool_size the size of address pool
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_wasi_addr_pool2(WASMRuntime *runtime, wasm_module_t module,
                                 const char *addr_pool[],
                                 uint32_t addr_pool_size);

/**
 * Set WASI NS lookup pool.
 *
 * @param runtime the runtime context
 * @param module the module to set WASI NS lookup pool
 * @param ns_lookup_pool the NS lookup pool
 * @param ns_lookup_pool_size the size of NS lookup pool
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_wasi_ns_lookup_pool2(WASMRuntime *runtime,
                                      wasm_module_t module,
                                      const char *ns_lookup_pool[],
                                      uint32_t ns_lookup_pool_size);

/**
 * Instantiate a WASM module.
 *
 * @param runtime the runtime context
 * @param module the WASM module to instantiate
 * @param default_stack_size the default stack size of the module instance
 * @param host_managed_heap_size the default heap size of the module instance
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return return the instantiated WASM module instance, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_inst_t
wasm_runtime_instantiate2(WASMRuntime *runtime, const wasm_module_t module,
                          uint32_t default_stack_size,
                          uint32_t host_managed_heap_size, char *error_buf,
                          uint32_t error_buf_size);

/**
 * Instantiate a WASM module, with specified instantiation arguments
 *
 * @param runtime the runtime context
 * @param module the WASM module to instantiate
 * @param args the instantiation arguments
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return return the instantiated WASM module instance, NULL if failed
 */
WASM_RUNTIME_API_EXTERN wasm_module_inst_t
wasm_runtime_instantiate_ex2(WASMRuntime *runtime, const wasm_module_t module,
                             const InstantiationArgs *args, char *error_buf,
                             uint32_t error_buf_size);

/**
 * Set the running mode of a WASM module instance, override the
 * default running mode of the runtime. Note that it only makes sense when
 * the input is a wasm bytecode file: for the AOT file, runtime always runs
 * it with AOT engine, and this function always returns true.
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance to set running mode
 * @param running_mode the running mode to set
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_set_running_mode2(WASMRuntime *runtime,
                               wasm_module_inst_t module_inst,
                               RunningMode running_mode);

/**
 * Get the running mode of a WASM module instance, if no running mode
 * is explicitly set the default running mode of runtime will
 * be used and returned. Note that it only makes sense when the input is a
 * wasm bytecode file: for the AOT file, this function always returns 0.
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance to query for running mode
 *
 * @return the running mode this module instance currently use
 */
WASM_RUNTIME_API_EXTERN RunningMode
wasm_runtime_get_running_mode2(WASMRuntime *runtime,
                               wasm_module_inst_t module_inst);

/**
 * Deinstantiate a WASM module instance, destroy the resources.
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance to destroy
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_deinstantiate2(WASMRuntime *runtime,
                            wasm_module_inst_t module_inst);

/**
 * Get WASM module from WASM module instance
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance to retrieve
 *
 * @return the WASM module
 */
WASM_RUNTIME_API_EXTERN wasm_module_t
wasm_runtime_get_module2(WASMRuntime *runtime, wasm_module_inst_t module_inst);

/**
 * Check if the module instance is in WASI mode
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance
 *
 * @return true if in WASI mode, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_is_wasi_mode2(WASMRuntime *runtime,
                           wasm_module_inst_t module_inst);

/**
 * Look up WASI start function
 *
 * @param runtime the runtime context
 * @param module_inst the WASM module instance
 *
 * @return the WASI start function if found, NULL otherwise
 */
WASM_RUNTIME_API_EXTERN wasm_function_inst_t
wasm_runtime_lookup_wasi_start_function2(WASMRuntime *runtime,
                                         wasm_module_inst_t module_inst);

/**
 * Get WASI exit code.
 *
 * @param runtime the runtime context
 * @param module_inst the module instance
 *
 * @return the WASI exit code
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_get_wasi_exit_code2(WASMRuntime *runtime,
                                 wasm_module_inst_t module_inst);

/**
 * Lookup an exported function in the WASM module instance.
 *
 * @param runtime the runtime context
 * @param module_inst the module instance
 * @param name the name of the function
 *
 * @return the function instance found, NULL if not found
 */
WASM_RUNTIME_API_EXTERN wasm_function_inst_t
wasm_runtime_lookup_function2(WASMRuntime *runtime,
                              const wasm_module_inst_t module_inst,
                              const char *name);

/**
 * Get parameter count of the function instance
 *
 * @param runtime the runtime context
 * @param func_inst the function instance
 * @param module_inst the module instance the function instance belongs to
 *
 * @return the parameter count of the function instance
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_func_get_param_count2(WASMRuntime *runtime,
                           const wasm_function_inst_t func_inst,
                           const wasm_module_inst_t module_inst);

/**
 * Get result count of the function instance
 *
 * @param runtime the runtime context
 * @param func_inst the function instance
 * @param module_inst the module instance the function instance belongs to
 *
 * @return the result count of the function instance
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_func_get_result_count2(WASMRuntime *runtime,
                            const wasm_function_inst_t func_inst,
                            const wasm_module_inst_t module_inst);

/**
 * Get parameter types of the function instance
 *
 * @param runtime the runtime context
 * @param func_inst the function instance
 * @param module_inst the module instance the function instance belongs to
 * @param param_types the parameter types returned
 */
WASM_RUNTIME_API_EXTERN void
wasm_func_get_param_types2(WASMRuntime *runtime,
                           const wasm_function_inst_t func_inst,
                           const wasm_module_inst_t module_inst,
                           wasm_valkind_t *param_types);

/**
 * Get result types of the function instance
 *
 * @param runtime the runtime context
 * @param func_inst the function instance
 * @param module_inst the module instance the function instance belongs to
 * @param result_types the result types returned
 */
WASM_RUNTIME_API_EXTERN void
wasm_func_get_result_types2(WASMRuntime *runtime,
                            const wasm_function_inst_t func_inst,
                            const wasm_module_inst_t module_inst,
                            wasm_valkind_t *result_types);

/**
 * Create execution environment for a WASM module instance.
 *
 * @param runtime the runtime context
 * @param module_inst the module instance
 * @param stack_size the stack size to execute a WASM function
 *
 * @return the execution environment, NULL if failed, e.g. invalid stack size
 */
WASM_RUNTIME_API_EXTERN wasm_exec_env_t
wasm_runtime_create_exec_env2(WASMRuntime *runtime,
                              wasm_module_inst_t module_inst,
                              uint32_t stack_size);

/**
 * Destroy the execution environment.
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment to destroy
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_destroy_exec_env2(WASMRuntime *runtime, wasm_exec_env_t exec_env);

/**
 * Copy callstack frames.
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment that containes frames
 * @param buffer the buffer to copy frames to
 * @param length the number of frames to copy
 * @param skip_n the number of frames to skip from the top of the stack
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return number of copied frames
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_copy_callstack2(WASMRuntime *runtime, const wasm_exec_env_t exec_env,
                     wasm_frame_t *buffer, const uint32_t length,
                     const uint32_t skip_n, char *error_buf,
                     uint32_t error_buf_size);

/**
 * Get the singleton execution environment for the instance.
 *
 * @param runtime the runtime context
 * @param module_inst the module instance
 *
 * @return exec_env the execution environment to destroy
 */
WASM_RUNTIME_API_EXTERN wasm_exec_env_t
wasm_runtime_get_exec_env_singleton2(WASMRuntime *runtime,
                                     wasm_module_inst_t module_inst);

/**
 * Start debug instance based on given execution environment.
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment to start debug instance
 * @param port the port for the debug server to listen on
 *
 * @return debug port if success, 0 otherwise
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_start_debug_instance_with_port2(WASMRuntime *runtime,
                                             wasm_exec_env_t exec_env,
                                             int32_t port);

/**
 * Start debug instance with default port.
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment to start debug instance
 *
 * @return debug port if success, 0 otherwise
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_runtime_start_debug_instance2(WASMRuntime *runtime,
                                   wasm_exec_env_t exec_env);

/**
 * Initialize the thread environment.
 *
 * @param runtime the runtime context
 *
 * @return true if success, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_init_thread_env2(WASMRuntime *runtime);

/**
 * Destroy the thread environment
 *
 * @param runtime the runtime context
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_destroy_thread_env2(WASMRuntime *runtime);

/**
 * Whether the thread environment is initialized
 *
 * @param runtime the runtime context
 *
 * @return true if initialized, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_thread_env_inited2(WASMRuntime *runtime);

/**
 * Get WASM module instance from execution environment
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment to retrieve
 *
 * @return the WASM module instance
 */
WASM_RUNTIME_API_EXTERN wasm_module_inst_t
wasm_runtime_get_module_inst2(WASMRuntime *runtime, wasm_exec_env_t exec_env);

/**
 * Set WASM module instance of execution environment
 *
 * @param runtime the runtime context
 * @param exec_env the execution environment
 * @param module_inst the WASM module instance to set
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_set_module_inst2(WASMRuntime *runtime, wasm_exec_env_t exec_env,
                              const wasm_module_inst_t module_inst);

/**
 * Lookup a memory instance by name
 *
 * @param runtime the runtime context
 * @param module_inst The module instance
 * @param name The name of the memory instance
 *
 * @return The memory instance if found, NULL otherwise
 */
WASM_RUNTIME_API_EXTERN wasm_memory_inst_t
wasm_runtime_lookup_memory2(WASMRuntime *runtime,
                            const wasm_module_inst_t module_inst,
                            const char *name);

/**
 * Get the default memory instance
 *
 * @param runtime the runtime context
 * @param module_inst The module instance
 *
 * @return The memory instance if found, NULL otherwise
 */
WASM_RUNTIME_API_EXTERN wasm_memory_inst_t
wasm_runtime_get_default_memory2(WASMRuntime *runtime,
                                 const wasm_module_inst_t module_inst);

/**
 * Get a memory instance by index
 *
 * @param runtime the runtime context
 * @param module_inst The module instance
 * @param index The index of the memory instance
 *
 * @return The memory instance if found, NULL otherwise
 */
WASM_RUNTIME_API_EXTERN wasm_memory_inst_t
wasm_runtime_get_memory2(WASMRuntime *runtime,
                         const wasm_module_inst_t module_inst, uint32_t index);

/**
 * Get the current number of pages for a memory instance
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 *
 * @return The current number of pages
 */
WASM_RUNTIME_API_EXTERN uint64_t
wasm_memory_get_cur_page_count2(WASMRuntime *runtime,
                                const wasm_memory_inst_t memory_inst);

/**
 * Get the maximum number of pages for a memory instance
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 *
 * @return The maximum number of pages
 */
WASM_RUNTIME_API_EXTERN uint64_t
wasm_memory_get_max_page_count2(WASMRuntime *runtime,
                                const wasm_memory_inst_t memory_inst);

/**
 * Get the number of bytes per page for a memory instance
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 *
 * @return The number of bytes per page
 */
WASM_RUNTIME_API_EXTERN uint64_t
wasm_memory_get_bytes_per_page2(WASMRuntime *runtime,
                                const wasm_memory_inst_t memory_inst);

/**
 * Get the shared status for a memory instance
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 *
 * @return True if shared, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_memory_get_shared2(WASMRuntime *runtime,
                        const wasm_memory_inst_t memory_inst);

/**
 * Get the base address for a memory instance
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 *
 * @return The base address on success, NULL otherwise
 */
WASM_RUNTIME_API_EXTERN void *
wasm_memory_get_base_address2(WASMRuntime *runtime,
                              const wasm_memory_inst_t memory_inst);

/**
 * Enlarge a memory instance by a number of pages
 *
 * @param runtime the runtime context
 * @param memory_inst The memory instance
 * @param inc_page_count The number of pages to add
 *
 * @return True if successful, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_memory_enlarge2(WASMRuntime *runtime, wasm_memory_inst_t memory_inst,
                     uint64_t inc_page_count);

#ifdef __cplusplus
}
#endif

#endif /* _WASM_EXPORT_2_H */
