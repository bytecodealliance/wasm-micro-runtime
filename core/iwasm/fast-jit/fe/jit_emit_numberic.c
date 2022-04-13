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
compile_int_div(JitCompContext *cc, IntArithmetic arith_op, bool is_i32,
                uint8 **p_frame_ip)
{
    JitReg left, right, res;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg eax_hreg = jit_codegen_get_hreg_by_name("eax");
    JitReg edx_hreg = jit_codegen_get_hreg_by_name("edx");
    JitReg rax_hreg = jit_codegen_get_hreg_by_name("rax");
    JitReg rdx_hreg = jit_codegen_get_hreg_by_name("rdx");
#endif

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
                                         JIT_OP_JMP, false, NULL)))
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

                    /* Throw conditional exception if overflow */
                    if (!(jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW,
                                             JIT_OP_BEQ, true, NULL)))
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
                    /* fall to default */
                    goto handle_default;
                }
            }
            handle_default:
            default:
            {
                /* Build div and rem */
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
            }
        }
    }
    else {
#if 0
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
                                         EXCE_INTEGER_OVERFLOW, true, overflow,
                                         check_overflow_succ)))
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
                return compile_rems(comp_ctx, func_ctx, left, right, overflow,
                                    is_i32);
            case INT_REM_U:
                LLVM_BUILD_OP(URem, left, right, res, "rem_u", false);
                PUSH_INT(res);
                return true;
            default:
                bh_assert(0);
                return false;
        }
#endif
    }

    return true;
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
