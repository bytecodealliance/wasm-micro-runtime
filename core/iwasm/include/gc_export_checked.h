
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!
 */
#ifndef GC_EXPORT_CHECKED_H
#define GC_EXPORT_CHECKED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "gc_export.h"

typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        wasm_array_obj_t wasm_array_obj_t_value;
        wasm_struct_obj_t wasm_struct_obj_t_value;
        wasm_exec_env_t wasm_exec_env_t_value;
        wasm_func_type_t wasm_func_type_t_value;
        wasm_memory_inst_t wasm_memory_inst_t_value;
        wasm_module_t wasm_module_t_value;
        wasm_defined_type_t wasm_defined_type_t_value;
        wasm_function_inst_t wasm_function_inst_t_value;
        wasm_externref_obj_t wasm_externref_obj_t_value;
        package_type_t package_type_t_value;
        wasm_i31_obj_t wasm_i31_obj_t_value;
        wasm_ref_type_t wasm_ref_type_t_value;
        wasm_module_inst_t wasm_module_inst_t_value;
        _Bool _Bool_value;
        double double_value;
        wasm_func_obj_t wasm_func_obj_t_value;
        wasm_obj_t wasm_obj_t_value;
        uint64_t uint64_t_value;
        wasm_shared_heap_t wasm_shared_heap_t_value;
        RunningMode RunningMode_value;
        int32_t int32_t_value;
        wasm_valkind_t wasm_valkind_t_value;
        uint32_t uint32_t_value;
        wasm_anyref_obj_t wasm_anyref_obj_t_value;
        // Add other types as needed
    } value;
} Result;

static inline Result
get_base_lib_export_apis_checked(void *p_base_lib_apis)
{
    Result res;
    // Check for null pointer parameter: p_base_lib_apis
    if (p_base_lib_apis == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result = get_base_lib_export_apis(p_base_lib_apis);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_init_checked(void)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_init();
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_full_init_checked(void *init_args)
{
    Result res;
    // Check for null pointer parameter: init_args
    if (init_args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_full_init(init_args);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_set_log_level_checked(log_level_t level)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_log_level(level);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_is_running_mode_supported_checked(RunningMode running_mode)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_is_running_mode_supported(running_mode);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_set_default_running_mode_checked(RunningMode running_mode)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_set_default_running_mode(running_mode);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_destroy_checked(void)
{
    Result res;
    // Execute the original function
    wasm_runtime_destroy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_malloc_checked(unsigned int size)
{
    Result res;
    // Execute the original function
    wasm_runtime_malloc(size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_realloc_checked(void *ptr, unsigned int size)
{
    Result res;
    // Check for null pointer parameter: ptr
    if (ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_realloc(ptr, size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_free_checked(void *ptr)
{
    Result res;
    // Check for null pointer parameter: ptr
    if (ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_free(ptr);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_mem_alloc_info_checked(void *mem_alloc_info)
{
    Result res;
    // Check for null pointer parameter: mem_alloc_info
    if (mem_alloc_info == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_get_mem_alloc_info(mem_alloc_info);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
get_package_type_checked(void *buf, uint32_t size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    package_type_t original_result = get_package_type(buf, size);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.package_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_file_package_type_checked(void *buf, uint32_t size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    package_type_t original_result =
        wasm_runtime_get_file_package_type(buf, size);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.package_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_module_package_type_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    package_type_t original_result =
        wasm_runtime_get_module_package_type(module);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.package_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_file_package_version_checked(void *buf, uint32_t size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result = wasm_runtime_get_file_package_version(buf, size);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_module_package_version_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_runtime_get_module_package_version(module);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_current_package_version_checked(package_type_t package_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        wasm_runtime_get_current_package_version(package_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_is_xip_file_checked(void *buf, uint32_t size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_is_xip_file(buf, size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_set_module_reader_checked(module_reader reader,
                                       module_destroyer destroyer)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_module_reader(reader, destroyer);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_register_module_checked(void *module_name, wasm_module_t module,
                                     void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_register_module(
        module_name, module, error_buf, error_buf_size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_find_module_registered_checked(void *module_name)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_t original_result =
        wasm_runtime_find_module_registered(module_name);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_load_checked(void *buf, uint32_t size, void *error_buf,
                          uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_t original_result =
        wasm_runtime_load(buf, size, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_load_ex_checked(void *buf, uint32_t size, void *args,
                             void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_t original_result =
        wasm_runtime_load_ex(buf, size, args, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_resolve_symbols_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_resolve_symbols(module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_load_from_sections_checked(wasm_section_list_t section_list,
                                        _Bool is_aot, void *error_buf,
                                        uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_t original_result = wasm_runtime_load_from_sections(
        section_list, is_aot, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_unload_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    wasm_runtime_unload(module);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_module_hash_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_module_hash(module);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_wasi_args_ex_checked(wasm_module_t module, void *dir_list,
                                      uint32_t dir_count, void *map_dir_list,
                                      uint32_t map_dir_count, void *env,
                                      uint32_t env_count, void *argv, int argc,
                                      int64_t stdinfd, int64_t stdoutfd,
                                      int64_t stderrfd)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_wasi_args_ex(module, dir_list, dir_count, map_dir_list,
                                  map_dir_count, env, env_count, argv, argc,
                                  stdinfd, stdoutfd, stderrfd);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_wasi_args_checked(wasm_module_t module, void *dir_list,
                                   uint32_t dir_count, void *map_dir_list,
                                   uint32_t map_dir_count, void *env,
                                   uint32_t env_count, void *argv, int argc)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_wasi_args(module, dir_list, dir_count, map_dir_list,
                               map_dir_count, env, env_count, argv, argc);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_wasi_addr_pool_checked(wasm_module_t module, void *addr_pool,
                                        uint32_t addr_pool_size)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_wasi_addr_pool(module, addr_pool, addr_pool_size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_wasi_ns_lookup_pool_checked(wasm_module_t module,
                                             void *ns_lookup_pool,
                                             uint32_t ns_lookup_pool_size)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_wasi_ns_lookup_pool(module, ns_lookup_pool,
                                         ns_lookup_pool_size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_instantiate_checked(wasm_module_t module,
                                 uint32_t default_stack_size,
                                 uint32_t host_managed_heap_size,
                                 void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_inst_t original_result = wasm_runtime_instantiate(
        module, default_stack_size, host_managed_heap_size, error_buf,
        error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_instantiate_ex_checked(wasm_module_t module, void *args,
                                    void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_inst_t original_result =
        wasm_runtime_instantiate_ex(module, args, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_instantiation_args_create_checked(void *p)
{
    Result res;
    // Check for null pointer parameter: p
    if (p == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_instantiation_args_create(p);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_instantiation_args_destroy_checked(void *p)
{
    Result res;
    // Check for null pointer parameter: p
    if (p == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_instantiation_args_destroy(p);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_instantiation_args_set_default_stack_size_checked(void *p,
                                                               uint32_t v)
{
    Result res;
    // Check for null pointer parameter: p
    if (p == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_instantiation_args_set_default_stack_size(p, v);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_instantiation_args_set_host_managed_heap_size_checked(void *p,
                                                                   uint32_t v)
{
    Result res;
    // Check for null pointer parameter: p
    if (p == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_instantiation_args_set_host_managed_heap_size(p, v);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_instantiation_args_set_max_memory_pages_checked(void *p,
                                                             uint32_t v)
{
    Result res;
    // Check for null pointer parameter: p
    if (p == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_instantiation_args_set_max_memory_pages(p, v);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_instantiate_ex2_checked(wasm_module_t module, void *args,
                                     void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_inst_t original_result =
        wasm_runtime_instantiate_ex2(module, args, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_set_running_mode_checked(wasm_module_inst_t module_inst,
                                      RunningMode running_mode)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_set_running_mode(module_inst, running_mode);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_running_mode_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    RunningMode original_result = wasm_runtime_get_running_mode(module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.RunningMode_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_deinstantiate_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_deinstantiate(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_module_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_module_t original_result = wasm_runtime_get_module(module_inst);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_is_wasi_mode_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_is_wasi_mode(module_inst);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_lookup_wasi_start_function_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_function_inst_t original_result =
        wasm_runtime_lookup_wasi_start_function(module_inst);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_function_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_wasi_exit_code_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_runtime_get_wasi_exit_code(module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_lookup_function_checked(wasm_module_inst_t module_inst, void *name)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_function_inst_t original_result =
        wasm_runtime_lookup_function(module_inst, name);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_function_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_get_param_count_checked(wasm_function_inst_t func_inst,
                                  wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        wasm_func_get_param_count(func_inst, module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_get_result_count_checked(wasm_function_inst_t func_inst,
                                   wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        wasm_func_get_result_count(func_inst, module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_get_param_types_checked(wasm_function_inst_t func_inst,
                                  wasm_module_inst_t module_inst,
                                  void *param_types)
{
    Result res;
    // Check for null pointer parameter: param_types
    if (param_types == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_get_param_types(func_inst, module_inst, param_types);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_get_result_types_checked(wasm_function_inst_t func_inst,
                                   wasm_module_inst_t module_inst,
                                   void *result_types)
{
    Result res;
    // Check for null pointer parameter: result_types
    if (result_types == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_get_result_types(func_inst, module_inst, result_types);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_create_exec_env_checked(wasm_module_inst_t module_inst,
                                     uint32_t stack_size)
{
    Result res;
    // Execute the original function
    wasm_exec_env_t original_result =
        wasm_runtime_create_exec_env(module_inst, stack_size);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_exec_env_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_destroy_exec_env_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_destroy_exec_env(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_copy_callstack_checked(wasm_exec_env_t exec_env, void *buffer,
                            uint32_t length, uint32_t skip_n, void *error_buf,
                            uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: buffer
    if (buffer == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result = wasm_copy_callstack(
        exec_env, buffer, length, skip_n, error_buf, error_buf_size);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_exec_env_singleton_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_exec_env_t original_result =
        wasm_runtime_get_exec_env_singleton(module_inst);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_exec_env_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_start_debug_instance_with_port_checked(wasm_exec_env_t exec_env,
                                                    int32_t port)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        wasm_runtime_start_debug_instance_with_port(exec_env, port);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_start_debug_instance_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_runtime_start_debug_instance(exec_env);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_init_thread_env_checked(void)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_init_thread_env();
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_destroy_thread_env_checked(void)
{
    Result res;
    // Execute the original function
    wasm_runtime_destroy_thread_env();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_thread_env_inited_checked(void)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_thread_env_inited();
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_module_inst_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_module_inst_t original_result = wasm_runtime_get_module_inst(exec_env);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_module_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_set_module_inst_checked(wasm_exec_env_t exec_env,
                                     wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_module_inst(exec_env, module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_lookup_memory_checked(wasm_module_inst_t module_inst, void *name)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_inst_t original_result =
        wasm_runtime_lookup_memory(module_inst, name);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_memory_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_default_memory_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_memory_inst_t original_result =
        wasm_runtime_get_default_memory(module_inst);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_memory_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_memory_checked(wasm_module_inst_t module_inst, uint32_t index)
{
    Result res;
    // Execute the original function
    wasm_memory_inst_t original_result =
        wasm_runtime_get_memory(module_inst, index);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_memory_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_get_cur_page_count_checked(wasm_memory_inst_t memory_inst)
{
    Result res;
    // Execute the original function
    uint64_t original_result = wasm_memory_get_cur_page_count(memory_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_get_max_page_count_checked(wasm_memory_inst_t memory_inst)
{
    Result res;
    // Execute the original function
    uint64_t original_result = wasm_memory_get_max_page_count(memory_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_get_bytes_per_page_checked(wasm_memory_inst_t memory_inst)
{
    Result res;
    // Execute the original function
    uint64_t original_result = wasm_memory_get_bytes_per_page(memory_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_get_shared_checked(wasm_memory_inst_t memory_inst)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_memory_get_shared(memory_inst);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_memory_get_base_address_checked(wasm_memory_inst_t memory_inst)
{
    Result res;
    // Execute the original function
    wasm_memory_get_base_address(memory_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_enlarge_checked(wasm_memory_inst_t memory_inst,
                            uint64_t inc_page_count)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_memory_enlarge(memory_inst, inc_page_count);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_wasm_checked(wasm_exec_env_t exec_env,
                               wasm_function_inst_t function, uint32_t argc,
                               void *argv)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_call_wasm(exec_env, function, argc, argv);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_wasm_a_checked(wasm_exec_env_t exec_env,
                                 wasm_function_inst_t function,
                                 uint32_t num_results, void *results,
                                 uint32_t num_args, void *args)
{
    Result res;
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_call_wasm_a(
        exec_env, function, num_results, results, num_args, args);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_wasm_v_checked(wasm_exec_env_t exec_env,
                                 wasm_function_inst_t function,
                                 uint32_t num_results, void *results,
                                 uint32_t num_args, ...)
{
    Result res;
    va_list args;
    // Execute the original function
    va_start(args, num_args);
    _Bool original_result = wasm_runtime_call_wasm_v(
        exec_env, function, num_results, results, num_args, args);
    va_end(args);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_indirect_checked(wasm_exec_env_t exec_env,
                                   uint32_t element_index, uint32_t argc,
                                   void *argv)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_call_indirect(exec_env, element_index, argc, argv);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_application_execute_main_checked(wasm_module_inst_t module_inst,
                                      int32_t argc, void *argv)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_application_execute_main(module_inst, argc, argv);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_application_execute_func_checked(wasm_module_inst_t module_inst,
                                      void *name, int32_t argc, void *argv)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_application_execute_func(module_inst, name, argc, argv);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_exception_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_exception(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_exception_checked(wasm_module_inst_t module_inst,
                                   void *exception)
{
    Result res;
    // Check for null pointer parameter: exception
    if (exception == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_exception(module_inst, exception);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_clear_exception_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_clear_exception(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_terminate_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_terminate(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_custom_data_checked(wasm_module_inst_t module_inst,
                                     void *custom_data)
{
    Result res;
    // Check for null pointer parameter: custom_data
    if (custom_data == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_custom_data(module_inst, custom_data);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_custom_data_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_custom_data(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_bounds_checks_checked(wasm_module_inst_t module_inst,
                                       _Bool enable)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_bounds_checks(module_inst, enable);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_is_bounds_checks_enabled_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_is_bounds_checks_enabled(module_inst);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_module_malloc_checked(wasm_module_inst_t module_inst,
                                   uint64_t size, void *p_native_addr)
{
    Result res;
    // Check for null pointer parameter: p_native_addr
    if (p_native_addr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint64_t original_result =
        wasm_runtime_module_malloc(module_inst, size, p_native_addr);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_module_free_checked(wasm_module_inst_t module_inst, uint64_t ptr)
{
    Result res;
    // Execute the original function
    wasm_runtime_module_free(module_inst, ptr);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_module_dup_data_checked(wasm_module_inst_t module_inst, void *src,
                                     uint64_t size)
{
    Result res;
    // Check for null pointer parameter: src
    if (src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint64_t original_result =
        wasm_runtime_module_dup_data(module_inst, src, size);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_validate_app_addr_checked(wasm_module_inst_t module_inst,
                                       uint64_t app_offset, uint64_t size)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_validate_app_addr(module_inst, app_offset, size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_validate_app_str_addr_checked(wasm_module_inst_t module_inst,
                                           uint64_t app_str_offset)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_validate_app_str_addr(module_inst, app_str_offset);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_validate_native_addr_checked(wasm_module_inst_t module_inst,
                                          void *native_ptr, uint64_t size)
{
    Result res;
    // Check for null pointer parameter: native_ptr
    if (native_ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_validate_native_addr(module_inst, native_ptr, size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_addr_app_to_native_checked(wasm_module_inst_t module_inst,
                                        uint64_t app_offset)
{
    Result res;
    // Execute the original function
    wasm_runtime_addr_app_to_native(module_inst, app_offset);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_addr_native_to_app_checked(wasm_module_inst_t module_inst,
                                        void *native_ptr)
{
    Result res;
    // Check for null pointer parameter: native_ptr
    if (native_ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint64_t original_result =
        wasm_runtime_addr_native_to_app(module_inst, native_ptr);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_app_addr_range_checked(wasm_module_inst_t module_inst,
                                        uint64_t app_offset,
                                        void *p_app_start_offset,
                                        void *p_app_end_offset)
{
    Result res;
    // Check for null pointer parameter: p_app_start_offset
    if (p_app_start_offset == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: p_app_end_offset
    if (p_app_end_offset == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_get_app_addr_range(
        module_inst, app_offset, p_app_start_offset, p_app_end_offset);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_native_addr_range_checked(wasm_module_inst_t module_inst,
                                           void *native_ptr,
                                           void *p_native_start_addr,
                                           void *p_native_end_addr)
{
    Result res;
    // Check for null pointer parameter: native_ptr
    if (native_ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: p_native_start_addr
    if (p_native_start_addr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: p_native_end_addr
    if (p_native_end_addr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_get_native_addr_range(
        module_inst, native_ptr, p_native_start_addr, p_native_end_addr);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_import_count_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    int32_t original_result = wasm_runtime_get_import_count(module);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_import_type_checked(wasm_module_t module, int32_t import_index,
                                     void *import_type)
{
    Result res;
    // Check for null pointer parameter: import_type
    if (import_type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_get_import_type(module, import_index, import_type);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_export_count_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    int32_t original_result = wasm_runtime_get_export_count(module);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_export_type_checked(wasm_module_t module, int32_t export_index,
                                     void *export_type)
{
    Result res;
    // Check for null pointer parameter: export_type
    if (export_type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_get_export_type(module, export_index, export_type);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_type_get_param_count_checked(wasm_func_type_t func_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_func_type_get_param_count(func_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_type_get_param_valkind_checked(wasm_func_type_t func_type,
                                         uint32_t param_index)
{
    Result res;
    // Execute the original function
    wasm_valkind_t original_result =
        wasm_func_type_get_param_valkind(func_type, param_index);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_valkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_type_get_result_count_checked(wasm_func_type_t func_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_func_type_get_result_count(func_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_type_get_result_valkind_checked(wasm_func_type_t func_type,
                                          uint32_t result_index)
{
    Result res;
    // Execute the original function
    wasm_valkind_t original_result =
        wasm_func_type_get_result_valkind(func_type, result_index);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_valkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_global_type_get_valkind_checked(wasm_global_type_t global_type)
{
    Result res;
    // Execute the original function
    wasm_valkind_t original_result = wasm_global_type_get_valkind(global_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_valkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_global_type_get_mutable_checked(wasm_global_type_t global_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_global_type_get_mutable(global_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_memory_type_get_shared_checked(wasm_memory_type_t memory_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_memory_type_get_shared(memory_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_memory_type_get_init_page_count_checked(wasm_memory_type_t memory_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        wasm_memory_type_get_init_page_count(memory_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_type_get_max_page_count_checked(wasm_memory_type_t memory_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_memory_type_get_max_page_count(memory_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_table_type_get_elem_kind_checked(wasm_table_type_t table_type)
{
    Result res;
    // Execute the original function
    wasm_valkind_t original_result = wasm_table_type_get_elem_kind(table_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_valkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_table_type_get_shared_checked(wasm_table_type_t table_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_table_type_get_shared(table_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_table_type_get_init_size_checked(wasm_table_type_t table_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_table_type_get_init_size(table_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_table_type_get_max_size_checked(wasm_table_type_t table_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_table_type_get_max_size(table_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_register_natives_checked(void *module_name, void *native_symbols,
                                      uint32_t n_native_symbols)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: native_symbols
    if (native_symbols == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_register_natives(
        module_name, native_symbols, n_native_symbols);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_register_natives_raw_checked(void *module_name,
                                          void *native_symbols,
                                          uint32_t n_native_symbols)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: native_symbols
    if (native_symbols == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_register_natives_raw(
        module_name, native_symbols, n_native_symbols);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_unregister_natives_checked(void *module_name, void *native_symbols)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: native_symbols
    if (native_symbols == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_unregister_natives(module_name, native_symbols);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_export_global_inst_checked(wasm_module_inst_t module_inst,
                                            void *name, void *global_inst)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: global_inst
    if (global_inst == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_get_export_global_inst(module_inst, name, global_inst);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_export_table_inst_checked(wasm_module_inst_t module_inst,
                                           void *name, void *table_inst)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: table_inst
    if (table_inst == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_get_export_table_inst(module_inst, name, table_inst);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_table_get_func_inst_checked(wasm_module_inst_t module_inst,
                                 void *table_inst, uint32_t idx)
{
    Result res;
    // Check for null pointer parameter: table_inst
    if (table_inst == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_function_inst_t original_result =
        wasm_table_get_func_inst(module_inst, table_inst, idx);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_function_inst_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_function_attachment_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_function_attachment(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_user_data_checked(wasm_exec_env_t exec_env, void *user_data)
{
    Result res;
    // Check for null pointer parameter: user_data
    if (user_data == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_user_data(exec_env, user_data);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_user_data_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_user_data(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_native_stack_boundary_checked(wasm_exec_env_t exec_env,
                                               void *native_stack_boundary)
{
    Result res;
    // Check for null pointer parameter: native_stack_boundary
    if (native_stack_boundary == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_native_stack_boundary(exec_env, native_stack_boundary);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_instruction_count_limit_checked(wasm_exec_env_t exec_env,
                                                 int instruction_count)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_instruction_count_limit(exec_env, instruction_count);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_dump_mem_consumption_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_dump_mem_consumption(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_dump_perf_profiling_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_dump_perf_profiling(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_sum_wasm_exec_time_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    double original_result = wasm_runtime_sum_wasm_exec_time(module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.double_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_wasm_func_exec_time_checked(wasm_module_inst_t inst,
                                             void *func_name)
{
    Result res;
    // Check for null pointer parameter: func_name
    if (func_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    double original_result =
        wasm_runtime_get_wasm_func_exec_time(inst, func_name);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.double_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_set_max_thread_num_checked(uint32_t num)
{
    Result res;
    // Execute the original function
    wasm_runtime_set_max_thread_num(num);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_spawn_exec_env_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_exec_env_t original_result = wasm_runtime_spawn_exec_env(exec_env);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_exec_env_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_destroy_spawned_exec_env_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_destroy_spawned_exec_env(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_spawn_thread_checked(wasm_exec_env_t exec_env, void *tid,
                                  wasm_thread_callback_t callback, void *arg)
{
    Result res;
    // Check for null pointer parameter: tid
    if (tid == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: arg
    if (arg == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int32_t original_result =
        wasm_runtime_spawn_thread(exec_env, tid, callback, arg);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_join_thread_checked(wasm_thread_t tid, void *retval)
{
    Result res;
    // Check for null pointer parameter: retval
    if (retval == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int32_t original_result = wasm_runtime_join_thread(tid, retval);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_externref_obj2ref_checked(wasm_module_inst_t module_inst, void *extern_obj,
                               void *p_externref_idx)
{
    Result res;
    // Check for null pointer parameter: extern_obj
    if (extern_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: p_externref_idx
    if (p_externref_idx == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_externref_obj2ref(module_inst, extern_obj, p_externref_idx);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_externref_objdel_checked(wasm_module_inst_t module_inst, void *extern_obj)
{
    Result res;
    // Check for null pointer parameter: extern_obj
    if (extern_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_externref_objdel(module_inst, extern_obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_externref_set_cleanup_checked(wasm_module_inst_t module_inst,
                                   void *extern_obj, void *extern_obj_cleanup)
{
    Result res;
    // Check for null pointer parameter: extern_obj
    if (extern_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: extern_obj_cleanup
    if (extern_obj_cleanup == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_externref_set_cleanup(module_inst, extern_obj, extern_obj_cleanup);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_externref_ref2obj_checked(uint32_t externref_idx, void *p_extern_obj)
{
    Result res;
    // Check for null pointer parameter: p_extern_obj
    if (p_extern_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_externref_ref2obj(externref_idx, p_extern_obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_externref_retain_checked(uint32_t externref_idx)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_externref_retain(externref_idx);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_dump_call_stack_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_dump_call_stack(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_call_stack_buf_size_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_runtime_get_call_stack_buf_size(exec_env);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_dump_call_stack_to_buf_checked(wasm_exec_env_t exec_env, void *buf,
                                            uint32_t len)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result =
        wasm_runtime_dump_call_stack_to_buf(exec_env, buf, len);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_pgo_prof_data_size_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_runtime_get_pgo_prof_data_size(module_inst);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_dump_pgo_prof_data_to_buf_checked(wasm_module_inst_t module_inst,
                                               void *buf, uint32_t len)
{
    Result res;
    // Check for null pointer parameter: buf
    if (buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result =
        wasm_runtime_dump_pgo_prof_data_to_buf(module_inst, buf, len);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_get_custom_section_checked(wasm_module_t module_comm, void *name,
                                        void *len)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: len
    if (len == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_get_custom_section(module_comm, name, len);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_version_checked(void *major, void *minor, void *patch)
{
    Result res;
    // Check for null pointer parameter: major
    if (major == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: minor
    if (minor == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: patch
    if (patch == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_get_version(major, minor, patch);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_is_import_func_linked_checked(void *module_name, void *func_name)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: func_name
    if (func_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_is_import_func_linked(module_name, func_name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_is_import_global_linked_checked(void *module_name,
                                             void *global_name)
{
    Result res;
    // Check for null pointer parameter: module_name
    if (module_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: global_name
    if (global_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_is_import_global_linked(module_name, global_name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_enlarge_memory_checked(wasm_module_inst_t module_inst,
                                    uint64_t inc_page_count)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_enlarge_memory(module_inst, inc_page_count);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_set_enlarge_mem_error_callback_checked(
    enlarge_memory_error_callback_t callback, void *user_data)
{
    Result res;
    // Check for null pointer parameter: user_data
    if (user_data == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_enlarge_mem_error_callback(callback, user_data);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_create_context_key_checked(void *dtor)
{
    Result res;
    // Check for null pointer parameter: dtor
    if (dtor == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_create_context_key(dtor);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_destroy_context_key_checked(void *key)
{
    Result res;
    // Check for null pointer parameter: key
    if (key == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_destroy_context_key(key);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_context_checked(wasm_module_inst_t inst, void *key, void *ctx)
{
    Result res;
    // Check for null pointer parameter: key
    if (key == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: ctx
    if (ctx == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_context(inst, key, ctx);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_context_spread_checked(wasm_module_inst_t inst, void *key,
                                        void *ctx)
{
    Result res;
    // Check for null pointer parameter: key
    if (key == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: ctx
    if (ctx == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_set_context_spread(inst, key, ctx);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_context_checked(wasm_module_inst_t inst, void *key)
{
    Result res;
    // Check for null pointer parameter: key
    if (key == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_get_context(inst, key);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_begin_blocking_op_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_begin_blocking_op(exec_env);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_end_blocking_op_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_end_blocking_op(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_set_module_name_checked(wasm_module_t module, void *name,
                                     void *error_buf, uint32_t error_buf_size)
{
    Result res;
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: error_buf
    if (error_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_runtime_set_module_name(module, name, error_buf, error_buf_size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_get_module_name_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_module_name(module);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_detect_native_stack_overflow_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_detect_native_stack_overflow(exec_env);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_detect_native_stack_overflow_size_checked(wasm_exec_env_t exec_env,
                                                       uint32_t required_size)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_detect_native_stack_overflow_size(exec_env, required_size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_is_underlying_binary_freeable_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_is_underlying_binary_freeable(module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_create_shared_heap_checked(void *init_args)
{
    Result res;
    // Check for null pointer parameter: init_args
    if (init_args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_shared_heap_t original_result =
        wasm_runtime_create_shared_heap(init_args);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_shared_heap_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_chain_shared_heaps_checked(wasm_shared_heap_t head,
                                        wasm_shared_heap_t body)
{
    Result res;
    // Execute the original function
    wasm_shared_heap_t original_result =
        wasm_runtime_chain_shared_heaps(head, body);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_shared_heap_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_unchain_shared_heaps_checked(wasm_shared_heap_t head,
                                          _Bool entire_chain)
{
    Result res;
    // Execute the original function
    wasm_shared_heap_t original_result =
        wasm_runtime_unchain_shared_heaps(head, entire_chain);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_shared_heap_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_attach_shared_heap_checked(wasm_module_inst_t module_inst,
                                        wasm_shared_heap_t shared_heap)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_attach_shared_heap(module_inst, shared_heap);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_detach_shared_heap_checked(wasm_module_inst_t module_inst)
{
    Result res;
    // Execute the original function
    wasm_runtime_detach_shared_heap(module_inst);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_shared_heap_malloc_checked(wasm_module_inst_t module_inst,
                                        uint64_t size, void *p_native_addr)
{
    Result res;
    // Check for null pointer parameter: p_native_addr
    if (p_native_addr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint64_t original_result =
        wasm_runtime_shared_heap_malloc(module_inst, size, p_native_addr);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint64_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_shared_heap_free_checked(wasm_module_inst_t module_inst,
                                      uint64_t ptr)
{
    Result res;
    // Execute the original function
    wasm_runtime_shared_heap_free(module_inst, ptr);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_get_defined_type_count_checked(wasm_module_t module)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_get_defined_type_count(module);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_get_defined_type_checked(wasm_module_t module, uint32_t index)
{
    Result res;
    // Execute the original function
    wasm_defined_type_t original_result = wasm_get_defined_type(module, index);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_defined_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_obj_get_defined_type_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    wasm_defined_type_t original_result = wasm_obj_get_defined_type(obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_defined_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_obj_get_defined_type_idx_checked(wasm_module_t module, wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    int32_t original_result = wasm_obj_get_defined_type_idx(module, obj);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_defined_type_is_func_type_checked(wasm_defined_type_t def_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_defined_type_is_func_type(def_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_defined_type_is_struct_type_checked(wasm_defined_type_t def_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_defined_type_is_struct_type(def_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_defined_type_is_array_type_checked(wasm_defined_type_t def_type)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_defined_type_is_array_type(def_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_func_type_get_param_type_checked(wasm_func_type_t func_type,
                                      uint32_t param_idx)
{
    Result res;
    // Execute the original function
    wasm_ref_type_t original_result =
        wasm_func_type_get_param_type(func_type, param_idx);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_ref_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_type_get_result_type_checked(wasm_func_type_t func_type,
                                       uint32_t result_idx)
{
    Result res;
    // Execute the original function
    wasm_ref_type_t original_result =
        wasm_func_type_get_result_type(func_type, result_idx);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_ref_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_struct_type_get_field_count_checked(wasm_struct_type_t struct_type)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_struct_type_get_field_count(struct_type);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_struct_type_get_field_type_checked(wasm_struct_type_t struct_type,
                                        uint32_t field_idx, void *p_is_mutable)
{
    Result res;
    // Check for null pointer parameter: p_is_mutable
    if (p_is_mutable == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_type_t original_result =
        wasm_struct_type_get_field_type(struct_type, field_idx, p_is_mutable);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_ref_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_array_type_get_elem_type_checked(wasm_array_type_t array_type,
                                      void *p_is_mutable)
{
    Result res;
    // Check for null pointer parameter: p_is_mutable
    if (p_is_mutable == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_type_t original_result =
        wasm_array_type_get_elem_type(array_type, p_is_mutable);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_ref_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_defined_type_equal_checked(wasm_defined_type_t def_type1,
                                wasm_defined_type_t def_type2,
                                wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_defined_type_equal(def_type1, def_type2, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_defined_type_is_subtype_of_checked(wasm_defined_type_t def_type1,
                                        wasm_defined_type_t def_type2,
                                        wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_defined_type_is_subtype_of(def_type1, def_type2, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_ref_type_set_type_idx_checked(void *ref_type, _Bool nullable,
                                   int32_t type_idx)
{
    Result res;
    // Check for null pointer parameter: ref_type
    if (ref_type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_type_set_type_idx(ref_type, nullable, type_idx);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_type_set_heap_type_checked(void *ref_type, _Bool nullable,
                                    int32_t heap_type)
{
    Result res;
    // Check for null pointer parameter: ref_type
    if (ref_type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_type_set_heap_type(ref_type, nullable, heap_type);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_type_equal_checked(void *ref_type1, void *ref_type2,
                            wasm_module_t module)
{
    Result res;
    // Check for null pointer parameter: ref_type1
    if (ref_type1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: ref_type2
    if (ref_type2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_ref_type_equal(ref_type1, ref_type2, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_ref_type_is_subtype_of_checked(void *ref_type1, void *ref_type2,
                                    wasm_module_t module)
{
    Result res;
    // Check for null pointer parameter: ref_type1
    if (ref_type1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: ref_type2
    if (ref_type2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result =
        wasm_ref_type_is_subtype_of(ref_type1, ref_type2, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_struct_obj_new_with_typeidx_checked(wasm_exec_env_t exec_env,
                                         uint32_t type_idx)
{
    Result res;
    // Execute the original function
    wasm_struct_obj_t original_result =
        wasm_struct_obj_new_with_typeidx(exec_env, type_idx);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_struct_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_struct_obj_new_with_type_checked(wasm_exec_env_t exec_env,
                                      wasm_struct_type_t type)
{
    Result res;
    // Execute the original function
    wasm_struct_obj_t original_result =
        wasm_struct_obj_new_with_type(exec_env, type);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_struct_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_struct_obj_set_field_checked(wasm_struct_obj_t obj, uint32_t field_idx,
                                  void *value)
{
    Result res;
    // Check for null pointer parameter: value
    if (value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_struct_obj_set_field(obj, field_idx, value);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_struct_obj_get_field_checked(wasm_struct_obj_t obj, uint32_t field_idx,
                                  _Bool sign_extend, void *value)
{
    Result res;
    // Check for null pointer parameter: value
    if (value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_struct_obj_get_field(obj, field_idx, sign_extend, value);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_struct_obj_get_field_count_checked(wasm_struct_obj_t obj)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_struct_obj_get_field_count(obj);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_array_obj_new_with_typeidx_checked(wasm_exec_env_t exec_env,
                                        uint32_t type_idx, uint32_t length,
                                        void *init_value)
{
    Result res;
    // Check for null pointer parameter: init_value
    if (init_value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_array_obj_t original_result =
        wasm_array_obj_new_with_typeidx(exec_env, type_idx, length, init_value);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_array_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_array_obj_new_with_type_checked(wasm_exec_env_t exec_env,
                                     wasm_array_type_t type, uint32_t length,
                                     void *init_value)
{
    Result res;
    // Check for null pointer parameter: init_value
    if (init_value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_array_obj_t original_result =
        wasm_array_obj_new_with_type(exec_env, type, length, init_value);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_array_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_array_obj_set_elem_checked(wasm_array_obj_t array_obj, uint32_t elem_idx,
                                void *value)
{
    Result res;
    // Check for null pointer parameter: value
    if (value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_array_obj_set_elem(array_obj, elem_idx, value);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_array_obj_get_elem_checked(wasm_array_obj_t array_obj, uint32_t elem_idx,
                                _Bool sign_extend, void *value)
{
    Result res;
    // Check for null pointer parameter: value
    if (value == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_array_obj_get_elem(array_obj, elem_idx, sign_extend, value);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_array_obj_copy_checked(wasm_array_obj_t dst_obj, uint32_t dst_idx,
                            wasm_array_obj_t src_obj, uint32_t src_idx,
                            uint32_t len)
{
    Result res;
    // Execute the original function
    wasm_array_obj_copy(dst_obj, dst_idx, src_obj, src_idx, len);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_array_obj_length_checked(wasm_array_obj_t array_obj)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_array_obj_length(array_obj);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_array_obj_first_elem_addr_checked(wasm_array_obj_t array_obj)
{
    Result res;
    // Execute the original function
    wasm_array_obj_first_elem_addr(array_obj);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_array_obj_elem_addr_checked(wasm_array_obj_t array_obj, uint32_t elem_idx)
{
    Result res;
    // Execute the original function
    wasm_array_obj_elem_addr(array_obj, elem_idx);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_obj_new_with_typeidx_checked(wasm_exec_env_t exec_env,
                                       uint32_t type_idx,
                                       uint32_t func_idx_bound)
{
    Result res;
    // Execute the original function
    wasm_func_obj_t original_result =
        wasm_func_obj_new_with_typeidx(exec_env, type_idx, func_idx_bound);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_func_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_obj_new_with_type_checked(wasm_exec_env_t exec_env,
                                    wasm_func_type_t type,
                                    uint32_t func_idx_bound)
{
    Result res;
    // Execute the original function
    wasm_func_obj_t original_result =
        wasm_func_obj_new_with_type(exec_env, type, func_idx_bound);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_func_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_obj_get_func_idx_bound_checked(wasm_func_obj_t func_obj)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_func_obj_get_func_idx_bound(func_obj);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_obj_get_func_type_checked(wasm_func_obj_t func_obj)
{
    Result res;
    // Execute the original function
    wasm_func_type_t original_result = wasm_func_obj_get_func_type(func_obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_func_type_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_call_func_ref_checked(wasm_exec_env_t exec_env,
                                   wasm_func_obj_t func_obj, uint32_t argc,
                                   void *argv)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_runtime_call_func_ref(exec_env, func_obj, argc, argv);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_func_ref_a_checked(wasm_exec_env_t exec_env,
                                     wasm_func_obj_t func_obj,
                                     uint32_t num_results, void *results,
                                     uint32_t num_args, void *args)
{
    Result res;
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_runtime_call_func_ref_a(
        exec_env, func_obj, num_results, results, num_args, args);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_call_func_ref_v_checked(wasm_exec_env_t exec_env,
                                     wasm_func_obj_t func_obj,
                                     uint32_t num_results, void *results,
                                     uint32_t num_args, ...)
{
    Result res;
    va_list args;
    // Execute the original function
    va_start(args, num_args);
    _Bool original_result = wasm_runtime_call_func_ref_v(
        exec_env, func_obj, num_results, results, num_args, args);
    va_end(args);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_externref_obj_new_checked(wasm_exec_env_t exec_env, void *host_obj)
{
    Result res;
    // Check for null pointer parameter: host_obj
    if (host_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externref_obj_t original_result =
        wasm_externref_obj_new(exec_env, host_obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_externref_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_externref_obj_get_value_checked(wasm_externref_obj_t externref_obj)
{
    Result res;
    // Execute the original function
    wasm_externref_obj_get_value(externref_obj);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_anyref_obj_new_checked(wasm_exec_env_t exec_env, void *host_obj)
{
    Result res;
    // Check for null pointer parameter: host_obj
    if (host_obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_anyref_obj_t original_result = wasm_anyref_obj_new(exec_env, host_obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_anyref_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_anyref_obj_get_value_checked(wasm_anyref_obj_t anyref_obj)
{
    Result res;
    // Execute the original function
    wasm_anyref_obj_get_value(anyref_obj);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externref_obj_to_internal_obj_checked(wasm_externref_obj_t externref_obj)
{
    Result res;
    // Execute the original function
    wasm_obj_t original_result =
        wasm_externref_obj_to_internal_obj(externref_obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_internal_obj_to_externref_obj_checked(wasm_exec_env_t exec_env,
                                           wasm_obj_t internal_obj)
{
    Result res;
    // Execute the original function
    wasm_externref_obj_t original_result =
        wasm_internal_obj_to_externref_obj(exec_env, internal_obj);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.wasm_externref_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_i31_obj_new_checked(uint32_t i31_value)
{
    Result res;
    // Execute the original function
    wasm_i31_obj_t original_result = wasm_i31_obj_new(i31_value);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_i31_obj_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_i31_obj_get_value_checked(wasm_i31_obj_t i31_obj, _Bool sign_extend)
{
    Result res;
    // Execute the original function
    uint32_t original_result = wasm_i31_obj_get_value(i31_obj, sign_extend);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_runtime_pin_object_checked(wasm_exec_env_t exec_env, wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_pin_object(exec_env, obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_unpin_object_checked(wasm_exec_env_t exec_env, wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_runtime_unpin_object(exec_env, obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_struct_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_struct_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_array_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_array_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_func_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_func_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_i31_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_i31_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_externref_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_externref_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_anyref_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_anyref_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_internal_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_internal_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_eq_obj_checked(wasm_obj_t obj)
{
    Result res;
    // Execute the original function
    _Bool original_result = wasm_obj_is_eq_obj(obj);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_instance_of_defined_type_checked(wasm_obj_t obj,
                                             wasm_defined_type_t defined_type,
                                             wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_obj_is_instance_of_defined_type(obj, defined_type, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_instance_of_type_idx_checked(wasm_obj_t obj, uint32_t type_idx,
                                         wasm_module_t module)
{
    Result res;
    // Execute the original function
    _Bool original_result =
        wasm_obj_is_instance_of_type_idx(obj, type_idx, module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_is_instance_of_ref_type_checked(wasm_obj_t obj, void *ref_type)
{
    Result res;
    // Check for null pointer parameter: ref_type
    if (ref_type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_obj_is_instance_of_ref_type(obj, ref_type);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_runtime_push_local_obj_ref_checked(wasm_exec_env_t exec_env,
                                        void *local_obj_ref)
{
    Result res;
    // Check for null pointer parameter: local_obj_ref
    if (local_obj_ref == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_runtime_push_local_obj_ref(exec_env, local_obj_ref);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_pop_local_obj_ref_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_pop_local_obj_ref(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_pop_local_obj_refs_checked(wasm_exec_env_t exec_env, uint32_t n)
{
    Result res;
    // Execute the original function
    wasm_runtime_pop_local_obj_refs(exec_env, n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_runtime_get_cur_local_obj_ref_checked(wasm_exec_env_t exec_env)
{
    Result res;
    // Execute the original function
    wasm_runtime_get_cur_local_obj_ref(exec_env);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_obj_set_gc_finalizer_checked(wasm_exec_env_t exec_env, wasm_obj_t obj,
                                  wasm_obj_finalizer_t cb, void *data)
{
    Result res;
    // Check for null pointer parameter: data
    if (data == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_obj_set_gc_finalizer(exec_env, obj, cb, data);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_obj_unset_gc_finalizer_checked(wasm_exec_env_t exec_env, void *obj)
{
    Result res;
    // Check for null pointer parameter: obj
    if (obj == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_obj_unset_gc_finalizer(exec_env, obj);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

#endif // GC_EXPORT_CHECKED_H
