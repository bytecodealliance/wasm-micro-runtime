/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_conversion.h"
#include "aot_emit_exception.h"
#include "aot_emit_numberic.h"
#include "../aot/aot_runtime.h"

static bool
trunc_float_to_int(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                   LLVMValueRef operand, LLVMTypeRef dest_type,
                   LLVMValueRef min_value, LLVMValueRef max_value,
                   char *name, bool sign)
{
    LLVMBasicBlockRef check_nan_succ, check_overflow_succ;
    LLVMValueRef is_less, is_greater, res;

    if (!(res = LLVMBuildFCmp(comp_ctx->builder, LLVMRealUNO,
                              operand, operand, "fcmp_is_nan"))) {
        aot_set_last_error("llvm build fcmp failed.");
        goto fail;
    }

    if (!(check_nan_succ =
              LLVMAppendBasicBlockInContext(comp_ctx->context,
                                            func_ctx->func,
                                            "check_nan_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_nan_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_INVALID_CONVERSION_TO_INTEGER,
                             true, res, check_nan_succ)))
        goto fail;

    if (!(is_less = LLVMBuildFCmp(comp_ctx->builder, LLVMRealOLE, operand,
                                  min_value, "fcmp_min_value"))) {
        aot_set_last_error("llvm build fcmp failed.");
        goto fail;
    }

    if (!(is_greater = LLVMBuildFCmp(comp_ctx->builder, LLVMRealOGE, operand,
                                     max_value, "fcmp_max_value"))) {
        aot_set_last_error("llvm build fcmp failed.");
        goto fail;
    }

    if (!(res = LLVMBuildOr(comp_ctx->builder, is_less, is_greater, "is_overflow"))) {
        aot_set_last_error("llvm build logic and failed.");
        goto fail;
    }

    /* Check if float value out of range */
    if (!(check_overflow_succ =
              LLVMAppendBasicBlockInContext(comp_ctx->context,
                                            func_ctx->func,
                                            "check_overflow_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_overflow_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx, EXCE_INTEGER_OVERFLOW,
                             true, res, check_overflow_succ)))
        goto fail;

    if (sign)
        res = LLVMBuildFPToSI(comp_ctx->builder, operand, dest_type, name);
    else
        res = LLVMBuildFPToUI(comp_ctx->builder, operand, dest_type, name);
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    if (dest_type == I32_TYPE)
        PUSH_I32(res);
    else if (dest_type == I64_TYPE)
        PUSH_I64(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i32_wrap_i64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef value, res;

    POP_I64(value);

    if (!(res = LLVMBuildTrunc(comp_ctx->builder, value, I32_TYPE, "i32_wrap_i64"))) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_I32(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i32_trunc_f32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             bool sign)
{
    LLVMValueRef value;
    LLVMValueRef min_value, max_value;

    POP_F32(value);

    if (sign) {
        min_value = F32_CONST(-2147483904.0f);
        max_value = F32_CONST(2147483648.0f);
    }
    else {
        min_value = F32_CONST(-1.0f);
        max_value = F32_CONST(4294967296.0f);
    }

    return trunc_float_to_int(comp_ctx, func_ctx, value,
                              I32_TYPE, min_value, max_value,
                              sign ? "i32_trunc_f32_s" : "i32_trunc_f32_u", sign);
fail:
    return false;
}

bool
aot_compile_op_i32_trunc_f64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             bool sign)
{
    LLVMValueRef value;
    LLVMValueRef min_value, max_value;

    POP_F64(value);

    if (sign) {
        min_value = F64_CONST(-2147483649.0);
        max_value = F64_CONST(2147483648.0);
    }
    else {
        min_value = F64_CONST(-1.0);
        max_value = F64_CONST(4294967296.0);
    }

    return trunc_float_to_int(comp_ctx, func_ctx, value,
                              I32_TYPE, min_value, max_value,
                              sign ? "i32_trunc_f64_s" : "i32_trunc_f64_u", sign);
fail:
    return false;
}

bool
aot_compile_op_i64_extend_i32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                              bool sign)
{
    LLVMValueRef value, res;

    POP_I32(value);

    if (sign)
        res = LLVMBuildSExt(comp_ctx->builder, value, I64_TYPE, "i64_extend_i32_s");
    else
        res = LLVMBuildZExt(comp_ctx->builder, value, I64_TYPE, "i64_extend_i32_u");
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_I64(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_trunc_f32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             bool sign)
{
    LLVMValueRef value;
    LLVMValueRef min_value, max_value;

    POP_F32(value);

    if (sign) {
        min_value = F32_CONST(-9223373136366403584.0f);
        max_value = F32_CONST(9223372036854775808.0f);
    }
    else {
        min_value = F32_CONST(-1.0f);
        max_value = F32_CONST(18446744073709551616.0f);
    }

    return trunc_float_to_int(comp_ctx, func_ctx, value,
                              I64_TYPE, min_value, max_value,
                              sign ? "i64_trunc_f32_s" : "i64_trunc_f32_u", sign);
fail:
    return false;
}

bool
aot_compile_op_i64_trunc_f64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             bool sign)
{
    LLVMValueRef value;
    LLVMValueRef min_value, max_value;

    POP_F64(value);

    if (sign) {
        min_value = F64_CONST(-9223372036854777856.0);
        max_value = F64_CONST(9223372036854775808.0);
    }
    else {
        min_value = F64_CONST(-1.0);
        max_value = F64_CONST(18446744073709551616.0);
    }

    return trunc_float_to_int(comp_ctx, func_ctx, value,
                              I64_TYPE, min_value, max_value,
                              sign ? "i64_trunc_f64_s" : "i64_trunc_f64_u", sign);
fail:
    return false;
}

bool
aot_compile_op_f32_convert_i32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                               bool sign)
{
    LLVMValueRef value, res;

    POP_I32(value);

    if (sign)
        res = LLVMBuildSIToFP(comp_ctx->builder, value, F32_TYPE, "f32_convert_i32_s");
    else
        res = LLVMBuildUIToFP(comp_ctx->builder, value, F32_TYPE, "f32_convert_i32_u");
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F32(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_convert_i64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                               bool sign)
{
    LLVMValueRef value, res;

    POP_I64(value);

    if (sign)
        res = LLVMBuildSIToFP(comp_ctx->builder, value, F32_TYPE, "f32_convert_i64_s");
    else
        res = LLVMBuildUIToFP(comp_ctx->builder, value, F32_TYPE, "f32_convert_i64_u");
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F32(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_demote_f64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef value, res;

    POP_F64(value);

    if (!(res = LLVMBuildFPTrunc(comp_ctx->builder, value, F32_TYPE, "f32_demote_f64"))) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F32(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_convert_i32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                               bool sign)
{
    LLVMValueRef value, res;

    POP_I32(value);

    if (sign)
        res = LLVMBuildSIToFP(comp_ctx->builder, value, F64_TYPE, "f64_convert_i32_s");
    else
        res = LLVMBuildUIToFP(comp_ctx->builder, value, F64_TYPE, "f64_convert_i32_u");
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F64(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_convert_i64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                               bool sign)
{
    LLVMValueRef value, res;

    POP_I64(value);

    if (sign)
        res = LLVMBuildSIToFP(comp_ctx->builder, value, F64_TYPE, "f64_convert_i64_s");
    else
        res = LLVMBuildUIToFP(comp_ctx->builder, value, F64_TYPE, "f64_convert_i64_u");
    if (!res) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F64(res);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_promote_f32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef value, res;

    POP_F32(value);

    if (!(res = LLVMBuildFPExt(comp_ctx->builder, value, F64_TYPE, "f64_promote_f32"))) {
        aot_set_last_error("llvm build conversion failed.");
        return false;
    }

    PUSH_F64(res);

    /* Avoid the promote being optimized away */
    PUSH_F64(F64_CONST(1.0));
    return aot_compile_op_f64_arithmetic(comp_ctx, func_ctx, FLOAT_MUL);
fail:
    return false;
}

bool
aot_compile_op_i64_reinterpret_f64(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx)
{
    LLVMValueRef value;
    POP_F64(value);
    if (!(value = LLVMBuildBitCast(comp_ctx->builder, value,
                                   I64_TYPE, "i64"))) {
        aot_set_last_error("llvm build fp to si failed.");
        return false;
    }
    PUSH_I64(value);
    return true;
fail:
    return false;
}


bool
aot_compile_op_i32_reinterpret_f32(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx)
{
    LLVMValueRef value;
    POP_F32(value);
    if (!(value = LLVMBuildBitCast(comp_ctx->builder, value,
                                   I32_TYPE, "i32"))) {
        aot_set_last_error("llvm build fp to si failed.");
        return false;
    }
    PUSH_I32(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_reinterpret_i64(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx)
{
    LLVMValueRef value;
    POP_I64(value);
    if (!(value = LLVMBuildBitCast(comp_ctx->builder, value,
                                   F64_TYPE, "f64"))) {
        aot_set_last_error("llvm build si to fp failed.");
        return false;
    }
    PUSH_F64(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_reinterpret_i32(AOTCompContext *comp_ctx,
                                   AOTFuncContext *func_ctx)
{
    LLVMValueRef value;
    POP_I32(value);
    if (!(value = LLVMBuildBitCast(comp_ctx->builder, value,
                                   F32_TYPE, "f32"))) {
        aot_set_last_error("llvm build si to fp failed.");
        return false;
    }
    PUSH_F32(value);
    return true;
fail:
    return false;
}

