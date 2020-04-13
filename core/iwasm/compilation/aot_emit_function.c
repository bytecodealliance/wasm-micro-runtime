/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_function.h"
#include "aot_emit_exception.h"
#include "aot_emit_control.h"
#include "../aot/aot_runtime.h"

static bool
create_func_return_block(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;

    /* Create function return block if it isn't created */
    if (!func_ctx->func_return_block) {
        if (!(func_ctx->func_return_block =
                    LLVMAppendBasicBlockInContext(comp_ctx->context,
                                                  func_ctx->func, "func_ret"))) {
            aot_set_last_error("llvm add basic block failed.");
            return false;
        }

        /* Create return IR */
        LLVMPositionBuilderAtEnd(comp_ctx->builder, func_ctx->func_return_block);
        if (aot_func_type->result_count) {
            switch (aot_func_type->types[aot_func_type->param_count]) {
                case VALUE_TYPE_I32:
                    LLVMBuildRet(comp_ctx->builder, I32_ZERO);
                    break;
                case VALUE_TYPE_I64:
                    LLVMBuildRet(comp_ctx->builder, I64_ZERO);
                    break;
                case VALUE_TYPE_F32:
                    LLVMBuildRet(comp_ctx->builder, F32_ZERO);
                    break;
                case VALUE_TYPE_F64:
                    LLVMBuildRet(comp_ctx->builder, F64_ZERO);
                    break;
            }
        }
        else {
            LLVMBuildRetVoid(comp_ctx->builder);
        }
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    return true;
}

/* Check whether there was exception thrown, if yes, return directly */
static bool
check_exception_thrown(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMBasicBlockRef block_curr, check_exce_succ;
    LLVMValueRef value, cmp;

    /* Create function return block if it isn't created */
    if (!create_func_return_block(comp_ctx, func_ctx))
        return false;

    /* Load the first byte of aot_module_inst->cur_exception, and check
       whether it is '\0'. If yes, no exception was thrown. */
    if (!(value = LLVMBuildLoad(comp_ctx->builder, func_ctx->cur_exception,
                                 "exce_value"))
        || !(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                  value, I8_ZERO, "cmp"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    /* Add check exection success block */
    if (!(check_exce_succ = LLVMAppendBasicBlockInContext(comp_ctx->context,
                                                          func_ctx->func,
                                                          "check_exce_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        return false;
    }

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMMoveBasicBlockAfter(check_exce_succ, block_curr);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp,
                         check_exce_succ, func_ctx->func_return_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_exce_succ);
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

    if (!(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE,
                              res, I8_ZERO, "cmp"))) {
        aot_set_last_error("llvm build icmp failed.");
        return false;
    }

    /* Add check exection success block */
    if (!(check_call_succ = LLVMAppendBasicBlockInContext(comp_ctx->context,
                                                          func_ctx->func,
                                                          "check_exce_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        return false;
    }

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMMoveBasicBlockAfter(check_call_succ, block_curr);

    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp,
                         check_call_succ, func_ctx->func_return_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_call_succ);
    return true;
}

static bool
call_aot_invoke_native_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef func_idx, AOTFuncType *aot_func_type,
                            LLVMTypeRef *param_types, LLVMValueRef *param_values,
                            uint32 param_count, uint32 param_cell_num,
                            LLVMTypeRef ret_type, uint8 wasm_ret_type,
                            LLVMValueRef *p_value_ret, LLVMValueRef *p_res)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[4];
    LLVMTypeRef ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[4], value_ret = NULL, res;
    char buf[32], *func_name = "aot_invoke_native";
    uint32 i, cell_num = 0;

    /* prepare function type of aot_invoke_native */
    func_param_types[0] = comp_ctx->exec_env_type;  /* exec_env */
    func_param_types[1] = I32_TYPE;                 /* func_idx */
    func_param_types[2] = I32_TYPE;                 /* argc */
    func_param_types[3] = INT32_PTR_TYPE;           /* argv */
    if (!(func_type = LLVMFunctionType(INT8_TYPE, func_param_types, 4, false))) {
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
    else {
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module,
                                        func_name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return false;
        }
    }

    if (param_count > 64) {
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
    func_param_values[1] = func_idx;
    func_param_values[2] = I32_CONST(param_cell_num);
    func_param_values[3] = func_ctx->argv_buf;

    if (!func_param_values[2]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_invoke_native() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func,
                              func_param_values, 4, "res"))) {
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
}

bool
aot_compile_op_call(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                    uint32 func_idx, uint8 **p_frame_ip)
{
    uint32 import_func_count = comp_ctx->comp_data->import_func_count;
    AOTImportFunc *import_funcs = comp_ctx->comp_data->import_funcs;
    uint32 func_count = comp_ctx->func_ctx_count, param_cell_num = 0;
    AOTFuncContext **func_ctxes = comp_ctx->func_ctxes;
    AOTFuncType *func_type;
    LLVMTypeRef *param_types = NULL, ret_type;
    LLVMValueRef *param_values = NULL, value_ret = NULL, func;
    LLVMValueRef import_func_idx, res;
    int32 i, j = 0, param_count;
    uint64 total_size;
    uint8 wasm_ret_type;
    bool ret = false;

    /* Check function index */
    if (func_idx >= import_func_count + func_count) {
        aot_set_last_error("Function index out of range.");
        return false;
    }

    /* Get function type */
    if (func_idx < import_func_count)
        func_type = import_funcs[func_idx].func_type;
    else
        func_type = func_ctxes[func_idx - import_func_count]->
                                                aot_func->func_type;

    /* Get param cell number */
    param_cell_num = wasm_type_param_cell_num(func_type);

    /* Allocate memory for parameters */
    param_count = (int32)func_type->param_count;
    total_size = sizeof(LLVMValueRef) * (uint64)(param_count + 1);
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("Allocate memory failed.");
        return false;
    }

    /* First parameter is exec env */
    param_values[j++] = func_ctx->exec_env;

    /* Pop parameters from stack */
    for (i = param_count - 1; i >= 0; i--)
        POP(param_values[i + j], func_type->types[i]);

    if (func_idx < import_func_count) {
        if (!(import_func_idx = I32_CONST(func_idx))) {
            aot_set_last_error("llvm build inbounds gep failed.");
            goto fail;
        }

        /* Initialize parameter types of the LLVM function */
        total_size = sizeof(LLVMTypeRef) * (uint64)(param_count + 1);
        if (total_size >= UINT32_MAX
            || !(param_types = wasm_runtime_malloc((uint32)total_size))) {
            aot_set_last_error("Allocate memory failed.");
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

        /* call aot_invoke_native() */
        if (!call_aot_invoke_native_func(comp_ctx, func_ctx, import_func_idx, func_type,
                                         param_types + 1, param_values + 1,
                                         param_count, param_cell_num,
                                         ret_type, wasm_ret_type, &value_ret, &res))
            goto fail;

        /* Check whether there was exception thrown when executing the function */
        if (!check_call_return(comp_ctx, func_ctx, res))
            goto fail;
    }
    else {
        func = func_ctxes[func_idx - import_func_count]->func;

        /* Call the function */
        if (!(value_ret = LLVMBuildCall(comp_ctx->builder, func,
                                        param_values, (uint32)param_count + 1,
                                        (func_type->result_count > 0
                                         ? "call" : "")))) {
            aot_set_last_error("LLVM build call failed.");
            goto fail;
        }

        /* Set calling convention for the call with the func's calling convention */
        LLVMSetInstructionCallConv(value_ret, LLVMGetFunctionCallConv(func));

        /* Check whether there was exception thrown when executing the function */
        if (!check_exception_thrown(comp_ctx, func_ctx))
            goto fail;
    }

    if (func_type->result_count > 0)
        PUSH(value_ret, func_type->types[func_type->param_count]);

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
                            LLVMValueRef func_type_idx, LLVMValueRef table_elem_idx,
                            LLVMTypeRef *param_types, LLVMValueRef *param_values,
                            uint32 param_count, uint32 param_cell_num,
                            LLVMTypeRef ret_type, uint8 wasm_ret_type,
                            LLVMValueRef *p_value_ret, LLVMValueRef *p_res)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[6];
    LLVMTypeRef ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[6], value_ret = NULL, res = NULL;
    char buf[32], *func_name = "aot_call_indirect";
    uint32 i, cell_num = 0;

    /* prepare function type of aot_call_indirect */
    func_param_types[0] = comp_ctx->exec_env_type;  /* exec_env */
    func_param_types[1] = INT8_TYPE;                /* check_func_type */
    func_param_types[2] = I32_TYPE;                 /* func_type_idx */
    func_param_types[3] = I32_TYPE;                 /* table_elem_idx */
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
        if (!(func = I64_CONST((uint64)(uintptr_t)aot_call_indirect))
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

    if (param_count > 64) {
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
    func_param_values[1] = I8_CONST(true);
    func_param_values[2] = func_type_idx;
    func_param_values[3] = table_elem_idx;
    func_param_values[4] = I32_CONST(param_cell_num);
    func_param_values[5] = func_ctx->argv_buf;

    if (!func_param_values[1] || !func_param_values[4]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_call_indirect() function */
    if (!(res = LLVMBuildCall(comp_ctx->builder, func,
                              func_param_values, 6, "res"))) {
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
}

bool
aot_compile_op_call_indirect(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 type_idx)
{
    AOTFuncType *func_type;
    LLVMValueRef elem_idx, ftype_idx;
    LLVMValueRef *param_values = NULL, value_ret = NULL, res = NULL;
    LLVMTypeRef *param_types = NULL, ret_type;
    int32 i, param_count;
    uint32 param_cell_num;
    uint64 total_size;
    uint8 wasm_ret_type;
    bool ret;

    /* Check function type index */
    if (type_idx >= comp_ctx->comp_data->func_type_count) {
        aot_set_last_error("type index is overflow");
        return false;
    }

    ftype_idx = I32_CONST(type_idx);
    CHECK_LLVM_CONST(ftype_idx);

    func_type = comp_ctx->comp_data->func_types[type_idx];

    param_cell_num = wasm_type_param_cell_num(func_type);

    POP_I32(elem_idx);

    /* Initialize parameter types of the LLVM function */
    param_count = (int32)func_type->param_count;
    total_size = sizeof(LLVMTypeRef) * (uint64)param_count;
    if (total_size >= UINT32_MAX
        || !(param_types = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("Allocate memory failed.");
        goto fail;
    }

    for (i = 0; i < param_count; i++)
        param_types[i] = TO_LLVM_TYPE(func_type->types[i]);

    /* Resolve return type of the LLVM function */
    if (func_type->result_count) {
        wasm_ret_type = func_type->types[func_type->param_count];
        ret_type = TO_LLVM_TYPE(wasm_ret_type);
    }
    else {
        wasm_ret_type = VALUE_TYPE_VOID;
        ret_type = VOID_TYPE;
    }

    /* Allocate memory for parameters */
    total_size = sizeof(LLVMValueRef) * (uint64)param_count;
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("Allocate memory failed.");
        goto fail;
    }

    /* Pop parameters from stack */
    for (i = param_count - 1; i >= 0; i--)
        POP(param_values[i], func_type->types[i]);

    if (!call_aot_call_indirect_func(comp_ctx, func_ctx,
                                     func_type, ftype_idx, elem_idx,
                                     param_types, param_values,
                                     param_count, param_cell_num,
                                     ret_type, wasm_ret_type,
                                     &value_ret, &res))
        goto fail;

    if (func_type->result_count > 0)
        PUSH(value_ret, func_type->types[func_type->param_count]);

    /* Check whether there was exception thrown when executing the function */
    if (!check_call_return(comp_ctx, func_ctx, res))
        goto fail;

    ret = true;

fail:
    if (param_values)
        wasm_runtime_free(param_values);
    if (param_types)
        wasm_runtime_free(param_types);
    return ret;
}

