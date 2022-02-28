/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_function.h"
#include "aot_emit_exception.h"
#include "aot_emit_control.h"
#include "aot_emit_table.h"
#include "../aot/aot_runtime.h"

#define ADD_BASIC_BLOCK(block, name)                                          \
    do {                                                                      \
        if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,        \
                                                    func_ctx->func, name))) { \
            aot_set_last_error("llvm add basic block failed.");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

static bool
create_func_return_block(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;

    /* Create function return block if it isn't created */
    if (!func_ctx->func_return_block) {
        if (!(func_ctx->func_return_block = LLVMAppendBasicBlockInContext(
                  comp_ctx->context, func_ctx->func, "func_ret"))) {
            aot_set_last_error("llvm add basic block failed.");
            return false;
        }

        /* Create return IR */
        LLVMPositionBuilderAtEnd(comp_ctx->builder,
                                 func_ctx->func_return_block);
        if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
            return false;
        }
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    return true;
}

/* Check whether there was exception thrown, if yes, return directly */
static bool
check_exception_thrown(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMBasicBlockRef block_curr, check_except_succ;
    LLVMValueRef value, cmp;

    /* Create function return block if it isn't created */
    if (!create_func_return_block(comp_ctx, func_ctx))
        return false;

    /* Load the first byte of aot_module_inst->cur_exception, and check
       whether it is '\0'. If yes, no exception was thrown. */
    if (!(value = LLVMBuildLoad(comp_ctx->builder, func_ctx->cur_exception,
                                "exce_value"))
        || !(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ, value, I8_ZERO,
                                 "cmp"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    /* Add check exection success block */
    if (!(check_except_succ = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_except_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        return false;
    }

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMMoveBasicBlockAfter(check_except_succ, block_curr);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp, check_except_succ,
                         func_ctx->func_return_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_except_succ);
    return true;
}

/* Check whether there was exception thrown, if yes, return directly */
static bool
check_call_return(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                  LLVMValueRef res)
{
    LLVMBasicBlockRef block_curr, check_call_succ;
    LLVMValueRef cmp;

    /* Create function return block if it isn't created */
    if (!create_func_return_block(comp_ctx, func_ctx))
        return false;

    if (!(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, res, I8_ZERO,
                              "cmp"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    /* Add check exection success block */
    if (!(check_call_succ = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_call_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        return false;
    }

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMMoveBasicBlockAfter(check_call_succ, block_curr);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp, check_call_succ,
                         func_ctx->func_return_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_call_succ);
    return true;
}

#if 0
static bool
call_aot_invoke_native_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef func_idx, AOTFuncType *aot_func_type,
                            LLVMTypeRef *param_types,
                            LLVMValueRef *param_values, uint32 param_count,
                            uint32 param_cell_num, LLVMTypeRef ret_type,
                            uint8 wasm_ret_type, LLVMValueRef *p_value_ret,
                            LLVMValueRef *p_res)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[4];
    LLVMTypeRef ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[4], value_ret = NULL, res;
    char buf[32], *func_name = "aot_invoke_native";
    uint32 i, cell_num = 0;

    /* prepare function type of aot_invoke_native */
    func_param_types[0] = comp_ctx->exec_env_type; /* exec_env */
    func_param_types[1] = I32_TYPE;                /* func_idx */
    func_param_types[2] = I32_TYPE;                /* argc */
    func_param_types[3] = INT32_PTR_TYPE;          /* argv */
    if (!(func_type =
              LLVMFunctionType(INT8_TYPE, func_param_types, 4, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    /* prepare function pointer */
    if (comp_ctx->is_jit_mode) {
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        /* JIT mode, call the function directly */
        if (!(func = I64_CONST((uint64)(uintptr_t)aot_invoke_native))
            || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else if (comp_ctx->is_indirect_mode) {
        int32 func_index;
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }
        func_index = aot_get_native_symbol_index(comp_ctx, func_name);
        if (func_index < 0) {
            return false;
        }
        if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->native_symbol,
                                             func_ptr_type, func_index))) {
            return false;
        }
    }
    else {
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func =
                     LLVMAddFunction(comp_ctx->module, func_name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return false;
        }
    }

    if (param_cell_num > 64) {
        aot_set_last_error("prepare native arguments failed: "
                           "maximum 64 parameter cell number supported.");
        return false;
    }

    /* prepare frame_lp */
    for (i = 0; i < param_count; i++) {
        if (!(elem_idx = I32_CONST(cell_num))
            || !(elem_ptr_type = LLVMPointerType(param_types[i], 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "%s%d", "elem", i);
        if (!(elem_ptr = LLVMBuildInBoundsGEP(
                  comp_ctx->builder, func_ctx->argv_buf, &elem_idx, 1, buf))
            || !(elem_ptr = LLVMBuildBitCast(comp_ctx->builder, elem_ptr,
                                             elem_ptr_type, buf))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }

        if (!(res = LLVMBuildStore(comp_ctx->builder, param_values[i],
                                   elem_ptr))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
        LLVMSetAlignment(res, 1);

        cell_num += wasm_value_type_cell_num(aot_func_type->types[i]);
    }

    func_param_values[0] = func_ctx->exec_env;
    func_param_values[1] = func_idx;
    func_param_values[2] = I32_CONST(param_cell_num);
    func_param_values[3] = func_ctx->argv_buf;

    if (!func_param_values[2]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_invoke_native() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func, func_param_values, 4,
                              "res"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    /* get function return value */
    if (wasm_ret_type != VALUE_TYPE_VOID) {
        if (!(ret_ptr_type = LLVMPointerType(ret_type, 0))) {
            aot_set_last_error("llvm add pointer type failed.");
            return false;
        }

        if (!(value_ret =
                  LLVMBuildBitCast(comp_ctx->builder, func_ctx->argv_buf,
                                   ret_ptr_type, "argv_ret"))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }
        if (!(*p_value_ret =
                  LLVMBuildLoad(comp_ctx->builder, value_ret, "value_ret"))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
    }

    *p_res = res;
    return true;
}
#endif

#if (WASM_ENABLE_DUMP_CALL_STACK != 0) || (WASM_ENABLE_PERF_PROFILING != 0)
static bool
call_aot_alloc_frame_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          LLVMValueRef func_idx)
{
    LLVMValueRef param_values[2], ret_value, value, func;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef frame_alloc_fail, frame_alloc_success;
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;

    param_types[0] = comp_ctx->exec_env_type;
    param_types[1] = I32_TYPE;
    ret_type = INT8_TYPE;

    GET_AOT_FUNCTION(aot_alloc_frame, 2);

    param_values[0] = func_ctx->exec_env;
    param_values[1] = func_idx;

    if (!(ret_value = LLVMBuildCall(comp_ctx->builder, func, param_values, 2,
                                    "call_aot_alloc_frame"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    if (!(ret_value = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGT, ret_value,
                                    I8_ZERO, "frame_alloc_ret"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    ADD_BASIC_BLOCK(frame_alloc_fail, "frame_alloc_fail");
    ADD_BASIC_BLOCK(frame_alloc_success, "frame_alloc_success");

    LLVMMoveBasicBlockAfter(frame_alloc_fail, block_curr);
    LLVMMoveBasicBlockAfter(frame_alloc_success, block_curr);

    if (!LLVMBuildCondBr(comp_ctx->builder, ret_value, frame_alloc_success,
                         frame_alloc_fail)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    /* If frame alloc failed, return this function
        so the runtime can catch the exception */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, frame_alloc_fail);
    if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, frame_alloc_success);

    return true;

fail:
    return false;
}

static bool
call_aot_free_frame_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[1], ret_value, value, func;
    LLVMTypeRef param_types[1], ret_type, func_type, func_ptr_type;

    param_types[0] = comp_ctx->exec_env_type;
    ret_type = INT8_TYPE;

    GET_AOT_FUNCTION(aot_free_frame, 1);

    param_values[0] = func_ctx->exec_env;

    if (!(ret_value = LLVMBuildCall(comp_ctx->builder, func, param_values, 1,
                                    "call_aot_free_frame"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    return true;
}
#endif /* end of (WASM_ENABLE_DUMP_CALL_STACK != 0) \
                 || (WASM_ENABLE_PERF_PROFILING != 0) */

static bool
check_stack_boundary(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                     uint32 callee_cell_num)
{
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef check_stack;
    LLVMValueRef callee_local_size, stack_bound, cmp;

    if (!(callee_local_size = I32_CONST(callee_cell_num * 4))) {
        aot_set_last_error("llvm build const failed.");
        return false;
    }

    if (!(stack_bound = LLVMBuildInBoundsGEP(
              comp_ctx->builder, func_ctx->native_stack_bound,
              &callee_local_size, 1, "stack_bound"))) {
        aot_set_last_error("llvm build inbound gep failed.");
        return false;
    }

    if (!(check_stack = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_stack"))) {
        aot_set_last_error("llvm add basic block failed.");
        return false;
    }

    LLVMMoveBasicBlockAfter(check_stack, block_curr);

    if (!(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntULT,
                              func_ctx->last_alloca, stack_bound, "cmp"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_NATIVE_STACK_OVERFLOW,
                            true, cmp, check_stack)) {
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_stack);
    return true;
}

#if WASM_ENABLE_DYNAMIC_LINKING != 0
bool
call_aot_resolve_and_call_import_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        LLVMValueRef import_func_id, AOTFuncType *aot_func_type,
                            LLVMTypeRef *param_types, LLVMValueRef *param_values,
                            uint32 param_count, uint32 param_cell_num,
                            LLVMTypeRef ret_type, uint8 wasm_ret_type,
                            LLVMValueRef *p_value_ret, LLVMValueRef *p_res)
{
#define PARAM_COUNT 4
    LLVMTypeRef func_type, ret_ptr_type, elem_ptr_type;
    LLVMTypeRef func_ptr_type, func_param_types[PARAM_COUNT];
    LLVMValueRef func, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[PARAM_COUNT], value_ret = NULL, res = NULL;
    char buf[32], *func_name = "aot_resolve_function";
    uint32 i = 0, cell_num = 0;

    /* prepare function type of aot_call_indirect */
    func_param_types[0] = comp_ctx->exec_env_type;  /* exec_env */
    func_param_types[1] = I32_TYPE;           /* import func id */
    func_param_types[2] = I32_TYPE;                 /* argc */
    func_param_types[3] = INT32_PTR_TYPE;           /* argv */
    if (!(func_type = LLVMFunctionType(INT8_TYPE, func_param_types, PARAM_COUNT, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    /* prepare function pointer */
    if (comp_ctx->is_jit_mode) {
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        /* JIT mode, call the function directly */
        if (!(func = I64_CONST((uint64)(uintptr_t)aot_resolve_function))
            || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else {
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module,
                                        func_name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return false;
        }
    }

    if (param_cell_num > 64) {
        aot_set_last_error("prepare native arguments failed: "
                           "maximum 64 parameter cell number supported.");
        return false;
    }

    /* prepare frame_lp */
    for (i = 0; i < param_count; i++) {
        if (!(elem_idx = I32_CONST(cell_num))
            || !(elem_ptr_type = LLVMPointerType(param_types[i], 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "%s%d", "elem", i);
        if (!(elem_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                              func_ctx->argv_buf, &elem_idx, 1, buf))
            || !(elem_ptr = LLVMBuildBitCast(comp_ctx->builder, elem_ptr,
                                             elem_ptr_type, buf))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }

        if (!(res = LLVMBuildStore(comp_ctx->builder, param_values[i], elem_ptr))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
        LLVMSetAlignment(res, 1);

        cell_num += wasm_value_type_cell_num(aot_func_type->types[i]);
    }

    func_param_values[0] = func_ctx->exec_env;
    func_param_values[1] = import_func_id;
    func_param_values[2] = I32_CONST(param_cell_num);
    func_param_values[3] = func_ctx->argv_buf;

    if (!func_param_values[2]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call wasm_program_resolve_aot_function() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func,
                              func_param_values, PARAM_COUNT, "res"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    /* get function return value */
    if (wasm_ret_type != VALUE_TYPE_VOID) {
        if (!(ret_ptr_type = LLVMPointerType(ret_type, 0))) {
            aot_set_last_error("llvm add pointer type failed.");
            return false;
        }

        if (!(value_ret = LLVMBuildBitCast(comp_ctx->builder, func_ctx->argv_buf,
                                           ret_ptr_type, "argv_ret"))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }
        if (!(*p_value_ret = LLVMBuildLoad(comp_ctx->builder, value_ret,
                                           "value_ret"))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
    }

    *p_res = res;
    return true;
#undef PARAM_COUNT
}
#endif

static LLVMValueRef
aot_emit_call_native(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                    uint32 func_idx, AOTFuncType *func_type,
                    LLVMTypeRef ret_type, int32 ext_ret_count,
                    LLVMTypeRef *param_types, LLVMValueRef *param_values,
                    int32 param_count)
{
    LLVMTypeRef func_ref_type;
    LLVMValueRef func, value_ret;

    if (!(func_ref_type =
            LLVMFunctionType(ret_type, param_types, param_count + 1, false))) {
        aot_set_last_error("llvm add function type failed.");
        return NULL;
    }

    func_ref_type = LLVMPointerType(func_ref_type, 0);

    if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->func_ptrs,
                                            func_ref_type, func_idx))) {
        return NULL;
    }

    if (!(value_ret =
                LLVMBuildCall(comp_ctx->builder, func, param_values,
                            (uint32)param_count + 1 + ext_ret_count,
                            (func_type->result_count > 0 ? "call" : "")))) {
        aot_set_last_error("LLVM build call failed.");
        return NULL;
    }

    return value_ret;
}

bool
aot_compile_op_call(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                    uint32 func_idx, bool tail_call)
{
    uint32 import_func_count = comp_ctx->comp_data->import_func_count;
    AOTImportFunc *import_funcs = comp_ctx->comp_data->import_funcs;
    uint32 func_count = comp_ctx->func_ctx_count;
    uint32 ext_ret_cell_num = 0, cell_num = 0;
    AOTFuncContext **func_ctxes = comp_ctx->func_ctxes;
    AOTFuncType *func_type;
    AOTFunc *aot_func;
    LLVMTypeRef *param_types = NULL, ret_type;
    LLVMTypeRef ext_ret_ptr_type;
    LLVMValueRef *param_values = NULL, func;
    LLVMValueRef import_func_idx;
    LLVMValueRef ext_ret, ext_ret_ptr, ext_ret_idx;
    LLVMValueRef value_ret_phi = NULL;
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    uint32 param_cell_num = 0;
    LLVMBasicBlockRef block_func_not_linked, block_func_linked, check_call_succ, check_except_succ;
    LLVMValueRef value_ret_incoming[2];
    LLVMValueRef result;
    LLVMValueRef if_func_linked, if_res_succ, const_zero_ref;
    LLVMTypeRef pointer_type;
#endif
    int32 i, j = 0, param_count, result_count, ext_ret_count;
    uint64 total_size;
    uint32 callee_cell_num;
    uint8 wasm_ret_type;
    uint8 *ext_ret_types = NULL;
    bool ret = false;
    char buf[32];

#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (comp_ctx->enable_thread_mgr) {
        if (!check_suspend_flags(comp_ctx, func_ctx))
            return false;
    }
#endif

    /* Check function index */
    if (func_idx >= import_func_count + func_count) {
        aot_set_last_error("Function index out of range.");
        return false;
    }

    /* Get function type */
    if (func_idx < import_func_count)
        func_type = import_funcs[func_idx].func_type;
    else
        func_type =
            func_ctxes[func_idx - import_func_count]->aot_func->func_type;


#if (WASM_ENABLE_DUMP_CALL_STACK != 0) || (WASM_ENABLE_PERF_PROFILING != 0)
    if (comp_ctx->enable_aux_stack_frame) {
        LLVMValueRef func_idx_const;

        if (!(func_idx_const = I32_CONST(func_idx))) {
            aot_set_last_error("llvm build const failed.");
            return false;
        }
        if (!call_aot_alloc_frame_func(comp_ctx, func_ctx, func_idx_const))
            return false;
    }
#endif

    /* Allocate memory for parameters.
     * Parameters layout:
     *   - exec env
     *   - wasm function's parameters
     *   - extra results'(except the first one) addresses
     */
    param_count = (int32)func_type->param_count;
    result_count = (int32)func_type->result_count;
    ext_ret_count = result_count > 1 ? result_count - 1 : 0;
    total_size =
        sizeof(LLVMValueRef) * (uint64)(param_count + 1 + ext_ret_count);
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("allocate memory failed.");
        return false;
    }

    /* First parameter is exec env */
    param_values[j++] = func_ctx->exec_env;

    /* Pop parameters from stack */
    for (i = param_count - 1; i >= 0; i--)
        POP(param_values[i + j], func_type->types[i]);

    /* Set parameters for multiple return values, the first return value
       is returned by function return value, and the other return values
       are returned by function parameters with pointer types */
    if (ext_ret_count > 0) {
        ext_ret_types = func_type->types + param_count + 1;
        ext_ret_cell_num = wasm_get_cell_num(ext_ret_types, ext_ret_count);
        if (ext_ret_cell_num > 64) {
            aot_set_last_error("prepare extra results's return "
                               "address arguments failed: "
                               "maximum 64 parameter cell number supported.");
            goto fail;
        }

        for (i = 0; i < ext_ret_count; i++) {
            if (!(ext_ret_idx = I32_CONST(cell_num))
                || !(ext_ret_ptr_type =
                         LLVMPointerType(TO_LLVM_TYPE(ext_ret_types[i]), 0))) {
                aot_set_last_error("llvm add const or pointer type failed.");
                goto fail;
            }

            snprintf(buf, sizeof(buf), "ext_ret%d_ptr", i);
            if (!(ext_ret_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                                     func_ctx->argv_buf,
                                                     &ext_ret_idx, 1, buf))) {
                aot_set_last_error("llvm build GEP failed.");
                goto fail;
            }
            snprintf(buf, sizeof(buf), "ext_ret%d_ptr_cast", i);
            if (!(ext_ret_ptr = LLVMBuildBitCast(comp_ctx->builder, ext_ret_ptr,
                                                 ext_ret_ptr_type, buf))) {
                aot_set_last_error("llvm build bit cast failed.");
                goto fail;
            }
            param_values[param_count + 1 + i] = ext_ret_ptr;
            cell_num += wasm_value_type_cell_num(ext_ret_types[i]);
        }
    }

    if (func_idx < import_func_count) {
        if (!(import_func_idx = I32_CONST(func_idx))) {
            aot_set_last_error("llvm build inbounds gep failed.");
            goto fail;
        }

        /* Initialize parameter types of the LLVM function */
        total_size = sizeof(LLVMTypeRef) * (uint64)(param_count + 1);
        if (total_size >= UINT32_MAX
            || !(param_types = wasm_runtime_malloc((uint32)total_size))) {
            aot_set_last_error("allocate memory failed.");
            goto fail;
        }

        j = 0;
        param_types[j++] = comp_ctx->exec_env_type;

        for (i = 0; i < param_count; i++)
            param_types[j++] = TO_LLVM_TYPE(func_type->types[i]);

        if (func_type->result_count) {
            wasm_ret_type = func_type->types[func_type->param_count];
            ret_type = TO_LLVM_TYPE(wasm_ret_type);
        }
        else {
            wasm_ret_type = VALUE_TYPE_VOID;
            ret_type = VOID_TYPE;
        }
#if WASM_ENABLE_DYNAMIC_LINKING != 0
        /* Get param cell number */
        param_cell_num = func_type->param_cell_num;

        //aot_call_debugtrap_intrinsic(comp_ctx, func_ctx);
        LLVMValueRef func_id_ref = I32_CONST(func_idx);
        if (!(func =
              LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->func_ptrs,
                                   &func_id_ref, 1, "func_ptr_tmp"))) {
            aot_set_last_error("llvm build inbounds gep failed.");
            goto fail;
        }

        if (!(func = LLVMBuildLoad(comp_ctx->builder, func, "func_ptr"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }

        if (comp_ctx->pointer_size == sizeof(uint64)) {
            pointer_type = I64_TYPE;
            const_zero_ref = I64_ZERO;
        } else {
            pointer_type = I32_TYPE;
            const_zero_ref = I32_ZERO;
        }

        func = LLVMBuildPtrToInt(comp_ctx->builder, func, pointer_type, "func_pointer_value");

        if (!(if_func_linked = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE,
                                    func, const_zero_ref, "is_func_linked"))) {
            aot_set_last_error("llvm build icmp failed.");
            goto fail;
        }

        if (!(block_func_not_linked =
                    LLVMAppendBasicBlockInContext(comp_ctx->context,
                                                func_ctx->func,
                                                "block_func_not_linked"))) {
            aot_set_last_error("llvm add basic block failed.");
            goto fail;
        }

        block_func_linked =
            LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_func_linked");

        if (!(check_call_succ = LLVMAppendBasicBlockInContext(
                comp_ctx->context, func_ctx->func, "check_call_succ"))) {
            aot_set_last_error("llvm add basic block failed.");
            goto fail;
        }

        LLVMMoveBasicBlockAfter(block_func_linked,
                            LLVMGetInsertBlock(comp_ctx->builder));
        LLVMMoveBasicBlockAfter(block_func_not_linked, block_func_linked);

        LLVMMoveBasicBlockAfter(check_call_succ, block_func_not_linked);

        if (!LLVMBuildCondBr(comp_ctx->builder, if_func_linked,
                                block_func_linked, block_func_not_linked)) {
            aot_set_last_error("llvm build cond br failed.");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, check_call_succ);

        if (wasm_ret_type != VALUE_TYPE_VOID) {
            if (!(value_ret_phi = LLVMBuildPhi(comp_ctx->builder,
                                                ret_type, "value_ret_phi"))) {
                aot_set_last_error("llvm build phi failed.");
                goto fail;
            }
        }

        /* Check whether exception was thrown when executing the function */
        if (!check_exception_thrown(comp_ctx, func_ctx))
            goto fail;

        check_except_succ = LLVMGetInsertBlock(comp_ctx->builder);

        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_func_linked);

        //aot_call_debugtrap_intrinsic(comp_ctx, func_ctx);

        value_ret_incoming[0] = aot_emit_call_native(comp_ctx, func_ctx,
                                                    func_idx, func_type,
                                                    ret_type, ext_ret_count,
                                                    param_types, param_values,
                                                    param_count);
        if (!value_ret_incoming[0])
            goto fail;

        if (wasm_ret_type != VALUE_TYPE_VOID) {
            LLVMAddIncoming(value_ret_phi, &value_ret_incoming[0], &block_func_linked, 1);
        }

        if (!LLVMBuildBr(comp_ctx->builder, check_call_succ)) {
            aot_set_last_error("llvm build br failed.");
            goto fail;
        }


        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_func_not_linked);
        //  lazy link other module's export func.
        if (!call_aot_resolve_and_call_import_func(comp_ctx, func_ctx,
                                                import_func_idx,
                                                func_type,
                                                param_types + 1, param_values + 1,
                                                param_count, param_cell_num,
                                                ret_type, wasm_ret_type, &value_ret_incoming[1],
                                                &result))
            goto fail;

        /* Create function return block if it isn't created */
        if (!create_func_return_block(comp_ctx, func_ctx))
            goto fail;

        if (!(if_res_succ = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, result, I8_ZERO,
                                "if_res_succ"))) {
            aot_set_last_error("llvm build icmp failed.");
            goto fail;
        }

        /* Create condition br */
        if (!LLVMBuildCondBr(comp_ctx->builder, if_res_succ, check_call_succ,
                            func_ctx->func_return_block)) {
            aot_set_last_error("llvm build cond br failed.");
            goto fail;
        }

        if (wasm_ret_type != VALUE_TYPE_VOID) {
            LLVMAddIncoming(value_ret_phi, &value_ret_incoming[1], &block_func_not_linked, 1);
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, check_except_succ);

#else
        value_ret_phi = aot_emit_call_native(comp_ctx, func_ctx,
                                            func_idx, func_type,
                                            ret_type, ext_ret_count,
                                            param_types, param_values,
                                            param_count);
        if (!value_ret_phi)
            goto fail;

        /* Check whether exception was thrown when executing the function */
        if (!check_exception_thrown(comp_ctx, func_ctx))
            goto fail;

#endif
    }
    else {
        if (comp_ctx->is_indirect_mode) {
            LLVMTypeRef func_ptr_type;

            if (!(func_ptr_type = LLVMPointerType(
                      func_ctxes[func_idx - import_func_count]->func_type,
                      0))) {
                aot_set_last_error("construct func ptr type failed.");
                goto fail;
            }
            if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->func_ptrs,
                                                 func_ptr_type, func_idx))) {
                goto fail;
            }
        }
        else {
            func = func_ctxes[func_idx - import_func_count]->func;
        }
        aot_func = func_ctxes[func_idx - import_func_count]->aot_func;
        callee_cell_num =
            aot_func->param_cell_num + aot_func->local_cell_num + 1;

        if (comp_ctx->enable_bound_check
            && !check_stack_boundary(comp_ctx, func_ctx, callee_cell_num))
            goto fail;

        /* Call the function */
        if (!(value_ret_phi =
                  LLVMBuildCall(comp_ctx->builder, func, param_values,
                                (uint32)param_count + 1 + ext_ret_count,
                                (func_type->result_count > 0 ? "call" : "")))) {
            aot_set_last_error("LLVM build call failed.");
            goto fail;
        }

        /* Set calling convention for the call with the func's calling
           convention */
        LLVMSetInstructionCallConv(value_ret_phi, LLVMGetFunctionCallConv(func));

        if (tail_call)
            LLVMSetTailCall(value_ret_phi, true);

        /* Check whether there was exception thrown when executing
           the function */
        if (!tail_call && !check_exception_thrown(comp_ctx, func_ctx))
            goto fail;
    }

    if (func_type->result_count > 0) {
        /* Push the first result to stack */
        PUSH(value_ret_phi, func_type->types[func_type->param_count]);
        /* Load extra result from its address and push to stack */
        for (i = 0; i < ext_ret_count; i++) {
            snprintf(buf, sizeof(buf), "func%d_ext_ret%d", func_idx, i);
            if (!(ext_ret =
                      LLVMBuildLoad(comp_ctx->builder,
                                    param_values[1 + param_count + i], buf))) {
                aot_set_last_error("llvm build load failed.");
                goto fail;
            }
            PUSH(ext_ret, ext_ret_types[i]);
        }
    }

#if (WASM_ENABLE_DUMP_CALL_STACK != 0) || (WASM_ENABLE_PERF_PROFILING != 0)
    if (comp_ctx->enable_aux_stack_frame) {
        if (!call_aot_free_frame_func(comp_ctx, func_ctx))
            goto fail;
    }
#endif

    ret = true;
fail:
    if (param_types)
        wasm_runtime_free(param_types);
    if (param_values)
        wasm_runtime_free(param_values);
    return ret;
}

static bool
call_aot_call_indirect_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            AOTFuncType *aot_func_type,
                            LLVMValueRef func_type_idx, LLVMValueRef table_idx,
                            LLVMValueRef table_elem_idx,
                            LLVMTypeRef *param_types,
                            LLVMValueRef *param_values, uint32 param_count,
                            uint32 param_cell_num, uint32 result_count,
                            uint8 *wasm_ret_types, LLVMValueRef *value_rets,
                            LLVMValueRef *p_res)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[6];
    LLVMTypeRef ret_type, ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, ret_idx, ret_ptr, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[6], res = NULL;
    char buf[32], *func_name = "aot_call_indirect";
    uint32 i, cell_num = 0, ret_cell_num, argv_cell_num;

    /* prepare function type of aot_call_indirect */
    func_param_types[0] = comp_ctx->exec_env_type; /* exec_env */
    func_param_types[1] = I32_TYPE;                /* table_idx */
    func_param_types[2] = I32_TYPE;                /* table_elem_idx */
    func_param_types[3] = I32_TYPE;                /* argc */
    func_param_types[4] = INT32_PTR_TYPE;          /* argv */
    if (!(func_type =
              LLVMFunctionType(INT8_TYPE, func_param_types, 5, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    /* prepare function pointer */
    if (comp_ctx->is_jit_mode) {
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        /* JIT mode, call the function directly */
        if (!(func = I64_CONST((uint64)(uintptr_t)aot_call_indirect))
            || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else if (comp_ctx->is_indirect_mode) {
        int32 func_index;
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }
        func_index = aot_get_native_symbol_index(comp_ctx, func_name);
        if (func_index < 0) {
            return false;
        }
        if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->native_symbol,
                                             func_ptr_type, func_index))) {
            return false;
        }
    }
    else {
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func =
                     LLVMAddFunction(comp_ctx->module, func_name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return false;
        }
    }

    ret_cell_num = wasm_get_cell_num(wasm_ret_types, result_count);
    argv_cell_num =
        param_cell_num > ret_cell_num ? param_cell_num : ret_cell_num;
    if (argv_cell_num > 64) {
        aot_set_last_error("prepare native arguments failed: "
                           "maximum 64 parameter cell number supported.");
        return false;
    }

    /* prepare frame_lp */
    for (i = 0; i < param_count; i++) {
        if (!(elem_idx = I32_CONST(cell_num))
            || !(elem_ptr_type = LLVMPointerType(param_types[i], 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "%s%d", "elem", i);
        if (!(elem_ptr = LLVMBuildInBoundsGEP(
                  comp_ctx->builder, func_ctx->argv_buf, &elem_idx, 1, buf))
            || !(elem_ptr = LLVMBuildBitCast(comp_ctx->builder, elem_ptr,
                                             elem_ptr_type, buf))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }

        if (!(res = LLVMBuildStore(comp_ctx->builder, param_values[i],
                                   elem_ptr))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
        LLVMSetAlignment(res, 1);

        cell_num += wasm_value_type_cell_num(aot_func_type->types[i]);
    }

    func_param_values[0] = func_ctx->exec_env;
    func_param_values[1] = table_idx;
    func_param_values[2] = table_elem_idx;
    func_param_values[3] = I32_CONST(param_cell_num);
    func_param_values[4] = func_ctx->argv_buf;

    if (!func_param_values[3]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_call_indirect() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func, func_param_values, 5,
                              "res"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    /* get function result values */
    cell_num = 0;
    for (i = 0; i < result_count; i++) {
        ret_type = TO_LLVM_TYPE(wasm_ret_types[i]);
        if (!(ret_idx = I32_CONST(cell_num))
            || !(ret_ptr_type = LLVMPointerType(ret_type, 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "argv_ret%d", i);
        if (!(ret_ptr = LLVMBuildInBoundsGEP(
                  comp_ctx->builder, func_ctx->argv_buf, &ret_idx, 1, buf))
            || !(ret_ptr = LLVMBuildBitCast(comp_ctx->builder, ret_ptr,
                                            ret_ptr_type, buf))) {
            aot_set_last_error("llvm build GEP or bit cast failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "ret%d", i);
        if (!(value_rets[i] = LLVMBuildLoad(comp_ctx->builder, ret_ptr, buf))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
        cell_num += wasm_value_type_cell_num(wasm_ret_types[i]);
    }

    *p_res = res;
    return true;
}

#if WASM_ENABLE_DYNAMIC_LINKING != 0
static bool
call_aot_call_indirect_with_type_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            AOTFuncType *aot_func_type, LLVMValueRef func_type_idx,
                            LLVMValueRef table_idx, LLVMValueRef table_elem_idx,
                            LLVMTypeRef *param_types, LLVMValueRef *param_values,
                            uint32 param_count, uint32 param_cell_num,
                            uint32 result_count, uint8 *wasm_ret_types,
                            LLVMValueRef *value_rets, LLVMValueRef *p_res)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[6];
    LLVMTypeRef ret_type, ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, ret_idx, ret_ptr, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[7], res = NULL;
    char buf[32], *func_name = "aot_call_indirect_with_type";
    uint32 i, cell_num = 0, ret_cell_num, argv_cell_num;

    /* prepare function type of aot_call_indirect_with_type */
    func_param_types[0] = comp_ctx->exec_env_type;  /* exec_env */
    func_param_types[1] = I32_TYPE;                 /* table_idx */
    func_param_types[2] = I32_TYPE;                 /* table_elem_idx */
    func_param_types[3] = I32_TYPE;                 /* type idx */
    func_param_types[4] = I32_TYPE;                 /* argc */
    func_param_types[5] = INT32_PTR_TYPE;           /* argv */
    if (!(func_type = LLVMFunctionType(INT8_TYPE, func_param_types, 6, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    /* prepare function pointer */
    if (comp_ctx->is_jit_mode) {
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        /* JIT mode, call the function directly */
        if (!(func = I64_CONST((uint64)(uintptr_t)aot_call_indirect_with_type))
            || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else {
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module,
                                        func_name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return false;
        }
    }

    ret_cell_num = wasm_get_cell_num(wasm_ret_types, result_count);
    argv_cell_num = param_cell_num > ret_cell_num ? param_cell_num : ret_cell_num;
    if (argv_cell_num > 64) {
        aot_set_last_error("prepare native arguments failed: "
                           "maximum 64 parameter cell number supported.");
        return false;
    }

    /* prepare frame_lp */
    for (i = 0; i < param_count; i++) {
        if (!(elem_idx = I32_CONST(cell_num))
            || !(elem_ptr_type = LLVMPointerType(param_types[i], 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "%s%d", "elem", i);
        if (!(elem_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                              func_ctx->argv_buf, &elem_idx, 1, buf))
            || !(elem_ptr = LLVMBuildBitCast(comp_ctx->builder, elem_ptr,
                                             elem_ptr_type, buf))) {
            aot_set_last_error("llvm build bit cast failed.");
            return false;
        }

        if (!(res = LLVMBuildStore(comp_ctx->builder, param_values[i], elem_ptr))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
        LLVMSetAlignment(res, 1);

        cell_num += wasm_value_type_cell_num(aot_func_type->types[i]);
    }

    func_param_values[0] = func_ctx->exec_env;
    func_param_values[1] = table_idx;
    func_param_values[2] = table_elem_idx;
    func_param_values[3] = func_type_idx;
    func_param_values[4] = I32_CONST(param_cell_num);
    func_param_values[5] = func_ctx->argv_buf;

    if (!func_param_values[4]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_call_indirect() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func,
                              func_param_values, 6, "res"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    /* get function result values */
    cell_num = 0;
    for (i = 0; i < result_count; i++) {
        ret_type = TO_LLVM_TYPE(wasm_ret_types[i]);
        if (!(ret_idx = I32_CONST(cell_num))
            || !(ret_ptr_type = LLVMPointerType(ret_type, 0))) {
            aot_set_last_error("llvm add const or pointer type failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "argv_ret%d", i);
        if (!(ret_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                             func_ctx->argv_buf, &ret_idx, 1, buf))
            || !(ret_ptr = LLVMBuildBitCast(comp_ctx->builder, ret_ptr,
                                            ret_ptr_type, buf))) {
            aot_set_last_error("llvm build GEP or bit cast failed.");
            return false;
        }

        snprintf(buf, sizeof(buf), "ret%d", i);
        if (!(value_rets[i] = LLVMBuildLoad(comp_ctx->builder, ret_ptr, buf))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
        cell_num += wasm_value_type_cell_num(wasm_ret_types[i]);
    }

    *p_res = res;

    return true;
}

#if 0
static bool
aot_compile_program_cache_resolve_result(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                                        LLVMValueRef program_inst_value, LLVMValueRef elem_idx,
                                        LLVMValueRef func_ptr, LLVMValueRef module_inst)
{
    LLVMValueRef const_cache_offset, const_func_ptr_offset, const_cached_aot_inst_offset;
    LLVMValueRef const_cache_entry_size, const_cache_line_size;
    LLVMValueRef update_cache_entry_offset, cache_entry_offset, cache_line_offset, resolve_caches_offset, cached_func_offset;
    LLVMValueRef resolve_id_offset, cached_inst_offset;
    LLVMValueRef cached_func_val_offset, cached_func_val;
    LLVMValueRef resolve_cache, mask, cache_line_index;
    LLVMValueRef program_inst, if_program_inst_exists, const_zero_ref;
    LLVMValueRef if_cache_entry_available;
    LLVMBasicBlockRef block_cache_resolve_result, block_call_func, block_check_second_entry, block_update_entry, block_reset_update_entry;
    LLVMTypeRef const_pointer_type;

    if (comp_ctx->pointer_size == sizeof(uint64)) {
        // note: for convenience, the offsets are hardcoded here according to target machine word size
        // in the future, they will be automatically generated before compilation.
        const_pointer_type = INT64_PTR_TYPE;
        const_cache_offset = I32_CONST(40);
        const_func_ptr_offset = I32_CONST(8);
        const_cached_aot_inst_offset = I32_CONST(16);
        const_zero_ref = I64_ZERO;
    } else {
        const_pointer_type = INT32_PTR_TYPE;
        const_cache_offset = I32_CONST(24);
        const_func_ptr_offset = I32_CONST(4);
        const_cached_aot_inst_offset = I32_CONST(8);
        const_zero_ref = I32_ZERO;
    }

    const_cache_entry_size = I32_CONST(sizeof(wasm_resolving_cache_entry));
    const_cache_line_size = I32_CONST(sizeof(wasm_resolving_cache_entry) * 2);

    block_cache_resolve_result =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_cache_resolve_result");
    block_call_func =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_call_func");
    block_update_entry =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_update_entry");
    LLVMMoveBasicBlockAfter(block_cache_resolve_result,
                            LLVMGetInsertBlock(comp_ctx->builder));
    LLVMMoveBasicBlockAfter(block_call_func,
                            block_cache_resolve_result);

    //aot_call_debugtrap_intrinsic(comp_ctx, func_ctx);

    if_program_inst_exists = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, program_inst_value, const_zero_ref, "if_program_inst_exists");
    LLVMBuildCondBr(comp_ctx->builder, if_program_inst_exists, block_cache_resolve_result, block_call_func);

    // block at which check first cache entry if available
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_cache_resolve_result);

    program_inst = LLVMBuildIntToPtr(comp_ctx->builder, program_inst_value, INT8_PTR_TYPE, "program_inst");

    if (!(resolve_caches_offset =
            LLVMBuildGEP(comp_ctx->builder, program_inst, &const_cache_offset, 1,
                         "resolving_cache_offset"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        return false;
    }

    if (!(resolve_caches_offset =
            LLVMBuildBitCast(comp_ctx->builder, resolve_caches_offset,
                            comp_ctx->basic_types.int8_pptr_type, "resolving_cache_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(resolve_cache = LLVMBuildLoad(comp_ctx->builder, resolve_caches_offset, "resolve_cache"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    mask = I32_CONST(PROGRAM_RESOLVING_CACHE_LINE_COUNT - 1);

    cache_line_index = LLVMBuildAnd(comp_ctx->builder, elem_idx, mask, "cache_line_index");

    cache_line_offset = LLVMBuildMul(comp_ctx->builder, cache_line_index, const_cache_line_size, "cache_line_rela_offset");

    //cache_line_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &offset, 1, "cache_line_offset");
    //cache_entry_offset = cache_line_offset;

    cached_func_offset = LLVMBuildAdd(comp_ctx->builder, cache_line_offset, const_func_ptr_offset, "cached_func_offset");
    cached_func_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_func_offset, 1, "cached_func_offset");

    if (!(cached_func_val_offset =
            LLVMBuildBitCast(comp_ctx->builder, cached_func_offset,
                            const_pointer_type, "cached_func_value_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(cached_func_val = LLVMBuildLoad(comp_ctx->builder, cached_func_val_offset, "cached_func_val"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    if (!(if_cache_entry_available = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    cached_func_val, const_zero_ref, "if_cache_entry_available"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    // block at which update cache entry
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_update_entry);

    update_cache_entry_offset = LLVMBuildPhi(comp_ctx->builder, I32_TYPE, "cache_entry_offset_phi");

    resolve_id_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &update_cache_entry_offset, 1, "cache_line_offset");
    if (!(resolve_id_offset =
            LLVMBuildBitCast(comp_ctx->builder, resolve_id_offset,
                            INT32_PTR_TYPE, "resolve_id_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }
    LLVMBuildStore(comp_ctx->builder, elem_idx, resolve_id_offset);

    cached_func_offset = LLVMBuildAdd(comp_ctx->builder, update_cache_entry_offset, const_func_ptr_offset, "cached_func_offset");
    cached_func_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_func_offset, 1, "cached_func_offset");
    if (!(cached_func_offset =
            LLVMBuildBitCast(comp_ctx->builder, cached_func_offset,
                            comp_ctx->basic_types.int8_pptr_type, "cached_func_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }
    LLVMBuildStore(comp_ctx->builder, func_ptr, cached_func_offset);

    cached_inst_offset = LLVMBuildAdd(comp_ctx->builder, update_cache_entry_offset, const_cached_aot_inst_offset, "cached_inst_offset");
    cached_inst_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_inst_offset, 1, "cached_inst_offset");
    if (!(cached_inst_offset =
            LLVMBuildBitCast(comp_ctx->builder, cached_inst_offset,
                            comp_ctx->basic_types.int8_pptr_type, "cached_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }
    LLVMBuildStore(comp_ctx->builder, module_inst, cached_inst_offset);

    LLVMBuildBr(comp_ctx->builder, block_call_func);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_cache_resolve_result);
    LLVMAddIncoming(update_cache_entry_offset, &cache_line_offset, &block_cache_resolve_result, 1);

    block_check_second_entry =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_check_second_entry");
    block_reset_update_entry =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_reset_update_entry");

    LLVMMoveBasicBlockAfter(block_check_second_entry,
                            LLVMGetInsertBlock(comp_ctx->builder));
    LLVMMoveBasicBlockAfter(block_reset_update_entry,
                            block_check_second_entry);
    LLVMMoveBasicBlockAfter(block_update_entry,
                            block_reset_update_entry);

    if (!LLVMBuildCondBr(comp_ctx->builder, if_cache_entry_available,
                            block_update_entry, block_check_second_entry)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    // block at which check second cache entry if available
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_check_second_entry);

    cache_entry_offset = LLVMBuildAdd(comp_ctx->builder, cache_line_offset, const_cache_entry_size, "next_cache_entry_offset");
    cached_func_offset = LLVMBuildAdd(comp_ctx->builder, cache_entry_offset, const_func_ptr_offset, "cached_func_offset");
    cached_func_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_func_offset, 1, "cached_func_offset");
    if (!(cached_func_val_offset =
            LLVMBuildBitCast(comp_ctx->builder, cached_func_offset,
                            const_pointer_type, "cached_func_value_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }
    if (!(cached_func_val = LLVMBuildLoad(comp_ctx->builder, cached_func_val_offset, "cached_func_val"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    LLVMAddIncoming(update_cache_entry_offset, &cache_entry_offset, &block_check_second_entry, 1);

    if (!(if_cache_entry_available = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    cached_func_val, const_zero_ref, "if_cache_entry_available"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }
    if (!LLVMBuildCondBr(comp_ctx->builder, if_cache_entry_available,
                            block_update_entry, block_reset_update_entry)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    // block at which reset update entry to first entry
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_reset_update_entry);
    LLVMAddIncoming(update_cache_entry_offset, &cache_line_offset, &block_reset_update_entry, 1);
    LLVMBuildBr(comp_ctx->builder, block_update_entry);

    // block at which call the real function.
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_call_func);
    return true;
}
#endif

static bool
aot_compile_program_op_call_indirect_cached_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                                                LLVMValueRef elem_idx, LLVMValueRef program_inst,
                                                LLVMTypeRef *param_types, LLVMValueRef *param_values,
                                                uint32 param_count, LLVMTypeRef ret_type, uint32 result_count,
                                                LLVMValueRef *result_phis,
                                                LLVMBasicBlockRef block_return)
{
    LLVMValueRef const_cache_offset, const_func_ptr_offset, offset;
    LLVMValueRef cache_entry_size, cache_line_size, cache_entry_offset, cache_line_offset, resolve_caches_offset, cached_func_offset;
    LLVMValueRef env_ref, env_aot_inst_offset, const_env_aot_inst_offset, cached_aot_inst_offset, const_cached_aot_inst_offset, saved_inst, cached_inst;
    LLVMValueRef resolve_cache, mask, cache_line_index, resolve_id;
    LLVMValueRef if_cache_hit;
    LLVMValueRef func, func_ptr, value_ret, ext_ret;
    LLVMTypeRef llvm_func_type, llvm_func_ptr_type;
    LLVMBasicBlockRef block_curr, block_cache_hit, block_cache_lookup_retry, block_resolving;
    uint32 i;
    char buf[128] = "";

    if (comp_ctx->pointer_size == sizeof(uint64)) {
        // note: for convenience, the offsets are hardcoded here according to target machine word size
        // in the future, they will be automatically generated before compilation.
        const_cache_offset = I32_CONST(40);
        const_func_ptr_offset = I32_CONST(8);
        const_env_aot_inst_offset = I32_CONST(16);
        const_cached_aot_inst_offset = I32_CONST(16);
    } else {
        const_cache_offset = I32_CONST(24);
        const_func_ptr_offset = I32_CONST(4);
        const_env_aot_inst_offset = I32_CONST(8);
        const_cached_aot_inst_offset = I32_CONST(8);
    }

    cache_entry_size = I32_CONST(sizeof(wasm_resolving_cache_entry));
    cache_line_size = I32_CONST(sizeof(wasm_resolving_cache_entry) * 2);

    if (!(resolve_caches_offset =
            LLVMBuildGEP(comp_ctx->builder, program_inst, &const_cache_offset, 1,
                         "resolving_cache_offset"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        return false;
    }

    if (!(resolve_caches_offset =
            LLVMBuildBitCast(comp_ctx->builder, resolve_caches_offset,
                            comp_ctx->basic_types.int8_pptr_type, "resolving_cache_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(resolve_cache = LLVMBuildLoad(comp_ctx->builder, resolve_caches_offset, "resolve_cache"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    mask = I32_CONST(PROGRAM_RESOLVING_CACHE_LINE_COUNT - 1);
    cache_line_index = LLVMBuildAnd(comp_ctx->builder, elem_idx, mask, "cache_line_index");

    offset = LLVMBuildMul(comp_ctx->builder, cache_line_index, cache_line_size, "cache_line_rela_offset");

    cache_line_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &offset, 1, "cache_line_offset");

    if (!(cache_line_offset =
            LLVMBuildBitCast(comp_ctx->builder, cache_line_offset,
                            INT32_PTR_TYPE, "resolve_id_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(resolve_id = LLVMBuildLoad(comp_ctx->builder, cache_line_offset, "resolve id"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    if (!(if_cache_hit = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    elem_idx, resolve_id, "if_cache_hit"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    block_cache_hit =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "cache_hit");
    block_cache_lookup_retry =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "cache_lookup_retry");
    block_resolving =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_resolving");

    LLVMMoveBasicBlockAfter(block_cache_hit,
                            LLVMGetInsertBlock(comp_ctx->builder));
    LLVMMoveBasicBlockAfter(block_cache_lookup_retry,
                            block_cache_hit);
    LLVMMoveBasicBlockAfter(block_resolving,
                            block_cache_lookup_retry);

    if (!LLVMBuildCondBr(comp_ctx->builder, if_cache_hit,
                            block_cache_hit, block_cache_lookup_retry)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_cache_lookup_retry);
    cache_entry_offset = LLVMBuildAdd(comp_ctx->builder, offset, cache_entry_size, "second_entry_offset");

    cache_entry_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cache_entry_offset, 1, "cache_func_ptr_offset");

    if (!(cache_entry_offset =
            LLVMBuildBitCast(comp_ctx->builder, cache_entry_offset,
                            INT32_PTR_TYPE, "resolve_id_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(resolve_id = LLVMBuildLoad(comp_ctx->builder, cache_entry_offset, "resolve id"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return false;
    }

    if (!(if_cache_hit = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    elem_idx, resolve_id, "if_cache_hit"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    if (!LLVMBuildCondBr(comp_ctx->builder, if_cache_hit,
                            block_cache_hit, block_resolving)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_cache_hit);

    cached_func_offset = LLVMBuildAdd(comp_ctx->builder, offset, const_func_ptr_offset, "cached_func_ptr_offset");

    cached_func_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_func_offset, 1, "cache_func_ptr_offset");

    cached_func_offset = LLVMBuildBitCast(comp_ctx->builder, cached_func_offset, comp_ctx->basic_types.int8_pptr_type, "func_ptr");

    if (!(func_ptr = LLVMBuildLoad(comp_ctx->builder, cached_func_offset, "func_ptr"))) {
        aot_set_last_error("llvm build load failed.");
        return false;
    }

    if (!(llvm_func_type =
              LLVMFunctionType(ret_type, param_types, param_count, false))
        || !(llvm_func_ptr_type = LLVMPointerType(llvm_func_type, 0))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    if (!(func = LLVMBuildBitCast(comp_ctx->builder, func_ptr,
                                  llvm_func_ptr_type, "indirect_func"))) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }

    env_ref = LLVMBuildBitCast(comp_ctx->builder, func_ctx->exec_env, INT8_PTR_TYPE, "env_ref");

    if (!(env_aot_inst_offset =
            LLVMBuildGEP(comp_ctx->builder, env_ref, &const_env_aot_inst_offset, 1,
                         "env_aot_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        return false;
    }

    if (!(env_aot_inst_offset =
            LLVMBuildBitCast(comp_ctx->builder, env_aot_inst_offset,
                            comp_ctx->basic_types.int8_pptr_type, "env_aot_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return false;
    }

    if (!(saved_inst = LLVMBuildLoad(comp_ctx->builder, env_aot_inst_offset, "old_inst"))) {
        aot_set_last_error("llvm build load failed.");
        return false;
    }

    cached_aot_inst_offset = LLVMBuildAdd(comp_ctx->builder, offset, const_cached_aot_inst_offset, "cached_aot_inst_offset");
    cached_aot_inst_offset = LLVMBuildGEP(comp_ctx->builder, resolve_cache, &cached_aot_inst_offset, 1, "cached_aot_inst_offset");
    cached_aot_inst_offset = LLVMBuildBitCast(comp_ctx->builder, cached_aot_inst_offset, comp_ctx->basic_types.int8_pptr_type, "cached_aot_inst_offset");
    if (!(cached_inst = LLVMBuildLoad(comp_ctx->builder, cached_aot_inst_offset, "cached_aot_inst"))) {
        aot_set_last_error("llvm build load failed.");
        return false;
    }
    //aot_call_debugtrap_intrinsic(comp_ctx, func_ctx);
    LLVMBuildStore(comp_ctx->builder, cached_inst, env_aot_inst_offset);

    if (!(value_ret = LLVMBuildCall(comp_ctx->builder, func, param_values,
                                    param_count,
                                    result_count > 0 ? "ret" : ""))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    /* Check whether exception was thrown when executing the function */
    if (!check_exception_thrown(comp_ctx, func_ctx))
        return false;

    LLVMBuildStore(comp_ctx->builder, saved_inst, env_aot_inst_offset);

    if (result_count > 0) {

        block_curr = LLVMGetInsertBlock(comp_ctx->builder);

        /* Push the first result to stack */
        LLVMAddIncoming(result_phis[0], &value_ret, &block_curr, 1);

        /* Load extra result from its address and push to stack */
        for (i = 1; i < result_count; i++) {
            snprintf(buf, sizeof(buf), "ext_ret%d", i - 1);
            if (!(ext_ret =
                      LLVMBuildLoad(comp_ctx->builder,
                                    param_values[param_count + i], buf))) {
                aot_set_last_error("llvm build load failed.");
                return false;
            }
            LLVMAddIncoming(result_phis[i], &ext_ret, &block_cache_hit, 1);
        }
    }

    if (!LLVMBuildBr(comp_ctx->builder, block_return)) {
        aot_set_last_error("llvm build br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_resolving);

    return true;
}

static LLVMValueRef
aot_compile_program_op_call_indirect(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                                    LLVMValueRef type_idx, AOTFuncType *func_type,
                                    LLVMValueRef elem_idx, LLVMValueRef *result_phis, LLVMValueRef elem_idx_phi,
                                    uint32 tbl_idx,
                                    LLVMTypeRef *param_types, LLVMValueRef *param_values,
                                    uint32 param_count, LLVMTypeRef ret_type,
                                    LLVMBasicBlockRef block_func, LLVMBasicBlockRef block_non_program_mode, LLVMBasicBlockRef block_return)
{
    LLVMValueRef offset, elem_idx_base, elem_idx_in_tbl;
    LLVMBasicBlockRef block_program_call_indirect, block_call_indirect_and_export, block_curr, block_call_local_tbl;
    LLVMValueRef program_offset, inst_id_offset, program_inst, program_inst_value, inst_id, inst_id_from_elem, if_program_inst_exists, if_call_local_table;
    LLVMValueRef *value_rets = NULL;
    LLVMValueRef tbl_idx_value, res, const_zero_ref;
    LLVMTypeRef pointer_type;
    uint32 param_cell_num;
    uint32 func_param_count, func_result_count;
    uint8 *wasm_ret_types;
    uint32 total_size = 0, i;
    bool ret = false;

    func_param_count = func_type->param_count;
    func_result_count = func_type->result_count;

    if (!(offset = I32_CONST(offsetof(AOTModuleInstance, program)))) {
        HANDLE_FAILURE("LLVMConstInt");
        return NULL;
    }

    if (!(program_offset =
            LLVMBuildGEP(comp_ctx->builder, func_ctx->aot_inst, &offset, 1,
                         "program_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        return NULL;
    }

    if (comp_ctx->pointer_size == sizeof(uint64)) {
        pointer_type = INT64_PTR_TYPE;
        const_zero_ref = I64_ZERO;
    } else {
        pointer_type = INT32_PTR_TYPE;
        const_zero_ref = I32_ZERO;
    }

    if (!(program_offset =
            LLVMBuildBitCast(comp_ctx->builder, program_offset,
                            pointer_type, "program_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return NULL;
    }

    if (!(program_inst_value = LLVMBuildLoad(comp_ctx->builder, program_offset, "program"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return NULL;
    }

    if (!(if_program_inst_exists = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    program_inst_value, const_zero_ref, "is_in_dlopen_mode"))) {
        aot_set_last_error("llvm build icmp failed.");
        return NULL;
    }

    //if (!(non_program_mode =
    //            LLVMAppendBasicBlockInContext(comp_ctx->context,
    //                                          func_ctx->func,
    //                                          "non_program_mode"))) {
    //    aot_set_last_error("llvm add basic block failed.");
    //    return false;
    //}

#if 0
    // old way to invoke block_call_import, reserved for debugging.
    if (!LLVMBuildCondBr(comp_ctx->builder, if_program_inst_exists,
                            block_non_program_mode, block_func)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }
#endif
//#if 0
    block_program_call_indirect =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "program_call_indirect");
    block_call_indirect_and_export =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "call_indirect_and_export");

    block_call_local_tbl =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_call_local_tbl");

    LLVMMoveBasicBlockAfter(block_program_call_indirect,
                            LLVMGetInsertBlock(comp_ctx->builder));

    LLVMMoveBasicBlockAfter(block_call_indirect_and_export,
                            block_program_call_indirect);

    LLVMMoveBasicBlockAfter(block_call_local_tbl,
                            block_call_indirect_and_export);

    if (!LLVMBuildCondBr(comp_ctx->builder, if_program_inst_exists,
                            block_non_program_mode, block_program_call_indirect)) {
        aot_set_last_error("llvm build cond br failed.");
        return NULL;
    }
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_program_call_indirect);

    //aot_call_debugtrap_intrinsic(comp_ctx, func_ctx);
    if (!(program_offset =
            LLVMBuildBitCast(comp_ctx->builder, program_offset,
                            comp_ctx->basic_types.int8_pptr_type, "program_inst_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return NULL;
    }

    if (!(program_inst = LLVMBuildLoad(comp_ctx->builder, program_offset, "program"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return NULL;
    }
#if 0
    if (!aot_compile_program_op_call_indirect_cached_func(comp_ctx, func_ctx,
                                                elem_idx, program_inst,
                                                param_types, param_values,
                                                param_count, ret_type, func_result_count,
                                                result_phis, block_return)) {
        return NULL;
    }
#endif
    if (!(offset = I32_CONST(offsetof(AOTModuleInstance, inst_id)))) {
        HANDLE_FAILURE("LLVMConstInt");
        return NULL;
    }

    if (!(inst_id_offset =
            LLVMBuildGEP(comp_ctx->builder, func_ctx->aot_inst, &offset, 1,
                         "inst_id_offset"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        return NULL;
    }

    if (!(inst_id_offset =
            LLVMBuildBitCast(comp_ctx->builder, inst_id_offset,
                            INT32_PTR_TYPE, "inst_id_offset"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        return NULL;
    }

    if (!(inst_id = LLVMBuildLoad(comp_ctx->builder, inst_id_offset, "inst_id"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        return NULL;
    }

    inst_id_from_elem = LLVMBuildAShr(comp_ctx->builder, elem_idx, I32_CONST(TABLE_SPACE_BITS_LEN), "inst_id_from_elem_id");
    inst_id_from_elem = LLVMBuildAdd(comp_ctx->builder, inst_id_from_elem, I32_ONE, "inst_id_from_elem_id");

    if (!(if_call_local_table = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                    inst_id_from_elem, inst_id, "if_call_local_table_func"))) {
        aot_set_last_error("llvm build icmp failed.");
        return NULL;
    }

    if (!LLVMBuildCondBr(comp_ctx->builder, if_call_local_table,
                            block_call_local_tbl, block_call_indirect_and_export)) {
        aot_set_last_error("llvm build cond br failed.");
        return NULL;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_call_indirect_and_export);

    if (!aot_compile_program_op_call_indirect_cached_func(comp_ctx, func_ctx,
                                                elem_idx, program_inst,
                                                param_types, param_values,
                                                param_count, ret_type, func_result_count,
                                                result_phis, block_return)) {
        return NULL;
    }

    /* Allocate memory for result values */
    if (func_result_count > 0) {
        total_size = sizeof(LLVMValueRef) * (uint64)func_result_count;
        if (total_size >= UINT32_MAX
            || !(value_rets = wasm_runtime_malloc((uint32)total_size))) {
            aot_set_last_error("allocate memory failed.");
            return NULL;
        }
        memset(value_rets, 0, (uint32)total_size);
    }

    param_cell_num = func_type->param_cell_num;
    wasm_ret_types = func_type->types + func_type->param_count;

    tbl_idx_value = I32_CONST(tbl_idx);
    if (!tbl_idx_value) {
        aot_set_last_error("llvm create const failed.");
        goto fail;
    }

    if (!call_aot_call_indirect_with_type_func(comp_ctx, func_ctx,
                                     func_type, type_idx,
                                     tbl_idx_value, elem_idx,
                                     param_types + 1, param_values + 1,
                                     func_param_count, param_cell_num,
                                     func_result_count, wasm_ret_types,
                                     value_rets, &res)) {
        goto fail;
    }

    /* Check whether exception was thrown when executing the function */
    if (!check_call_return(comp_ctx, func_ctx, res))
        goto fail;

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    for (i = 0; i < func_result_count; i++) {
        LLVMAddIncoming(result_phis[i], &value_rets[i], &block_curr, 1);
    }

    if (!LLVMBuildBr(comp_ctx->builder, block_return)) {
        aot_set_last_error("llvm build br failed.");
        goto fail;
    }
//#endif
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_call_local_tbl);

    inst_id = LLVMBuildAdd(comp_ctx->builder, inst_id, I32_NEG_ONE, "inst_idx");
    elem_idx_base = LLVMBuildMul(comp_ctx->builder, inst_id, I32_CONST(TABLE_SPACE_SLOT_SIZE), "elem_idx_base");
    elem_idx_in_tbl = LLVMBuildSub(comp_ctx->builder, elem_idx, elem_idx_base, "elem_idx_in_tbl");
    LLVMAddIncoming(elem_idx_phi, &elem_idx_in_tbl, &block_call_local_tbl, 1);
    LLVMBuildBr(comp_ctx->builder, block_non_program_mode);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_non_program_mode);
    ret = true;
fail:
    if (value_rets)
        wasm_runtime_free(value_rets);

    if (!ret)
        return NULL;

    return program_inst_value;
}
#endif

bool
aot_compile_op_call_indirect(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 type_idx, uint32 tbl_idx)
{
    AOTFuncType *func_type;
    LLVMValueRef tbl_idx_value, elem_idx, table_elem, func_idx;
    LLVMValueRef ftype_idx_ptr, ftype_idx, ftype_idx_const;
    LLVMValueRef cmp_elem_idx, cmp_func_idx, cmp_ftype_idx;
    LLVMValueRef func, func_ptr, table_size_const;
    LLVMValueRef ext_ret_offset, ext_ret_ptr, ext_ret, res;
    LLVMValueRef *param_values = NULL, *value_rets = NULL;
    LLVMValueRef *result_phis = NULL, value_ret, import_func_count;
    LLVMTypeRef *param_types = NULL, ret_type;
    LLVMTypeRef llvm_func_type, llvm_func_ptr_type;
    LLVMTypeRef ext_ret_ptr_type;
    LLVMBasicBlockRef check_elem_idx_succ, check_ftype_idx_succ;
    LLVMBasicBlockRef check_func_idx_succ, block_return, block_curr;
    LLVMBasicBlockRef block_call_import, block_call_non_import;
    LLVMValueRef offset;
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    LLVMValueRef program_inst_value, elem_idx_phi;//, origin_elem_idx;
    LLVMBasicBlockRef block_non_program_mode;
#endif
    uint32 total_param_count, func_param_count, func_result_count;
    uint32 ext_cell_num, param_cell_num, i, j;
    uint8 wasm_ret_type, *wasm_ret_types;
    uint64 total_size;
    char buf[32];
    bool ret = false;

    /* Check function type index */
    if (type_idx >= comp_ctx->comp_data->func_type_count) {
        aot_set_last_error("function type index out of range");
        return false;
    }

    /* Find the equivalent function type whose type index is the smallest:
       the callee function's type index is also converted to the smallest
       one in wasm loader, so we can just check whether the two type indexes
       are equal (the type index of call_indirect opcode and callee func),
       we don't need to check whether the whole function types are equal,
       including param types and result types. */
    type_idx = wasm_get_smallest_type_idx(comp_ctx->comp_data->func_types,
                                          comp_ctx->comp_data->func_type_count,
                                          type_idx);
    ftype_idx_const = I32_CONST(type_idx);
    CHECK_LLVM_CONST(ftype_idx_const);

    func_type = comp_ctx->comp_data->func_types[type_idx];
    func_param_count = func_type->param_count;
    func_result_count = func_type->result_count;

    POP_I32(elem_idx);

    block_call_import =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "call_import");
    block_call_non_import =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "call_non_import");
    block_return =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "func_return");
    if (!block_call_import || !block_call_non_import || !block_return) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(block_call_import,
                            LLVMGetInsertBlock(comp_ctx->builder));
    LLVMMoveBasicBlockAfter(block_call_non_import, block_call_import);
    LLVMMoveBasicBlockAfter(block_return, block_call_non_import);

    //prepare function parameters and returns

    /* Initialize parameter types of the LLVM function */
    total_param_count = 1 + func_param_count;

    /* Extra function results' addresses (except the first one) are
     * appended to aot function parameters. */
    if (func_result_count > 1)
      total_param_count += func_result_count - 1;

    total_size = sizeof(LLVMTypeRef) * (uint64)total_param_count;
    if (total_size >= UINT32_MAX
        || !(param_types = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("allocate memory failed.");
        goto fail;
    }

    /* Prepare param types */
    j = 0;
    param_types[j++] = comp_ctx->exec_env_type;
    for (i = 0; i < func_param_count; i++)
        param_types[j++] = TO_LLVM_TYPE(func_type->types[i]);

    for (i = 1; i < func_result_count; i++, j++) {
        param_types[j] =
            TO_LLVM_TYPE(func_type->types[func_param_count + i]);
        if (!(param_types[j] = LLVMPointerType(param_types[j], 0))) {
            aot_set_last_error("llvm get pointer type failed.");
            goto fail;
        }
    }

    /* Resolve return type of the LLVM function */
    if (func_result_count) {
        wasm_ret_type = func_type->types[func_param_count];
        ret_type = TO_LLVM_TYPE(wasm_ret_type);
    }
    else {
        wasm_ret_type = VALUE_TYPE_VOID;
        ret_type = VOID_TYPE;
    }

    /* Allocate memory for parameters */
    total_size = sizeof(LLVMValueRef) * (uint64)total_param_count;
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("allocate memory failed.");
        goto fail;
    }

    /* First parameter is exec env */
    j = 0;
    param_values[j++] = func_ctx->exec_env;

    /* Pop parameters from stack */
    for (i = func_param_count - 1; (int32)i >= 0; i--)
        POP(param_values[i + j], func_type->types[i]);

    /* Prepare extra parameters */
    ext_cell_num = 0;
    for (i = 1; i < func_result_count; i++) {
        ext_ret_offset = I32_CONST(ext_cell_num);
        CHECK_LLVM_CONST(ext_ret_offset);

        snprintf(buf, sizeof(buf), "ext_ret%d_ptr", i - 1);
        if (!(ext_ret_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                                 func_ctx->argv_buf,
                                                 &ext_ret_offset, 1, buf))) {
            aot_set_last_error("llvm build GEP failed.");
            goto fail;
        }

        ext_ret_ptr_type = param_types[func_param_count + i];
        snprintf(buf, sizeof(buf), "ext_ret%d_ptr_cast", i - 1);
        if (!(ext_ret_ptr = LLVMBuildBitCast(comp_ctx->builder,
                                             ext_ret_ptr, ext_ret_ptr_type,
                                             buf))) {
            aot_set_last_error("llvm build bit cast failed.");
            goto fail;
        }

        param_values[func_param_count + i] = ext_ret_ptr;
        ext_cell_num += wasm_value_type_cell_num(
                func_type->types[func_param_count + i]);
    }

    if (ext_cell_num > 64) {
        aot_set_last_error("prepare call-indirect arguments failed: "
                           "maximum 64 extra cell number supported.");
        goto fail;
    }

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);

    /* Add result phis for return block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_return);

    if (func_result_count > 0) {
        total_size = sizeof(LLVMValueRef) * (uint64)func_result_count;
        if (total_size >= UINT32_MAX
            || !(result_phis = wasm_runtime_malloc((uint32)total_size))) {
            aot_set_last_error("allocate memory failed.");
            goto fail;
        }
        memset(result_phis, 0, (uint32)total_size);
        for (i = 0; i < func_result_count; i++) {
            LLVMTypeRef tmp_type =
                TO_LLVM_TYPE(func_type->types[func_param_count + i]);
            if (!(result_phis[i] = LLVMBuildPhi(comp_ctx->builder,
                                                tmp_type, "phi"))) {
                aot_set_last_error("llvm build phi failed.");
                goto fail;
            }
        }
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);

#if WASM_ENABLE_DYNAMIC_LINKING != 0
    /* Add basic blocks */
    block_non_program_mode =
        LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                      "block_non_program_mode");
    LLVMMoveBasicBlockAfter(block_non_program_mode,
                            block_curr);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_non_program_mode);
    elem_idx_phi = LLVMBuildPhi(comp_ctx->builder, I32_TYPE, "phi");

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);

    LLVMAddIncoming(elem_idx_phi, &elem_idx, &block_curr, 1);

    if (!(program_inst_value = aot_compile_program_op_call_indirect(comp_ctx, func_ctx, ftype_idx_const, func_type, elem_idx, result_phis, elem_idx_phi,
                                        tbl_idx, param_types, param_values, total_param_count, ret_type,
                                        block_call_import, block_non_program_mode, block_return)))
        goto fail;

    // origin_elem_idx = elem_idx;
    elem_idx = elem_idx_phi;
#endif

    /* get the cur size of the table instance */
    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, cur_size)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(table_size_const =
            LLVMBuildGEP(comp_ctx->builder, func_ctx->aot_inst, &offset, 1,
                         "cur_size_i8p"))) {
        HANDLE_FAILURE("LLVMBuildGEP");
        goto fail;
    }

    if (!(table_size_const =
            LLVMBuildBitCast(comp_ctx->builder, table_size_const,
                             INT32_PTR_TYPE, "cur_siuze_i32p"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    if (!(table_size_const = LLVMBuildLoad(comp_ctx->builder, table_size_const, "cur_size"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        goto fail;
    }

    /* Check if (uint32)elem index >= table size */
    if (!(cmp_elem_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGE,
                                       elem_idx, table_size_const,
                                       "cmp_elem_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* Throw exception if elem index >= table size */
    if (!(check_elem_idx_succ =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "check_elem_idx_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_elem_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_UNDEFINED_ELEMENT, true,
                             cmp_elem_idx, check_elem_idx_succ)))
        goto fail;

    /* load data as i32* */
    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, data)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(table_elem = LLVMBuildGEP(comp_ctx->builder, func_ctx->aot_inst,
                                    &offset, 1, "table_elem_i8p"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }

    if (!(table_elem = LLVMBuildBitCast(comp_ctx->builder, table_elem,
                                        INT32_PTR_TYPE, "table_elem_i32p"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    /* Load function index */
    if (!(table_elem = LLVMBuildGEP(comp_ctx->builder, table_elem, &elem_idx, 1,
                                    "table_elem"))) {
        HANDLE_FAILURE("LLVMBuildNUWAdd");
        goto fail;
    }

    if (!(func_idx =
              LLVMBuildLoad(comp_ctx->builder, table_elem, "func_idx"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    /* Check if func_idx == -1 */
    if (!(cmp_func_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ, func_idx,
                                       I32_NEG_ONE, "cmp_func_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* Throw exception if func_idx == -1 */
    if (!(check_func_idx_succ = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_func_idx_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_func_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_UNINITIALIZED_ELEMENT,
                             true, cmp_func_idx, check_func_idx_succ)))
        goto fail;

    /* Load function type index */
    if (!(ftype_idx_ptr = LLVMBuildInBoundsGEP(
              comp_ctx->builder, func_ctx->func_type_indexes, &func_idx, 1,
              "ftype_idx_ptr"))) {
        aot_set_last_error("llvm build inbounds gep failed.");
        goto fail;
    }

    if (!(ftype_idx =
              LLVMBuildLoad(comp_ctx->builder, ftype_idx_ptr, "ftype_idx"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    /* Check if function type index not equal */
    if (!(cmp_ftype_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, ftype_idx,
                                        ftype_idx_const, "cmp_ftype_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* Throw exception if ftype_idx != ftype_idx_const */
    if (!(check_ftype_idx_succ = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_ftype_idx_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_ftype_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx,
                             EXCE_INVALID_FUNCTION_TYPE_INDEX, true,
                             cmp_ftype_idx, check_ftype_idx_succ)))
        goto fail;

#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (comp_ctx->enable_thread_mgr) {
        if (!check_suspend_flags(comp_ctx, func_ctx))
            goto fail;
    }
#endif

#if (WASM_ENABLE_DUMP_CALL_STACK != 0) || (WASM_ENABLE_PERF_PROFILING != 0)
    if (comp_ctx->enable_aux_stack_frame) {
        if (!call_aot_alloc_frame_func(comp_ctx, func_ctx, func_idx))
            goto fail;
    }
#endif

    import_func_count = I32_CONST(comp_ctx->comp_data->import_func_count);
    CHECK_LLVM_CONST(import_func_count);

    /* Check if func_idx < import_func_count */
    if (!(cmp_func_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntULT, func_idx,
                                       import_func_count, "cmp_func_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* If func_idx < import_func_count, jump to call import block,
       else jump to call non-import block */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp_func_idx, block_call_import,
                         block_call_non_import)) {
        aot_set_last_error("llvm build cond br failed.");
        goto fail;
    }

    /* Translate call import block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_call_import);

    /* Allocate memory for result values */
    if (func_result_count > 0) {
        total_size = sizeof(LLVMValueRef) * (uint64)func_result_count;
        if (total_size >= UINT32_MAX
            || !(value_rets = wasm_runtime_malloc((uint32)total_size))) {
            aot_set_last_error("allocate memory failed.");
            goto fail;
        }
        memset(value_rets, 0, (uint32)total_size);
    }

    param_cell_num = func_type->param_cell_num;
    wasm_ret_types = func_type->types + func_type->param_count;

    tbl_idx_value = I32_CONST(tbl_idx);
    if (!tbl_idx_value) {
        aot_set_last_error("llvm create const failed.");
        goto fail;
    }

    if (!call_aot_call_indirect_func(
            comp_ctx, func_ctx, func_type, ftype_idx, tbl_idx_value, elem_idx,
            param_types + 1, param_values + 1, func_param_count, param_cell_num,
            func_result_count, wasm_ret_types, value_rets, &res))
        goto fail;

    /* Check whether exception was thrown when executing the function */
    if (!check_call_return(comp_ctx, func_ctx, res))
        goto fail;

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    for (i = 0; i < func_result_count; i++) {
        LLVMAddIncoming(result_phis[i], &value_rets[i], &block_curr, 1);
    }

    if (!LLVMBuildBr(comp_ctx->builder, block_return)) {
        aot_set_last_error("llvm build br failed.");
        goto fail;
    }

    /* Translate call non-import block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_call_non_import);

    if (comp_ctx->enable_bound_check
        && !check_stack_boundary(comp_ctx, func_ctx,
                                 param_cell_num + ext_cell_num
                                     + 1
                                     /* Reserve some local variables */
                                     + 16))
        goto fail;

    /* Load function pointer */
    if (!(func_ptr =
              LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->func_ptrs,
                                   &func_idx, 1, "func_ptr_tmp"))) {
        aot_set_last_error("llvm build inbounds gep failed.");
        goto fail;
    }

    if (!(func_ptr = LLVMBuildLoad(comp_ctx->builder, func_ptr, "func_ptr"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    if (!(llvm_func_type =
              LLVMFunctionType(ret_type, param_types, total_param_count, false))
        || !(llvm_func_ptr_type = LLVMPointerType(llvm_func_type, 0))) {
        aot_set_last_error("llvm add function type failed.");
        goto fail;
    }

    if (!(func = LLVMBuildBitCast(comp_ctx->builder, func_ptr,
                                  llvm_func_ptr_type, "indirect_func"))) {
        aot_set_last_error("llvm build bit cast failed.");
        goto fail;
    }

#if WASM_ENABLE_DYNAMIC_LINKING != 0
    // seems in aot mode, resolve cache is not helpful for local table function.
    //aot_compile_program_cache_resolve_result(comp_ctx, func_ctx, program_inst_value, origin_elem_idx, func_ptr, func_ctx->aot_inst);
#endif
    if (!(value_ret = LLVMBuildCall(comp_ctx->builder, func, param_values,
                                    total_param_count,
                                    func_result_count > 0 ? "ret" : ""))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    /* Check whether exception was thrown when executing the function */
    if (!check_exception_thrown(comp_ctx, func_ctx))
        goto fail;

    if (func_result_count > 0) {
        block_curr = LLVMGetInsertBlock(comp_ctx->builder);

        /* Push the first result to stack */
        LLVMAddIncoming(result_phis[0], &value_ret, &block_curr, 1);

        /* Load extra result from its address and push to stack */
        for (i = 1; i < func_result_count; i++) {
            snprintf(buf, sizeof(buf), "ext_ret%d", i - 1);
            if (!(ext_ret =
                      LLVMBuildLoad(comp_ctx->builder,
                                    param_values[func_param_count + i], buf))) {
                aot_set_last_error("llvm build load failed.");
                goto fail;
            }
            LLVMAddIncoming(result_phis[i], &ext_ret, &block_curr, 1);
        }
    }

    if (!LLVMBuildBr(comp_ctx->builder, block_return)) {
        aot_set_last_error("llvm build br failed.");
        goto fail;
    }

    /* Translate function return block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_return);

    for (i = 0; i < func_result_count; i++) {
        PUSH(result_phis[i], func_type->types[func_param_count + i]);
    }

#if (WASM_ENABLE_DUMP_CALL_STACK != 0) || (WASM_ENABLE_PERF_PROFILING != 0)
    if (comp_ctx->enable_aux_stack_frame) {
        if (!call_aot_free_frame_func(comp_ctx, func_ctx))
            goto fail;
    }
#endif

    ret = true;

fail:
    if (param_values)
        wasm_runtime_free(param_values);
    if (param_types)
        wasm_runtime_free(param_types);
    if (value_rets)
        wasm_runtime_free(value_rets);
    if (result_phis)
        wasm_runtime_free(result_phis);
    return ret;
}

bool
aot_compile_op_ref_null(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    PUSH_I32(REF_NULL);

    return true;
fail:
    return false;
}

bool
aot_compile_op_ref_is_null(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef lhs, res;

    POP_I32(lhs);

    if (!(res = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ, lhs, REF_NULL,
                              "cmp_w_null"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(res = LLVMBuildZExt(comp_ctx->builder, res, I32_TYPE, "r_i"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    PUSH_I32(res);

    return true;
fail:
    return false;
}

bool
aot_compile_op_ref_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 func_idx)
{
    LLVMValueRef ref_idx;

    if (!(ref_idx = I32_CONST(func_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    PUSH_I32(ref_idx);

    return true;
fail:
    return false;
}
