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
        .syntax unified
        .cfi_startproc

/*
 * invokeNative - Invoke a native function (VFP version)
 *
 * FUNCTION SIGNATURE:
 *   uint32_t invokeNative(void *func_ptr, uint32_t *argv, uint32_t argc)
 *
 * INPUT PARAMETERS:
 *   r0: func_ptr - Pointer to the native function to invoke
 *   r1: argv     - Array of arguments to pass to the function
 *   r2: argc     - Number of arguments in the argv array
 *
 * RETURN VALUE:
 *   r0: Return value from the invoked native function
 *
 * CALLING CONVENTION:
 *   - First 4 integer arguments (argv[0] to argv[3]) are passed in registers r0-r3
 *   - First 16 floating point arguments (s0-s15) are passed in VFP registers
 *   - Additional arguments are passed on the stack
 *   - The first argument (argv[0]) is always the execution environment
 *   - Return value is in r0
 *
 * STACK FRAME LAYOUT:
 *   The function establishes a stack frame with the following layout:
 *   [Higher addresses]
 *   +------------------+
 *   | return address   | <- saved lr
 *   +------------------+
 *   | saved r7         | <- frame pointer
 *   +------------------+
 *   | saved r6         |
 *   +------------------+
 *   | saved r5         |
 *   +------------------+
 *   | saved r4         | <- r7 points here initially
 *   +------------------+
 *   | padding (4 bytes)| <- alignment padding
 *   +------------------+
 *   [Lower addresses]
 *
 * REGISTER USAGE:
 *   - r0-r3: Integer function parameters and return value
 *   - r4-r7: Callee-saved registers (r7 is frame pointer)
 *   - r12 (ip): Temporary register for function pointer
 *   - lr: Return address (callee-saved)
 *   - sp: Stack pointer
 *   - s0-s15: VFP floating point registers for float/double arguments
 *     NOTE: VFP registers are NOT saved in the stack frame - they are used
 *     only for argument passing according to ARM ABI. The function does not
 *     preserve these registers as they are caller-saved for VFP operations.
 */

        /*
         * Function prologue
         * Save callee-saved registers and establish stack frame
         * Uses 24 bytes of stack space (6 registers × 4 bytes each)
         * Maintains 8-byte stack alignment for VFP operations
         */
        push    {r7, lr}
        .cfi_def_cfa_offset 8
        .cfi_offset lr, -4
        .cfi_offset r7, -8

        /*
         * Save remaining callee-saved registers (r4-r6)
         * These are needed for register allocation during argument processing
         */
        push    {r4, r5, r6}
        .cfi_adjust_cfa_offset 12
        .cfi_offset r4, -12
        .cfi_offset r5, -16
        .cfi_offset r6, -20

        /*
         * Add 4-byte alignment padding to maintain 8-byte stack alignment
         * This is required by the ARM ABI for proper VFP function calls
         */
        sub     sp, #4
        .cfi_adjust_cfa_offset 4

        /*
         * Establish frame pointer (r7) for stack unwinding
         * CFA (Canonical Frame Address) = r7 + 24
         */
        mov     r7, sp
        .cfi_def_cfa r7, 24

        /*
         * Save function parameters in preserved registers
         * r0=function ptr, r1=argv, r2=argc
         */
        mov     ip, r0
        mov     r4, r1
        mov     r5, r2

        /*
         * Load first 4 integer arguments into registers r0-r3
         * These correspond to argv[0] through argv[3]
         */
        ldr     r0, [r4, #0]
        ldr     r1, [r4, #4]
        ldr     r2, [r4, #8]
        ldr     r3, [r4, #12]
        add     r4, r4, #16

        /*
         * Load first 16 floating point arguments into VFP registers s0-s15
         * These are loaded from consecutive memory locations in argv array
         * Supports both float (32-bit) and double (64-bit) arguments
         */
        vldr    s0,  [r4, #0]
        vldr    s1,  [r4, #4]
        vldr    s2,  [r4, #8]
        vldr    s3,  [r4, #12]
        vldr    s4,  [r4, #16]
        vldr    s5,  [r4, #20]
        vldr    s6,  [r4, #24]
        vldr    s7,  [r4, #28]
        vldr    s8,  [r4, #32]
        vldr    s9,  [r4, #36]
        vldr    s10, [r4, #40]
        vldr    s11, [r4, #44]
        vldr    s12, [r4, #48]
        vldr    s13, [r4, #52]
        vldr    s14, [r4, #56]
        vldr    s15, [r4, #60]

        /*
         * Check if there are any additional arguments to handle on the stack
         * If argc is 0, proceed directly to function call
         */
        cmp     r5, #0
        beq     .Lcall

        /*
         * Handle variable arguments beyond the first 16
         * Save r2 (third integer argument) in lr before stack operations
         */
        mov     lr, r2
        add     r4, r4, #64

        /*
         * Calculate stack space needed for remaining arguments
         * Align stack pointer to 8-byte boundary for VFP compliance
         */
        mov     r6, sp
        mov     r3, #7
        bic     r6, r6, r3
        lsl     r2, r5, #2
        add     r2, r2, #7
        bic     r2, r2, r3
        sub     r6, r6, r2
        mov     sp, r6

        /*
         * Copy remaining arguments to stack
         * Loop through remaining arguments and push them onto the stack
         */
.Lloop:
        cmp     r5, #0
        beq     .Ldone_stack
        ldr     r2, [r4]
        add     r4, r4, #4
        str     r2, [r6]
        add     r6, r6, #4
        sub     r5, r5, #1
        b       .Lloop

        /*
         * Restore register r2
         * Retrieve the third argument that was saved before the loop
         */
.Ldone_stack:
        mov     r2, lr

        /*
         * Invoke the target function
         * Call the native function using the prepared arguments
         * Branch to function pointer with link, switching to ARM mode if needed
         */
.Lcall:
        blx     ip

        /*
         * Function epilogue
         * Clean up stack frame and restore registers
         */

        /*
         * Restore stack pointer
         * Restore SP from frame pointer to clean up our stack frame
         */
        mov     sp, r7

        /*
         * Clean up stack frame
         * Remove alignment padding and restore saved registers
         */
        add     sp, sp, #4
        pop     {r4, r5, r6}
        .cfi_def_cfa_offset 12
        pop     {r7, lr}
        .cfi_def_cfa_offset 4
        mov     sp, r7
        bx      lr

        .cfi_endproc
#if defined(__linux__) && defined(__ELF__)
        .section .note.GNU-stack,"",%progbits
#endif
