
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!
 */
#ifndef AOT_EXPORT_CHECKED_H
#define AOT_EXPORT_CHECKED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "aot_export.h"

typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        uint32_t uint32_t_value;
        _Bool _Bool_value;
        aot_obj_data_t aot_obj_data_t_value;
        aot_comp_data_t aot_comp_data_t_value;
        aot_comp_context_t aot_comp_context_t_value;
        // Add other types as needed
    } value;
} Result;

static inline Result
aot_call_stack_features_init_default_checked(void *features)
{
    Result res;
    // Check for null pointer parameter: features
    if (features == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    aot_call_stack_features_init_default(features);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_create_comp_data_checked(void *wasm_module, void *target_arch,
                             _Bool gc_enabled)
{
    Result res;
    // Check for null pointer parameter: wasm_module
    if (wasm_module == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: target_arch
    if (target_arch == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    aot_comp_data_t original_result =
        aot_create_comp_data(wasm_module, target_arch, gc_enabled);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.aot_comp_data_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
aot_destroy_comp_data_checked(aot_comp_data_t comp_data)
{
    Result res;
    // Execute the original function
    aot_destroy_comp_data(comp_data);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_compiler_init_checked(void)
{
    Result res;
    // Execute the original function
    _Bool original_result = aot_compiler_init();
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_compiler_destroy_checked(void)
{
    Result res;
    // Execute the original function
    aot_compiler_destroy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_create_comp_context_checked(aot_comp_data_t comp_data,
                                aot_comp_option_t option)
{
    Result res;
    // Execute the original function
    aot_comp_context_t original_result =
        aot_create_comp_context(comp_data, option);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.aot_comp_context_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
aot_destroy_comp_context_checked(aot_comp_context_t comp_ctx)
{
    Result res;
    // Execute the original function
    aot_destroy_comp_context(comp_ctx);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_compile_wasm_checked(aot_comp_context_t comp_ctx)
{
    Result res;
    // Execute the original function
    _Bool original_result = aot_compile_wasm(comp_ctx);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_obj_data_create_checked(aot_comp_context_t comp_ctx)
{
    Result res;
    // Execute the original function
    aot_obj_data_t original_result = aot_obj_data_create(comp_ctx);
    // Assign return value and error code
    if (original_result != NULL) {
        res.error_code = 0;
        res.value.aot_obj_data_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
aot_obj_data_destroy_checked(aot_obj_data_t obj_data)
{
    Result res;
    // Execute the original function
    aot_obj_data_destroy(obj_data);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_get_aot_file_size_checked(aot_comp_context_t comp_ctx,
                              aot_comp_data_t comp_data,
                              aot_obj_data_t obj_data)
{
    Result res;
    // Execute the original function
    uint32_t original_result =
        aot_get_aot_file_size(comp_ctx, comp_data, obj_data);
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
aot_emit_aot_file_buf_checked(aot_comp_context_t comp_ctx,
                              aot_comp_data_t comp_data, void *p_aot_file_size)
{
    Result res;
    // Check for null pointer parameter: p_aot_file_size
    if (p_aot_file_size == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    aot_emit_aot_file_buf(comp_ctx, comp_data, p_aot_file_size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_emit_aot_file_buf_ex_checked(aot_comp_context_t comp_ctx,
                                 aot_comp_data_t comp_data,
                                 aot_obj_data_t obj_data, void *aot_file_buf,
                                 uint32_t aot_file_size)
{
    Result res;
    // Check for null pointer parameter: aot_file_buf
    if (aot_file_buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = aot_emit_aot_file_buf_ex(
        comp_ctx, comp_data, obj_data, aot_file_buf, aot_file_size);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_emit_llvm_file_checked(aot_comp_context_t comp_ctx, void *file_name)
{
    Result res;
    // Check for null pointer parameter: file_name
    if (file_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = aot_emit_llvm_file(comp_ctx, file_name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_emit_object_file_checked(aot_comp_context_t comp_ctx, void *file_name)
{
    Result res;
    // Check for null pointer parameter: file_name
    if (file_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = aot_emit_object_file(comp_ctx, file_name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_emit_aot_file_checked(aot_comp_context_t comp_ctx,
                          aot_comp_data_t comp_data, void *file_name)
{
    Result res;
    // Check for null pointer parameter: file_name
    if (file_name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = aot_emit_aot_file(comp_ctx, comp_data, file_name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
aot_destroy_aot_file_checked(void *aot_file)
{
    Result res;
    // Check for null pointer parameter: aot_file
    if (aot_file == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    aot_destroy_aot_file(aot_file);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_get_last_error_checked(void)
{
    Result res;
    // Execute the original function
    aot_get_last_error();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
aot_get_plt_table_size_checked(void)
{
    Result res;
    // Execute the original function
    uint32_t original_result = aot_get_plt_table_size();
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

#endif // AOT_EXPORT_CHECKED_H
