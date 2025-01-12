/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#if defined(__loongarch_soft_float)
#define LA_FPREG_SIZE    0
#elif defined(__loongarch_single_float)
#define LA_OP_LOADFPREG  fld.s
#define LA_OP_STROEFPREG fst.s
#define LA_FPREG_SIZE    4
#elif defined(__loongarch_double_float)
#define LA_OP_LOADFPREG  fld.d
#define LA_OP_STROEFPREG fst.d
#define LA_FPREG_SIZE    8
#endif

#if __loongarch_grlen == 32
#define LA_OP_ADDI     addi.w
#define LA_OP_SLLI     slli.w
#define LA_OP_SUB      sub.w
#define LA_OP_BSTRINS  bstrins.w
#define LA_OP_LOADREG  ld.w
#define LA_OP_STOREREG st.w
#define LA_REG_SIZE    4
#define LA_REG_SHIFT   2
#define LA_FP_OFFSET   (8 * LA_REG_SIZE)
#define LA_INT_OFFSET  0
#else
#define LA_OP_ADDI     addi.d
#define LA_OP_SLLI     slli.d
#define LA_OP_SUB      sub.d
#define LA_OP_BSTRINS  bstrins.d
#define LA_OP_LOADREG  ld.d
#define LA_OP_STOREREG st.d
#define LA_REG_SIZE    8
#define LA_REG_SHIFT   3
#define LA_FP_OFFSET   0
#define LA_INT_OFFSET  (8 * LA_FPREG_SIZE)
#endif

        .text
        .align  2
#ifndef BH_PLATFORM_DARWIN
        .globl invokeNative
        .type  invokeNative, function
invokeNative:
#else
        .globl _invokeNative
_invokeNative:
#endif /* end of BH_PLATFORM_DARWIN */

/*
 * Arguments passed in:
 *
 * a0 function ptr
 * a1 argv
 * a2 nstacks
 */

/*
 * $sp (stack pointer)
 *    |- st.d/st.w to store 64/32-bit values from register to memory
 *    |- ld.d/ld.w to load from stack to register
 * $fp/$s9 (frame pointer)
 * $a0-$a7 (8 integer arguments)
 *    |- st.d/st.w to store
 *    |- ld.d/ld.w to load
 * $fa0-$a7 (8 float arguments)
 *    |- fst.d/fst.s to store
 *    |- fld.d/fld.s to load
 * $t0-$t8 (temporaries regisgers)
 *    |- caller saved
 */

        /* reserve space on stack to save return address and frame pointer */
        LA_OP_ADDI       $sp, $sp, - 2 * LA_REG_SIZE
        LA_OP_STOREREG   $fp, $sp, 0 * LA_REG_SIZE    /* save frame pointer */
        LA_OP_STOREREG   $ra, $sp, 1 * LA_REG_SIZE    /* save return address */

        move             $fp, $sp                     /* set frame pointer to bottom of fixed frame */

        /* save function ptr, argv & nstacks */
        move             $t0, $a0                     /* $t0 = function ptr */
        move             $t1, $a1                     /* $t1 = argv array address */
        move             $t2, $a2                     /* $t2 = nstack */

#ifndef __loongarch_soft_float
        /* fill in $fa0-7 float-registers*/
        LA_OP_LOADFPREG  $fa0, $t1, LA_FP_OFFSET + 0 * LA_FPREG_SIZE /* $fa0 */
        LA_OP_LOADFPREG  $fa1, $t1, LA_FP_OFFSET + 1 * LA_FPREG_SIZE /* $fa1 */
        LA_OP_LOADFPREG  $fa2, $t1, LA_FP_OFFSET + 2 * LA_FPREG_SIZE /* $fa2 */
        LA_OP_LOADFPREG  $fa3, $t1, LA_FP_OFFSET + 3 * LA_FPREG_SIZE /* $fa3 */
        LA_OP_LOADFPREG  $fa4, $t1, LA_FP_OFFSET + 4 * LA_FPREG_SIZE /* $fa4 */
        LA_OP_LOADFPREG  $fa5, $t1, LA_FP_OFFSET + 5 * LA_FPREG_SIZE /* $fa5 */
        LA_OP_LOADFPREG  $fa6, $t1, LA_FP_OFFSET + 6 * LA_FPREG_SIZE /* $fa6 */
        LA_OP_LOADFPREG  $fa7, $t1, LA_FP_OFFSET + 7 * LA_FPREG_SIZE /* $fa7 */
#endif

        /* fill in $a0-7 integer-registers*/
        LA_OP_LOADREG    $a0, $t1, LA_INT_OFFSET + 0 * LA_REG_SIZE    /* $a0 */
        LA_OP_LOADREG    $a1, $t1, LA_INT_OFFSET + 1 * LA_REG_SIZE    /* $a1 */
        LA_OP_LOADREG    $a2, $t1, LA_INT_OFFSET + 2 * LA_REG_SIZE    /* $a2 */
        LA_OP_LOADREG    $a3, $t1, LA_INT_OFFSET + 3 * LA_REG_SIZE    /* $a3 */
        LA_OP_LOADREG    $a4, $t1, LA_INT_OFFSET + 4 * LA_REG_SIZE    /* $a4 */
        LA_OP_LOADREG    $a5, $t1, LA_INT_OFFSET + 5 * LA_REG_SIZE    /* $a5 */
        LA_OP_LOADREG    $a6, $t1, LA_INT_OFFSET + 6 * LA_REG_SIZE    /* $a6 */
        LA_OP_LOADREG    $a7, $t1, LA_INT_OFFSET + 7 * LA_REG_SIZE    /* $a7 */

        /* $t1 points to stack args */

        /* LA_FPREG_SIZE is zero when __loongarch_soft_float defined */
        LA_OP_ADDI       $t1, $t1, LA_REG_SIZE * 8 + LA_FPREG_SIZE * 8

        /* directly call the function if no args in stack */
        beqz             $t2, call_func

        /* reserve enough stack space for function arguments */
        LA_OP_SLLI       $t3, $t2, LA_REG_SHIFT       /* shift left 3 bits. $t3 = n_stacks * 8 */
        LA_OP_SUB        $sp, $sp, $t3

        /* make 16-byte aligned */
        LA_OP_BSTRINS    $sp, $zero, 3, 0

        /* save $sp in t4 register */
        move             $t4, $sp

        /* copy left arguments from caller stack to own frame stack */
loop_stack_args:
        beqz             $t2, call_func
        LA_OP_LOADREG    $t5, $t1, 0                  /* load stack argument, $t5 = argv[i] */
        LA_OP_STOREREG   $t5, $t4, 0                  /* store $t5 to reseved stack, $sp[j] = $t5 */
        LA_OP_ADDI       $t1, $t1, LA_REG_SIZE        /* move to next stack argument */
        LA_OP_ADDI       $t4, $t4, LA_REG_SIZE        /* move to next stack pointer */
        LA_OP_ADDI       $t2, $t2, -1                 /* decrease $t2 every loop, nstacks = nstacks -1 */
        b loop_stack_args

call_func:
        jirl             $ra, $t0, 0

        /* restore registers pushed in stack or saved in another register */
return:
        move             $sp, $fp                     /* restore $sp saved in fp before function call */
        LA_OP_LOADREG    $fp, $sp, 0 * LA_REG_SIZE    /* load previous frame poniter to $fp register */
        LA_OP_LOADREG    $ra, $sp, 1 * LA_REG_SIZE    /* load previous return address to $ra register */
        LA_OP_ADDI       $sp, $sp, 2 * LA_REG_SIZE    /* pop frame, restore $sp */
        jr               $ra
