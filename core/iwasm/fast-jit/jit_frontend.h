/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _JIT_FRONTEND_H_
#define _JIT_FRONTEND_H_

#include "jit_utils.h"
#include "jit_ir.h"
#include "../interpreter/wasm_interp.h"

typedef enum JitExceptionID {
    EXCE_UNREACHABLE = 0,
    EXCE_OUT_OF_MEMORY,
    EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS,
    EXCE_INTEGER_OVERFLOW,
    EXCE_INTEGER_DIVIDE_BY_ZERO,
    EXCE_INVALID_CONVERSION_TO_INTEGER,
    EXCE_INVALID_FUNCTION_TYPE_INDEX,
    EXCE_INVALID_FUNCTION_INDEX,
    EXCE_UNDEFINED_ELEMENT,
    EXCE_UNINITIALIZED_ELEMENT,
    EXCE_CALL_UNLINKED_IMPORT_FUNC,
    EXCE_NATIVE_STACK_OVERFLOW,
    EXCE_UNALIGNED_ATOMIC,
    EXCE_AUX_STACK_OVERFLOW,
    EXCE_AUX_STACK_UNDERFLOW,
    EXCE_OUT_OF_BOUNDS_TABLE_ACCESS,
    EXCE_OPERAND_STACK_OVERFLOW,
    EXCE_NUM,
} JitExceptionID;

/**
 * Translate instructions in a function. The translated block must
 * end with a branch instruction whose targets are offsets relating to
 * the end bcip of the translated block, which are integral constants.
 * If a target of a branch is really a constant value (which should be
 * rare), put it into a register and then jump to the register instead
 * of using the constant value directly in the target. In the
 * translation process, don't create any new labels. The code bcip of
 * the begin and end of the translated block is stored in the
 * jit_annl_begin_bcip and jit_annl_end_bcip annotations of the label
 * of the block, which must be the same as the bcips used in
 * profiling.
 *
 * NOTE: the function must explicitly set SP to correct value when the
 * entry's bcip is the function's entry address.
 *
 * @param cc containing compilation context of generated IR
 * @param entry entry of the basic block to be translated. If its
 * value is NULL, the function will clean up any pass local data that
 * might be created previously.
 * @param is_reached a bitmap recording which bytecode has been
 * reached as a block entry
 *
 * @return IR block containing translated instructions if succeeds,
 * NULL otherwise
 */
JitBasicBlock *
jit_frontend_translate_func(JitCompContext *cc);

/**
 * Generate a block leaving the compiled code, which must store the
 * target bcip and other necessary information for switching to
 * interpreter or other compiled code and then jump to the exit of the
 * cc.
 *
 * @param cc the compilation context
 * @param bcip the target bytecode instruction pointer
 * @param sp_offset stack pointer offset at the beginning of the block
 *
 * @return the leaving block if succeeds, NULL otherwise
 */
JitBlock *
jit_frontend_gen_leaving_block(JitCompContext *cc, void *bcip,
                               unsigned sp_offset);

#if 0
/**
 * Print the qualified name of the given function.
 *
 * @param function the function whose name to be printed
 */
void
jit_frontend_print_function_name(void *function);

/**
 * Get the full name of the function.  If the input buffer lengh
 * is less than the actual function name length, the function will
 * simply return the actuall length and won't write to the buffer.
 *
 * @param function pointer to a function
 * @param buf buffer for the returned name
 * @param buf_len lengh of the buffer
 *
 * @return actual length of the name
 */
unsigned
jit_frontend_get_function_name(void *function, char *buf, unsigned buf_len);

/**
 * Convert the bcip in the given function to an internal offset.
 *
 * @param function function containing the bcip
 * @param bcip bytecode instruction pointer
 *
 * @return converted offset of the bcip
 */
unsigned
jit_frontend_bcip_to_offset(void *function, void *bcip);
#endif

/**
 * Lower the IR of the given compilation context.
 *
 * @param cc the compilation context
 *
 * @return true if succeeds, false otherwise
 */
bool
jit_frontend_lower(JitCompContext *cc);

/**
 * Get the offset from frame pointer to the n-th local variable slot.
 *
 * @param n the index to the local variable array
 *
 * @return the offset from frame pointer to the local variable slot
 */
static inline unsigned
offset_of_local(unsigned n)
{
    return offsetof(WASMInterpFrame, lp) + n * 4;
}

/**
 * Generate instruction to load an integer from the frame.
 *
 * This and the below gen_load_X functions generate instructions to
 * load values from the frame into registers if the values have not
 * been loaded yet.
 *
 * @param frame the frame information
 * @param n slot index to the local variable array
 *
 * @return register holding the loaded value
 */
JitReg
gen_load_i32(JitFrame *frame, unsigned n);

/**
 * Generate instruction to load a i64 integer from the frame.
 *
 * @param frame the frame information
 * @param n slot index to the local variable array
 *
 * @return register holding the loaded value
 */
JitReg
gen_load_i64(JitFrame *frame, unsigned n);

/**
 * Generate instruction to load a floating point value from the frame.
 *
 * @param frame the frame information
 * @param n slot index to the local variable array
 *
 * @return register holding the loaded value
 */
JitReg
gen_load_f32(JitFrame *frame, unsigned n);

/**
 * Generate instruction to load a double value from the frame.
 *
 * @param frame the frame information
 * @param n slot index to the local variable array
 *
 * @return register holding the loaded value
 */
JitReg
gen_load_f64(JitFrame *frame, unsigned n);

/**
 * Generate instructions to commit computation result to the frame.
 * The general principle is to only commit values that will be used
 * through the frame.
 *
 * @param frame the frame information
 * @param begin the begin value slot to commit
 * @param end the end value slot to commit
 */
void
gen_commit_values(JitFrame *frame, JitValueSlot *begin, JitValueSlot *end);

/**
 * Generate instructions to commit SP and IP pointers to the frame.
 *
 * @param frame the frame information
 */
void
gen_commit_sp_ip(JitFrame *frame);

/**
 * Generate commit instructions for the block end.
 *
 * @param frame the frame information
 */
static inline void
gen_commit_for_branch(JitFrame *frame)
{
    gen_commit_values(frame, frame->lp, frame->sp);
}

/**
 * Generate commit instructions for exception checks.
 *
 * @param frame the frame information
 */
static inline void
gen_commit_for_exception(JitFrame *frame)
{
    gen_commit_values(frame, frame->lp, frame->lp + frame->max_locals);
}

/**
 * Generate commit instructions to commit all status.
 *
 * @param frame the frame information
 */
static inline void
gen_commit_for_all(JitFrame *frame)
{
    gen_commit_values(frame, frame->lp, frame->sp);
    gen_commit_sp_ip(frame);
}

static inline void
push_i32(JitFrame *frame, JitReg value)
{
    frame->sp->reg = value;
    frame->sp->dirty = 1;
    frame->sp++;
}

static inline void
push_i64(JitFrame *frame, JitReg value)
{
    frame->sp->reg = value;
    frame->sp->dirty = 1;
    frame->sp++;
    frame->sp->reg = value;
    frame->sp->dirty = 1;
    frame->sp++;
}

static inline void
push_f32(JitFrame *frame, JitReg value)
{
    push_i32(frame, value);
}

static inline void
push_f64(JitFrame *frame, JitReg value)
{
    push_i64(frame, value);
}

static inline JitReg
pop_i32(JitFrame *frame)
{
    frame->sp--;
    return gen_load_i32(frame, frame->sp - frame->lp);
}

static inline JitReg
pop_i64(JitFrame *frame)
{
    frame->sp -= 2;
    return gen_load_i64(frame, frame->sp - frame->lp);
}

static inline JitReg
pop_f32(JitFrame *frame)
{
    frame->sp--;
    return gen_load_f32(frame, frame->sp - frame->lp);
}

static inline JitReg
pop_f64(JitFrame *frame)
{
    frame->sp -= 2;
    return gen_load_f64(frame, frame->sp - frame->lp);
}

static inline void
pop(JitFrame *frame, int n)
{
    frame->sp -= n;
    memset(frame->sp, 0, n * sizeof(*frame->sp));
}

static inline JitReg
local_i32(JitFrame *frame, int n)
{
    return gen_load_i32(frame, n);
}

static inline JitReg
local_i64(JitFrame *frame, int n)
{
    return gen_load_i64(frame, n);
}

static inline JitReg
local_f32(JitFrame *frame, int n)
{
    return gen_load_f32(frame, n);
}

static inline JitReg
local_f64(JitFrame *frame, int n)
{
    return gen_load_f64(frame, n);
}

static void
set_local_i32(JitFrame *frame, int n, JitReg val)
{
    frame->lp[n].reg = val;
    frame->lp[n].dirty = 1;
}

static void
set_local_i64(JitFrame *frame, int n, JitReg val)
{
    frame->lp[n].reg = val;
    frame->lp[n].dirty = 1;
    frame->lp[n + 1].reg = val;
    frame->lp[n + 1].dirty = 1;
}

static inline void
set_local_f32(JitFrame *frame, int n, JitReg val)
{
    set_local_i32(frame, n, val);
}

static inline void
set_local_f64(JitFrame *frame, int n, JitReg val)
{
    set_local_i64(frame, n, val);
}

#endif
