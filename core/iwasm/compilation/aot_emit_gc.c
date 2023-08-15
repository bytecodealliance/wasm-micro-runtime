/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_gc.h"
#include "aot_emit_exception.h"

#if WASM_ENABLE_GC != 0

#define BUILD_ISNULL(ptr, res, name)                                  \
    do {                                                              \
        if (!(res = LLVMBuildIsNull(comp_ctx->builder, ptr, name))) { \
            aot_set_last_error("llvm build isnull failed.");          \
            goto fail;                                                \
        }                                                             \
    } while (0)

#define BUILD_ISNOTNULL(ptr, res, name)                                  \
    do {                                                                 \
        if (!(res = LLVMBuildIsNotNull(comp_ctx->builder, ptr, name))) { \
            aot_set_last_error("llvm build isnotnull failed.");          \
            goto fail;                                                   \
        }                                                                \
    } while (0)

#define BUILD_ICMP(op, left, right, res, name)                                \
    do {                                                                      \
        if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,        \
                                                    func_ctx->func, name))) { \
            aot_set_last_error("llvm add basic block failed.");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define CURR_BLOCK() LLVMGetInsertBlock(comp_ctx->builder)

#define MOVE_BLOCK_AFTER(llvm_block, llvm_block_after) \
    LLVMMoveBasicBlockAfter(llvm_block, llvm_block_after)

#define MOVE_BLOCK_AFTER_CURR(llvm_block) \
    LLVMMoveBasicBlockAfter(llvm_block, CURR_BLOCK())

#define MOVE_BLOCK_BEFORE(llvm_block, llvm_block_before) \
    LLVMMoveBasicBlockBefore(llvm_block, llvm_block_before)

#define BUILD_COND_BR(value_if, block_then, block_else)               \
    do {                                                              \
        if (!LLVMBuildCondBr(comp_ctx->builder, value_if, block_then, \
                             block_else)) {                           \
            aot_set_last_error("llvm build cond br failed.");         \
            goto fail;                                                \
        }                                                             \
    } while (0)

#define SET_BUILDER_POS(llvm_block) \
    LLVMPositionBuilderAtEnd(comp_ctx->builder, llvm_block)

#define BUILD_BR(llvm_block)                               \
    do {                                                   \
        if (!LLVMBuildBr(comp_ctx->builder, llvm_block)) { \
            aot_set_last_error("llvm build br failed.");   \
            goto fail;                                     \
        }                                                  \
    } while (0)

#define BUILD_ICMP(op, left, right, res, name)                                \
    do {                                                                      \
        if (!(res =                                                           \
                  LLVMBuildICmp(comp_ctx->builder, op, left, right, name))) { \
            aot_set_last_error("llvm build icmp failed.");                    \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

bool
aot_call_aot_create_func_obj(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef func_idx, LLVMValueRef *p_gc_obj)
{
    LLVMValueRef gc_obj, cmp_gc_obj, param_values[5], func, value;
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef init_gc_obj_fail, init_gc_obj_succ;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = I32_TYPE;
    ret_type = GC_REF_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_create_func_obj, 5);
    else
        GET_AOT_FUNCTION(aot_create_func_obj, 5);

    /* Call function llvm_jit/aot_create_func_obj()  */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = func_idx;
    param_values[2] = I8_CONST(1);
    param_values[3] = I8_PTR_NULL;
    param_values[4] = I32_ZERO;
    if (!(gc_obj = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                  param_values, 5, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ISNOTNULL(gc_obj, cmp_gc_obj, "gc_obj_not_null");

    ADD_BASIC_BLOCK(init_gc_obj_fail, "init_gc_obj_fail");
    ADD_BASIC_BLOCK(init_gc_obj_succ, "init_gc_obj_success");

    LLVMMoveBasicBlockAfter(init_gc_obj_fail, block_curr);
    LLVMMoveBasicBlockAfter(init_gc_obj_succ, block_curr);

    if (!LLVMBuildCondBr(comp_ctx->builder, cmp_gc_obj, init_gc_obj_succ,
                         init_gc_obj_fail)) {
        aot_set_last_error("llvm build cond br failed.");
        goto fail;
    }

    /* If init gc_obj failed, return this function
       so the runtime can catch the exception */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, init_gc_obj_fail);
    if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
        goto fail;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, init_gc_obj_succ);
    *p_gc_obj = gc_obj;

    return true;
fail:
    return false;
}

bool
aot_call_aot_obj_is_instance_of(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx, LLVMValueRef gc_obj,
                                LLVMValueRef heap_type, LLVMValueRef *castable)
{
    LLVMValueRef param_values[3], func, value, res;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = GC_REF_TYPE;
    param_types[2] = I32_TYPE;
    ret_type = INT8_TYPE;

    GET_AOT_FUNCTION(aot_obj_is_instance_of, 3);

    /* Call function wasm_obj_is_type_of() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = gc_obj;
    param_values[2] = heap_type;

    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               3, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *castable = res;

    return true;
fail:
    return false;
}

bool
aot_call_wasm_obj_is_type_of(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef gc_obj, LLVMValueRef heap_type,
                             LLVMValueRef *castable)
{
    LLVMValueRef param_values[2], func, value, res;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    param_types[0] = GC_REF_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = INT8_TYPE;

    GET_AOT_FUNCTION(wasm_obj_is_type_of, 2);

    /* Call function wasm_obj_is_type_of() */
    param_values[0] = gc_obj;
    param_values[1] = heap_type;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *castable = res;

    return true;
fail:
    return false;
}

bool
aot_call_wasm_externref_obj_to_internal_obj(AOTCompContext *comp_ctx,
                                            AOTFuncContext *func_ctx,
                                            LLVMValueRef externref_obj,
                                            LLVMValueRef *gc_obj)
{
    LLVMValueRef param_values[1], func, value, res;
    LLVMTypeRef param_types[1], ret_type, func_type, func_ptr_type;

    param_types[0] = GC_REF_TYPE;
    ret_type = GC_REF_TYPE;

    GET_AOT_FUNCTION(wasm_externref_obj_to_internal_obj, 1);

    /* Call function wasm_externref_obj_to_internal_obj */
    param_values[0] = externref_obj;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               1, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *gc_obj = res;

    return true;
fail:
    return false;
}

bool
aot_call_wasm_internal_obj_to_external_obj(AOTCompContext *comp_ctx,
                                           AOTFuncContext *func_ctx,
                                           LLVMValueRef gc_obj,
                                           LLVMValueRef *externref_obj)
{
    LLVMValueRef param_values[2], func, value, res;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = GC_REF_TYPE;
    ret_type = GC_REF_TYPE;

    GET_AOT_FUNCTION(wasm_externref_obj_to_internal_obj, 2);

    /* Call function wasm_externref_obj_to_internal_obj() */
    param_values[0] = func_ctx->exec_env;
    param_values[1] = gc_obj;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *externref_obj = res;

    return true;
fail:
    return false;
}

bool
aot_call_aot_rtt_type_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          LLVMValueRef type_index, LLVMValueRef *rtt_type)
{
    LLVMValueRef param_values[2], func, value, res;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = GC_REF_TYPE;

    GET_AOT_FUNCTION(aot_rtt_type_new, 2);

    /* Call function wasm_struct_obj_new() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = type_index;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *rtt_type = res;
    return true;
fail:
    return false;
}

bool
aot_call_wasm_struct_obj_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef rtt_type, LLVMValueRef *struct_obj)
{
    LLVMValueRef param_values[2], func, value, res;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    ret_type = GC_REF_TYPE;

    GET_AOT_FUNCTION(wasm_struct_obj_new, 2);

    /* Call function wasm_struct_obj_new() */
    param_values[0] = func_ctx->exec_env;
    param_values[1] = rtt_type;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *struct_obj = res;
    return true;
fail:
    return false;
}

bool
aot_call_wasm_struct_obj_set_field(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   LLVMValueRef struct_obj,
                                   LLVMValueRef field_idx,
                                   LLVMValueRef field_value)
{
    LLVMValueRef param_values[3], func, value, field_value_ptr;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    if (!(field_value_ptr = LLVMBuildAlloca(
              comp_ctx->builder, LLVMTypeOf(field_value), "field_value_ptr"))) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }
    if (!LLVMBuildStore(comp_ctx->builder, field_value, field_value_ptr)) {
        aot_set_last_error("llvm build store failed.");
        goto fail;
    }
    if (!(field_value_ptr =
              LLVMBuildBitCast(comp_ctx->builder, field_value_ptr,
                               INT8_PTR_TYPE, "field_value_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_PTR_TYPE;
    ret_type = VOID_TYPE;

    GET_AOT_FUNCTION(wasm_struct_obj_set_field, 3);

    /* Call function wasm_struct_obj_set_field() */
    param_values[0] = struct_obj;
    param_values[1] = field_idx;
    param_values[2] = field_value_ptr;
    if (!LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 3,
                        "call")) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_call_wasm_struct_obj_get_field(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx,
                                   LLVMValueRef struct_obj,
                                   LLVMValueRef field_idx,
                                   LLVMValueRef sign_extend,
                                   LLVMValueRef field_value_ptr)
{
    LLVMValueRef param_values[4], func, value;
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    ret_type = VOID_TYPE;

    GET_AOT_FUNCTION(wasm_struct_obj_get_field, 4);

    /* Call function wasm_struct_obj_get_field() */
    param_values[0] = struct_obj;
    param_values[1] = field_idx;
    param_values[2] = sign_extend;
    param_values[3] = field_value_ptr;
    if (!LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 4,
                        "call")) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_call_wasm_array_obj_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef rtt_type, LLVMValueRef array_len,
                            LLVMValueRef array_elem, LLVMValueRef *array_obj)
{
    LLVMValueRef param_values[4], func, value, res, array_elem_ptr;
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;

    if (!(array_elem_ptr = LLVMBuildAlloca(
              comp_ctx->builder, LLVMTypeOf(array_elem), "array_elem_ptr"))) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }
    if (!LLVMBuildStore(comp_ctx->builder, array_elem, array_elem_ptr)) {
        aot_set_last_error("llvm build store failed.");
        goto fail;
    }
    if (!(array_elem_ptr = LLVMBuildBitCast(comp_ctx->builder, array_elem_ptr,
                                            INT8_PTR_TYPE, "array_elem_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    ret_type = GC_REF_TYPE;

    GET_AOT_FUNCTION(wasm_array_obj_new, 4);

    /* Call function wasm_array_obj_new() */
    param_values[0] = func_ctx->exec_env;
    param_values[1] = rtt_type;
    param_values[2] = array_len;
    param_values[3] = array_elem_ptr;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               4, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *array_obj = res;
    return true;
fail:
    return false;
}

bool
aot_call_wasm_array_set_elem(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef array_obj, LLVMValueRef elem_idx,
                             LLVMValueRef array_elem)
{
    LLVMValueRef param_values[3], func, value, array_elem_ptr;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    if (!(array_elem_ptr = LLVMBuildAlloca(
              comp_ctx->builder, LLVMTypeOf(array_elem), "array_elem_ptr"))) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }
    if (!LLVMBuildStore(comp_ctx->builder, array_elem, array_elem_ptr)) {
        aot_set_last_error("llvm build store failed.");
        goto fail;
    }
    if (!(array_elem_ptr = LLVMBuildBitCast(comp_ctx->builder, array_elem_ptr,
                                            INT8_PTR_TYPE, "array_elem_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_PTR_TYPE;
    ret_type = VOID_TYPE;

    GET_AOT_FUNCTION(wasm_array_obj_set_elem, 3);

    /* Call function wasm_array_obj_set_elem() */
    param_values[0] = array_obj;
    param_values[1] = elem_idx;
    param_values[2] = array_elem_ptr;
    if (!LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 3,
                        "call")) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_call_aot_array_init_with_data(
    AOTCompContext *comp_ctx, AOTFuncContext *func_ctx, LLVMValueRef seg_index,
    LLVMValueRef data_seg_offset, LLVMValueRef array_obj,
    LLVMValueRef elem_size, LLVMValueRef array_len)
{
    LLVMValueRef param_values[6], func, value, res, cmp;
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;
    LLVMBasicBlockRef init_success;

    ADD_BASIC_BLOCK(init_success, "init success");
    MOVE_BLOCK_AFTER_CURR(init_success);

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = I32_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = INT8_TYPE;

    GET_AOT_FUNCTION(aot_array_init_with_data, 6);

    /* Call function aot_array_init_with_data() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = seg_index;
    param_values[2] = data_seg_offset;
    param_values[3] = array_obj;
    param_values[4] = elem_size;
    param_values[5] = array_len;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               6, "call"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    BUILD_ICMP(LLVMIntEQ, res, I8_ZERO, cmp, "array_init_ret");
    if (!aot_emit_exception(comp_ctx, func_ctx,
                            EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                            init_success))
        goto fail;

    return true;
fail:
    return false;
}

bool
aot_call_wasm_array_get_elem(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef array_obj, LLVMValueRef elem_idx,
                             LLVMValueRef sign, LLVMValueRef array_elem_ptr)
{
    LLVMValueRef param_values[4], func, value;
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    ret_type = VOID_TYPE;

    GET_AOT_FUNCTION(wasm_array_obj_get_elem, 4);

    /* Call function wasm_array_obj_get_elem() */
    param_values[0] = array_obj;
    param_values[1] = elem_idx;
    param_values[2] = sign;
    param_values[3] = array_elem_ptr;
    if (!LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 3,
                        "call")) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_ref_as_non_null(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    LLVMValueRef gc_obj, cmp_gc_obj;
    LLVMBasicBlockRef check_gc_obj_succ;

    GET_REF_FROM_STACK(gc_obj);

    /* Check if func object is NULL */
    BUILD_ICMP(LLVMIntEQ, gc_obj, GC_REF_NULL, cmp_gc_obj, "cmp_func_obj");

    /* Throw exception if func object is NULL */
    ADD_BASIC_BLOCK(check_gc_obj_succ, "check_func_obj_succ");
    MOVE_BLOCK_AFTER_CURR(check_gc_obj_succ);

    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true,
                            cmp_gc_obj, check_gc_obj_succ))
        goto fail;

    return true;
fail:
    return false;
}

bool
aot_compile_op_i31_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef i31_val, i31_obj;

    POP_I32(i31_val);

    /* Equivalent to wasm_i31_obj_new: but use I32 to represent i31_obj directly
       (WASMI31ObjectRef)((i31_value << 1) | 1) */
    if (!(i31_obj = LLVMBuildShl(comp_ctx->builder, i31_val, I32_ONE,
                                 "i31_value << 1"))) {
        aot_set_last_error("llvm build shl failed.");
        goto fail;
    }

    if (!(i31_obj = LLVMBuildOr(comp_ctx->builder, i31_obj, I32_ONE,
                                "((i31_value << 1) | 1)"))) {
        aot_set_last_error("llvm build or failed.");
        goto fail;
    }

    PUSH_REF(i31_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_i31_get(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       bool sign)
{
    LLVMValueRef i31_obj, i31_val, cmp_i31_obj, bit_30, bit_30_is_set,
        i31_sign_val;
    LLVMBasicBlockRef check_i31_obj_succ;

    POP_REF(i31_obj);

    ADD_BASIC_BLOCK(check_i31_obj_succ, "check_func_obj_succ");
    MOVE_BLOCK_AFTER_CURR(check_i31_obj_succ);

    /* Check if i31 object is NULL, throw exception if it is */
    BUILD_ICMP(LLVMIntEQ, i31_obj, GC_REF_NULL, cmp_i31_obj, "cmp_func_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true,
                            cmp_i31_obj, check_i31_obj_succ))
        goto fail;

    /* i31_obj >> 1 */
    if (!(i31_val = LLVMBuildLShr(comp_ctx->builder, i31_obj, I32_ONE,
                                  "i31_value"))) {
        aot_set_last_error("llvm build shr failed.");
        goto fail;
    }

    if (sign) {
        /* i31_val = i31_val & 0x40000000? i31_val |= 0x80000000 : i31_val */
        if (!(bit_30 = LLVMBuildAnd(comp_ctx->builder, i31_val,
                                    I32_CONST(0x40000000), "bit30"))) {
            aot_set_last_error("llvm build and failed.");
            goto fail;
        }
        BUILD_ICMP(LLVMIntNE, bit_30, I32_ZERO, bit_30_is_set, "bit30_is_set");
        if (!(i31_sign_val = LLVMBuildOr(comp_ctx->builder, i31_val,
                                         I32_CONST(0x80000000), "or"))) {
            aot_set_last_error("llvm build or failed.");
            goto fail;
        }
        if (!(i31_val =
                  LLVMBuildSelect(comp_ctx->builder, bit_30_is_set,
                                  i31_sign_val, i31_val, "final_value"))) {
            aot_set_last_error("llvm build select failed.");
            goto fail;
        }
    }

    PUSH_I32(i31_val);

    return true;
fail:
    return false;
}

bool
aot_compile_op_ref_test(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        int32 heap_type, bool nullable, bool cast)
{
    LLVMValueRef gc_obj, cmp, castable;
    LLVMBasicBlockRef gc_obj_null, gc_obj_non_null, end_block;

    POP_REF(gc_obj);

    /* Create if block */
    ADD_BASIC_BLOCK(gc_obj_null, "gc_obj_null");
    MOVE_BLOCK_AFTER_CURR(gc_obj_null);

    /* Create else block */
    ADD_BASIC_BLOCK(gc_obj_non_null, "gc_obj_non_null");
    MOVE_BLOCK_AFTER_CURR(gc_obj_non_null);

    /* Create end block */
    ADD_BASIC_BLOCK(end_block, "end_block");
    MOVE_BLOCK_AFTER_CURR(end_block);

    /* Check if gc object is NULL */
    BUILD_ICMP(LLVMIntEQ, gc_obj, GC_REF_NULL, cmp, "cmp_func_obj");
    BUILD_COND_BR(cmp, gc_obj_null, gc_obj_non_null);

    /* Move builder to gc_obj NULL block */
    SET_BUILDER_POS(gc_obj_null);
    if (!cast) {
        /* For WASM_OP_REF_TEST and WASM_OP_REF_TEST_NULLABLE */
        if (nullable)
            PUSH_I32(I32_ONE);
        else
            PUSH_I32(I32_ZERO);
    }
    else {
        /* For WASM_OP_REF_CAST and WASM_OP_REF_CAST_NULLABLE*/
        if (!nullable
            && !aot_emit_exception(comp_ctx, func_ctx, EXCE_TYPE_NONCASTABLE,
                                   false, NULL, NULL))
            goto fail;
    }

    BUILD_BR(end_block);

    /* Move builder to gc_obj not NULL block */
    SET_BUILDER_POS(gc_obj_non_null);
    if (heap_type >= 0) {
        if (!aot_call_aot_obj_is_instance_of(comp_ctx, func_ctx, gc_obj,
                                             I32_CONST(heap_type), &castable))
            goto fail;
    }
    else {
        if (!aot_call_wasm_obj_is_type_of(comp_ctx, func_ctx, gc_obj,
                                          I32_CONST(heap_type), &castable))
            goto fail;
    }

    if (!(castable = LLVMBuildZExt(comp_ctx->builder, castable, I32_TYPE,
                                   "castable_i32"))) {
        aot_set_last_error("llvm build zext failed.");
        goto fail;
    }

    if (!cast) {
        /* For WASM_OP_REF_TEST and WASM_OP_REF_TEST_NULLABLE */
        PUSH_I32(castable);
        BUILD_BR(end_block);
    }
    else {
        /* For WASM_OP_REF_CAST and WASM_OP_REF_CAST_NULLABLE*/
        BUILD_ICMP(LLVMIntEQ, castable, I32_ZERO, cmp, "cmp_castable");
        if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_TYPE_NONCASTABLE, true,
                                cmp, end_block))
            goto fail;
    }

    /* Move builder to end block */
    SET_BUILDER_POS(end_block);

    return true;
fail:
    return false;
}

bool
aot_compile_op_extern_internalize(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx)
{
    LLVMValueRef externref_obj, gc_obj, cmp;
    LLVMBasicBlockRef obj_null, obj_non_null, end_block;

    POP_REF(externref_obj);

    /* Create if block */
    ADD_BASIC_BLOCK(obj_null, "obj_null");
    MOVE_BLOCK_AFTER_CURR(obj_null);

    /* Create else block */
    ADD_BASIC_BLOCK(obj_non_null, "obj_non_null");
    MOVE_BLOCK_AFTER_CURR(obj_non_null);

    /* Create end block */
    ADD_BASIC_BLOCK(end_block, "end_block");
    MOVE_BLOCK_AFTER_CURR(end_block);

    /* Check if externref object is NULL */
    BUILD_ICMP(LLVMIntEQ, externref_obj, GC_REF_NULL, cmp, "cmp_externref_obj");
    BUILD_COND_BR(cmp, obj_null, obj_non_null);

    /* Move builder to obj NULL block */
    SET_BUILDER_POS(obj_null);
    PUSH_REF(GC_REF_NULL);
    BUILD_BR(end_block);

    /* Move builder to obj not NULL block */
    SET_BUILDER_POS(obj_non_null);
    if (!aot_call_wasm_externref_obj_to_internal_obj(comp_ctx, func_ctx,
                                                     externref_obj, &gc_obj))
        goto fail;
    PUSH_REF(gc_obj);
    BUILD_BR(end_block);

    /* Move builder to end block */
    SET_BUILDER_POS(end_block);

    return true;
fail:
    return false;
}

bool
aot_compile_op_extern_externalize(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx)
{
    LLVMValueRef gc_obj, externref_obj, cmp;
    LLVMBasicBlockRef obj_null, obj_non_null, end_block, externalize_succ;

    POP_REF(gc_obj);

    /* Create if block */
    ADD_BASIC_BLOCK(obj_null, "obj_null");
    MOVE_BLOCK_AFTER_CURR(obj_null);

    /* Create else block */
    ADD_BASIC_BLOCK(obj_non_null, "obj_non_null");
    MOVE_BLOCK_AFTER_CURR(obj_non_null);
    ADD_BASIC_BLOCK(externalize_succ, "externalized_succ");
    MOVE_BLOCK_AFTER(externalize_succ, obj_non_null);

    /* Create end block */
    ADD_BASIC_BLOCK(end_block, "end_block");
    MOVE_BLOCK_AFTER_CURR(end_block);

    /* Check if gc object is NULL */
    BUILD_ICMP(LLVMIntEQ, gc_obj, GC_REF_NULL, cmp, "cmp_gc_obj");
    BUILD_COND_BR(cmp, obj_null, obj_non_null);

    /* Move builder to obj NULL block */
    SET_BUILDER_POS(obj_null);
    PUSH_REF(GC_REF_NULL);
    BUILD_BR(end_block);

    /* Move builder to obj not NULL block */
    SET_BUILDER_POS(obj_non_null);
    if (!aot_call_wasm_internal_obj_to_external_obj(comp_ctx, func_ctx, gc_obj,
                                                    &externref_obj))
        goto fail;
    BUILD_ICMP(LLVMIntEQ, externref_obj, GC_REF_NULL, cmp, "cmp_externref_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            externalize_succ))
        goto fail;
    PUSH_REF(externref_obj);
    BUILD_BR(end_block);

    /* Move builder to end block */
    SET_BUILDER_POS(end_block);

    return true;
fail:
    return false;
}

static bool
struct_new_canon_init_fields(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 type_index, LLVMValueRef struct_obj)
{
    LLVMValueRef field_value;
    /* Used for distinguish what type of AOTValue POP */
    WASMStructType *compile_time_struct_type =
        (WASMStructType *)comp_ctx->comp_data->types[type_index];
    WASMStructFieldType *fields = compile_time_struct_type->fields;
    int32 field_count = (int32)compile_time_struct_type->field_count;
    int32 field_idx;
    uint8 field_type;

    for (field_idx = field_count - 1; field_idx >= 0; field_idx--) {
        field_type = fields[field_idx].field_type;
        if (wasm_is_type_reftype(field_type)) {
            POP_REF(field_value);
        }
        else if (field_type == VALUE_TYPE_I32 || field_type == VALUE_TYPE_F32
                 || field_type == PACKED_TYPE_I8
                 || field_type == PACKED_TYPE_I16) {
            POP_I32(field_value);
        }
        else {
            POP_I64(field_value);
        }

        if (!aot_call_wasm_struct_obj_set_field(comp_ctx, func_ctx, struct_obj,
                                                I32_CONST(field_idx),
                                                field_value))
            goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_struct_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 type_index, bool init_with_default)
{
    LLVMValueRef rtt_type, struct_obj, cmp;
    LLVMBasicBlockRef check_rtt_type_succ, check_struct_obj_succ;

    /* Generate call wasm_rtt_type_new and check for exception */
    if (!aot_call_aot_rtt_type_new(comp_ctx, func_ctx, I32_CONST(type_index),
                                   &rtt_type))
        goto fail;

    ADD_BASIC_BLOCK(check_rtt_type_succ, "check rtt type succ");
    MOVE_BLOCK_AFTER_CURR(check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, rtt_type, GC_REF_NULL, cmp, "cmp_rtt_type");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_rtt_type_succ))
        goto fail;

    /* Generate call wasm_struct_obj_new and check for exception */
    if (!aot_call_wasm_struct_obj_new(comp_ctx, func_ctx, rtt_type,
                                      &struct_obj))
        goto fail;

    ADD_BASIC_BLOCK(check_struct_obj_succ, "check struct obj succ");
    MOVE_BLOCK_AFTER(check_struct_obj_succ, check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, struct_obj, GC_REF_NULL, cmp, "cmp_struct_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_struct_obj_succ))
        goto fail;

    /* For WASM_OP_STRUCT_NEW_CANON, init filed with poped value */
    if (!init_with_default
        && !struct_new_canon_init_fields(comp_ctx, func_ctx, type_index,
                                         struct_obj)) {
        goto fail;
    }

    PUSH_REF(struct_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_struct_get(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 type_index, uint32 field_idx, bool sign)
{
    LLVMTypeRef field_value_type;
    LLVMValueRef struct_obj, cmp, field_value_ptr, field_value;
    LLVMBasicBlockRef check_struct_obj_succ;
    /* Used for distinguish what type of AOTValue PUSH */
    WASMStructType *compile_time_struct_type =
        (WASMStructType *)comp_ctx->comp_data->types[type_index];
    WASMStructFieldType *fields = compile_time_struct_type->fields;
    uint32 field_count = compile_time_struct_type->field_count;
    uint8 field_type;

    if (field_idx >= field_count) {
        aot_set_last_error("struct field index out of bounds");
        goto fail;
    }

    /* Get LLVM type based on field_type */
    field_type = fields[field_idx].field_type;
    if (wasm_is_type_reftype(field_type)) {
        field_value_type = GC_REF_PTR_TYPE;
    }
    else if (field_type == VALUE_TYPE_I32 || field_type == VALUE_TYPE_F32
             || field_type == PACKED_TYPE_I8 || field_type == PACKED_TYPE_I16) {
        field_value_type = I32_TYPE;
    }
    else {
        field_value_type = I64_TYPE;
    }

    POP_REF(struct_obj);

    ADD_BASIC_BLOCK(check_struct_obj_succ, "check struct obj succ");
    MOVE_BLOCK_AFTER_CURR(check_struct_obj_succ);

    BUILD_ICMP(LLVMIntEQ, struct_obj, GC_REF_NULL, cmp, "cmp_struct_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_struct_obj_succ))
        goto fail;

    if (!(field_value_ptr = LLVMBuildAlloca(comp_ctx->builder, field_value_type,
                                            "field_value_ptr"))) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }
    if (!(field_value_ptr =
              LLVMBuildBitCast(comp_ctx->builder, field_value_ptr,
                               INT8_PTR_TYPE, "field_value_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    if (!aot_call_wasm_struct_obj_get_field(comp_ctx, func_ctx, struct_obj,
                                            I32_CONST(field_idx),
                                            I8_CONST(sign), field_value_ptr))
        goto fail;

    if (!(field_value = LLVMBuildLoad2(comp_ctx->builder, field_value_type,
                                       field_value_ptr, ""))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    if (wasm_is_type_reftype(field_type)) {
        PUSH_REF(field_value);
    }
    else if (field_type == VALUE_TYPE_I32 || field_type == VALUE_TYPE_F32
             || field_type == PACKED_TYPE_I8 || field_type == PACKED_TYPE_I16) {
        PUSH_I32(field_value);
    }
    else {
        PUSH_I64(field_value);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_struct_set(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 type_index, uint32 field_idx)
{
    LLVMValueRef struct_obj, cmp, field_value;
    LLVMBasicBlockRef check_struct_obj_succ;
    /* Used for distinguish what type of AOTValue POP */
    WASMStructType *compile_time_struct_type =
        (WASMStructType *)comp_ctx->comp_data->types[type_index];
    WASMStructFieldType *fields = compile_time_struct_type->fields;
    uint32 field_count = compile_time_struct_type->field_count;
    uint8 field_type = fields[field_idx].field_type;

    if (field_idx >= field_count) {
        aot_set_last_error("struct field index out of bounds");
        goto fail;
    }

    if (wasm_is_type_reftype(field_type)) {
        POP_REF(field_value);
    }
    else if (field_type == VALUE_TYPE_I32 || field_type == VALUE_TYPE_F32
             || field_type == PACKED_TYPE_I8 || field_type == PACKED_TYPE_I16) {
        POP_I32(field_value);
    }
    else {
        POP_I64(field_value);
    }

    POP_REF(struct_obj);

    ADD_BASIC_BLOCK(check_struct_obj_succ, "check struct obj succ");
    MOVE_BLOCK_AFTER_CURR(check_struct_obj_succ);

    BUILD_ICMP(LLVMIntEQ, struct_obj, GC_REF_NULL, cmp, "cmp_struct_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_struct_obj_succ))
        goto fail;

    if (!aot_call_wasm_struct_obj_set_field(comp_ctx, func_ctx, struct_obj,
                                            I32_CONST(field_idx), field_value))
        goto fail;

    return true;
fail:
    return false;
}

bool
aot_compile_op_array_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 type_index, bool init_with_default,
                         bool fixed_size, uint32 array_len)
{
    LLVMValueRef array_length, rtt_type, array_elem, array_obj, cmp;
    LLVMBasicBlockRef check_rtt_type_succ, check_array_obj_succ;
    /* Used for distinguish what type of AOTValue POP */
    WASMArrayType *compile_time_array_type =
        (WASMArrayType *)comp_ctx->comp_data->types[type_index];
    uint8 array_elem_type = compile_time_array_type->elem_type;
    int32 elem_idx;

    /* Generate call wasm_rtt_type_new and check for exception */
    if (!aot_call_aot_rtt_type_new(comp_ctx, func_ctx, I32_CONST(type_index),
                                   &rtt_type))
        goto fail;

    ADD_BASIC_BLOCK(check_rtt_type_succ, "check rtt type succ");
    MOVE_BLOCK_AFTER_CURR(check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, rtt_type, GC_REF_NULL, cmp, "cmp_rtt_type");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_rtt_type_succ))
        goto fail;

    if (!fixed_size)
        POP_I32(array_length);
    else
        array_length = I32_CONST(array_len);

    /* For WASM_OP_ARRAY_NEW_CANON */
    if (!fixed_size && !init_with_default) {
        if (wasm_is_type_reftype(array_elem_type)) {
            POP_REF(array_elem);
        }
        else if (array_elem_type == VALUE_TYPE_I32
                 || array_elem_type == VALUE_TYPE_F32
                 || array_elem_type == PACKED_TYPE_I8
                 || array_elem_type == PACKED_TYPE_I16) {
            POP_I32(array_elem);
        }
        else {
            POP_I64(array_elem);
        }
    }

    /* Generate call wasm_array_obj_new and check for exception */
    if (!aot_call_wasm_array_obj_new(comp_ctx, func_ctx, rtt_type, array_length,
                                     array_elem, &array_obj))
        goto fail;

    ADD_BASIC_BLOCK(check_array_obj_succ, "check array obj succ");
    MOVE_BLOCK_AFTER(check_array_obj_succ, check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, array_obj, GC_REF_NULL, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_array_obj_succ))
        goto fail;

    if (fixed_size) {

        for (elem_idx = array_len - 1; elem_idx >= 0; elem_idx--) {

            if (wasm_is_type_reftype(array_elem_type)) {
                POP_REF(array_elem);
            }
            else if (array_elem_type == VALUE_TYPE_I32
                     || array_elem_type == VALUE_TYPE_F32
                     || array_elem_type == PACKED_TYPE_I8
                     || array_elem_type == PACKED_TYPE_I16) {
                POP_I32(array_elem);
            }
            else {
                POP_I64(array_elem);
            }

            if (!aot_call_wasm_array_set_elem(comp_ctx, func_ctx, array_obj,
                                              I32_CONST(elem_idx), array_elem))
                goto fail;
        }
    }

    PUSH_REF(array_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_array_new_data(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, uint32 type_index,
                              uint32 data_seg_index)
{
    LLVMValueRef array_length, data_seg_offset, rtt_type, elem_size, array_elem,
        array_obj, cmp;
    LLVMBasicBlockRef check_rtt_type_succ, check_array_obj_succ;
    /* Used for distinguish what type of element in array */
    WASMArrayType *compile_time_array_type =
        (WASMArrayType *)comp_ctx->comp_data->types[type_index];
    uint8 array_elem_type = compile_time_array_type->elem_type;

    /* Generate call wasm_rtt_type_new and check for exception */
    if (!aot_call_aot_rtt_type_new(comp_ctx, func_ctx, I32_CONST(type_index),
                                   &rtt_type))
        goto fail;

    ADD_BASIC_BLOCK(check_rtt_type_succ, "check rtt type succ");
    MOVE_BLOCK_AFTER_CURR(check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, rtt_type, GC_REF_NULL, cmp, "cmp_rtt_type");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_rtt_type_succ))
        goto fail;

    POP_I32(array_length);
    POP_I32(data_seg_offset);

    switch (array_elem_type) {
        case PACKED_TYPE_I8:
            elem_size = I32_ONE;
            break;
        case PACKED_TYPE_I16:
            elem_size = I32_TWO;
            break;
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            elem_size = I32_FOUR;
            break;
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            elem_size = I32_EIGHT;
            break;
        default:
            bh_assert(0);
    }

    /* Generate call wasm_array_obj_new and check for exception */
    if (elem_size == I32_EIGHT)
        array_elem = I64_ZERO;
    else
        array_elem = I32_ZERO;
    if (!aot_call_wasm_array_obj_new(comp_ctx, func_ctx, rtt_type, array_length,
                                     array_elem, &array_obj))
        goto fail;

    ADD_BASIC_BLOCK(check_array_obj_succ, "check array obj succ");
    MOVE_BLOCK_AFTER(check_array_obj_succ, check_rtt_type_succ);

    BUILD_ICMP(LLVMIntEQ, array_obj, GC_REF_NULL, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_array_obj_succ))
        goto fail;

    if (!aot_call_aot_array_init_with_data(
            comp_ctx, func_ctx, I32_CONST(data_seg_index), data_seg_offset,
            array_obj, elem_size, array_length))
        goto fail;

    PUSH_REF(array_obj);

    return true;
fail:
    return false;
}

/* array_obj->length >> WASM_ARRAY_LENGTH_SHIFT */
static bool
aot_get_array_obj_length(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         LLVMValueRef array_obj, LLVMValueRef *array_len)
{
    LLVMValueRef offset;

    if (!(offset = I32_CONST(offsetof(WASMArrayObject, length)))) {
        aot_set_last_error("llvm build const failed.");
        goto fail;
    }

    if (!(*array_len =
              LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, array_obj,
                                    &offset, 1, "array_obj_length_i8p"))) {
        aot_set_last_error("llvm build gep failed.");
        goto fail;
    }

    if (!(*array_len =
              LLVMBuildBitCast(comp_ctx->builder, *array_len, INT32_PTR_TYPE,
                               "array_obj_length_i32ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    if (!(*array_len = LLVMBuildLoad2(comp_ctx->builder, I32_TYPE, *array_len,
                                      "array_obj_length"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    if (!(*array_len = LLVMBuildLShr(comp_ctx->builder, *array_len,
                                     I32_CONST(WASM_ARRAY_LENGTH_SHIFT),
                                     "array_obj_length_shr"))) {
        aot_set_last_error("llvm build shr failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_array_get(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 type_index, bool sign)
{
    LLVMValueRef elem_idx, array_obj, cmp, array_len, array_elem_ptr,
        array_elem;
    LLVMTypeRef elem_type;
    LLVMBasicBlockRef check_array_obj_succ, check_boundary_succ;
    /* Used for distinguish what type of AOTValue PUSH */
    WASMArrayType *compile_time_array_type =
        (WASMArrayType *)comp_ctx->comp_data->types[type_index];
    uint8 array_elem_type = compile_time_array_type->elem_type;

    /* Get LLVM type based on array_elem_type */
    if (wasm_is_type_reftype(array_elem_type)) {
        elem_type = INT8_PTR_TYPE;
    }
    else if (array_elem_type == VALUE_TYPE_I32
             || array_elem_type == VALUE_TYPE_F32
             || array_elem_type == PACKED_TYPE_I8
             || array_elem_type == PACKED_TYPE_I16) {
        elem_type = I32_TYPE;
    }
    else {
        elem_type = I64_TYPE;
    }

    POP_I32(elem_idx);
    POP_REF(array_obj);

    ADD_BASIC_BLOCK(check_array_obj_succ, "check array obj succ");
    MOVE_BLOCK_AFTER_CURR(check_array_obj_succ);

    BUILD_ICMP(LLVMIntEQ, array_obj, GC_REF_NULL, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_array_obj_succ))
        goto fail;

    if (!aot_get_array_obj_length(comp_ctx, func_ctx, array_obj, &array_len))
        goto fail;

    ADD_BASIC_BLOCK(check_boundary_succ, "check boundary succ");
    MOVE_BLOCK_AFTER(check_boundary_succ, check_array_obj_succ);

    BUILD_ICMP(LLVMIntUGE, elem_idx, array_len, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx,
                            EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                            check_boundary_succ))
        goto fail;

    if (!(array_elem_ptr = LLVMBuildAlloca(comp_ctx->builder, elem_type,
                                           "array_elem_ptr"))) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }
    if (!(array_elem_ptr = LLVMBuildBitCast(comp_ctx->builder, array_elem_ptr,
                                            INT8_PTR_TYPE, "array_elem_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    if (!aot_call_wasm_array_get_elem(comp_ctx, func_ctx, array_obj, elem_idx,
                                      I8_CONST(sign), array_elem_ptr))
        goto fail;

    if (!(array_elem = LLVMBuildLoad2(comp_ctx->builder, elem_type,
                                      array_elem_ptr, ""))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    if (wasm_is_type_reftype(array_elem_type)) {
        PUSH_REF(array_elem);
    }
    else if (array_elem_type == VALUE_TYPE_I32
             || array_elem_type == VALUE_TYPE_F32
             || array_elem_type == PACKED_TYPE_I8
             || array_elem_type == PACKED_TYPE_I16) {
        PUSH_I32(array_elem);
    }
    else {
        PUSH_I64(array_elem);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_array_set(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 type_index)
{
    LLVMValueRef elem_idx, array_obj, cmp, array_len, array_elem;
    LLVMBasicBlockRef check_array_obj_succ, check_boundary_succ;
    /* Used for distinguish what type of AOTValue POP */
    WASMArrayType *compile_time_array_type =
        (WASMArrayType *)comp_ctx->comp_data->types[type_index];
    uint8 array_elem_type = compile_time_array_type->elem_type;

    /* Get LLVM type based on array_elem_type */
    if (wasm_is_type_reftype(array_elem_type)) {
        POP_REF(array_elem);
    }
    else if (array_elem_type == VALUE_TYPE_I32
             || array_elem_type == VALUE_TYPE_F32
             || array_elem_type == PACKED_TYPE_I8
             || array_elem_type == PACKED_TYPE_I16) {
        POP_I32(array_elem);
    }
    else {
        POP_I64(array_elem);
    }

    POP_I32(elem_idx);
    POP_REF(array_obj);

    ADD_BASIC_BLOCK(check_array_obj_succ, "check array obj succ");
    MOVE_BLOCK_AFTER_CURR(check_array_obj_succ);

    BUILD_ICMP(LLVMIntEQ, array_obj, GC_REF_NULL, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_array_obj_succ))
        goto fail;

    if (!aot_get_array_obj_length(comp_ctx, func_ctx, array_obj, &array_len))
        goto fail;

    ADD_BASIC_BLOCK(check_boundary_succ, "check boundary succ");
    MOVE_BLOCK_AFTER(check_boundary_succ, check_array_obj_succ);

    BUILD_ICMP(LLVMIntUGE, elem_idx, array_len, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx,
                            EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                            check_boundary_succ))
        goto fail;

    if (!aot_call_wasm_array_set_elem(comp_ctx, func_ctx, array_elem, elem_idx,
                                      array_elem)) {
        aot_set_last_error("llvm build alloca failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

// #if WASM_ENABLE_GC_BINARYEN != 0
bool
aot_compile_op_array_copy(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 type_index, uint32 src_type_index)
{
    LLVMValueRef len, src_offset, src_obj, dst_offset, dst_obj, array_len,
        cmp[4], boundary;
    LLVMBasicBlockRef check_objs_succ, len_gt_zero, end_block;
    int i;

    POP_I32(len);
    POP_I32(src_offset);
    POP_REF(src_obj);
    POP_I32(dst_offset);
    POP_REF(dst_obj);

    ADD_BASIC_BLOCK(check_objs_succ, "check array objs succ");
    MOVE_BLOCK_AFTER_CURR(check_objs_succ);

    BUILD_ICMP(LLVMIntEQ, src_obj, GC_REF_NULL, cmp[0], "cmp_src_obj");
    BUILD_ICMP(LLVMIntEQ, dst_obj, GC_REF_NULL, cmp[1], "cmp_dst_obj");

    /* src_obj is null or dst_obj is null, throw exception */
    if (!(cmp[0] = LLVMBuildOr(comp_ctx->builder, cmp[0], cmp[1], ""))) {
        aot_set_last_error("llvm build or failed.");
        goto fail;
    }

    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp[0],
                            check_objs_succ))
        goto fail;

    /* Create if block */
    ADD_BASIC_BLOCK(len_gt_zero, "len_gt_zero");
    MOVE_BLOCK_AFTER_CURR(len_gt_zero);

    /* Create else(end) block */
    ADD_BASIC_BLOCK(end_block, "end block");
    MOVE_BLOCK_AFTER_CURR(end_block);

    BUILD_ICMP(LLVMIntSGT, len, I32_ZERO, cmp[0], "cmp_len");
    BUILD_COND_BR(cmp[0], len_gt_zero, end_block);

    /* Move builder to len > 0 block */
    SET_BUILDER_POS(len_gt_zero);
    /* dst_offset > UINT32_MAX - len */
    if (!(boundary = LLVMBuildAdd(comp_ctx->builder, dst_offset, len, ""))) {
        aot_set_last_error("llvm build failed.");
        goto fail;
    }
    BUILD_ICMP(LLVMIntUGT, boundary, I32_CONST(UINT32_MAX), cmp[0],
               "boundary_check1");
    /* dst_offset + len > wasm_array_obj_length(dst_obj) */
    if (!aot_get_array_obj_length(comp_ctx, func_ctx, dst_obj, &array_len))
        goto fail;
    BUILD_ICMP(LLVMIntUGT, boundary, array_len, cmp[1], "boundary_check2");
    /* src_offset > UINT32_MAX - len */
    if (!(boundary = LLVMBuildAdd(comp_ctx->builder, src_offset, len, ""))) {
        aot_set_last_error("llvm build failed.");
        goto fail;
    }
    BUILD_ICMP(LLVMIntUGT, boundary, I32_CONST(UINT32_MAX), cmp[2],
               "boundary_check3");
    /* src_offset + len > wasm_array_obj_length(src_obj) */
    if (!aot_get_array_obj_length(comp_ctx, func_ctx, src_obj, &array_len))
        goto fail;
    BUILD_ICMP(LLVMIntUGT, boundary, array_len, cmp[3], "boundary_check4");

    /* logical or above 4 boundary checks */
    for (i = 1; i < 4; ++i) {
        if (!(cmp[0] = LLVMBuildOr(comp_ctx->builder, cmp[0], cmp[i], ""))) {
            aot_set_last_error("llvm build failed.");
            goto fail;
        }
    }

    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp[0],
                            end_block))
        goto fail;

    return true;
fail:
    return false;
}
// #endif

bool
aot_compile_op_array_len(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef array_obj, cmp, array_len;
    LLVMBasicBlockRef check_array_obj_succ;

    POP_REF(array_obj);

    ADD_BASIC_BLOCK(check_array_obj_succ, "check array obj succ");
    MOVE_BLOCK_AFTER_CURR(check_array_obj_succ);

    BUILD_ICMP(LLVMIntEQ, array_obj, GC_REF_NULL, cmp, "cmp_array_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true, cmp,
                            check_array_obj_succ))
        goto fail;

    if (!aot_get_array_obj_length(comp_ctx, func_ctx, array_obj, &array_len))
        goto fail;

    PUSH_I32(array_len);

    return true;
fail:
    return false;
}

#endif /* end of WASM_ENABLE_GC != 0 */
