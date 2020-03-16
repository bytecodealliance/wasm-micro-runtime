/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_function.h"
#include "aot_emit_exception.h"
#include "aot_emit_control.h"
#include "../aot/aot_runtime.h"

/* Check whether there was exception thrown, if yes, return directly */
static bool
check_exception_thrown(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;
    LLVMBasicBlockRef block_curr, check_exce_succ;
    LLVMValueRef value, cmp;

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

    /* Create function return block if it isn't created */
    if (!func_ctx->func_return_block) {
        if (!(func_ctx->func_return_block =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "func_ret"))) {
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
    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp,
                         check_exce_succ, func_ctx->func_return_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, check_exce_succ);
    return true;
}

static bool
call_aot_invoke_native_func(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef func_idx, AOTFuncType *aot_func_type,
                            LLVMTypeRef *param_types, LLVMValueRef *param_values,
                            uint32 param_count, uint32 param_cell_num,
                            LLVMTypeRef ret_type, uint8 wasm_ret_type,
                            LLVMValueRef *p_value_ret)
{
    LLVMTypeRef func_type, func_ptr_type, func_param_types[5];
    LLVMTypeRef ret_ptr_type, elem_ptr_type;
    LLVMValueRef func, elem_idx, elem_ptr;
    LLVMValueRef func_param_values[5], value_ret, value_ret_ptr, res;
    char buf[32], *func_name = "aot_invoke_native";
    uint32 i, cell_num = 0;

    /* prepare function type of aot_invoke_native */
    func_param_types[0] = comp_ctx->exec_env_type;  /* exec_env */
    func_param_types[1] = I32_TYPE;                 /* func_idx */
    func_param_types[2] = INT32_PTR_TYPE;           /* frame_lp */
    func_param_types[3] = I32_TYPE;                 /* argc */
    func_param_types[4] = INT32_PTR_TYPE;           /* argv_ret */
    if (!(func_type = LLVMFunctionType(VOID_TYPE, func_param_types, 5, false))) {
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

        /* convert to int32 pointer */
        if (!(value_ret_ptr = LLVMBuildBitCast(comp_ctx->builder, value_ret,
                                               INT32_PTR_TYPE, "argv_ret_ptr"))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
    }
    else {
        value_ret_ptr = LLVMConstNull(INT32_PTR_TYPE);
    }

    func_param_values[0] = func_ctx->exec_env;
    func_param_values[1] = func_idx;
    func_param_values[2] = func_ctx->argv_buf;
    func_param_values[3] = I32_CONST(param_cell_num);
    func_param_values[4] = value_ret_ptr;

    if (!func_param_values[3]) {
        aot_set_last_error("llvm create const failed.");
        return false;
    }

    /* call aot_invoke_native() function */
    if (!(LLVMBuildCall(comp_ctx->builder, func, func_param_values, 5, ""))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    if (wasm_ret_type != VALUE_TYPE_VOID)
        /* get function return value */
        *p_value_ret = LLVMBuildLoad(comp_ctx->builder, value_ret, "value_ret");

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
    LLVMValueRef import_func_idx;
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
                                         ret_type, wasm_ret_type, &value_ret))
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
    }

    if (func_type->result_count > 0)
        PUSH(value_ret, func_type->types[func_type->param_count]);

    /* Check whether there was exception thrown when executing the function */
    if (!check_exception_thrown(comp_ctx, func_ctx))
        goto fail;

    ret = true;
fail:
    if (param_types)
        wasm_runtime_free(param_types);
    if (param_values)
        wasm_runtime_free(param_values);
  return ret;
}

bool
aot_compile_op_call_indirect(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 type_idx)
{
    AOTFuncType *func_type;
    LLVMValueRef elem_idx, table_elem, func_idx, ftype_idx_ptr, ftype_idx;
    LLVMValueRef cmp_elem_idx, cmp_func_idx, is_ftype_match, is_ftype_mismatch;
    LLVMValueRef func, func_ptr, func_const, table_size_const, cmp_func_ptr;
    LLVMValueRef *param_values = NULL, param_values_tmp[3], value_ret;
    LLVMTypeRef *param_types = NULL, param_types_tmp[3], ret_type,
                f_type, f_ptr_type;
    LLVMBasicBlockRef check_elem_idx_succ, check_ftype_idx_succ;
    LLVMBasicBlockRef check_func_idx_succ, check_func_ptr_succ;
    char *func_name = "aot_is_wasm_type_equal";
    int32 i, j = 0, param_count;
    uint32 param_cell_num;
    uint64 total_size;
    uint8 wasm_ret_type;
    bool ret;

    /* Check function type index */
    if (type_idx >= comp_ctx->comp_data->func_type_count) {
        aot_set_last_error("type index is overflow");
        return false;
    }

    func_type = comp_ctx->comp_data->func_types[type_idx];

    param_cell_num = wasm_type_param_cell_num(func_type);

    POP_I32(elem_idx);

    table_size_const = I32_CONST(comp_ctx->comp_data->table_size);
    CHECK_LLVM_CONST(table_size_const);

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

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_UNDEFINED_ELEMENT,
                             true, cmp_elem_idx, check_elem_idx_succ)))
        goto fail;

    /* Load function index */
    if (!(table_elem = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                            func_ctx->table_base,
                                            &elem_idx, 1, "table_elem"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }

    if (!(func_idx = LLVMBuildLoad(comp_ctx->builder, table_elem, "func_idx"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    /* Check if func_idx == -1 */
    if (!(cmp_func_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                       func_idx, I32_NEG_ONE,
                                       "cmp_func_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* Throw exception if func_idx == -1 */
    if (!(check_func_idx_succ =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "check_func_idx_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_func_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_UNINITIALIZED_ELEMENT,
                             true, cmp_func_idx, check_func_idx_succ)))
        goto fail;

    /* Load function type index */
    if (!(ftype_idx_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                               func_ctx->func_type_indexes,
                                               &func_idx, 1,
                                               "ftype_idx_ptr"))) {
        aot_set_last_error("llvm build inbounds gep failed.");
        goto fail;
    }

    if (!(ftype_idx = LLVMBuildLoad(comp_ctx->builder, ftype_idx_ptr,
                                    "ftype_idx"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    /* Call aot_is_type_equal() to check whether function type match */
    param_types_tmp[0] = INT8_PTR_TYPE;
    param_types_tmp[1] = I32_TYPE;
    param_types_tmp[2] = I32_TYPE;
    ret_type = INT8_TYPE;

    /* Create function type */
    if (!(f_type = LLVMFunctionType(ret_type, param_types_tmp,
                                    3, false))) {
        aot_set_last_error("create LLVM function type failed.");
        goto fail;
    }

    if (comp_ctx->is_jit_mode) {
        /* Create function type */
        if (!(f_ptr_type = LLVMPointerType(f_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            goto fail;
        }
        /* Create LLVM function with const function pointer */
        if (!(func_const = I64_CONST((uint64)(uintptr_t)aot_is_wasm_type_equal))
            || !(func = LLVMConstIntToPtr(func_const, f_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            goto fail;
        }
    }
    else {
        /* Create LLVM function with external function pointer */
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module, func_name, f_type))) {
            aot_set_last_error("add LLVM function failed.");
            goto fail;
        }
    }

    /* Call the aot_is_type_equal() function */
    param_values_tmp[0] = func_ctx->aot_inst;
    param_values_tmp[1] = I32_CONST(type_idx);
    param_values_tmp[2] = ftype_idx;

    CHECK_LLVM_CONST(param_values_tmp[1]);

    if (!(is_ftype_match = LLVMBuildCall(comp_ctx->builder, func,
                                         param_values_tmp, 3,
                                         "is_ftype_match"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    if (!(is_ftype_mismatch = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ,
                                            is_ftype_match, I8_ZERO,
                                            "is_ftype_mismatch"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    if (!(check_ftype_idx_succ =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "check_ftype_idx_success"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_ftype_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_INVALID_FUNCTION_TYPE_INDEX,
                             true, is_ftype_mismatch, check_ftype_idx_succ)))
        goto fail;

    /* Load function pointer */
    if (!(func_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->func_ptrs,
                                          &func_idx, 1, "func_ptr"))) {
        aot_set_last_error("llvm build inbounds gep failed.");
        goto fail;
    }

    if (!(func = LLVMBuildLoad(comp_ctx->builder, func_ptr, "func_tmp"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    /* Check whether import function is NULL */
    if (!(cmp_func_ptr = LLVMBuildIsNull(comp_ctx->builder, func, "is_func_null"))) {
        aot_set_last_error("llvm build is null failed.");
        goto fail;
    }

    /* Throw exception if import function is NULL */
    if (!(check_func_ptr_succ =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "check_func_ptr_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_func_ptr_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx,
                             EXCE_CALL_UNLINKED_IMPORT_FUNC,
                             true, cmp_func_ptr, check_func_ptr_succ)))
        goto fail;

    /* Initialize parameter types of the LLVM function */
    param_count = (int32)func_type->param_count;
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
    total_size = sizeof(LLVMValueRef) * (uint64)(param_count + 1);
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_runtime_malloc((uint32)total_size))) {
        aot_set_last_error("Allocate memory failed.");
        goto fail;
    }

    /* First parameter is exec env */
    j = 0;
    param_values[j++] = func_ctx->exec_env;

    /* Pop parameters from stack */
    for (i = param_count - 1; i >= 0; i--)
        POP(param_values[i + j], func_type->types[i]);

    if (!call_aot_invoke_native_func(comp_ctx, func_ctx, func_idx, func_type,
                                     param_types + 1, param_values + 1,
                                     param_count, param_cell_num,
                                     ret_type, wasm_ret_type, &value_ret))
        goto fail;

    if (func_type->result_count > 0)
        PUSH(value_ret, func_type->types[func_type->param_count]);

    /* Check whether there was exception thrown when executing the function */
    if (!check_exception_thrown(comp_ctx, func_ctx))
        goto fail;

    ret = true;

fail:
    if (param_values)
        wasm_runtime_free(param_values);
    if (param_types)
        wasm_runtime_free(param_types);
    return ret;
}

