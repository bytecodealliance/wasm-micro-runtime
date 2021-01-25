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
 *    |- sw to store 32-bit values from register to memory
 *    |- lw to load from stack to register
 * fp/s0 (frame pointer)
 * a0-a7 (8 integer arguments)
 *    |- sw to store
 *    |- lw to load
 * t0-t6 (temporaries regisgers)
 *    |- caller saved
 */

        /* reserve space on stack to save return address and frame pointer */
        addi      sp, sp, -8
        sw        fp, 0(sp)            /* save frame pointer */
        sw        ra, 4(sp)            /* save return address */

        mv        fp, sp               /* set frame pointer to bottom of fixed frame */

        /* save function ptr, argv & nstacks */
        mv        t0, a0               /* t0 = function ptr */
        mv        t1, a1               /* t1 = argv array address */
        mv        t2, a2               /* t2 = nstack */

        /* fill in a0-7 integer-registers */
        lw        a0, 0(t1)            /* a0 = argv[0] */
        lw        a1, 4(t1)            /* a1 = argv[1] */
        lw        a2, 8(t1)            /* a2 = argv[2] */
        lw        a3, 12(t1)           /* a3 = argv[3] */
        lw        a4, 16(t1)           /* a4 = argv[4] */
        lw        a5, 20(t1)           /* a5 = argv[5] */
        lw        a6, 24(t1)           /* a6 = argv[6] */
        lw        a7, 28(t1)           /* a7 = argv[7] */

        addi      t1, t1, 32           /* t1 points to stack args */

        /* directly call the function if no args in stack,
           x0 always holds 0 */
        beq       t2, x0, call_func

        /* reserve enough stack space for function arguments */
        sll       t3, t2, 2             /* shift left 2 bits. t3 = n_stacks * 4 */
        sub       sp, sp, t3

        /* make 16-byte aligned */
        and       sp, sp, ~15

        /* save sp in t4 register */
        mv        t4, sp

        /* copy left arguments from caller stack to own frame stack */
loop_stack_args:
        beq       t2, x0, call_func
        lw        t5, 0(t1)             /* load stack argument, t5 = argv[i] */
        sw        t5, 0(t4)             /* store t5 to reseved stack, sp[j] = t5 */
        addi      t1, t1, 4             /* move to next stack argument */
        addi      t4, t4, 4             /* move to next stack pointer */
        addi      t2, t2, -1            /* decrease t2 every loop, nstacks = nstacks -1 */
        j loop_stack_args

call_func:
        jalr      t0

        /* restore registers pushed in stack or saved in another register */
return:
        mv        sp, fp                /* restore sp saved in fp before function call */
        lw        fp, 0(sp)             /* load previous frame poniter to fp register */
        lw        ra, 4(sp)             /* load previous return address to ra register */
        addi      sp, sp, 8             /* pop frame, restore sp */
        jr        ra

