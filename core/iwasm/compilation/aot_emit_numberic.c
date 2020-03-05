/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_numberic.h"
#include "aot_emit_exception.h"
#include "aot_emit_control.h"
#include "../aot/aot_runtime.h"

#include <stdarg.h>

#define LLVM_BUILD_ICMP(op, left, right, res, name) do {                    \
    if (!(res = LLVMBuildICmp(comp_ctx->builder, op, left, right, name))) { \
        aot_set_last_error("llvm build "name" fail.");                      \
        return false;                                                       \
    }                                                                       \
} while (0)

#define LLVM_BUILD_OP(Op, left, right, res, name, err_ret) do {             \
    if (!(res = LLVMBuild##Op(comp_ctx->builder, left, right, name))) {     \
        aot_set_last_error("llvm build " #name " fail.");                   \
        return err_ret;                                                     \
    }                                                                       \
} while (0)

#define ADD_BASIC_BLOCK(block, name) do {                                  \
    if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,         \
                                                func_ctx->func,            \
                                                name))) { \
        aot_set_last_error("llvm add basic block failed.");                \
        goto fail;                                                         \
    }                                                                      \
                                                                           \
    LLVMMoveBasicBlockAfter(block, LLVMGetInsertBlock(comp_ctx->builder)); \
} while (0)

#define IS_CONST_ZERO(val)                                                 \
    (LLVMIsConstant(val)                                                   \
     && ((is_i32 && (int32)LLVMConstIntGetZExtValue(val) == 0)             \
         || (!is_i32 && (int64)LLVMConstIntGetSExtValue(val) == 0)))

#define CHECK_INT_OVERFLOW(type) do {                                      \
    LLVMValueRef cmp_min_int, cmp_neg_one;                                 \
    LLVM_BUILD_ICMP(LLVMIntEQ, left, type##_MIN,                           \
                    cmp_min_int, "cmp_min_int");                           \
    LLVM_BUILD_ICMP(LLVMIntEQ, right, type##_NEG_ONE,                      \
                    cmp_neg_one, "cmp_neg_one");                           \
    LLVM_BUILD_OP(And, cmp_min_int, cmp_neg_one,                           \
                  overflow, "overflow", false);                            \
} while (0)

#define PUSH_INT(v) do {                \
    if (is_i32)                         \
        PUSH_I32(v);                    \
    else                                \
        PUSH_I64(v);                    \
} while (0)

#define POP_INT(v) do {                 \
    if (is_i32)                         \
        POP_I32(v);                     \
    else                                \
        POP_I64(v);                     \
} while (0)

#define PUSH_FLOAT(v) do {              \
    if (is_f32)                         \
        PUSH_F32(v);                    \
    else                                \
        PUSH_F64(v);                    \
} while (0)

#define POP_FLOAT(v) do {               \
    if (is_f32)                         \
        POP_F32(v);                     \
    else                                \
        POP_F64(v);                     \
} while (0)

#define DEF_INT_UNARY_OP(op, err) do {            \
    LLVMValueRef res, operand;                    \
    POP_INT(operand);                             \
    if (!(res = op)) {                            \
        if (err)                                  \
            aot_set_last_error(err);              \
        return false;                             \
    }                                             \
    PUSH_INT(res);                                \
} while (0)

#define DEF_INT_BINARY_OP(op, err) do {           \
    LLVMValueRef res, left, right;                \
    POP_INT(right);                               \
    POP_INT(left);                                \
    if (!(res = op)) {                            \
        if (err)                                  \
            aot_set_last_error(err);              \
        return false;                             \
    }                                             \
    PUSH_INT(res);                                \
} while (0)

#define DEF_FP_UNARY_OP(op, err) do {             \
    LLVMValueRef res, operand;                    \
    POP_FLOAT(operand);                           \
    if (!(res = op)) {                            \
        if (err)                                  \
            aot_set_last_error(err);              \
        return false;                             \
    }                                             \
    PUSH_FLOAT(res);                              \
} while (0)

#define DEF_FP_BINARY_OP(op, err) do {            \
    LLVMValueRef res, left, right;                \
    POP_FLOAT(right);                             \
    POP_FLOAT(left);                              \
    if (!(res = op)) {                            \
        if (err)                                  \
            aot_set_last_error(err);              \
        return false;                             \
    }                                             \
    PUSH_FLOAT(res);                              \
} while (0)

#define SHIFT_COUNT_MASK do {                                                \
    /* LLVM has undefined behavior if shift count is greater than bits count \
     * while Webassembly spec requires the shift count be wrapped. */        \
    LLVMValueRef shift_count_mask, bits_minus_one;                           \
    bits_minus_one = is_i32 ? I32_31 : I64_63;                               \
    LLVM_BUILD_OP(And, right, bits_minus_one,                                \
                  shift_count_mask, "shift_count_mask", NULL);               \
    right = shift_count_mask;                                                \
} while (0)


static LLVMValueRef
__call_llvm_intrinsic(AOTCompContext *comp_ctx,
                      const char *name,
                      LLVMTypeRef ret_type,
                      LLVMTypeRef *param_types,
                      int param_count,
                      LLVMValueRef *param_values)
{
    LLVMValueRef func, ret;
    LLVMTypeRef func_type;

    /* Declare llvm intrinsic function if necessary */
    if (!(func = LLVMGetNamedFunction(comp_ctx->module, name))) {
        if (!(func_type =
                LLVMFunctionType(ret_type, param_types, (uint32)param_count, false))) {
            aot_set_last_error("create LLVM function type failed.");
            return NULL;
        }

        if (!(func = LLVMAddFunction(comp_ctx->module, name, func_type))) {
            aot_set_last_error("add LLVM function failed.");
            return NULL;
        }
    }

    /* Call the LLVM intrinsic function */
    if (!(ret = LLVMBuildCall(comp_ctx->builder, func, param_values,
                              (uint32)param_count, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return NULL;
    }

    return ret;
}

static LLVMValueRef
call_llvm_intrinsic(AOTCompContext *comp_ctx,
                    const char *name,
                    LLVMTypeRef ret_type,
                    LLVMTypeRef *param_types,
                    int param_count,
                    ...)
{
    LLVMValueRef *param_values, ret;
    va_list argptr;
    uint64 total_size;
    int i = 0;

    /* Create param values */
    total_size = sizeof(LLVMValueRef) * (uint64)param_count;
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_malloc((uint32)total_size))) {
        aot_set_last_error("allocate memory for param values failed.");
        return false;
    }

    /* Load each param value */
    va_start(argptr, param_count);
    while (i < param_count)
        param_values[i++] = va_arg(argptr, LLVMValueRef);
    va_end(argptr);

    ret = __call_llvm_intrinsic(comp_ctx, name, ret_type,
                                param_types, param_count,
                                param_values);

    wasm_free(param_values);

    return ret;
}

static LLVMValueRef
call_llvm_intrinsic_v(AOTCompContext *comp_ctx,
                      const char *name,
                      LLVMTypeRef ret_type,
                      LLVMTypeRef *param_types,
                      int param_count,
                      va_list param_value_list)
{
    LLVMValueRef *param_values, ret;
    uint64 total_size;
    int i = 0;

    /* Create param values */
    total_size = sizeof(LLVMValueRef) * (uint64)param_count;
    if (total_size >= UINT32_MAX
        || !(param_values = wasm_malloc((uint32)total_size))) {
        aot_set_last_error("allocate memory for param values failed.");
        return false;
    }

    /* Load each param value */
    while (i < param_count)
        param_values[i++] = va_arg(param_value_list, LLVMValueRef);

    ret = __call_llvm_intrinsic(comp_ctx, name, ret_type,
                                param_types, param_count,
                                param_values);

    wasm_free(param_values);

    return ret;
}

/* Call llvm constrained floating-point intrinsic */
static LLVMValueRef
call_llvm_float_expermental_constrained_intrinsic(AOTCompContext *comp_ctx,
                                                  const char *intrinsic,
                                                  bool is_f32,
                                                  ...)
{
    va_list param_value_list;
    LLVMValueRef ret;
    LLVMTypeRef param_types[4], ret_type = is_f32 ? F32_TYPE : F64_TYPE;

    param_types[0] = param_types[1] = ret_type;
    param_types[2] = param_types[3] = MD_TYPE;

    va_start(param_value_list, is_f32);

    ret = call_llvm_intrinsic_v(comp_ctx,
                                intrinsic,
                                ret_type,
                                param_types,
                                4,
                                param_value_list);

    va_end(param_value_list);

    return ret;
}

/* Call llvm constrained libm-equivalent intrinsic */
static LLVMValueRef
call_llvm_libm_expermental_constrained_intrinsic(AOTCompContext *comp_ctx,
                                                 const char *intrinsic,
                                                 bool is_f32,
                                                 ...)
{
    va_list param_value_list;
    LLVMValueRef ret;
    LLVMTypeRef param_types[3], ret_type = is_f32 ? F32_TYPE : F64_TYPE;

    param_types[0] = ret_type;
    param_types[1] = param_types[2] = MD_TYPE;

    va_start(param_value_list, is_f32);

    ret = call_llvm_intrinsic_v(comp_ctx,
                                intrinsic,
                                ret_type,
                                param_types,
                                3,
                                param_value_list);

    va_end(param_value_list);

    return ret;
}

static LLVMValueRef
compile_op_float_min_max(AOTCompContext *comp_ctx,
                         bool is_f32,
                         LLVMValueRef left,
                         LLVMValueRef right,
                         bool is_min)
{
    LLVMTypeRef param_types[2], ret_type = is_f32 ? F32_TYPE : F64_TYPE,
                int_type = is_f32 ? I32_TYPE : I64_TYPE;
    LLVMValueRef cmp, is_eq, is_nan, ret, left_int, right_int, tmp,
                 nan = LLVMConstRealOfString(ret_type, "NaN");
    char *intrinsic = is_min ?
                      (is_f32 ? "llvm.minnum.f32" : "llvm.minnum.f64") :
                      (is_f32 ? "llvm.maxnum.f32" : "llvm.maxnum.f64");

    CHECK_LLVM_CONST(nan);

    param_types[0] = param_types[1] = ret_type;

    if (!(is_nan = LLVMBuildFCmp(comp_ctx->builder, LLVMRealUNO,
                                 left, right, "is_nan"))
        || !(is_eq = LLVMBuildFCmp(comp_ctx->builder, LLVMRealOEQ,
                                   left, right, "is_eq"))) {
        aot_set_last_error("llvm build fcmp fail.");
        return NULL;
    }

    /* If left and right are equal, they may be zero with different sign.
     * Webassembly spec assert -0 < +0. So do a bitwise here. */
    if (!(left_int = LLVMBuildBitCast(comp_ctx->builder, left,
                                      int_type, "left_int"))
        || !(right_int = LLVMBuildBitCast(comp_ctx->builder, right,
                                      int_type, "right_int"))) {
        aot_set_last_error("llvm build bitcast fail.");
        return NULL;
    }

    if (is_min)
        LLVM_BUILD_OP(Or, left_int, right_int, tmp, "tmp_int", NULL);
    else
        LLVM_BUILD_OP(And, left_int, right_int, tmp, "tmp_int", NULL);

    if (!(tmp = LLVMBuildBitCast(comp_ctx->builder, tmp, ret_type, "tmp"))) {
        aot_set_last_error("llvm build bitcast fail.");
        return NULL;
    }

    if (!(cmp = call_llvm_intrinsic(comp_ctx,
                                    intrinsic,
                                    ret_type,
                                    param_types,
                                    2,
                                    left,
                                    right)))
        return NULL;

    if (!(cmp = LLVMBuildSelect(comp_ctx->builder,
                                is_eq,
                                tmp,
                                cmp,
                                "cmp"))) {
        aot_set_last_error("llvm build select fail.");
        return NULL;
    }

    if (!(ret = LLVMBuildSelect(comp_ctx->builder,
                                is_nan,
                                nan,
                                cmp,
                                is_min ? "min" : "max"))) {
        aot_set_last_error("llvm build select fail.");
        return NULL;
    }

    return ret;
fail:
    return NULL;
}

typedef enum BitCountType {
  CLZ32 = 0,
  CLZ64,
  CTZ32,
  CTZ64,
  POP_CNT32,
  POP_CNT64
} BitCountType;

static char *bit_cnt_llvm_intrinsic[] = { "llvm.ctlz.i32",
                                          "llvm.ctlz.i64",
                                          "llvm.cttz.i32",
                                          "llvm.cttz.i64",
                                          "llvm.ctpop.i32",
                                          "llvm.ctpop.i64",
                                        };

static bool
aot_compile_int_bit_count(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          BitCountType type, bool is_i32)
{
    LLVMValueRef zero_undef;
    LLVMTypeRef ret_type, param_types[2];

    param_types[0] = ret_type = is_i32 ? I32_TYPE : I64_TYPE;
    param_types[1] = LLVMInt1TypeInContext(comp_ctx->context);

    zero_undef = LLVMConstInt(param_types[1], false, true);
    CHECK_LLVM_CONST(zero_undef);

    /* Call the LLVM intrinsic function */
    if (type < POP_CNT32)
        DEF_INT_UNARY_OP(call_llvm_intrinsic(comp_ctx,
                                             bit_cnt_llvm_intrinsic[type],
                                             ret_type,
                                             param_types,
                                             2,
                                             operand,
                                             zero_undef),
                         NULL);
    else
        DEF_INT_UNARY_OP(call_llvm_intrinsic(comp_ctx,
                                             bit_cnt_llvm_intrinsic[type],
                                             ret_type,
                                             param_types,
                                             1,
                                             operand),
                         NULL);

    return true;

fail:
    return false;
}

static bool
compile_rems(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
             LLVMValueRef left, LLVMValueRef right,
             LLVMValueRef overflow_cond, bool is_i32)
{
    LLVMValueRef phi, no_overflow_value, zero = is_i32 ? I32_ZERO : I64_ZERO;
    LLVMBasicBlockRef block_curr, no_overflow_block, rems_end_block;

    block_curr = LLVMGetInsertBlock(comp_ctx->builder);

    /* Add 2 blocks: no_overflow_block and rems_end block */
    ADD_BASIC_BLOCK(rems_end_block, "rems_end");
    ADD_BASIC_BLOCK(no_overflow_block, "rems_no_overflow");

    /* Create condition br */
    if (!LLVMBuildCondBr(comp_ctx->builder, overflow_cond,
                         rems_end_block, no_overflow_block)) {
        aot_set_last_error("llvm build cond br failed.");
        return false;
    }

    /* Translate no_overflow_block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, no_overflow_block);

    /* Calculate the rem value */
    LLVM_BUILD_OP(SRem, left, right, no_overflow_value, "rem_s", false);

    /* Jump to rems_end block */
    if (!LLVMBuildBr(comp_ctx->builder, rems_end_block)) {
        aot_set_last_error("llvm build br failed.");
        return false;
    }

    /* Translate rems_end_block */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, rems_end_block);

    /* Create result phi */
    if (!(phi = LLVMBuildPhi(comp_ctx->builder,
                             is_i32 ? I32_TYPE : I64_TYPE,
                             "rems_result_phi"))) {
        aot_set_last_error("llvm build phi failed.");
        return false;
    }

    /* Add phi incoming values */
    LLVMAddIncoming(phi, &no_overflow_value, &no_overflow_block, 1);
    LLVMAddIncoming(phi, &zero, &block_curr, 1);

    if (is_i32)
        PUSH_I32(phi);
    else
        PUSH_I64(phi);

    return true;

fail:
    return false;
}

static bool
compile_int_div(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                IntArithmetic arith_op, bool is_i32, uint8 **p_frame_ip)
{
    LLVMValueRef left, right, cmp_div_zero, overflow, res;
    LLVMBasicBlockRef check_div_zero_succ, check_overflow_succ;

    bh_assert(arith_op == INT_DIV_S
              || arith_op == INT_DIV_U
              || arith_op == INT_REM_S
              || arith_op == INT_REM_U);

    POP_INT(right);
    POP_INT(left);

    if (LLVMIsConstant(right)) {
        int64 right_val = (int64)LLVMConstIntGetSExtValue(right);
        switch (right_val) {
            case 0:
                /* Directly throw exception if divided by zero */
                if (!(aot_emit_exception(comp_ctx, func_ctx,
                                         EXCE_INTEGER_DIVIDE_BY_ZERO,
                                         false, NULL, NULL)))
                    goto fail;

                return aot_handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
            case 1:
                if (arith_op == INT_DIV_S || arith_op == INT_DIV_U)
                    PUSH_INT(left);
                else
                    PUSH_INT(is_i32 ? I32_ZERO : I64_ZERO);
                return true;
            case -1:
                if (arith_op == INT_DIV_S) {
                    LLVM_BUILD_ICMP(LLVMIntEQ, left,
                                    is_i32 ? I32_MIN : I64_MIN,
                                    overflow, "overflow");
                    ADD_BASIC_BLOCK(check_overflow_succ,
                                    "check_overflow_success");

                    /* Throw conditional exception if overflow */
                    if (!(aot_emit_exception(comp_ctx, func_ctx,
                                             EXCE_INTEGER_OVERFLOW,
                                             true, overflow,
                                             check_overflow_succ)))
                        goto fail;

                    /* Push -(left) to stack */
                    if (!(res = LLVMBuildNeg(comp_ctx->builder, left, "neg"))) {
                        aot_set_last_error("llvm build neg fail.");
                        return false;
                    }
                    PUSH_INT(res);
                    return true;
                }
                else if (arith_op == INT_REM_S) {
                    PUSH_INT(is_i32 ? I32_ZERO : I64_ZERO);
                    return true;
                }
                else {
                    /* fall to default */
                    goto handle_default;
                }
handle_default:
            default:
                /* Build div */
                switch (arith_op) {
                    case INT_DIV_S:
                        LLVM_BUILD_OP(SDiv, left, right, res, "div_s", false);
                        break;
                    case INT_DIV_U:
                        LLVM_BUILD_OP(UDiv, left, right, res, "div_u", false);
                        break;
                    case INT_REM_S:
                        LLVM_BUILD_OP(SRem, left, right, res, "rem_s", false);
                        break;
                    case INT_REM_U:
                        LLVM_BUILD_OP(URem, left, right, res, "rem_u", false);
                        break;
                    default:
                        bh_assert(0);
                        return false;
                }

                PUSH_INT(res);
                return true;
        }
    }
    else {
        /* Check divied by zero */
        LLVM_BUILD_ICMP(LLVMIntEQ, right, is_i32 ? I32_ZERO : I64_ZERO,
                        cmp_div_zero, "cmp_div_zero");
        ADD_BASIC_BLOCK(check_div_zero_succ, "check_div_zero_success");

        /* Throw conditional exception if divided by zero */
        if (!(aot_emit_exception(comp_ctx, func_ctx,
                                 EXCE_INTEGER_DIVIDE_BY_ZERO, true,
                                 cmp_div_zero, check_div_zero_succ)))
            goto fail;

        switch (arith_op) {
            case INT_DIV_S:
                /* Check integer overflow */
                if (is_i32)
                    CHECK_INT_OVERFLOW(I32);
                else
                    CHECK_INT_OVERFLOW(I64);

                ADD_BASIC_BLOCK(check_overflow_succ, "check_overflow_success");

                /* Throw conditional exception if integer overflow */
                if (!(aot_emit_exception(comp_ctx, func_ctx,
                                         EXCE_INTEGER_OVERFLOW,
                                         true, overflow, check_overflow_succ)))
                    goto fail;

                LLVM_BUILD_OP(SDiv, left, right, res, "div_s", false);
                PUSH_INT(res);
                return true;
            case INT_DIV_U:
                LLVM_BUILD_OP(UDiv, left, right, res, "div_u", false);
                PUSH_INT(res);
                return true;
            case INT_REM_S:
                /*  Webassembly spec requires it return 0 */
                if (is_i32)
                    CHECK_INT_OVERFLOW(I32);
                else
                    CHECK_INT_OVERFLOW(I64);
                return compile_rems(comp_ctx, func_ctx,
                                    left, right, overflow,
                                    is_i32);
            case INT_REM_U:
                LLVM_BUILD_OP(URem, left, right, res, "rem_u", false);
                PUSH_INT(res);
                return true;
            default:
                bh_assert(0);
                return false;;
        }
    }

fail:
    return false;
}

static LLVMValueRef
compile_int_add(AOTCompContext *comp_ctx,
                LLVMValueRef left, LLVMValueRef right,
                bool is_i32)
{
    /* If one of the operands is 0, just return the other */
    if (IS_CONST_ZERO(left))
        return right;
    if (IS_CONST_ZERO(right))
        return left;

    /* Build add */
    return LLVMBuildAdd(comp_ctx->builder, left, right, "add");
}

static LLVMValueRef
compile_int_sub(AOTCompContext *comp_ctx,
                LLVMValueRef left, LLVMValueRef right,
                bool is_i32)
{
    /* If the right operand is 0, just return the left */
    if (IS_CONST_ZERO(right))
        return left;

    /* Build sub */
    return LLVMBuildSub(comp_ctx->builder, left, right, "sub");
}

static LLVMValueRef
compile_int_mul(AOTCompContext *comp_ctx,
                LLVMValueRef left, LLVMValueRef right,
                bool is_i32)
{
    /* If one of the operands is 0, just return constant 0 */
    if (IS_CONST_ZERO(left) || IS_CONST_ZERO(right))
        return is_i32 ? I32_ZERO : I64_ZERO;

    /* Build mul */
    return LLVMBuildMul(comp_ctx->builder, left, right, "mul");
}

static bool
compile_op_int_arithmetic(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          IntArithmetic arith_op, bool is_i32, uint8 **p_frame_ip)
{
    switch (arith_op) {
        case INT_ADD:
            DEF_INT_BINARY_OP(compile_int_add(comp_ctx, left, right, is_i32),
                              "compile int add fail.");
            return true;
        case INT_SUB:
            DEF_INT_BINARY_OP(compile_int_sub(comp_ctx, left, right, is_i32),
                              "compile int sub fail.");
            return true;
        case INT_MUL:
            DEF_INT_BINARY_OP(compile_int_mul(comp_ctx, left, right, is_i32),
                              "compile int mul fail.");
            return true;
        case INT_DIV_S:
        case INT_DIV_U:
        case INT_REM_S:
        case INT_REM_U:
            return compile_int_div(comp_ctx, func_ctx, arith_op, is_i32, p_frame_ip);
        default:
            bh_assert(0);
            return false;
    }

fail:
    return false;
}

static bool
compile_op_int_bitwise(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       IntBitwise bitwise_op, bool is_i32)
{
    switch (bitwise_op) {
        case INT_AND:
            DEF_INT_BINARY_OP(LLVMBuildAnd(comp_ctx->builder,
                                           left, right, "and"),
                              "llvm build and fail.");
            return true;
        case INT_OR:
            DEF_INT_BINARY_OP(LLVMBuildOr(comp_ctx->builder,
                                           left, right, "or"),
                              "llvm build or fail.");
            return true;
        case INT_XOR:
            DEF_INT_BINARY_OP(LLVMBuildXor(comp_ctx->builder,
                                           left, right, "xor"),
                              "llvm build xor fail.");
            return true;
        default:
            bh_assert(0);
            return false;
    }

fail:
    return false;
}

static LLVMValueRef
compile_int_shl(AOTCompContext *comp_ctx,
                LLVMValueRef left, LLVMValueRef right,
                bool is_i32)
{
    LLVMValueRef res;

    if (strcmp(comp_ctx->target_arch, "x86_64") != 0
        && strcmp(comp_ctx->target_arch, "i386") != 0)
        SHIFT_COUNT_MASK;

    /* Build shl */
    LLVM_BUILD_OP(Shl, left, right, res, "shl", NULL);

    return res;
}

static LLVMValueRef
compile_int_shr_s(AOTCompContext *comp_ctx,
                  LLVMValueRef left, LLVMValueRef right,
                  bool is_i32)
{
    LLVMValueRef res;

    if (strcmp(comp_ctx->target_arch, "x86_64") != 0
        && strcmp(comp_ctx->target_arch, "i386") != 0)
        SHIFT_COUNT_MASK;

    /* Build shl */
    LLVM_BUILD_OP(AShr, left, right, res, "shr_s", NULL);

    return res;
}

static LLVMValueRef
compile_int_shr_u(AOTCompContext *comp_ctx,
                  LLVMValueRef left, LLVMValueRef right,
                  bool is_i32)
{
    LLVMValueRef res;

    if (strcmp(comp_ctx->target_arch, "x86_64") != 0
        && strcmp(comp_ctx->target_arch, "i386") != 0)
        SHIFT_COUNT_MASK;

    /* Build shl */
    LLVM_BUILD_OP(LShr, left, right, res, "shr_u", NULL);

    return res;
}

static LLVMValueRef
compile_int_rot(AOTCompContext *comp_ctx,
                 LLVMValueRef left, LLVMValueRef right,
                 bool is_rotl,
                 bool is_i32)
{
    LLVMValueRef bits_minus_shift_count, res, tmp_l, tmp_r;
    char *name = is_rotl ? "rotl" : "rotr";

    SHIFT_COUNT_MASK;

    /* Calculate (bits - shif_count) */
    LLVM_BUILD_OP(Sub,
                  is_i32 ? I32_32 : I64_64,
                  right,
                  bits_minus_shift_count,
                  "bits_minus_shift_count",
                  NULL);

    if (is_rotl) {
        /* left<<count | left>>(BITS-count) */
        LLVM_BUILD_OP(Shl, left, right, tmp_l, "tmp_l", NULL);
        LLVM_BUILD_OP(LShr, left, bits_minus_shift_count, tmp_r, "tmp_r", NULL);
    }
    else {
        /* left>>count | left<<(BITS-count) */
        LLVM_BUILD_OP(LShr, left, right, tmp_l, "tmp_l", NULL);
        LLVM_BUILD_OP(Shl, left, bits_minus_shift_count, tmp_r, "tmp_r", NULL);
    }

    LLVM_BUILD_OP(Or, tmp_l, tmp_r, res, name, NULL);

    return res;
}

static bool
compile_op_int_shift(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                     IntShift shift_op, bool is_i32)
{
    switch (shift_op) {
        case INT_SHL:
            DEF_INT_BINARY_OP(compile_int_shl(comp_ctx, left, right, is_i32),
                              NULL);
            return true;
        case INT_SHR_S:
            DEF_INT_BINARY_OP(compile_int_shr_s(comp_ctx, left, right, is_i32),
                              NULL);
           return true;
        case INT_SHR_U:
            DEF_INT_BINARY_OP(compile_int_shr_u(comp_ctx, left, right, is_i32),
                              NULL);
            return true;
        case INT_ROTL:
            DEF_INT_BINARY_OP(compile_int_rot(comp_ctx, left, right,
                                              true, is_i32),
                              NULL);
            return true;
        case INT_ROTR:
            DEF_INT_BINARY_OP(compile_int_rot(comp_ctx, left, right,
                                              false, is_i32),
                              NULL);
            return true;
        default:
            bh_assert(0);
            return false;
    }

fail:
    return false;
}

static bool
is_targeting_soft_float(LLVMTargetMachineRef target_machine)
{
    bool ret = false;
    char *feature_string;

    if (!(feature_string =
                LLVMGetTargetMachineFeatureString(target_machine))) {
        aot_set_last_error("llvm get target machine feature string fail.");
        return false;
    }

    ret = strstr(feature_string, "+soft-float") ? true : false;
    LLVMDisposeMessage(feature_string);
    return ret;
}

static bool
compile_op_float_arithmetic(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            FloatArithmetic arith_op, bool is_f32)
{
    switch (arith_op) {
        case FLOAT_ADD:
            if (is_targeting_soft_float(comp_ctx->target_machine))
                DEF_FP_BINARY_OP(LLVMBuildFAdd(comp_ctx->builder, left, right, "fadd"),
                                 "llvm build fadd fail.");
            else
                DEF_FP_BINARY_OP(call_llvm_float_expermental_constrained_intrinsic(
                                    comp_ctx,
                                    (is_f32
                                     ? "llvm.experimental.constrained.fadd.f32"
                                     : "llvm.experimental.constrained.fadd.f64"),
                                    is_f32,
                                    left,
                                    right,
                                    comp_ctx->fp_rounding_mode,
                                    comp_ctx->fp_exception_behavior),
                                 NULL);
            return true;
        case FLOAT_SUB:
            if (is_targeting_soft_float(comp_ctx->target_machine))
                DEF_FP_BINARY_OP(LLVMBuildFSub(comp_ctx->builder, left, right, "fsub"),
                                 "llvm build fsub fail.");
            else
                DEF_FP_BINARY_OP(call_llvm_float_expermental_constrained_intrinsic(
                                    comp_ctx,
                                    (is_f32
                                     ? "llvm.experimental.constrained.fsub.f32"
                                     : "llvm.experimental.constrained.fsub.f64"),
                                    is_f32,
                                    left,
                                    right,
                                    comp_ctx->fp_rounding_mode,
                                    comp_ctx->fp_exception_behavior),
                                 NULL);
            return true;
        case FLOAT_MUL:
            if (is_targeting_soft_float(comp_ctx->target_machine))
                DEF_FP_BINARY_OP(LLVMBuildFMul(comp_ctx->builder, left, right, "fmul"),
                                 "llvm build fmul fail.");
            else
                DEF_FP_BINARY_OP(call_llvm_float_expermental_constrained_intrinsic(
                                    comp_ctx,
                                    (is_f32
                                     ? "llvm.experimental.constrained.fmul.f32"
                                     : "llvm.experimental.constrained.fmul.f64"),
                                    is_f32,
                                    left,
                                    right,
                                    comp_ctx->fp_rounding_mode,
                                    comp_ctx->fp_exception_behavior),
                                 NULL);
            return true;
        case FLOAT_DIV:
            if (is_targeting_soft_float(comp_ctx->target_machine))
                DEF_FP_BINARY_OP(LLVMBuildFDiv(comp_ctx->builder, left, right, "fdiv"),
                                 "llvm build fdiv fail.");
            else
                DEF_FP_BINARY_OP(call_llvm_float_expermental_constrained_intrinsic(
                                    comp_ctx,
                                    (is_f32
                                     ? "llvm.experimental.constrained.fdiv.f32"
                                     : "llvm.experimental.constrained.fdiv.f64"),
                                    is_f32,
                                    left,
                                    right,
                                    comp_ctx->fp_rounding_mode,
                                    comp_ctx->fp_exception_behavior),
                                 NULL);
            return true;
        case FLOAT_MIN:
            DEF_FP_BINARY_OP(compile_op_float_min_max(comp_ctx,
                                                      is_f32,
                                                      left,
                                                      right,
                                                      true),
                             NULL);
            return true;
        case FLOAT_MAX:
            DEF_FP_BINARY_OP(compile_op_float_min_max(comp_ctx,
                                                      is_f32,
                                                      left,
                                                      right,
                                                      false),
                             NULL);

            return true;
        default:
            bh_assert(0);
            return false;
    }

fail:
    return false;
}

static LLVMValueRef
call_llvm_float_math_intrinsic(AOTCompContext *comp_ctx,
                                const char *intrinsic,
                                bool is_f32,
                                ...)
{
    va_list param_value_list;
    LLVMValueRef ret;
    LLVMTypeRef param_type, ret_type = is_f32 ? F32_TYPE : F64_TYPE;

    param_type = ret_type;

    va_start(param_value_list, is_f32);

    ret = call_llvm_intrinsic_v(comp_ctx,
                                intrinsic,
                                ret_type,
                                &param_type,
                                1,
                                param_value_list);

    va_end(param_value_list);

    return ret;
}

static bool
compile_op_float_math(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      FloatMath math_op, bool is_f32)
{
    switch (math_op) {
        case FLOAT_ABS:
            DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.fabs.f32" :
                                                            "llvm.fabs.f64",
                                                   is_f32,
                                                   operand),
                            NULL);
            return true;
        case FLOAT_NEG:
            DEF_FP_UNARY_OP(LLVMBuildFNeg(comp_ctx->builder, operand, "fneg"),
                            "llvm build fneg fail.");
            return true;

        case FLOAT_CEIL:
            DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.ceil.f32" :
                                                            "llvm.ceil.f64",
                                                   is_f32,
                                                   operand),
                            NULL);
            return true;
        case FLOAT_FLOOR:
            DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.floor.f32" :
                                                            "llvm.floor.f64",
                                                   is_f32,
                                                   operand),
                            NULL);
            return true;
        case FLOAT_TRUNC:
            DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.trunc.f32" :
                                                            "llvm.trunc.f64",
                                                   is_f32,
                                                   operand),
                            NULL);
            return true;
        case FLOAT_NEAREST:
            DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.rint.f32" :
                                                            "llvm.rint.f64",
                                                   is_f32,
                                                   operand),
                            NULL);
            return true;
        case FLOAT_SQRT:
            if (is_targeting_soft_float(comp_ctx->target_machine))
                DEF_FP_UNARY_OP(call_llvm_float_math_intrinsic(comp_ctx,
                                                   is_f32 ? "llvm.sqrt.f32" :
                                                            "llvm.sqrt.f64",
                                                   is_f32,
                                                   operand),
                                NULL);
            else
                DEF_FP_UNARY_OP(call_llvm_libm_expermental_constrained_intrinsic(
                                           comp_ctx,
                                           (is_f32
                                            ? "llvm.experimental.constrained.sqrt.f32"
                                            : "llvm.experimental.constrained.sqrt.f64"),
                                           is_f32,
                                           operand,
                                           comp_ctx->fp_rounding_mode,
                                           comp_ctx->fp_exception_behavior),
                                NULL);
            return true;
        default:
            bh_assert(0);
            return false;
    }

    return true;

fail:
    return false;
}

static bool
compile_float_copysign(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       bool is_f32)
{
    LLVMTypeRef ret_type, param_types[2];

    param_types[0] = param_types[1] = ret_type = is_f32 ? F32_TYPE : F64_TYPE;

    DEF_FP_BINARY_OP(call_llvm_intrinsic(comp_ctx,
                                         is_f32 ? "llvm.copysign.f32" :
                                                  "llvm.copysign.f64",
                                         ret_type,
                                         param_types,
                                         2,
                                         left,
                                         right),
                    NULL);
    return true;

fail:
    return false;
}

bool
aot_compile_op_i32_clz(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, CLZ32, true);
}

bool
aot_compile_op_i32_ctz(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, CTZ32, true);
}

bool
aot_compile_op_i32_popcnt(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, POP_CNT32, true);
}

bool
aot_compile_op_i64_clz(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, CLZ64, false);
}

bool
aot_compile_op_i64_ctz(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, CTZ64, false);
}

bool
aot_compile_op_i64_popcnt(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return aot_compile_int_bit_count(comp_ctx, func_ctx, POP_CNT64, false);
}

bool
aot_compile_op_i32_arithmetic(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                              IntArithmetic arith_op, uint8 **p_frame_ip)
{
    return compile_op_int_arithmetic(comp_ctx, func_ctx, arith_op, true, p_frame_ip);
}

bool
aot_compile_op_i64_arithmetic(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                              IntArithmetic arith_op, uint8 **p_frame_ip)
{
    return compile_op_int_arithmetic(comp_ctx, func_ctx, arith_op, false, p_frame_ip);
}

bool
aot_compile_op_i32_bitwise(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           IntBitwise bitwise_op)
{
    return compile_op_int_bitwise(comp_ctx, func_ctx, bitwise_op, true);
}

bool
aot_compile_op_i64_bitwise(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           IntBitwise bitwise_op)
{
    return compile_op_int_bitwise(comp_ctx, func_ctx, bitwise_op, false);
}

bool
aot_compile_op_i32_shift(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         IntShift shift_op)
{
    return compile_op_int_shift(comp_ctx, func_ctx, shift_op, true);
}

bool
aot_compile_op_i64_shift(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         IntShift shift_op)
{
    return compile_op_int_shift(comp_ctx, func_ctx, shift_op, false);
}

bool
aot_compile_op_f32_math(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        FloatMath math_op)
{
    return compile_op_float_math(comp_ctx, func_ctx, math_op, true);
}

bool
aot_compile_op_f64_math(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        FloatMath math_op)
{
    return compile_op_float_math(comp_ctx, func_ctx, math_op, false);
}

bool
aot_compile_op_f32_arithmetic(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx,
                              FloatArithmetic arith_op)
{
    return compile_op_float_arithmetic(comp_ctx, func_ctx, arith_op, true);
}

bool
aot_compile_op_f64_arithmetic(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx,
                              FloatArithmetic arith_op)
{
    return compile_op_float_arithmetic(comp_ctx, func_ctx, arith_op, false);
}

bool
aot_compile_op_f32_copysign(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return compile_float_copysign(comp_ctx, func_ctx, true);
}

bool
aot_compile_op_f64_copysign(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return compile_float_copysign(comp_ctx, func_ctx, false);
}

