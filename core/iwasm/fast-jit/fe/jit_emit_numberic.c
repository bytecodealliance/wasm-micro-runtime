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

#define IS_CONST_ALL_ONE(val, is_i32)                    \
    (jit_reg_is_const(val)                               \
     && ((is_i32 && jit_cc_get_const_I32(cc, val) == -1) \
         || (!is_i32 && jit_cc_get_const_I64(cc, val) == -1LL)))

#define IS_CONST_ZERO(val)                              \
    (jit_reg_is_const(val)                              \
     && ((is_i32 && jit_cc_get_const_I32(cc, val) == 0) \
         || (!is_i32 && jit_cc_get_const_I64(cc, val) == 0)))

/* macros for integer binary operations (ibinop) */

#define __DEF_BI_INT_CONST_OPS(bits, opname, op)                               \
    static int##bits do_i##bits##_const_##opname(int##bits lhs, int##bits rhs) \
    {                                                                          \
        return lhs op rhs;                                                     \
    }

#define DEF_BI_INT_CONST_OPS(opname, op)   \
    __DEF_BI_INT_CONST_OPS(32, opname, op) \
    __DEF_BI_INT_CONST_OPS(64, opname, op)

#define DEF_UNI_INT_CONST_OPS(opname)            \
    static JitReg compile_int_##opname##_consts( \
        JitCompContext *cc, JitReg left, JitReg right, bool is_i32)

typedef JitReg (*uni_const_handler)(JitCompContext *, JitReg, JitReg, bool);
typedef int32 (*bin_i32_consts_handler)(int32, int32);
typedef int64 (*bin_i64_consts_handler)(int64, int64);

/* ibinopt for integer binary operations */
static JitReg
compile_op_ibinopt_const(JitCompContext *cc, JitReg left, JitReg right,
                         bool is_i32, uni_const_handler handle_one_const,
                         bin_i32_consts_handler handle_two_i32_const,
                         bin_i64_consts_handler handle_two_i64_const)
{
    JitReg res;

    if (jit_reg_is_const(left) && jit_reg_is_const(right)) {
        if (is_i32) {
            int32 left_val = jit_cc_get_const_I32(cc, left);
            int32 right_val = jit_cc_get_const_I32(cc, right);
            res = NEW_CONST(I32, handle_two_i32_const(left_val, right_val));
        }
        else {
            int64 left_val = jit_cc_get_const_I64(cc, left);
            int64 right_val = jit_cc_get_const_I64(cc, right);
            res = NEW_CONST(I64, handle_two_i64_const(left_val, right_val));
        }
        goto shortcut;
    }

    if (jit_reg_is_const(left) || jit_reg_is_const(right)) {
        res = handle_one_const(cc, left, right, is_i32);
        if (!res)
            goto shortcut;
    }

    return 0;
shortcut:
    return res;
}

#define CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, opname) \
    compile_op_ibinopt_const(cc, left, right, is_i32,                 \
                             compile_int_##opname##_consts,           \
                             do_i32_const_##opname, do_i64_const_##opname)

DEF_UNI_INT_CONST_OPS(add)
{
    /* If one of the operands is 0, just return the other */
    if (IS_CONST_ZERO(left))
        return right;
    if (IS_CONST_ZERO(right))
        return left;

    return 0;
}

DEF_BI_INT_CONST_OPS(add, +)

static JitReg
compile_int_add(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, add);
    if (res)
        goto shortcut;

    /* Build add */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(ADD, res, left, right);

shortcut:
    return res;
}

DEF_UNI_INT_CONST_OPS(sub)
{
    /* If the right operand is 0, just return the left */
    if (IS_CONST_ZERO(right))
        return left;

    return 0;
}

DEF_BI_INT_CONST_OPS(sub, -)

static JitReg
compile_int_sub(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, sub);
    if (res)
        goto shortcut;

    /* Build sub */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(SUB, res, left, right);

shortcut:
    return res;
}

DEF_UNI_INT_CONST_OPS(mul)
{
    /* If one of the operands is 0, just return constant 0 */
    if (IS_CONST_ZERO(left) || IS_CONST_ZERO(right))
        return is_i32 ? NEW_CONST(I32, 0) : NEW_CONST(I64, 0);

    return 0;
}

DEF_BI_INT_CONST_OPS(mul, *)

static JitReg
compile_int_mul(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, mul);
    if (res)
        goto shortcut;

    /* Build mul */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(MUL, res, left, right);

shortcut:
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

    if (jit_reg_is_const(right) && jit_reg_is_const(left)) {
        if (INT_DIV_S == arith_op || INT_REM_S == arith_op) {
            if (is_i32) {
                int32 lhs = jit_cc_get_const_I32(cc, left);
                int32 rhs = jit_cc_get_const_I32(cc, right);
                if (INT_DIV_S == arith_op) {
                    res = NEW_CONST(I32, lhs / rhs);
                }
                else {
                    res = NEW_CONST(I32, lhs % rhs);
                }
                PUSH_I32(res);
                return true;
            }
            else {
                int64 lhs = jit_cc_get_const_I64(cc, left);
                int64 rhs = jit_cc_get_const_I64(cc, right);
                if (INT_DIV_S == arith_op) {
                    res = NEW_CONST(I64, lhs / rhs);
                }
                else {
                    res = NEW_CONST(I64, lhs % rhs);
                }
                PUSH_I64(res);
                return true;
            }
        }
        else {
            if (is_i32) {
                uint32 lhs = (uint32)jit_cc_get_const_I32(cc, left);
                uint32 rhs = (uint32)jit_cc_get_const_I32(cc, right);
                if (INT_DIV_U == arith_op) {
                    res = NEW_CONST(I32, lhs / rhs);
                }
                else {
                    res = NEW_CONST(I32, lhs % rhs);
                }
                PUSH_I32(res);
                return true;
            }
            else {
                uint64 lhs = (uint64)jit_cc_get_const_I64(cc, left);
                uint64 rhs = (uint64)jit_cc_get_const_I64(cc, right);
                if (INT_DIV_U == arith_op) {
                    res = NEW_CONST(I64, lhs / rhs);
                }
                else {
                    res = NEW_CONST(I64, lhs % rhs);
                }
                PUSH_I64(res);
                return true;
            }
        }
    }

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

DEF_UNI_INT_CONST_OPS(and)
{
    JitReg res;
    if (IS_CONST_ZERO(left) || IS_CONST_ZERO(right)) {
        res = is_i32 ? NEW_CONST(I32, 0) : NEW_CONST(I64, 0);
        goto shortcut;
    }

    if (IS_CONST_ALL_ONE(left, is_i32)) {
        res = right;
        goto shortcut;
    }

    if (IS_CONST_ALL_ONE(right, is_i32)) {
        res = left;
        goto shortcut;
    }

    return 0;
shortcut:
    return res;
}

DEF_BI_INT_CONST_OPS(and, &)

static JitReg
compile_int_and(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* shortcuts */
    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, and);
    if (res)
        goto shortcut;

    /* do and */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(AND, res, left, right);

shortcut:
    return res;
}

DEF_UNI_INT_CONST_OPS(or)
{
    JitReg res;

    if (IS_CONST_ZERO(left)) {
        res = right;
        goto shortcut;
    }

    if (IS_CONST_ZERO(right)) {
        res = left;
        goto shortcut;
    }

    if (IS_CONST_ALL_ONE(left, is_i32) || IS_CONST_ALL_ONE(right, is_i32)) {
        res = is_i32 ? NEW_CONST(I32, -1) : NEW_CONST(I64, -1LL);
        goto shortcut;
    }

    return 0;
shortcut:
    return res;
}

DEF_BI_INT_CONST_OPS(or, |)

static JitReg
compile_int_or(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* shortcuts */
    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, or);
    if (res)
        goto shortcut;

    /* do or */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(OR, res, left, right);

shortcut:
    return res;
}

DEF_UNI_INT_CONST_OPS(xor)
{
    if (IS_CONST_ZERO(left))
        return right;

    if (IS_CONST_ZERO(right))
        return left;

    if (is_i32) {
        if (jit_cc_get_const_I32(cc, left) == jit_cc_get_const_I32(cc, right))
            return NEW_CONST(I32, 0);
    }
    else {
        if (jit_cc_get_const_I64(cc, left) == jit_cc_get_const_I64(cc, right))
            return NEW_CONST(I64, 0);
    }

    return 0;
}

DEF_BI_INT_CONST_OPS(xor, ^)

static JitReg
compile_int_xor(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;

    /* shortcuts */
    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, xor);
    if (res)
        goto shortcut;

    /* do xor */
    res = is_i32 ? jit_cc_new_reg_I32(cc) : jit_cc_new_reg_I64(cc);
    GEN_INSN(XOR, res, left, right);

shortcut:
    return res;
}

static bool
compile_op_int_bitwise(JitCompContext *cc, IntBitwise arith_op, bool is_i32)
{
    JitReg left, right, res;

    POP_INT(right);
    POP_INT(left);

    switch (arith_op) {
        case INT_AND:
        {
            res = compile_int_and(cc, left, right, is_i32);
            break;
        }
        case INT_OR:
        {
            res = compile_int_or(cc, left, right, is_i32);
            break;
        }
        case INT_XOR:
        {
            res = compile_int_xor(cc, left, right, is_i32);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    PUSH_INT(res);
    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_bitwise(JitCompContext *cc, IntBitwise bitwise_op)
{
    return compile_op_int_bitwise(cc, bitwise_op, true);
}

bool
jit_compile_op_i64_bitwise(JitCompContext *cc, IntBitwise bitwise_op)
{
    return compile_op_int_bitwise(cc, bitwise_op, false);
}

DEF_UNI_INT_CONST_OPS(shl)
{
    if (IS_CONST_ZERO(right)) {
        return left;
    }
    return 0;
}

DEF_UNI_INT_CONST_OPS(shrs)
{
    if (IS_CONST_ZERO(right)) {
        return left;
    }
    return 0;
}

DEF_UNI_INT_CONST_OPS(shru)
{
    if (IS_CONST_ZERO(right)) {
        return left;
    }
    return 0;
}

DEF_BI_INT_CONST_OPS(shl, <<)

DEF_BI_INT_CONST_OPS(shrs, >>)

static int32
do_i32_const_shru(int32 lhs, int32 rhs)
{
    return (uint32)lhs >> rhs;
}

static int64
do_i64_const_shru(int64 lhs, int64 rhs)
{
    return (uint64)lhs >> rhs;
}

static JitReg
modulo_rhs(JitCompContext *cc, JitReg rhs, bool is_i32)
{
    JitReg res;
    if (jit_reg_is_const(rhs)) {
        if (is_i32) {
            int32 val = jit_cc_get_const_I32(cc, rhs);
            val = val & 0x1f;
            res = NEW_CONST(I32, val);
        }
        else {
            int64 val = jit_cc_get_const_I64(cc, rhs);
            val = val & 0x3f;
            res = NEW_CONST(I64, val);
        }
    }
    else {
        if (is_i32) {
            res = jit_cc_new_reg_I32(cc);
            GEN_INSN(AND, res, rhs, NEW_CONST(I32, 0x1f));
        }
        else {
            res = jit_cc_new_reg_I64(cc);
            GEN_INSN(AND, res, rhs, NEW_CONST(I64, 0x3f));
        }
    }
    return res;
}

static JitReg
compile_int_shl(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg ecx_hreg = jit_codegen_get_hreg_by_name("ecx");
    JitReg rcx_hreg = jit_codegen_get_hreg_by_name("rcx");
#endif

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, shl);
    if (res)
        goto shortcut;

    if (is_i32) {
        res = jit_cc_new_reg_I32(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, ecx_hreg, right);
#endif
    }
    else {
        res = jit_cc_new_reg_I64(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, rcx_hreg, right);
#endif
    }

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(SHL, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
    GEN_INSN(SHL, res, left, right);
#endif
    /**
     * Just to indicate that ecx is used, register allocator cannot spill
     * it out. Especially when rcx is ued.
     */
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(MOV, ecx_hreg, ecx_hreg);
#endif

shortcut:
    return res;
}

static JitReg
compile_int_shrs(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg ecx_hreg = jit_codegen_get_hreg_by_name("ecx");
    JitReg rcx_hreg = jit_codegen_get_hreg_by_name("rcx");
#endif

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, shrs);
    if (res)
        goto shortcut;

    if (is_i32) {
        res = jit_cc_new_reg_I32(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, ecx_hreg, right);
#endif
    }
    else {
        res = jit_cc_new_reg_I64(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, rcx_hreg, right);
#endif
    }

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(SHRS, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
    GEN_INSN(SHRS, res, left, right);
#endif
    /**
     * Just to indicate that ecx is used, register allocator cannot spill
     * it out. Especially when rcx is ued.
     */
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(MOV, ecx_hreg, ecx_hreg);
#endif

shortcut:
    return res;
}

static JitReg
compile_int_shru(JitCompContext *cc, JitReg left, JitReg right, bool is_i32)
{
    JitReg res;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg ecx_hreg = jit_codegen_get_hreg_by_name("ecx");
    JitReg rcx_hreg = jit_codegen_get_hreg_by_name("rcx");
#endif

    res = CHECK_AND_PROCESS_INT_CONSTS(cc, left, right, is_i32, shru);
    if (res)
        goto shortcut;

    if (is_i32) {
        res = jit_cc_new_reg_I32(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, ecx_hreg, right);
#endif
    }
    else {
        res = jit_cc_new_reg_I64(cc);
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        GEN_INSN(MOV, rcx_hreg, right);
#endif
    }

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(SHRU, res, left, is_i32 ? ecx_hreg : rcx_hreg);
#else
    GEN_INSN(SHRU, res, left, right);
#endif
    /**
     * Just to indicate that ecx is used, register allocator cannot spill
     * it out. Especially when rcx is ued.
     */
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    GEN_INSN(MOV, ecx_hreg, ecx_hreg);
#endif

shortcut:
    return res;
}

static bool
compile_op_int_shift(JitCompContext *cc, IntShift shift_op, bool is_i32)
{
    JitReg left, right, res;

    POP_INT(right);
    POP_INT(left);

    right = modulo_rhs(cc, right, true);

    switch (shift_op) {
        case INT_SHL:
        {
            res = compile_int_shl(cc, left, right, is_i32);
            break;
        }
        case INT_SHR_S:
        {
            res = compile_int_shrs(cc, left, right, is_i32);
            break;
        }
        case INT_SHR_U:
        {
            res = compile_int_shru(cc, left, right, is_i32);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    PUSH_INT(res);
    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_shift(JitCompContext *cc, IntShift shift_op)
{
    return compile_op_int_shift(cc, shift_op, true);
}

bool
jit_compile_op_i64_shift(JitCompContext *cc, IntShift shift_op)
{
    return compile_op_int_shift(cc, shift_op, false);
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
