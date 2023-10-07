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

#define ADD_BASIC_BLOCK(block, name)                                          \
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

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_obj_is_instance_of, 3);
    else
        GET_AOT_FUNCTION(aot_obj_is_instance_of, 3);

    /* Call function aot_obj_is_instance_of() or llvm_jit_obj_is_instance_of()
     */
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
aot_compile_op_ref_as_non_null(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    LLVMValueRef gc_obj, cmp_gc_obj;
    LLVMBasicBlockRef check_gc_obj_succ;

    GET_REF_FROM_STACK(gc_obj);

    /* Check if gc object is NULL */
    BUILD_ISNULL(gc_obj, cmp_gc_obj, "cmp_gc_obj");

    ADD_BASIC_BLOCK(check_gc_obj_succ, "check_gc_obj_succ");
    MOVE_BLOCK_AFTER_CURR(check_gc_obj_succ);

    /*  Throw exception if it is NULL */
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

    /* Equivalent to wasm_i31_obj_new: ((i31_value << 1) | 1) */
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

    /* if uintptr_t is 64 bits, extend i32 to i64, equivalent to:
     * (WASMI31ObjectRef)((i31_value << 1) | 1)  */
    if (comp_ctx->pointer_size == 8) {
        if (!(i31_obj = LLVMBuildZExt(comp_ctx->builder, i31_obj, I64_TYPE,
                                      "extend i32 to uintptr_t"))) {
            aot_set_last_error("llvm build zext failed.");
            goto fail;
        }
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

    ADD_BASIC_BLOCK(check_i31_obj_succ, "check_i31_obj_succ");
    MOVE_BLOCK_AFTER_CURR(check_i31_obj_succ);

    /* Check if i31 object is NULL, throw exception if it is */
    BUILD_ISNULL(i31_obj, cmp_i31_obj, "cmp_i31_obj");
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NULL_GC_REF, true,
                            cmp_i31_obj, check_i31_obj_succ))
        goto fail;

    /* if uintptr_t is 64 bits, trunc i64 to i32 */
    if (comp_ctx->pointer_size == 8) {
        if (!(i31_val = LLVMBuildTrunc(comp_ctx->builder, i31_obj, I32_TYPE,
                                       "trunc uintptr_t to i32"))) {
            aot_set_last_error("llvm build trunc failed.");
            goto fail;
        }
    }
    else {
        i31_val = i31_obj;
    }

    /* i31_val = i31_val >> 1 */
    if (!(i31_val = LLVMBuildLShr(comp_ctx->builder, i31_val, I32_ONE,
                                  "i31_value"))) {
        aot_set_last_error("llvm build lshr failed.");
        goto fail;
    }

    if (sign) {
        /* i31_val = i31_val & 0x40000000 ? i31_val |= 0x80000000 : i31_val */
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

    GET_REF_FROM_STACK(gc_obj);

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
    BUILD_ISNULL(gc_obj, cmp, "cmp_gc_obj");
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
        /* For WASM_OP_REF_CAST, exception */
        if (!nullable
            && !aot_emit_exception(comp_ctx, func_ctx, EXCE_TYPE_NONCASTABLE,
                                   false, NULL, NULL))
            goto fail;
        /* For WASM_OP_REF_CAST_NULLABLE, do nothing */
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
        /* For WASM_OP_REF_CAST and WASM_OP_REF_CAST_NULLABLE */
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

static bool
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
    BUILD_ISNULL(externref_obj, cmp, "cmp_externref_obj");
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

static bool
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

    GET_AOT_FUNCTION(wasm_internal_obj_to_externref_obj, 1);

    /* Call function wasm_internal_obj_to_externref_obj() */
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
    BUILD_ISNULL(gc_obj, cmp, "cmp_gc_obj");
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
    BUILD_ISNULL(externref_obj, cmp, "cmp_externref_obj");
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

#endif /* end of WASM_ENABLE_GC != 0 */
