/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_numberic.h"
#include "../jit_frontend.h"

#define PUSH_INT(v)      \
    do {                 \
        if (is_i32)      \
            PUSH_I32(v); \
        else             \
            PUSH_I64(v); \
    } while (0)

#define POP_INT(v)      \
    do {                \
        if (is_i32)     \
            POP_I32(v); \
        else            \
            POP_I64(v); \
    } while (0)

#define PUSH_FLOAT(v)    \
    do {                 \
        if (is_f32)      \
            PUSH_F32(v); \
        else             \
            PUSH_F64(v); \
    } while (0)

#define POP_FLOAT(v)    \
    do {                \
        if (is_f32)     \
            POP_F32(v); \
        else            \
            POP_F64(v); \
    } while (0)

#define DEF_INT_UNARY_OP(op, err)            \
    do {                                     \
        JitReg res, operand;                 \
        POP_INT(operand);                    \
        if (!(res = op)) {                   \
            if (err)                         \
                jit_set_last_error(cc, err); \
            goto fail;                       \
        }                                    \
        PUSH_INT(res);                       \
    } while (0)

#define DEF_INT_BINARY_OP(op, err)           \
    do {                                     \
        JitReg res, left, right;             \
        POP_INT(right);                      \
        POP_INT(left);                       \
        if (!(res = op)) {                   \
            if (err)                         \
                jit_set_last_error(cc, err); \
            goto fail;                       \
        }                                    \
        PUSH_INT(res);                       \
    } while (0)

#define DEF_FP_UNARY_OP(op, err)             \
    do {                                     \
        JitReg res, operand;                 \
        POP_FLOAT(operand);                  \
        if (!(res = op)) {                   \
            if (err)                         \
                jit_set_last_error(cc, err); \
            goto fail;                       \
        }                                    \
        PUSH_FLOAT(res);                     \
    } while (0)

#define DEF_FP_BINARY_OP(op, err)            \
    do {                                     \
        JitReg res, left, right;             \
        POP_FLOAT(right);                    \
        POP_FLOAT(left);                     \
        if (!(res = op)) {                   \
            if (err)                         \
                jit_set_last_error(cc, err); \
            goto fail;                       \
        }                                    \
        PUSH_FLOAT(res);                     \
    } while (0)

bool
jit_compile_op_i32_clz(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i32_ctz(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i32_popcnt(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i64_clz(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i64_ctz(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i64_popcnt(JitCompContext *cc)
{
    return false;
}

#define IS_CONST_ZERO(val)                              \
    (jit_reg_is_const(val)                              \
     && ((is_i32 && jit_cc_get_const_I32(cc, val) == 0) \
         || (!is_i32 && jit_cc_get_const_I64(cc, val) == 0)))

static JitReg
compile_int_add(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* If one of the operands is 0, just return the other */
    if (IS_CONST_ZERO(left))
        return right;
    if (IS_CONST_ZERO(right))
        return left;

    /* Build add */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(ADD, res, left, right);
    return res;
}

static JitReg
compile_int_sub(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* If the right operand is 0, just return the left */
    if (IS_CONST_ZERO(right))
        return left;

    /* Build sub */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(SUB, res, left, right);
    return res;
}

static JitReg
compile_int_mul(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* If one of the operands is 0, just return constant 0 */
    if (IS_CONST_ZERO(left) || IS_CONST_ZERO(right))
        return is_i32 ? NEW_CONST(I32, 0) : NEW_CONST(I64, 0);

    /* Build mul */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(MUL, res, left, right);
    return res;
}

static bool
compile_int_div(JitCompContext *cc, IntArithmetic arith_op, bool is_i32,
                uint8 **p_frame_ip)
{
    /* TODO */
    bh_assert(0);
    return false;
}

static bool
compile_op_int_arithmetic(JitCompContext *cc, IntArithmetic arith_op,
                          bool is_i32, uint8 **p_frame_ip)
{
    switch (arith_op) {
        case INT_ADD:
            DEF_INT_BINARY_OP(compile_int_add(cc, left, right, is_i32),
                              "compile int add fail.");
            return true;
        case INT_SUB:
            DEF_INT_BINARY_OP(compile_int_sub(cc, left, right, is_i32),
                              "compile int sub fail.");
            return true;
        case INT_MUL:
            DEF_INT_BINARY_OP(compile_int_mul(cc, left, right, is_i32),
                              "compile int mul fail.");
            return true;
        case INT_DIV_S:
        case INT_DIV_U:
        case INT_REM_S:
        case INT_REM_U:
            return compile_int_div(cc, arith_op, is_i32, p_frame_ip);
        default:
            bh_assert(0);
            return false;
    }

fail:
    return false;
}

bool
jit_compile_op_i32_arithmetic(JitCompContext *cc, IntArithmetic arith_op,
                              uint8 **p_frame_ip)
{
    return compile_op_int_arithmetic(cc, arith_op, true, p_frame_ip);
}

bool
jit_compile_op_i64_arithmetic(JitCompContext *cc, IntArithmetic arith_op,
                              uint8 **p_frame_ip)
{
    return compile_op_int_arithmetic(cc, arith_op, false, p_frame_ip);
}

bool
jit_compile_op_i32_bitwise(JitCompContext *cc, IntBitwise bitwise_op)
{
    return false;
}

bool
jit_compile_op_i64_bitwise(JitCompContext *cc, IntBitwise bitwise_op)
{
    return false;
}

bool
jit_compile_op_i32_shift(JitCompContext *cc, IntShift shift_op)
{
    return false;
}

bool
jit_compile_op_i64_shift(JitCompContext *cc, IntShift shift_op)
{
    return false;
}

bool
jit_compile_op_f32_math(JitCompContext *cc, FloatMath math_op)
{
    return false;
}

bool
jit_compile_op_f64_math(JitCompContext *cc, FloatMath math_op)
{
    return false;
}

bool
jit_compile_op_f32_arithmetic(JitCompContext *cc, FloatArithmetic arith_op)
{
    return false;
}

bool
jit_compile_op_f64_arithmetic(JitCompContext *cc, FloatArithmetic arith_op)
{
    return false;
}

bool
jit_compile_op_f32_copysign(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_f64_copysign(JitCompContext *cc)
{
    return false;
}
