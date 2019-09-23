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
        .align  2
        .global invokeNative
        .type   invokeNative,function

/*
 * Arguments passed in:
 *
 * r0 function pntr
 * r1 argv
 * r2 argc
 */

invokeNative:
        stmfd   sp!, {r4, r5, r6, r7, lr}
        mov     ip, r0           /* get function ptr */
        mov     r4, r1           /* get argv */
        mov     r5, r2           /* get argc */

        cmp     r5, #2           /* is argc < 2 ? */
        blt     return

        ldr     r0, [r4], #4     /* argv[0] */
        ldr     r1, [r4], #4     /* argv[1] */

        mov     r6, #0

        cmp     r5, #2
        beq     call_func
        ldr     r2, [r4], #4
        cmp     r5, #3
        beq     call_func
        ldr     r3, [r4], #4

        subs    r5, r5, #4       /* now we have r0 ~ r3 */

        /* Ensure address is 8 byte aligned */
        mov     r6, r5, lsl#2
        add     r6, r6, #7
        bic     r6, r6, #7
        add     r6, r6, #4       /* +4 because only odd(5) registers are in stack */
        subs    sp, sp, r6       /* for stacked args */
        mov     r7, sp

loop_args:
        cmp     r5, #0
        beq     call_func
        ldr     lr, [r4], #4
        str     lr, [r7], #4
        subs    r5, r5, #1
        b       loop_args

call_func:
        blx     ip

        add     sp, sp, r6       /* recover sp */

return:
        ldmfd   sp!, {r4, r5, r6, r7, lr}
        bx      lr
