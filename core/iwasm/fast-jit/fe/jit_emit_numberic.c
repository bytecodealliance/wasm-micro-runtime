/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_numberic.h"
#include "jit_emit_exception.h"
#include "jit_emit_control.h"
#include "../jit_frontend.h"
#include "../jit_codegen.h"

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
compile_int_div_no_check(JitCompContext *cc, IntArithmetic arith_op,
                         bool is_i32, JitReg left, JitReg right, JitReg res)
{
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg eax_hreg = jit_codegen_get_hreg_by_name("eax");
    JitReg edx_hreg = jit_codegen_get_hreg_by_name("edx");
    JitReg rax_hreg = jit_codegen_get_hreg_by_name("rax");
    JitReg rdx_hreg = jit_codegen_get_hreg_by_name("rdx");
#endif

    switch (arith_op) {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        case INT_DIV_S:
        case INT_DIV_U:
            if (is_i32) {
                GEN_INSN(MOV, eax_hreg, left);
                if (arith_op == INT_DIV_S)
                    GEN_INSN(DIV_S, eax_hreg, eax_hreg, right);
                else
                    GEN_INSN(DIV_U, eax_hreg, eax_hreg, right);
                /* Just to indicate that edx is used,
                   register allocator cannot spill it out */
                GEN_INSN(MOV, edx_hreg, edx_hreg);
                res = eax_hreg;
            }
            else {
                GEN_INSN(MOV, rax_hreg, left);
                /* Just to indicate that eax is used,
                   register allocator cannot spill it out */
                GEN_INSN(MOV, eax_hreg, eax_hreg);
                if (arith_op == INT_DIV_S)
                    GEN_INSN(DIV_S, rax_hreg, rax_hreg, right);
                else
                    GEN_INSN(DIV_U, rax_hreg, rax_hreg, right);
                /* Just to indicate that edx is used,
                   register allocator cannot spill it out */
                GEN_INSN(MOV, edx_hreg, edx_hreg);
                res = rax_hreg;
            }
            break;
        case INT_REM_S:
        case INT_REM_U:
            if (is_i32) {
                GEN_INSN(MOV, eax_hreg, left);
                if (arith_op == INT_REM_S)
                    GEN_INSN(REM_S, edx_hreg, eax_hreg, right);
                else
                    GEN_INSN(REM_U, edx_hreg, eax_hreg, right);
                res = edx_hreg;
            }
            else {
                GEN_INSN(MOV, rax_hreg, left);
                /* Just to indicate that eax is used,
                   register allocator cannot spill it out */
                GEN_INSN(MOV, eax_hreg, eax_hreg);
                if (arith_op == INT_REM_S)
                    GEN_INSN(REM_S, rdx_hreg, rax_hreg, right);
                else
                    GEN_INSN(REM_U, rdx_hreg, rax_hreg, right);
                /* Just to indicate that edx is used,
                   register allocator cannot spill it out */
                GEN_INSN(MOV, edx_hreg, edx_hreg);
                res = rdx_hreg;
            }
            break;
#else
        case INT_DIV_S:
            GEN_INSN(DIV_S, res, left, right);
            break;
        case INT_DIV_U:
            GEN_INSN(DIV_U, res, left, right);
            break;
        case INT_REM_S:
            GEN_INSN(REM_S, res, left, right);
            break;
        case INT_REM_U:
            GEN_INSN(REM_U, res, left, right);
            break;
#endif /* defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64) */
        default:
            bh_assert(0);
            return false;
    }

    if (is_i32)
        PUSH_I32(res);
    else
        PUSH_I64(res);
    return true;
fail:
    return false;
}

static bool
compile_int_div(JitCompContext *cc, IntArithmetic arith_op, bool is_i32,
                uint8 **p_frame_ip)
{
    JitReg left, right, res;

    bh_assert(arith_op == INT_DIV_S || arith_op == INT_DIV_U
              || arith_op == INT_REM_S || arith_op == INT_REM_U);

    if (is_i32) {
        POP_I32(right);
        POP_I32(left);
        res = jit_cc_new_reg_I32(cc);
    }
    else {
        POP_I64(right);
        POP_I64(left);
        res = jit_cc_new_reg_I64(cc);
    }

    if (jit_reg_is_const(right)) {
        int64 right_val = is_i32 ? (int64)jit_cc_get_const_I32(cc, right)
                                 : jit_cc_get_const_I64(cc, right);

        switch (right_val) {
            case 0:
            {
                /* Directly throw exception if divided by zero */
                if (!(jit_emit_exception(cc, EXCE_INTEGER_DIVIDE_BY_ZERO,
                                         JIT_OP_JMP, 0, NULL)))
                    goto fail;

                return jit_handle_next_reachable_block(cc, p_frame_ip);
            }
            case 1:
            {
                if (arith_op == INT_DIV_S || arith_op == INT_DIV_U) {
                    if (is_i32)
                        PUSH_I32(left);
                    else
                        PUSH_I64(left);
                }
                else {
                    if (is_i32)
                        PUSH_I32(NEW_CONST(I32, 0));
                    else
                        PUSH_I64(NEW_CONST(I64, 0));
                }
                return true;
            }
            case -1:
            {
                if (arith_op == INT_DIV_S) {
                    if (is_i32)
                        GEN_INSN(CMP, cc->cmp_reg, left,
                                 NEW_CONST(I32, INT32_MIN));
                    else
                        GEN_INSN(CMP, cc->cmp_reg, left,
                                 NEW_CONST(I64, INT64_MIN));

                    /* Throw integer overflow exception if left is
                       INT32_MIN or INT64_MIN */
                    if (!(jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW,
                                             JIT_OP_BEQ, cc->cmp_reg, NULL)))
                        goto fail;

                    /* Push -(left) to stack */
                    GEN_INSN(NEG, res, left);
                    if (is_i32)
                        PUSH_I32(res);
                    else
                        PUSH_I64(res);
                    return true;
                }
                else if (arith_op == INT_REM_S) {
                    if (is_i32)
                        PUSH_I32(NEW_CONST(I32, 0));
                    else
                        PUSH_I64(NEW_CONST(I64, 0));
                    return true;
                }
                else {
                    /* Build default div and rem */
                    return compile_int_div_no_check(cc, arith_op, is_i32, left,
                                                    right, res);
                }
            }
            default:
            {
                /* Build default div and rem */
                return compile_int_div_no_check(cc, arith_op, is_i32, left,
                                                right, res);
            }
        }
    }
    else {
        JitReg cmp1 = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
        JitReg cmp2 = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);

        GEN_INSN(CMP, cc->cmp_reg, right,
                 is_i32 ? NEW_CONST(I32, 0) : NEW_CONST(I64, 0));
        /* Throw integer divided by zero exception if right is zero */
        if (!(jit_emit_exception(cc, EXCE_INTEGER_DIVIDE_BY_ZERO, JIT_OP_BEQ,
                                 cc->cmp_reg, NULL)))
            goto fail;

        switch (arith_op) {
            case INT_DIV_S:
            case INT_REM_S:
                /* Check integer overflow */
                GEN_INSN(CMP, cc->cmp_reg, left,
                         is_i32 ? NEW_CONST(I32, INT32_MIN)
                                : NEW_CONST(I64, INT64_MIN));
                GEN_INSN(SELECTEQ, cmp1, cc->cmp_reg, NEW_CONST(I32, 1),
                         NEW_CONST(I32, 0));
                GEN_INSN(CMP, cc->cmp_reg, right,
                         is_i32 ? NEW_CONST(I32, -1) : NEW_CONST(I64, -1LL));
                GEN_INSN(SELECTEQ, cmp2, cc->cmp_reg, NEW_CONST(I32, 1),
                         NEW_CONST(I32, 0));
                GEN_INSN(AND, cmp1, cmp1, cmp2);
                GEN_INSN(CMP, cc->cmp_reg, cmp1, NEW_CONST(I32, 1));
                /* Throw integer overflow exception if left is INT32_MIN or
                   INT64_MIN, and right is -1 */
                if (!(jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW, JIT_OP_BEQ,
                                         cc->cmp_reg, NULL)))
                    goto fail;

                /* Build default div and rem */
                return compile_int_div_no_check(cc, arith_op, is_i32, left,
                                                right, res);
                return true;
            default:
                /* Build default div and rem */
                return compile_int_div_no_check(cc, arith_op, is_i32, left,
                                                right, res);
        }
    }

fail:
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

static bool
compile_int_shift(JitCompContext *cc, IntShift shift_op, bool is_i32)
{
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg ecx_hreg = jit_codegen_get_hreg_by_name("ecx");
    JitReg rcx_hreg = jit_codegen_get_hreg_by_name("rcx");
#endif
    JitReg left, right, mod_right, res;

    POP_INT(right);
    POP_INT(left);

    /* right modulo N */
    if (jit_reg_is_const(right)) {
        if (is_i32) {
            int32 right_value = jit_cc_get_const_I32(cc, right);
            right_value = right_value & 0x1f;
            if (0 == right_value) {
                res = left;
                goto shortcut;
            }
            else {
                mod_right = NEW_CONST(I32, right_value);
            }
        }
        else {
            int64 right_value = jit_cc_get_const_I64(cc, right);
            right_value = right_value & 0x3f;
            if (0 == right_value) {
                res = left;
                goto shortcut;
            }
            else {
                mod_right = NEW_CONST(I64, right_value);
            }
        }
    }
    else {
        if (is_i32) {
            mod_right = jit_cc_new_reg_I32(cc);
            GEN_INSN(AND, mod_right, right, NEW_CONST(I32, 0x1f));
        }
        else {
            mod_right = jit_cc_new_reg_I64(cc);
            GEN_INSN(AND, mod_right, right, NEW_CONST(I64, 0x3f));
        }
    }

    /* do shift */
    if (is_i32) {
        res = jit_cc_new_reg_I32(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, ecx_hreg, mod_right);
#endif
    }
    else {
        res = jit_cc_new_reg_I64(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, rcx_hreg, mod_right);
#endif
    }

    switch (shift_op) {
        case INT_SHL:
        {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
            GEN_INSN(SHL, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
            GEN_INSN(SHL, res, left, mod_right);
#endif
            break;
        }
        case INT_SHR_S:
        {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
            GEN_INSN(SHRS, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
            GEN_INSN(SHRS, res, left, mod_right);
#endif
            break;
        }
        case INT_SHR_U:
        {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
            GEN_INSN(SHRU, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
            GEN_INSN(SHRU, res, left, mod_right);
#endif
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

        /**
         * Just to indicate that ecx is used, register allocator cannot spill
         * it out. Especially when rcx is ued.
         */
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(MOV, ecx_hreg, ecx_hreg);
#endif

shortcut:
    PUSH_INT(res);
    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_shift(JitCompContext *cc, IntShift shift_op)
{
    return compile_int_shift(cc, shift_op, true);
}

bool
jit_compile_op_i64_shift(JitCompContext *cc, IntShift shift_op)
{
    return compile_int_shift(cc, shift_op, false);
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
