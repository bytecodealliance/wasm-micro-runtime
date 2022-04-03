/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_compiler.h"
#include "jit_frontend.h"
#include "fe/jit_emit_compare.h"
#include "fe/jit_emit_const.h"
#include "fe/jit_emit_control.h"
#include "fe/jit_emit_conversion.h"
#include "fe/jit_emit_exception.h"
#include "fe/jit_emit_function.h"
#include "fe/jit_emit_memory.h"
#include "fe/jit_emit_numberic.h"
#include "fe/jit_emit_parametric.h"
#include "fe/jit_emit_table.h"
#include "fe/jit_emit_variable.h"
#include "../interpreter/wasm_interp.h"
#include "../interpreter/wasm_opcode.h"
#include "../common/wasm_exec_env.h"

/* clang-format off */
static const char *jit_exception_msgs[] = {
    "unreachable",                    /* EXCE_UNREACHABLE */
    "allocate memory failed",         /* EXCE_OUT_OF_MEMORY */
    "out of bounds memory access",    /* EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS */
    "integer overflow",               /* EXCE_INTEGER_OVERFLOW */
    "integer divide by zero",         /* EXCE_INTEGER_DIVIDE_BY_ZERO */
    "invalid conversion to integer",  /* EXCE_INVALID_CONVERSION_TO_INTEGER */
    "indirect call type mismatch",    /* EXCE_INVALID_FUNCTION_TYPE_INDEX */
    "invalid function index",         /* EXCE_INVALID_FUNCTION_INDEX */
    "undefined element",              /* EXCE_UNDEFINED_ELEMENT */
    "uninitialized element",          /* EXCE_UNINITIALIZED_ELEMENT */
    "failed to call unlinked import function", /* EXCE_CALL_UNLINKED_IMPORT_FUNC */
    "native stack overflow",          /* EXCE_NATIVE_STACK_OVERFLOW */
    "unaligned atomic",               /* EXCE_UNALIGNED_ATOMIC */
    "wasm auxiliary stack overflow",  /* EXCE_AUX_STACK_OVERFLOW */
    "wasm auxiliary stack underflow", /* EXCE_AUX_STACK_UNDERFLOW */
    "out of bounds table access",     /* EXCE_OUT_OF_BOUNDS_TABLE_ACCESS */
    "wasm operand stack overflow",    /* EXCE_OPERAND_STACK_OVERFLOW */
};
/* clang-format on */

JitReg
gen_load_i32(JitFrame *frame, unsigned n)
{
    if (!frame->lp[n].reg) {
        JitCompContext *cc = frame->cc;
        frame->lp[n].reg = jit_cc_new_reg_I32(cc);
        GEN_INSN(LDI32, frame->lp[n].reg, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(n)));
    }

    return frame->lp[n].reg;
}

JitReg
gen_load_i64(JitFrame *frame, unsigned n)
{
    if (!frame->lp[n].reg) {
        JitCompContext *cc = frame->cc;
        frame->lp[n].reg = frame->lp[n + 1].reg = jit_cc_new_reg_I64(cc);
        GEN_INSN(LDI64, frame->lp[n].reg, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(n)));
    }

    return frame->lp[n].reg;
}

JitReg
gen_load_f32(JitFrame *frame, unsigned n)
{
    if (!frame->lp[n].reg) {
        JitCompContext *cc = frame->cc;
        frame->lp[n].reg = jit_cc_new_reg_F32(cc);
        GEN_INSN(LDF32, frame->lp[n].reg, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(n)));
    }

    return frame->lp[n].reg;
}

JitReg
gen_load_f64(JitFrame *frame, unsigned n)
{
    if (!frame->lp[n].reg) {
        JitCompContext *cc = frame->cc;
        frame->lp[n].reg = frame->lp[n + 1].reg = jit_cc_new_reg_F64(cc);
        GEN_INSN(LDF64, frame->lp[n].reg, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(n)));
    }

    return frame->lp[n].reg;
}

void
gen_commit_values(JitFrame *frame, JitValueSlot *begin, JitValueSlot *end)
{
    JitCompContext *cc = frame->cc;
    JitValueSlot *p;
    int n;

    for (p = begin; p < end; p++) {
        if (!p->dirty)
            continue;

        p->dirty = 0;
        n = p - frame->lp;

        switch (jit_reg_kind(p->reg)) {
            case JIT_REG_KIND_I32:
                GEN_INSN(STI32, p->reg, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                break;

            case JIT_REG_KIND_I64:
                GEN_INSN(STI64, p->reg, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                (++p)->dirty = 0;
                break;

            case JIT_REG_KIND_F32:
                GEN_INSN(STF32, p->reg, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                break;

            case JIT_REG_KIND_F64:
                GEN_INSN(STF64, p->reg, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                (++p)->dirty = 0;
                break;
        }
    }
}

/**
 * Generate instructions to commit SP and IP pointers to the frame.
 *
 * @param frame the frame information
 */
void
gen_commit_sp_ip(JitFrame *frame)
{
    JitCompContext *cc = frame->cc;
    JitReg sp;

    if (frame->sp != frame->committed_sp) {
#if UINTPTR_MAX == UINT64_MAX
        sp = jit_cc_new_reg_I64(cc);
        GEN_INSN(ADD, sp, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(frame->sp - frame->lp)));
        GEN_INSN(STI64, sp, cc->fp_reg,
                 NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#else
        sp = jit_cc_new_reg_I32(cc);
        GEN_INSN(ADD, sp, cc->fp_reg,
                 NEW_CONST(I32, offset_of_local(frame->sp - frame->lp)));
        GEN_INSN(STI32, sp, cc->fp_reg,
                 NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#endif
        frame->committed_sp = frame->sp;
    }

    if (frame->ip != frame->committed_ip) {
#if UINTPTR_MAX == UINT64_MAX
        GEN_INSN(STI64, NEW_CONST(I64, (uint64)(uintptr_t)frame->ip),
                 cc->fp_reg, NEW_CONST(I32, offsetof(WASMInterpFrame, ip)));
#else
        GEN_INSN(STI32, NEW_CONST(I32, (uint32)(uintptr_t)frame->ip),
                 cc->fp_reg, NEW_CONST(I32, offsetof(WASMInterpFrame, ip)));
#endif
        frame->committed_ip = frame->ip;
    }
}

static void
jit_set_exception_with_id(WASMModuleInstance *module_inst, uint32 id)
{
    if (id < EXCE_NUM)
        wasm_set_exception(module_inst, jit_exception_msgs[id]);
    else
        wasm_set_exception(module_inst, "unknown exception");
}

static bool
form_and_translate_func(JitCompContext *cc)
{
    JitBasicBlock *func_entry_basic_block;
    JitReg func_entry_label;
    JitInsn *insn;
    JitIncomingInsn *incoming_insn, *incoming_insn_next;
    uint32 i;

    if (!(func_entry_basic_block = jit_frontend_translate_func(cc)))
        return false;

    jit_cc_reset_insn_hash(cc);

    /* The label of the func entry basic block. */
    func_entry_label = jit_basic_block_label(func_entry_basic_block);

    /* Create a JMP instruction jumping to the func entry. */
    if (!(insn = jit_cc_new_insn(cc, JMP, func_entry_label)))
        return false;

    /* Insert the instruction into the cc entry block. */
    jit_basic_block_append_insn(jit_cc_entry_basic_block(cc), insn);

    /* Patch INSNs jumping to exception basic blocks. */
    for (i = 0; i < EXCE_NUM; i++) {
        incoming_insn = cc->incoming_insns_for_exec_bbs[i];
        if (incoming_insn) {
            if (!(cc->exce_basic_blocks[i] = jit_cc_new_basic_block(cc, 0))) {
                jit_set_last_error(cc, "create basic block failed");
                return false;
            }
            while (incoming_insn) {
                incoming_insn_next = incoming_insn->next;
                insn = incoming_insn->insn;
                if (insn->opcode == JIT_OP_JMP) {
                    *(jit_insn_opnd(insn, 0)) =
                        jit_basic_block_label(cc->exce_basic_blocks[i]);
                }
                else if (insn->opcode >= JIT_OP_BNE
                         && insn->opcode <= JIT_OP_BLEU) {
                    *(jit_insn_opnd(insn, 1)) =
                        jit_basic_block_label(cc->exce_basic_blocks[i]);
                }
                incoming_insn = incoming_insn_next;
            }
            cc->cur_basic_block = cc->exce_basic_blocks[i];
#if UINTPTR_MAX == UINT64_MAX
            insn = GEN_INSN(
                CALLNATIVE, 0,
                NEW_CONST(I64, (uint64)(uintptr_t)jit_set_exception_with_id),
                1);
#else
            insn = GEN_INSN(
                CALLNATIVE, 0,
                NEW_CONST(I32, (uint32)(uintptr_t)jit_set_exception_with_id),
                1);
#endif
            if (insn) {
                *(jit_insn_opndv(insn, 2)) = NEW_CONST(I32, i);
            }
            GEN_INSN(RETURN, NEW_CONST(I32, JIT_INTERP_ACTION_THROWN));

            *(jit_annl_begin_bcip(cc,
                                  jit_basic_block_label(cc->cur_basic_block))) =
                *(jit_annl_end_bcip(
                    cc, jit_basic_block_label(cc->cur_basic_block))) =
                    cc->cur_wasm_module->load_addr;
        }
    }

    *(jit_annl_begin_bcip(cc, cc->entry_label)) =
        *(jit_annl_end_bcip(cc, cc->entry_label)) =
            *(jit_annl_begin_bcip(cc, cc->exit_label)) =
                *(jit_annl_end_bcip(cc, cc->exit_label)) =
                    cc->cur_wasm_module->load_addr;

    return true;
}

bool
jit_pass_frontend(JitCompContext *cc)
{
    /* Enable necessary annotations required at the current stage. */
    if (!jit_annl_enable_begin_bcip(cc) || !jit_annl_enable_end_bcip(cc)
        || !jit_annl_enable_end_sp(cc) || !jit_annr_enable_def_insn(cc)
        || !jit_cc_enable_insn_hash(cc, 127))
        return false;

    if (!(form_and_translate_func(cc)))
        return false;

    /* Release the annotations after local CSE and translation. */
    jit_cc_disable_insn_hash(cc);
    jit_annl_disable_end_sp(cc);

    return true;
}

#if 0
bool
jit_pass_lower_fe(JitCompContext *cc)
{
    return true;
}
#endif

static JitFrame *
init_func_translation(JitCompContext *cc)
{
    JitFrame *jit_frame;
    JitReg top, top_boundary, new_top, frame_boundary, frame_sp;
    WASMModule *cur_wasm_module = cc->cur_wasm_module;
    WASMFunction *cur_wasm_func = cc->cur_wasm_func;
    uint32 cur_wasm_func_idx = cc->cur_wasm_func_idx;
    uint32 max_locals =
        cur_wasm_func->param_cell_num + cur_wasm_func->local_cell_num;
    uint32 max_stacks = cur_wasm_func->max_stack_cell_num;
    uint64 total_cell_num =
        (uint64)cur_wasm_func->param_cell_num
        + (uint64)cur_wasm_func->local_cell_num
        + (uint64)cur_wasm_func->max_stack_cell_num
        + ((uint64)cur_wasm_func->max_block_num) * sizeof(WASMBranchBlock) / 4;
    uint32 frame_size, outs_size, local_size;

    if ((uint64)max_locals + (uint64)max_stacks >= UINT32_MAX
        || total_cell_num >= UINT32_MAX
        || !(jit_frame = jit_calloc(offsetof(JitFrame, lp)
                                    + sizeof(*jit_frame->lp)
                                          * (max_locals + max_stacks)))) {
        os_printf("allocate jit frame failed\n");
        return NULL;
    }

    jit_frame->cur_wasm_module = cur_wasm_module;
    jit_frame->cur_wasm_func = cur_wasm_func;
    jit_frame->cur_wasm_func_idx = cur_wasm_func_idx;
    jit_frame->cc = cc;
    jit_frame->max_locals = max_locals;
    jit_frame->max_stacks = max_stacks;
    jit_frame->sp = jit_frame->lp + max_locals;
    jit_frame->ip = cur_wasm_func->code;

    cc->jit_frame = jit_frame;
    cc->cur_basic_block = jit_cc_entry_basic_block(cc);
    cc->total_frame_size = wasm_interp_interp_frame_size(total_cell_num);
    cc->jitted_return_address_offset =
        offsetof(WASMInterpFrame, jitted_return_addr);
    cc->cur_basic_block = jit_cc_entry_basic_block(cc);

    frame_size = outs_size = cc->total_frame_size;
    local_size =
        (cur_wasm_func->param_cell_num + cur_wasm_func->local_cell_num) * 4;

#if UINTPTR_MAX == UINT64_MAX
    top = jit_cc_new_reg_I64(cc);
    top_boundary = jit_cc_new_reg_I64(cc);
    new_top = jit_cc_new_reg_I64(cc);
    frame_boundary = jit_cc_new_reg_I64(cc);
    frame_sp = jit_cc_new_reg_I64(cc);

    /* top = exec_env->wasm_stack.s.top */
    GEN_INSN(LDI64, top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
    /* top_boundary = exec_env->wasm_stack.s.top_boundary */
    GEN_INSN(LDI64, top_boundary, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top_boundary)));
    /* frame_boundary = top + frame_size + outs_size */
    GEN_INSN(ADD, frame_boundary, top, NEW_CONST(I64, frame_size + outs_size));
    /* if frame_boundary > top_boundary, throw stack overflow exception */
    GEN_INSN(CMP, cc->cmp_reg, frame_boundary, top_boundary);
    if (!jit_emit_exception(cc, EXCE_OPERAND_STACK_OVERFLOW, JIT_OP_BGTU,
                            cc->cmp_reg, 0)) {
        return NULL;
    }

    /* Add first and then sub to reduce one used register */
    /* new_top = frame_boundary - outs_size = top + frame_size */
    GEN_INSN(SUB, new_top, frame_boundary, NEW_CONST(I64, outs_size));
    /* exec_env->wasm_stack.s.top = new_top */
    GEN_INSN(STI64, new_top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
    /* frame_sp = frame->lp + local_size */
    GEN_INSN(ADD, frame_sp, top,
             NEW_CONST(I64, offsetof(WASMInterpFrame, lp) + local_size));
    /* frame->sp = frame_sp */
    GEN_INSN(STI64, frame_sp, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
    /* frame->prev_frame = fp_reg */
    GEN_INSN(STI64, cc->fp_reg, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, prev_frame)));
    /* TODO: do we need to set frame->function? */
    /*
    GEN_INSN(STI64, func_inst, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, function)));
    */
    /* exec_env->cur_frame = top */
    GEN_INSN(STI64, top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, cur_frame)));
    /* fp_reg = top */
    GEN_INSN(MOV, cc->fp_reg, top);
#else
    top = jit_cc_new_reg_I32(cc);
    top_boundary = jit_cc_new_reg_I32(cc);
    new_top = jit_cc_new_reg_I32(cc);
    frame_boundary = jit_cc_new_reg_I32(cc);
    frame_sp = jit_cc_new_reg_I32(cc);

    /* top = exec_env->wasm_stack.s.top */
    GEN_INSN(LDI32, top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
    /* top_boundary = exec_env->wasm_stack.s.top_boundary */
    GEN_INSN(LDI32, top_boundary, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top_boundary)));
    /* frame_boundary = top + frame_size + outs_size */
    GEN_INSN(ADD, frame_boundary, top, NEW_CONST(I32, frame_size + outs_size));
    /* if frame_boundary > top_boundary, throw stack overflow exception */
    GEN_INSN(CMP, cc->cmp_reg, frame_boundary, top_boundary);
    if (!jit_emit_exception(cc, EXCE_OPERAND_STACK_OVERFLOW, JIT_OP_BGTU,
                            cc->cmp_reg, 0)) {
        return NULL;
    }

    /* Add first and then sub to reduce one used register */
    /* new_top = frame_boundary - outs_size = top + frame_size */
    GEN_INSN(SUB, new_top, frame_boundary, NEW_CONST(I32, outs_size));
    /* exec_env->wasm_stack.s.top = new_top */
    GEN_INSN(STI32, new_top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
    /* frame_sp = frame->lp + local_size */
    GEN_INSN(ADD, frame_sp, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, lp) + local_size));
    /* frame->sp = frame_sp */
    GEN_INSN(STI32, frame_sp, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
    /* frame->prev_frame = fp_reg */
    GEN_INSN(STI32, cc->fp_reg, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, prev_frame)));
    /* TODO: do we need to set frame->function? */
    /*
    GEN_INSN(STI32, func_inst, top,
             NEW_CONST(I32, offsetof(WASMInterpFrame, function)));
    */
    /* exec_env->cur_frame = top */
    GEN_INSN(STI32, top, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, cur_frame)));
    /* fp_reg = top */
    GEN_INSN(MOV, cc->fp_reg, top);
#endif

    return jit_frame;
}

static void
free_block_memory(JitBlock *block)
{
    if (block->param_types)
        jit_free(block->param_types);
    if (block->result_types)
        jit_free(block->result_types);
    jit_free(block);
}

static JitBlock *
create_func_block(JitCompContext *cc)
{
    JitBlock *jit_block;
    WASMFunction *cur_func = cc->cur_wasm_func;
    WASMType *func_type = cur_func->func_type;
    uint32 param_count = func_type->param_count;
    uint32 result_count = func_type->result_count;

    if (!(jit_block = jit_calloc(sizeof(JitBlock)))) {
        return NULL;
    }

    if (param_count && !(jit_block->param_types = jit_calloc(param_count))) {
        goto fail;
    }
    if (result_count && !(jit_block->result_types = jit_calloc(result_count))) {
        goto fail;
    }

    /* Set block data */
    jit_block->label_type = LABEL_TYPE_FUNCTION;
    jit_block->param_count = param_count;
    if (param_count) {
        bh_memcpy_s(jit_block->param_types, param_count, func_type->types,
                    param_count);
    }
    jit_block->result_count = result_count;
    if (result_count) {
        bh_memcpy_s(jit_block->result_types, result_count,
                    func_type->types + param_count, result_count);
    }
    jit_block->wasm_code_end = cur_func->code + cur_func->code_size;
    jit_block->frame_sp_begin = cc->jit_frame->sp;

    /* Add function entry block */
    if (!(jit_block->basic_block_entry = jit_cc_new_basic_block(cc, 0))) {
        goto fail;
    }
    *(jit_annl_begin_bcip(
        cc, jit_basic_block_label(jit_block->basic_block_entry))) =
        cur_func->code;
    jit_block_stack_push(&cc->block_stack, jit_block);
    cc->cur_basic_block = jit_block->basic_block_entry;

    return jit_block;

fail:
    free_block_memory(jit_block);
    return NULL;
}

#define CHECK_BUF(buf, buf_end, length)                                 \
    do {                                                                \
        if (buf + length > buf_end) {                                   \
            jit_set_last_error(cc, "read leb failed: unexpected end."); \
            return false;                                               \
        }                                                               \
    } while (0)

static bool
read_leb(JitCompContext *cc, const uint8 *buf, const uint8 *buf_end,
         uint32 *p_offset, uint32 maxbits, bool sign, uint64 *p_result)
{
    uint64 result = 0;
    uint32 shift = 0;
    uint32 bcnt = 0;
    uint64 byte;

    while (true) {
        CHECK_BUF(buf, buf_end, 1);
        byte = buf[*p_offset];
        *p_offset += 1;
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
        bcnt += 1;
    }
    if (bcnt > (maxbits + 6) / 7) {
        jit_set_last_error(cc, "read leb failed: "
                               "integer representation too long");
        return false;
    }
    if (sign && (shift < maxbits) && (byte & 0x40)) {
        /* Sign extend */
        result |= (~((uint64)0)) << shift;
    }
    *p_result = result;
    return true;
}

#define read_leb_uint32(p, p_end, res)                        \
    do {                                                      \
        uint32 off = 0;                                       \
        uint64 res64;                                         \
        if (!read_leb(cc, p, p_end, &off, 32, false, &res64)) \
            return false;                                     \
        p += off;                                             \
        res = (uint32)res64;                                  \
    } while (0)

#define read_leb_int32(p, p_end, res)                        \
    do {                                                     \
        uint32 off = 0;                                      \
        uint64 res64;                                        \
        if (!read_leb(cc, p, p_end, &off, 32, true, &res64)) \
            return false;                                    \
        p += off;                                            \
        res = (int32)res64;                                  \
    } while (0)

#define read_leb_int64(p, p_end, res)                        \
    do {                                                     \
        uint32 off = 0;                                      \
        uint64 res64;                                        \
        if (!read_leb(cc, p, p_end, &off, 64, true, &res64)) \
            return false;                                    \
        p += off;                                            \
        res = (int64)res64;                                  \
    } while (0)

static bool
jit_compile_func(JitCompContext *cc)
{
    WASMFunction *cur_func = cc->cur_wasm_func;
    WASMType *func_type = NULL;
    uint8 *frame_ip = cur_func->code, opcode, *p_f32, *p_f64;
    uint8 *frame_ip_end = frame_ip + cur_func->code_size;
    uint8 *param_types = NULL, *result_types = NULL, value_type;
    uint16 param_count, result_count;
    uint32 br_depth, *br_depths, br_count;
    uint32 func_idx, type_idx, mem_idx, local_idx, global_idx, i;
    uint32 bytes = 4, align, offset;
    bool sign = true;
    int32 i32_const;
    int64 i64_const;
    float32 f32_const;
    float64 f64_const;

    while (frame_ip < frame_ip_end) {
        opcode = *frame_ip++;

        switch (opcode) {
            case WASM_OP_UNREACHABLE:
                if (!jit_compile_op_unreachable(cc, &frame_ip))
                    return false;
                break;

            case WASM_OP_NOP:
                break;

            case WASM_OP_BLOCK:
            case WASM_OP_LOOP:
            case WASM_OP_IF:
            {
                value_type = *frame_ip++;
                if (value_type == VALUE_TYPE_I32 || value_type == VALUE_TYPE_I64
                    || value_type == VALUE_TYPE_F32
                    || value_type == VALUE_TYPE_F64
                    || value_type == VALUE_TYPE_V128
                    || value_type == VALUE_TYPE_VOID
                    || value_type == VALUE_TYPE_FUNCREF
                    || value_type == VALUE_TYPE_EXTERNREF) {
                    param_count = 0;
                    param_types = NULL;
                    if (value_type == VALUE_TYPE_VOID) {
                        result_count = 0;
                        result_types = NULL;
                    }
                    else {
                        result_count = 1;
                        result_types = &value_type;
                    }
                }
                else {
                    jit_set_last_error(cc, "unsupported value type");
                    return false;
                }
                if (!jit_compile_op_block(
                        cc, &frame_ip, frame_ip_end,
                        (uint32)(LABEL_TYPE_BLOCK + opcode - WASM_OP_BLOCK),
                        param_count, param_types, result_count, result_types))
                    return false;
                break;
            }
            case EXT_OP_BLOCK:
            case EXT_OP_LOOP:
            case EXT_OP_IF:
            {
                read_leb_uint32(frame_ip, frame_ip_end, type_idx);
                func_type = cc->cur_wasm_module->types[type_idx];
                param_count = func_type->param_count;
                param_types = func_type->types;
                result_count = func_type->result_count;
                result_types = func_type->types + param_count;
                if (!jit_compile_op_block(
                        cc, &frame_ip, frame_ip_end,
                        (uint32)(LABEL_TYPE_BLOCK + opcode - EXT_OP_BLOCK),
                        param_count, param_types, result_count, result_types))
                    return false;
                break;
            }

            case WASM_OP_ELSE:
                if (!jit_compile_op_else(cc, &frame_ip))
                    return false;
                break;

            case WASM_OP_END:
                if (!jit_compile_op_end(cc, &frame_ip))
                    return false;
                break;

            case WASM_OP_BR:
                read_leb_uint32(frame_ip, frame_ip_end, br_depth);
                if (!jit_compile_op_br(cc, br_depth, &frame_ip))
                    return false;
                break;

            case WASM_OP_BR_IF:
                read_leb_uint32(frame_ip, frame_ip_end, br_depth);
                if (!jit_compile_op_br_if(cc, br_depth, &frame_ip))
                    return false;
                break;

            case WASM_OP_BR_TABLE:
                read_leb_uint32(frame_ip, frame_ip_end, br_count);
                if (!(br_depths = jit_calloc((uint32)sizeof(uint32)
                                             * (br_count + 1)))) {
                    jit_set_last_error(cc, "allocate memory failed.");
                    goto fail;
                }
                for (i = 0; i <= br_count; i++)
                    read_leb_uint32(frame_ip, frame_ip_end, br_depths[i]);

                if (!jit_compile_op_br_table(cc, br_depths, br_count,
                                             &frame_ip)) {
                    jit_free(br_depths);
                    return false;
                }

                jit_free(br_depths);
                break;

            case WASM_OP_RETURN:
                if (!jit_compile_op_return(cc, &frame_ip))
                    return false;
                break;

            case WASM_OP_CALL_INDIRECT:
            {
                uint32 tbl_idx;

                read_leb_uint32(frame_ip, frame_ip_end, type_idx);

#if WASM_ENABLE_REF_TYPES != 0
                if (cc->enable_ref_types) {
                    read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                }
                else
#endif
                {
                    frame_ip++;
                    tbl_idx = 0;
                }

                if (!jit_compile_op_call_indirect(cc, type_idx, tbl_idx))
                    return false;
                break;
            }

#if WASM_ENABLE_TAIL_CALL != 0
            case WASM_OP_RETURN_CALL:
                if (!cc->enable_tail_call) {
                    jit_set_last_error(cc, "unsupported opcode");
                    return false;
                }
                read_leb_uint32(frame_ip, frame_ip_end, func_idx);
                if (!jit_compile_op_call(cc, func_idx, true))
                    return false;
                if (!jit_compile_op_return(cc, &frame_ip))
                    return false;
                break;

            case WASM_OP_RETURN_CALL_INDIRECT:
            {
                uint32 tbl_idx;

                if (!cc->enable_tail_call) {
                    jit_set_last_error(cc, "unsupported opcode");
                    return false;
                }

                read_leb_uint32(frame_ip, frame_ip_end, type_idx);
#if WASM_ENABLE_REF_TYPES != 0
                if (cc->enable_ref_types) {
                    read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                }
                else
#endif
                {
                    frame_ip++;
                    tbl_idx = 0;
                }

                if (!jit_compile_op_call_indirect(cc, type_idx, tbl_idx))
                    return false;
                if (!jit_compile_op_return(cc, &frame_ip))
                    return false;
                break;
            }
#endif /* end of WASM_ENABLE_TAIL_CALL */

            case WASM_OP_DROP:
                if (!jit_compile_op_drop(cc, true))
                    return false;
                break;

            case WASM_OP_DROP_64:
                if (!jit_compile_op_drop(cc, false))
                    return false;
                break;

            case WASM_OP_SELECT:
                if (!jit_compile_op_select(cc, true))
                    return false;
                break;

            case WASM_OP_SELECT_64:
                if (!jit_compile_op_select(cc, false))
                    return false;
                break;

#if WASM_ENABLE_REF_TYPES != 0
            case WASM_OP_SELECT_T:
            {
                uint32 vec_len;

                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                read_leb_uint32(frame_ip, frame_ip_end, vec_len);
                bh_assert(vec_len == 1);
                (void)vec_len;

                type_idx = *frame_ip++;
                if (!jit_compile_op_select(cc,
                                           (type_idx != VALUE_TYPE_I64)
                                               && (type_idx != VALUE_TYPE_F64)))
                    return false;
                break;
            }
            case WASM_OP_TABLE_GET:
            {
                uint32 tbl_idx;

                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                if (!jit_compile_op_table_get(cc, tbl_idx))
                    return false;
                break;
            }
            case WASM_OP_TABLE_SET:
            {
                uint32 tbl_idx;

                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                if (!jit_compile_op_table_set(cc, tbl_idx))
                    return false;
                break;
            }
            case WASM_OP_REF_NULL:
            {
                uint32 type;

                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                read_leb_uint32(frame_ip, frame_ip_end, type);

                if (!jit_compile_op_ref_null(cc))
                    return false;

                (void)type;
                break;
            }
            case WASM_OP_REF_IS_NULL:
            {
                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                if (!jit_compile_op_ref_is_null(cc))
                    return false;
                break;
            }
            case WASM_OP_REF_FUNC:
            {
                if (!cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }

                read_leb_uint32(frame_ip, frame_ip_end, func_idx);
                if (!jit_compile_op_ref_func(cc, func_idx))
                    return false;
                break;
            }
#endif

            case WASM_OP_GET_LOCAL:
                read_leb_uint32(frame_ip, frame_ip_end, local_idx);
                if (!jit_compile_op_get_local(cc, local_idx))
                    return false;
                break;

            case WASM_OP_SET_LOCAL:
                read_leb_uint32(frame_ip, frame_ip_end, local_idx);
                if (!jit_compile_op_set_local(cc, local_idx))
                    return false;
                break;

            case WASM_OP_TEE_LOCAL:
                read_leb_uint32(frame_ip, frame_ip_end, local_idx);
                if (!jit_compile_op_tee_local(cc, local_idx))
                    return false;
                break;

            case WASM_OP_GET_GLOBAL:
            case WASM_OP_GET_GLOBAL_64:
                read_leb_uint32(frame_ip, frame_ip_end, global_idx);
                if (!jit_compile_op_get_global(cc, global_idx))
                    return false;
                break;

            case WASM_OP_SET_GLOBAL:
            case WASM_OP_SET_GLOBAL_64:
            case WASM_OP_SET_GLOBAL_AUX_STACK:
                read_leb_uint32(frame_ip, frame_ip_end, global_idx);
                if (!jit_compile_op_set_global(
                        cc, global_idx,
                        opcode == WASM_OP_SET_GLOBAL_AUX_STACK ? true : false))
                    return false;
                break;

            case WASM_OP_I32_LOAD:
                bytes = 4;
                sign = true;
                goto op_i32_load;
            case WASM_OP_I32_LOAD8_S:
            case WASM_OP_I32_LOAD8_U:
                bytes = 1;
                sign = (opcode == WASM_OP_I32_LOAD8_S) ? true : false;
                goto op_i32_load;
            case WASM_OP_I32_LOAD16_S:
            case WASM_OP_I32_LOAD16_U:
                bytes = 2;
                sign = (opcode == WASM_OP_I32_LOAD16_S) ? true : false;
            op_i32_load:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_i32_load(cc, align, offset, bytes, sign,
                                             false))
                    return false;
                break;

            case WASM_OP_I64_LOAD:
                bytes = 8;
                sign = true;
                goto op_i64_load;
            case WASM_OP_I64_LOAD8_S:
            case WASM_OP_I64_LOAD8_U:
                bytes = 1;
                sign = (opcode == WASM_OP_I64_LOAD8_S) ? true : false;
                goto op_i64_load;
            case WASM_OP_I64_LOAD16_S:
            case WASM_OP_I64_LOAD16_U:
                bytes = 2;
                sign = (opcode == WASM_OP_I64_LOAD16_S) ? true : false;
                goto op_i64_load;
            case WASM_OP_I64_LOAD32_S:
            case WASM_OP_I64_LOAD32_U:
                bytes = 4;
                sign = (opcode == WASM_OP_I64_LOAD32_S) ? true : false;
            op_i64_load:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_i64_load(cc, align, offset, bytes, sign,
                                             false))
                    return false;
                break;

            case WASM_OP_F32_LOAD:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_f32_load(cc, align, offset))
                    return false;
                break;

            case WASM_OP_F64_LOAD:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_f64_load(cc, align, offset))
                    return false;
                break;

            case WASM_OP_I32_STORE:
                bytes = 4;
                goto op_i32_store;
            case WASM_OP_I32_STORE8:
                bytes = 1;
                goto op_i32_store;
            case WASM_OP_I32_STORE16:
                bytes = 2;
            op_i32_store:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_i32_store(cc, align, offset, bytes, false))
                    return false;
                break;

            case WASM_OP_I64_STORE:
                bytes = 8;
                goto op_i64_store;
            case WASM_OP_I64_STORE8:
                bytes = 1;
                goto op_i64_store;
            case WASM_OP_I64_STORE16:
                bytes = 2;
                goto op_i64_store;
            case WASM_OP_I64_STORE32:
                bytes = 4;
            op_i64_store:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_i64_store(cc, align, offset, bytes, false))
                    return false;
                break;

            case WASM_OP_F32_STORE:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_f32_store(cc, align, offset))
                    return false;
                break;

            case WASM_OP_F64_STORE:
                read_leb_uint32(frame_ip, frame_ip_end, align);
                read_leb_uint32(frame_ip, frame_ip_end, offset);
                if (!jit_compile_op_f64_store(cc, align, offset))
                    return false;
                break;

            case WASM_OP_MEMORY_SIZE:
                read_leb_uint32(frame_ip, frame_ip_end, mem_idx);
                if (!jit_compile_op_memory_size(cc))
                    return false;
                (void)mem_idx;
                break;

            case WASM_OP_MEMORY_GROW:
                read_leb_uint32(frame_ip, frame_ip_end, mem_idx);
                if (!jit_compile_op_memory_grow(cc))
                    return false;
                break;

            case WASM_OP_I32_CONST:
                read_leb_int32(frame_ip, frame_ip_end, i32_const);
                if (!jit_compile_op_i32_const(cc, i32_const))
                    return false;
                break;

            case WASM_OP_I64_CONST:
                read_leb_int64(frame_ip, frame_ip_end, i64_const);
                if (!jit_compile_op_i64_const(cc, i64_const))
                    return false;
                break;

            case WASM_OP_F32_CONST:
                p_f32 = (uint8 *)&f32_const;
                for (i = 0; i < sizeof(float32); i++)
                    *p_f32++ = *frame_ip++;
                if (!jit_compile_op_f32_const(cc, f32_const))
                    return false;
                break;

            case WASM_OP_F64_CONST:
                p_f64 = (uint8 *)&f64_const;
                for (i = 0; i < sizeof(float64); i++)
                    *p_f64++ = *frame_ip++;
                if (!jit_compile_op_f64_const(cc, f64_const))
                    return false;
                break;

            case WASM_OP_I32_EQZ:
            case WASM_OP_I32_EQ:
            case WASM_OP_I32_NE:
            case WASM_OP_I32_LT_S:
            case WASM_OP_I32_LT_U:
            case WASM_OP_I32_GT_S:
            case WASM_OP_I32_GT_U:
            case WASM_OP_I32_LE_S:
            case WASM_OP_I32_LE_U:
            case WASM_OP_I32_GE_S:
            case WASM_OP_I32_GE_U:
                if (!jit_compile_op_i32_compare(cc, INT_EQZ + opcode
                                                        - WASM_OP_I32_EQZ))
                    return false;
                break;

            case WASM_OP_I64_EQZ:
            case WASM_OP_I64_EQ:
            case WASM_OP_I64_NE:
            case WASM_OP_I64_LT_S:
            case WASM_OP_I64_LT_U:
            case WASM_OP_I64_GT_S:
            case WASM_OP_I64_GT_U:
            case WASM_OP_I64_LE_S:
            case WASM_OP_I64_LE_U:
            case WASM_OP_I64_GE_S:
            case WASM_OP_I64_GE_U:
                if (!jit_compile_op_i64_compare(cc, INT_EQZ + opcode
                                                        - WASM_OP_I64_EQZ))
                    return false;
                break;

            case WASM_OP_F32_EQ:
            case WASM_OP_F32_NE:
            case WASM_OP_F32_LT:
            case WASM_OP_F32_GT:
            case WASM_OP_F32_LE:
            case WASM_OP_F32_GE:
                if (!jit_compile_op_f32_compare(cc, FLOAT_EQ + opcode
                                                        - WASM_OP_F32_EQ))
                    return false;
                break;

            case WASM_OP_F64_EQ:
            case WASM_OP_F64_NE:
            case WASM_OP_F64_LT:
            case WASM_OP_F64_GT:
            case WASM_OP_F64_LE:
            case WASM_OP_F64_GE:
                if (!jit_compile_op_f64_compare(cc, FLOAT_EQ + opcode
                                                        - WASM_OP_F64_EQ))
                    return false;
                break;

            case WASM_OP_I32_CLZ:
                if (!jit_compile_op_i32_clz(cc))
                    return false;
                break;

            case WASM_OP_I32_CTZ:
                if (!jit_compile_op_i32_ctz(cc))
                    return false;
                break;

            case WASM_OP_I32_POPCNT:
                if (!jit_compile_op_i32_popcnt(cc))
                    return false;
                break;

            case WASM_OP_I32_ADD:
            case WASM_OP_I32_SUB:
            case WASM_OP_I32_MUL:
            case WASM_OP_I32_DIV_S:
            case WASM_OP_I32_DIV_U:
            case WASM_OP_I32_REM_S:
            case WASM_OP_I32_REM_U:
                if (!jit_compile_op_i32_arithmetic(
                        cc, INT_ADD + opcode - WASM_OP_I32_ADD, &frame_ip))
                    return false;
                break;

            case WASM_OP_I32_AND:
            case WASM_OP_I32_OR:
            case WASM_OP_I32_XOR:
                if (!jit_compile_op_i32_bitwise(cc, INT_SHL + opcode
                                                        - WASM_OP_I32_AND))
                    return false;
                break;

            case WASM_OP_I32_SHL:
            case WASM_OP_I32_SHR_S:
            case WASM_OP_I32_SHR_U:
            case WASM_OP_I32_ROTL:
            case WASM_OP_I32_ROTR:
                if (!jit_compile_op_i32_shift(cc, INT_SHL + opcode
                                                      - WASM_OP_I32_SHL))
                    return false;
                break;

            case WASM_OP_I64_CLZ:
                if (!jit_compile_op_i64_clz(cc))
                    return false;
                break;

            case WASM_OP_I64_CTZ:
                if (!jit_compile_op_i64_ctz(cc))
                    return false;
                break;

            case WASM_OP_I64_POPCNT:
                if (!jit_compile_op_i64_popcnt(cc))
                    return false;
                break;

            case WASM_OP_I64_ADD:
            case WASM_OP_I64_SUB:
            case WASM_OP_I64_MUL:
            case WASM_OP_I64_DIV_S:
            case WASM_OP_I64_DIV_U:
            case WASM_OP_I64_REM_S:
            case WASM_OP_I64_REM_U:
                if (!jit_compile_op_i64_arithmetic(
                        cc, INT_ADD + opcode - WASM_OP_I64_ADD, &frame_ip))
                    return false;
                break;

            case WASM_OP_I64_AND:
            case WASM_OP_I64_OR:
            case WASM_OP_I64_XOR:
                if (!jit_compile_op_i64_bitwise(cc, INT_SHL + opcode
                                                        - WASM_OP_I64_AND))
                    return false;
                break;

            case WASM_OP_I64_SHL:
            case WASM_OP_I64_SHR_S:
            case WASM_OP_I64_SHR_U:
            case WASM_OP_I64_ROTL:
            case WASM_OP_I64_ROTR:
                if (!jit_compile_op_i64_shift(cc, INT_SHL + opcode
                                                      - WASM_OP_I64_SHL))
                    return false;
                break;

            case WASM_OP_F32_ABS:
            case WASM_OP_F32_NEG:
            case WASM_OP_F32_CEIL:
            case WASM_OP_F32_FLOOR:
            case WASM_OP_F32_TRUNC:
            case WASM_OP_F32_NEAREST:
            case WASM_OP_F32_SQRT:
                if (!jit_compile_op_f32_math(cc, FLOAT_ABS + opcode
                                                     - WASM_OP_F32_ABS))
                    return false;
                break;

            case WASM_OP_F32_ADD:
            case WASM_OP_F32_SUB:
            case WASM_OP_F32_MUL:
            case WASM_OP_F32_DIV:
            case WASM_OP_F32_MIN:
            case WASM_OP_F32_MAX:
                if (!jit_compile_op_f32_arithmetic(cc, FLOAT_ADD + opcode
                                                           - WASM_OP_F32_ADD))
                    return false;
                break;

            case WASM_OP_F32_COPYSIGN:
                if (!jit_compile_op_f32_copysign(cc))
                    return false;
                break;

            case WASM_OP_F64_ABS:
            case WASM_OP_F64_NEG:
            case WASM_OP_F64_CEIL:
            case WASM_OP_F64_FLOOR:
            case WASM_OP_F64_TRUNC:
            case WASM_OP_F64_NEAREST:
            case WASM_OP_F64_SQRT:
                if (!jit_compile_op_f64_math(cc, FLOAT_ABS + opcode
                                                     - WASM_OP_F64_ABS))
                    return false;
                break;

            case WASM_OP_F64_ADD:
            case WASM_OP_F64_SUB:
            case WASM_OP_F64_MUL:
            case WASM_OP_F64_DIV:
            case WASM_OP_F64_MIN:
            case WASM_OP_F64_MAX:
                if (!jit_compile_op_f64_arithmetic(cc, FLOAT_ADD + opcode
                                                           - WASM_OP_F64_ADD))
                    return false;
                break;

            case WASM_OP_F64_COPYSIGN:
                if (!jit_compile_op_f64_copysign(cc))
                    return false;
                break;

            case WASM_OP_I32_WRAP_I64:
                if (!jit_compile_op_i32_wrap_i64(cc))
                    return false;
                break;

            case WASM_OP_I32_TRUNC_S_F32:
            case WASM_OP_I32_TRUNC_U_F32:
                sign = (opcode == WASM_OP_I32_TRUNC_S_F32) ? true : false;
                if (!jit_compile_op_i32_trunc_f32(cc, sign, false))
                    return false;
                break;

            case WASM_OP_I32_TRUNC_S_F64:
            case WASM_OP_I32_TRUNC_U_F64:
                sign = (opcode == WASM_OP_I32_TRUNC_S_F64) ? true : false;
                if (!jit_compile_op_i32_trunc_f64(cc, sign, false))
                    return false;
                break;

            case WASM_OP_I64_EXTEND_S_I32:
            case WASM_OP_I64_EXTEND_U_I32:
                sign = (opcode == WASM_OP_I64_EXTEND_S_I32) ? true : false;
                if (!jit_compile_op_i64_extend_i32(cc, sign))
                    return false;
                break;

            case WASM_OP_I64_TRUNC_S_F32:
            case WASM_OP_I64_TRUNC_U_F32:
                sign = (opcode == WASM_OP_I64_TRUNC_S_F32) ? true : false;
                if (!jit_compile_op_i64_trunc_f32(cc, sign, false))
                    return false;
                break;

            case WASM_OP_I64_TRUNC_S_F64:
            case WASM_OP_I64_TRUNC_U_F64:
                sign = (opcode == WASM_OP_I64_TRUNC_S_F64) ? true : false;
                if (!jit_compile_op_i64_trunc_f64(cc, sign, false))
                    return false;
                break;

            case WASM_OP_F32_CONVERT_S_I32:
            case WASM_OP_F32_CONVERT_U_I32:
                sign = (opcode == WASM_OP_F32_CONVERT_S_I32) ? true : false;
                if (!jit_compile_op_f32_convert_i32(cc, sign))
                    return false;
                break;

            case WASM_OP_F32_CONVERT_S_I64:
            case WASM_OP_F32_CONVERT_U_I64:
                sign = (opcode == WASM_OP_F32_CONVERT_S_I64) ? true : false;
                if (!jit_compile_op_f32_convert_i64(cc, sign))
                    return false;
                break;

            case WASM_OP_F32_DEMOTE_F64:
                if (!jit_compile_op_f32_demote_f64(cc))
                    return false;
                break;

            case WASM_OP_F64_CONVERT_S_I32:
            case WASM_OP_F64_CONVERT_U_I32:
                sign = (opcode == WASM_OP_F64_CONVERT_S_I32) ? true : false;
                if (!jit_compile_op_f64_convert_i32(cc, sign))
                    return false;
                break;

            case WASM_OP_F64_CONVERT_S_I64:
            case WASM_OP_F64_CONVERT_U_I64:
                sign = (opcode == WASM_OP_F64_CONVERT_S_I64) ? true : false;
                if (!jit_compile_op_f64_convert_i64(cc, sign))
                    return false;
                break;

            case WASM_OP_F64_PROMOTE_F32:
                if (!jit_compile_op_f64_promote_f32(cc))
                    return false;
                break;

            case WASM_OP_I32_REINTERPRET_F32:
                if (!jit_compile_op_i32_reinterpret_f32(cc))
                    return false;
                break;

            case WASM_OP_I64_REINTERPRET_F64:
                if (!jit_compile_op_i64_reinterpret_f64(cc))
                    return false;
                break;

            case WASM_OP_F32_REINTERPRET_I32:
                if (!jit_compile_op_f32_reinterpret_i32(cc))
                    return false;
                break;

            case WASM_OP_F64_REINTERPRET_I64:
                if (!jit_compile_op_f64_reinterpret_i64(cc))
                    return false;
                break;

            case WASM_OP_I32_EXTEND8_S:
                if (!jit_compile_op_i32_extend_i32(cc, 8))
                    return false;
                break;

            case WASM_OP_I32_EXTEND16_S:
                if (!jit_compile_op_i32_extend_i32(cc, 16))
                    return false;
                break;

            case WASM_OP_I64_EXTEND8_S:
                if (!jit_compile_op_i64_extend_i64(cc, 8))
                    return false;
                break;

            case WASM_OP_I64_EXTEND16_S:
                if (!jit_compile_op_i64_extend_i64(cc, 16))
                    return false;
                break;

            case WASM_OP_I64_EXTEND32_S:
                if (!jit_compile_op_i64_extend_i64(cc, 32))
                    return false;
                break;

            case WASM_OP_MISC_PREFIX:
            {
                uint32 opcode1;

                read_leb_uint32(frame_ip, frame_ip_end, opcode1);
                opcode = (uint32)opcode1;

#if WASM_ENABLE_BULK_MEMORY != 0
                if (WASM_OP_MEMORY_INIT <= opcode
                    && opcode <= WASM_OP_MEMORY_FILL
                    && !cc->enable_bulk_memory) {
                    goto unsupport_bulk_memory;
                }
#endif

#if WASM_ENABLE_REF_TYPES != 0
                if (WASM_OP_TABLE_INIT <= opcode && opcode <= WASM_OP_TABLE_FILL
                    && !cc->enable_ref_types) {
                    goto unsupport_ref_types;
                }
#endif

                switch (opcode) {
                    case WASM_OP_I32_TRUNC_SAT_S_F32:
                    case WASM_OP_I32_TRUNC_SAT_U_F32:
                        sign = (opcode == WASM_OP_I32_TRUNC_SAT_S_F32) ? true
                                                                       : false;
                        if (!jit_compile_op_i32_trunc_f32(cc, sign, true))
                            return false;
                        break;
                    case WASM_OP_I32_TRUNC_SAT_S_F64:
                    case WASM_OP_I32_TRUNC_SAT_U_F64:
                        sign = (opcode == WASM_OP_I32_TRUNC_SAT_S_F64) ? true
                                                                       : false;
                        if (!jit_compile_op_i32_trunc_f64(cc, sign, true))
                            return false;
                        break;
                    case WASM_OP_I64_TRUNC_SAT_S_F32:
                    case WASM_OP_I64_TRUNC_SAT_U_F32:
                        sign = (opcode == WASM_OP_I64_TRUNC_SAT_S_F32) ? true
                                                                       : false;
                        if (!jit_compile_op_i64_trunc_f32(cc, sign, true))
                            return false;
                        break;
                    case WASM_OP_I64_TRUNC_SAT_S_F64:
                    case WASM_OP_I64_TRUNC_SAT_U_F64:
                        sign = (opcode == WASM_OP_I64_TRUNC_SAT_S_F64) ? true
                                                                       : false;
                        if (!jit_compile_op_i64_trunc_f64(cc, sign, true))
                            return false;
                        break;
#if WASM_ENABLE_BULK_MEMORY != 0
                    case WASM_OP_MEMORY_INIT:
                    {
                        uint32 seg_index;
                        read_leb_uint32(frame_ip, frame_ip_end, seg_index);
                        frame_ip++;
                        if (!jit_compile_op_memory_init(cc, seg_index))
                            return false;
                        break;
                    }
                    case WASM_OP_DATA_DROP:
                    {
                        uint32 seg_index;
                        read_leb_uint32(frame_ip, frame_ip_end, seg_index);
                        if (!jit_compile_op_data_drop(cc, seg_index))
                            return false;
                        break;
                    }
                    case WASM_OP_MEMORY_COPY:
                    {
                        frame_ip += 2;
                        if (!jit_compile_op_memory_copy(cc))
                            return false;
                        break;
                    }
                    case WASM_OP_MEMORY_FILL:
                    {
                        frame_ip++;
                        if (!jit_compile_op_memory_fill(cc))
                            return false;
                        break;
                    }
#endif /* WASM_ENABLE_BULK_MEMORY */
#if WASM_ENABLE_REF_TYPES != 0
                    case WASM_OP_TABLE_INIT:
                    {
                        uint32 tbl_idx, tbl_seg_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, tbl_seg_idx);
                        read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                        if (!jit_compile_op_table_init(cc, tbl_idx,
                                                       tbl_seg_idx))
                            return false;
                        break;
                    }
                    case WASM_OP_ELEM_DROP:
                    {
                        uint32 tbl_seg_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, tbl_seg_idx);
                        if (!jit_compile_op_elem_drop(cc, tbl_seg_idx))
                            return false;
                        break;
                    }
                    case WASM_OP_TABLE_COPY:
                    {
                        uint32 src_tbl_idx, dst_tbl_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, dst_tbl_idx);
                        read_leb_uint32(frame_ip, frame_ip_end, src_tbl_idx);
                        if (!jit_compile_op_table_copy(cc, src_tbl_idx,
                                                       dst_tbl_idx))
                            return false;
                        break;
                    }
                    case WASM_OP_TABLE_GROW:
                    {
                        uint32 tbl_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                        if (!jit_compile_op_table_grow(cc, tbl_idx))
                            return false;
                        break;
                    }

                    case WASM_OP_TABLE_SIZE:
                    {
                        uint32 tbl_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                        if (!jit_compile_op_table_size(cc, tbl_idx))
                            return false;
                        break;
                    }
                    case WASM_OP_TABLE_FILL:
                    {
                        uint32 tbl_idx;

                        read_leb_uint32(frame_ip, frame_ip_end, tbl_idx);
                        if (!jit_compile_op_table_fill(cc, tbl_idx))
                            return false;
                        break;
                    }
#endif /* WASM_ENABLE_REF_TYPES */
                    default:
                        jit_set_last_error(cc, "unsupported opcode");
                        return false;
                }
                break;
            }

#if WASM_ENABLE_SHARED_MEMORY != 0
            case WASM_OP_ATOMIC_PREFIX:
            {
                uint8 bin_op, op_type;

                if (frame_ip < frame_ip_end) {
                    opcode = *frame_ip++;
                }
                if (opcode != WASM_OP_ATOMIC_FENCE) {
                    read_leb_uint32(frame_ip, frame_ip_end, align);
                    read_leb_uint32(frame_ip, frame_ip_end, offset);
                }
                switch (opcode) {
                    case WASM_OP_ATOMIC_WAIT32:
                        if (!jit_compile_op_atomic_wait(cc, VALUE_TYPE_I32,
                                                        align, offset, 4))
                            return false;
                        break;
                    case WASM_OP_ATOMIC_WAIT64:
                        if (!jit_compile_op_atomic_wait(cc, VALUE_TYPE_I64,
                                                        align, offset, 8))
                            return false;
                        break;
                    case WASM_OP_ATOMIC_NOTIFY:
                        if (!jit_compiler_op_atomic_notify(cc, align, offset,
                                                           bytes))
                            return false;
                        break;
                    case WASM_OP_ATOMIC_I32_LOAD:
                        bytes = 4;
                        goto op_atomic_i32_load;
                    case WASM_OP_ATOMIC_I32_LOAD8_U:
                        bytes = 1;
                        goto op_atomic_i32_load;
                    case WASM_OP_ATOMIC_I32_LOAD16_U:
                        bytes = 2;
                    op_atomic_i32_load:
                        if (!jit_compile_op_i32_load(cc, align, offset, bytes,
                                                     sign, true))
                            return false;
                        break;

                    case WASM_OP_ATOMIC_I64_LOAD:
                        bytes = 8;
                        goto op_atomic_i64_load;
                    case WASM_OP_ATOMIC_I64_LOAD8_U:
                        bytes = 1;
                        goto op_atomic_i64_load;
                    case WASM_OP_ATOMIC_I64_LOAD16_U:
                        bytes = 2;
                        goto op_atomic_i64_load;
                    case WASM_OP_ATOMIC_I64_LOAD32_U:
                        bytes = 4;
                    op_atomic_i64_load:
                        if (!jit_compile_op_i64_load(cc, align, offset, bytes,
                                                     sign, true))
                            return false;
                        break;

                    case WASM_OP_ATOMIC_I32_STORE:
                        bytes = 4;
                        goto op_atomic_i32_store;
                    case WASM_OP_ATOMIC_I32_STORE8:
                        bytes = 1;
                        goto op_atomic_i32_store;
                    case WASM_OP_ATOMIC_I32_STORE16:
                        bytes = 2;
                    op_atomic_i32_store:
                        if (!jit_compile_op_i32_store(cc, align, offset, bytes,
                                                      true))
                            return false;
                        break;

                    case WASM_OP_ATOMIC_I64_STORE:
                        bytes = 8;
                        goto op_atomic_i64_store;
                    case WASM_OP_ATOMIC_I64_STORE8:
                        bytes = 1;
                        goto op_atomic_i64_store;
                    case WASM_OP_ATOMIC_I64_STORE16:
                        bytes = 2;
                        goto op_atomic_i64_store;
                    case WASM_OP_ATOMIC_I64_STORE32:
                        bytes = 4;
                    op_atomic_i64_store:
                        if (!jit_compile_op_i64_store(cc, align, offset, bytes,
                                                      true))
                            return false;
                        break;

                    case WASM_OP_ATOMIC_RMW_I32_CMPXCHG:
                        bytes = 4;
                        op_type = VALUE_TYPE_I32;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I64_CMPXCHG:
                        bytes = 8;
                        op_type = VALUE_TYPE_I64;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I32_CMPXCHG8_U:
                        bytes = 1;
                        op_type = VALUE_TYPE_I32;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I32_CMPXCHG16_U:
                        bytes = 2;
                        op_type = VALUE_TYPE_I32;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I64_CMPXCHG8_U:
                        bytes = 1;
                        op_type = VALUE_TYPE_I64;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I64_CMPXCHG16_U:
                        bytes = 2;
                        op_type = VALUE_TYPE_I64;
                        goto op_atomic_cmpxchg;
                    case WASM_OP_ATOMIC_RMW_I64_CMPXCHG32_U:
                        bytes = 4;
                        op_type = VALUE_TYPE_I64;
                    op_atomic_cmpxchg:
                        if (!jit_compile_op_atomic_cmpxchg(cc, op_type, align,
                                                           offset, bytes))
                            return false;
                        break;

                        COMPILE_ATOMIC_RMW(Add, ADD);
                        COMPILE_ATOMIC_RMW(Sub, SUB);
                        COMPILE_ATOMIC_RMW(And, AND);
                        COMPILE_ATOMIC_RMW(Or, OR);
                        COMPILE_ATOMIC_RMW(Xor, XOR);
                        COMPILE_ATOMIC_RMW(Xchg, XCHG);

                    build_atomic_rmw:
                        if (!jit_compile_op_atomic_rmw(cc, bin_op, op_type,
                                                       align, offset, bytes))
                            return false;
                        break;

                    default:
                        jit_set_last_error(cc, "unsupported opcode");
                        return false;
                }
                break;
            }
#endif /* end of WASM_ENABLE_SHARED_MEMORY */

            default:
                jit_set_last_error(cc, "unsupported opcode");
                return false;
        }
        /* Error may occur when creating registers, basic blocks, insns,
           consts and labels, in which the return value may be unchecked,
           here we check again */
        if (jit_get_last_error(cc)) {
            return false;
        }
    }

    (void)func_idx;
    return true;

#if WASM_ENABLE_REF_TYPES != 0
unsupport_ref_types:
    jit_set_last_error(cc, "reference type instruction was found, "
                           "try removing --disable-ref-types option");
    return false;
#endif

#if WASM_ENABLE_BULK_MEMORY != 0
unsupport_bulk_memory:
    jit_set_last_error(cc, "bulk memory instruction was found, "
                           "try removing --disable-bulk-memory option");
    return false;
#endif

fail:
    return false;
}

JitBasicBlock *
jit_frontend_translate_func(JitCompContext *cc)
{
    JitFrame *jit_frame;
    JitBlock *jit_block;

    if (!(jit_frame = init_func_translation(cc))) {
        return NULL;
    }

    if (!(jit_block = create_func_block(cc))) {
        return NULL;
    }

    if (!jit_compile_func(cc)) {
        return NULL;
    }

    return jit_block->basic_block_entry;
}
