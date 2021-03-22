/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
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
 * sp (stack pointer)
 *    |- sd to store 64-bit values from register to memory
 *    |- ld to load from stack to register
 * fp/s0 (frame pointer)
 * a0-a7 (8 integer arguments)
 *    |- sd to store
 *    |- ld to load
 * fa0-a7 (8 float arguments)
 *    |- fsd to store
 *    |- fld to load
 * t0-t6 (temporaries regisgers)
 *    |- caller saved
 */

        /* reserve space on stack to save return address and frame pointer */
        addi      sp, sp, -16
        sd        fp, 0(sp)             /* save frame pointer */
        sd        ra, 8(sp)             /* save return address */

        mv        fp, sp                /* set frame pointer to bottom of fixed frame */

        /* save function ptr, argv & nstacks */
        mv        t0, a0                /* t0 = function ptr */
        mv        t1, a1                /* t1 = argv array address */
        mv        t2, a2                /* t2 = nstack */

        /* fill in fa0-7 float-registers*/
        fld       fa0, 0(t1)            /* fa0 = argv[0] */
        fld       fa1, 8(t1)            /* fa1 = argv[1] */
        fld       fa2, 16(t1)           /* fa2 = argv[2] */
        fld       fa3, 24(t1)           /* fa3 = argv[3] */
        fld       fa4, 32(t1)           /* fa4 = argv[4] */
        fld       fa5, 40(t1)           /* fa5 = argv[5] */
        fld       fa6, 48(t1)           /* fa6 = argv[6] */
        fld       fa7, 56(t1)           /* fa7 = argv[7] */

        /* fill in a0-7 integer-registers*/
        ld        a0, 64(t1)            /* a0 = argv[8] */
        ld        a1, 72(t1)            /* a1 = argv[9] */
        ld        a2, 80(t1)            /* a2 = argv[10] */
        ld        a3, 88(t1)            /* a3 = argv[11] */
        ld        a4, 96(t1)            /* a4 = argv[12] */
        ld        a5, 104(t1)           /* a5 = argv[13] */
        ld        a6, 112(t1)           /* a6 = argv[14] */
        ld        a7, 120(t1)           /* a7 = argv[15] */

        addi      t1, t1, 128           /* t1 points to stack args */

        /* directly call the function if no args in stack,
           x0 always holds 0 */
        beq       t2, x0, call_func

        /* reserve enough stack space for function arguments */
        sll       t3, t2, 3             /* shift left 3 bits. t3 = n_stacks * 8 */
        sub       sp, sp, t3

        /* make 16-byte aligned */
        and       sp, sp, ~(15LL)

        /* save sp in t4 register */
        mv        t4, sp

        /* copy left arguments from caller stack to own frame stack */
loop_stack_args:
        beq       t2, x0, call_func
        ld        t5, 0(t1)             /* load stack argument, t5 = argv[i] */
        sd        t5, 0(t4)             /* store t5 to reseved stack, sp[j] = t5 */
        addi      t1, t1, 8             /* move to next stack argument */
        addi      t4, t4, 8             /* move to next stack pointer */
        addi      t2, t2, -1            /* decrease t2 every loop, nstacks = nstacks -1 */
        j loop_stack_args

call_func:
        jalr      t0

        /* restore registers pushed in stack or saved in another register */
return:
        mv        sp, fp                /* restore sp saved in fp before function call */
        ld        fp, 0(sp)             /* load previous frame poniter to fp register */
        ld        ra, 8(sp)             /* load previous return address to ra register */
        addi      sp, sp, 16            /* pop frame, restore sp */
        jr        ra


