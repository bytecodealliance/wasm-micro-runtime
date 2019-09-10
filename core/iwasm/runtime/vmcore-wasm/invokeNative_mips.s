/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

    .text
    .align 2
    .globl invokeNative
    .ent invokeNative
    .type invokeNative, @function

/**
 * On function entry parameters:
 * $4 = args
 * $5 = arg_num
 * $6 = func_ptr
 */

invokeNative:
    .frame $fp, 8, $0
    .mask 0x00000000, 0
    .fmask 0x00000000, 0

    /* Fixed part of frame */
    subu $sp, 8

    /* save registers */
    sw $31, 4($sp)
    sw $fp, 0($sp)

    /* set frame pointer to bottom of fixed frame */
    move $fp, $sp

    /* allocate enough stack space */
    sll $11, $5, 2
    subu $sp, $11

    /* make 8-byte aligned */
    and $sp, ~7 

    move $9, $sp
    move $25, $6    /* $25 = func_ptr */

push_args:
    beq $5, 0, done /* arg_num == 0 ? */
    lw $8, 0($4)
    sw $8, 0($9)
    addu $4, 4
    addu $9, 4
    subu $5, 1      /* arg_index-- */
    j push_args

done:
    lw $4, 0($sp)   /* Load $4..$7 from stack */
    lw $5, 4($sp)
    lw $6, 8($sp)
    lw $7, 12($sp)
    ldc1 $f12, 0($sp) /* Load $f12, $f13, $f14, $f15 */
    ldc1 $f14, 8($sp)

    jalr $25       /* call function */

    nop

    /* restore saved registers */
    move $sp, $fp
    lw $31, 4($sp)
    lw $fp, 0($sp)

    /* pop frame */
    addu $sp, $sp, 8

    j $31
    .end invokeNative
