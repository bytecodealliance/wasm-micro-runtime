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
 * invokeNative - Invoke a native function
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
 *   - First 4 arguments (argv[0] to argv[3]) are passed in registers r0-r3
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
 *   - r0-r3: Function parameters and return value
 *   - r4-r7: Callee-saved registers (r7 is frame pointer)
 *   - r12 (ip): Temporary register for function pointer
 *   - lr: Return address (callee-saved)
 *   - sp: Stack pointer
 */

        /*
         * Save callee-saved registers and establish stack frame
         * Uses 20 bytes of stack space (5 registers × 4 bytes each)
         */
        push    {r4, r5, r6, r7}
        push    {lr}
        .cfi_def_cfa sp, 0
        .cfi_adjust_cfa_offset 20

        /*
         * Add 4-byte alignment padding to maintain 8-byte stack alignment
         * This is required by the ARM ABI for proper function calls
         */
        sub     sp, #4
        .cfi_adjust_cfa_offset 4

        /*
         * Establish frame pointer (r7) for stack unwinding
         * CFA (Canonical Frame Address) = r7 + 24
         */
        mov     r7, sp
        .cfi_def_cfa r7, 24
        .cfi_offset lr, 4
        .cfi_offset r7, 8
        .cfi_offset r6, 12
        .cfi_offset r5, 16
        .cfi_offset r4, 20

        /*
         * Save function parameters in preserved registers
         */
        mov     ip, r0
        mov     r4, r1
        mov     r5, r2

        /*
         * Check that at least one argument (exec_env) is provided
         */
        cmp     r5, #1
        blt     .Lreturn

        /*
         * Load first argument (argv[0]) into register
         * The first argument is always the execution environment, passed in r0
         */
        ldr     r0, [r4]
        adds    r4, #4

        /*
         * Check if we have exactly 1 argument
         * If so, proceed directly to function call
         */
        cmp     r5, #1
        beq     .Lcall_func

        /*
         * Load second argument (argv[1]) into register
         * If we have at least 2 arguments, load the second one into r1
         */
        ldr     r1, [r4]
        adds    r4, #4

        /*
         * Check if we have exactly 2 arguments
         * If so, proceed to function call
         */
        cmp     r5, #2
        beq     .Lcall_func

        /*
         * Load third argument (argv[2]) into register
         * If we have at least 3 arguments, load the third one into r2
         */
        ldr     r2, [r4]
        adds    r4, #4

        /*
         * Check if we have exactly 3 arguments
         * If so, proceed to function call
         */
        cmp     r5, #3
        beq     .Lcall_func

        /*
         * Load fourth argument (argv[3]) into register
         * If we have at least 4 arguments, load the fourth one into r3
         */
        ldr     r3, [r4]
        adds    r4, #4

        /*
         * Check if we have exactly 4 arguments
         * If so, proceed to function call (we've loaded all register args)
         */
        cmp     r5, #4
        beq     .Lcall_func

        /*
         * Handle arguments beyond the first four
         * We have more than 4 arguments, need to handle stack-based arguments
         */
        subs    r5, r5, #4

        /*
         * Calculate stack space needed for remaining arguments
         *
         * Algorithm to round up to 8-byte multiple:
         * 1. Add 1 to round up odd numbers
         * 2. Divide by 2 and multiply by 8 to get byte count
         * 3. Add 4 for additional alignment/padding
         *
         * Formula: bytes = 8 * ceil(r5 / 2) + 4
         *
         * Example: r5=5 -> ceil(6/2)=3 -> 3*8=24 -> 24+4=28 bytes
         */
        adds    r6, r5, #1
        lsrs    r6, r6, #1
        lsls    r6, r6, #3
        adds    r6, r6, #4

        /*
         * Allocate stack space for variable arguments
         *
         * Calculate new stack pointer position:
         * new_sp = current_frame_pointer - stack_space_needed
         *
         * We use r7 (frame pointer) as reference to keep it stable
         */
        subs    r6, r7, r6
        mov     sp, r6
        mov     r6, sp

        /*
         * Preserve register r2
         * r2 currently holds the third argument, save it in lr (already on stack)
         */
        mov     lr, r2

        /*
         * Copy remaining arguments to stack
         * Loop through remaining arguments and push them onto the stack
         */
.Lloop_args:
        cmp     r5, #0
        beq     .Lrestore_r2

        ldr     r2, [r4]
        adds    r4, #4

        str     r2, [r6]
        adds    r6, #4

        subs    r5, #1
        bne     .Lloop_args

        /*
         * Restore register r2
         * Retrieve the third argument that was saved before the loop
         */
.Lrestore_r2:
        mov     r2, lr

        /*
         * Invoke the target function
         * Call the native function using the prepared arguments
         */
.Lcall_func:
        blx     ip

        /*
         * Function epilogue
         * Clean up stack frame and restore registers
         */

        /*
         * Restore stack pointer
         * Restore SP from frame pointer to clean up our stack frame
         *
         * Note: We use mov instead of add to avoid Thumb-2 specific instructions
         * and maintain compatibility with pure Thumb mode
         */
        mov     sp, r7

        /*
         * Clean up stack frame
         * Remove alignment padding and restore saved registers
         */
.Lreturn:
        add     sp, #4
        pop     {r3}
        pop     {r4, r5, r6, r7}
        mov     lr, r3
        bx      lr

        .cfi_endproc

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
