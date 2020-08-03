/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"

static char *exce_block_names[] = {
    "exce_unreachable",             /* EXCE_UNREACHABLE */
    "exce_out_of_memory",           /* EXCE_OUT_OF_MEMORY */
    "exce_out_of_bounds_mem_access",/* EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS */
    "exce_integer_overflow",        /* EXCE_INTEGER_OVERFLOW */
    "exce_divide_by_zero",          /* EXCE_INTEGER_DIVIDE_BY_ZERO */
    "exce_invalid_convert_to_int",  /* EXCE_INVALID_CONVERSION_TO_INTEGER */
    "exce_invalid_func_type_idx",   /* EXCE_INVALID_FUNCTION_TYPE_INDEX */
    "exce_invalid_func_idx",        /* EXCE_INVALID_FUNCTION_INDEX */
    "exce_undefined_element",       /* EXCE_UNDEFINED_ELEMENT */
    "exce_uninit_element",          /* EXCE_UNINITIALIZED_ELEMENT */
    "exce_call_unlinked",           /* EXCE_CALL_UNLINKED_IMPORT_FUNC */
    "exce_native_stack_overflow",   /* EXCE_NATIVE_STACK_OVERFLOW */
    "exce_unaligned_atomic"         /* EXCE_UNALIGNED_ATOMIC */
};

bool
aot_emit_exception(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                   int32 exception_id,
                   bool is_cond_br,
                   LLVMValueRef cond_br_if,
                   LLVMBasicBlockRef cond_br_else_block)
{
    LLVMBasicBlockRef exce_block;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMValueRef exce_id = I32_CONST((uint32)exception_id), func_const, func;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;
    LLVMValueRef param_values[2];

    bh_assert(exception_id >= 0 && exception_id < EXCE_NUM);

    CHECK_LLVM_CONST(exce_id);

    /* Create got_exception block if needed */
    if (!func_ctx->got_exception_block) {
        if (!(func_ctx->got_exception_block =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              "got_exception"))) {
            aot_set_last_error("add LLVM basic block failed.");
            return false;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder,
                                 func_ctx->got_exception_block);

        /* Create exection id phi */
        if (!(func_ctx->exception_id_phi =
                LLVMBuildPhi(comp_ctx->builder,
                             comp_ctx->basic_types.int32_type,
                             "exception_id_phi"))) {
            aot_set_last_error("llvm build phi failed.");
            return false;
        }

        /* Call aot_set_exception_with_id() to throw exception */
        param_types[0] = INT8_PTR_TYPE;
        param_types[1] = I32_TYPE;
        ret_type = VOID_TYPE;

        /* Create function type */
        if (!(func_type = LLVMFunctionType(ret_type, param_types,
                                           2, false))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        if (comp_ctx->is_jit_mode) {
            /* Create function type */
            if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
                aot_set_last_error("create LLVM function type failed.");
                return false;
            }
            /* Create LLVM function with const function pointer */
            if (!(func_const =
                    I64_CONST((uint64)(uintptr_t)aot_set_exception_with_id))
                || !(func = LLVMConstIntToPtr(func_const, func_ptr_type))) {
                aot_set_last_error("create LLVM value failed.");
                return false;
            }
        }
        else {
            /* Create LLVM function with external function pointer */
            if (!(func = LLVMGetNamedFunction(comp_ctx->module,
                                              "aot_set_exception_with_id"))
                && !(func = LLVMAddFunction(comp_ctx->module,
                                            "aot_set_exception_with_id",
                                            func_type))) {
                aot_set_last_error("add LLVM function failed.");
                return false;
            }
        }

        /* Call the aot_set_exception_with_id() function */
        param_values[0] = func_ctx->aot_inst;
        param_values[1] = func_ctx->exception_id_phi;
        if (!LLVMBuildCall(comp_ctx->builder, func, param_values,
                           2, "")) {
            aot_set_last_error("llvm build call failed.");
            return false;
        }

        /* Create return IR */
        AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;
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

        /* Resume the builder position */
        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);
    }

    /* Create exception block if needed */
    if (!(exce_block = func_ctx->exception_blocks[exception_id])) {
        if (!(func_ctx->exception_blocks[exception_id] = exce_block =
                LLVMAppendBasicBlockInContext(comp_ctx->context,
                                              func_ctx->func,
                                              exce_block_names[exception_id]))) {
            aot_set_last_error("add LLVM basic block failed.");
            return false;
        }

        /* Move before got_exception block */
        LLVMMoveBasicBlockBefore(exce_block, func_ctx->got_exception_block);

        /* Add phi incoming value to got_exception block */
        LLVMAddIncoming(func_ctx->exception_id_phi, &exce_id, &exce_block, 1);

        /* Jump to got exception block */
        LLVMPositionBuilderAtEnd(comp_ctx->builder, exce_block);
        if (!LLVMBuildBr(comp_ctx->builder, func_ctx->got_exception_block)) {
            aot_set_last_error("llvm build br failed.");
            return false;
        }
    }

    /* Resume builder position */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);

    if (!is_cond_br) {
        /* not condition br, create br IR */
        if (!LLVMBuildBr(comp_ctx->builder, exce_block)) {
            aot_set_last_error("llvm build br failed.");
            return false;
        }
    }
    else {
        /* Create condition br */
        if (!LLVMBuildCondBr(comp_ctx->builder, cond_br_if,
                             exce_block, cond_br_else_block)) {
            aot_set_last_error("llvm build cond br failed.");
            return false;
        }
        /* Start to translate the else block */
        LLVMPositionBuilderAtEnd(comp_ctx->builder, cond_br_else_block);
    }

    return true;
fail:
    return false;
}

