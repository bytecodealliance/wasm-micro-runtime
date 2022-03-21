/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_control.h"
#include "jit_emit_exception.h"
#include "../jit_frontend.h"
#include "../interpreter/wasm_loader.h"

#define CREATE_BASIC_BLOCK(new_basic_block)                       \
    do {                                                          \
        bh_assert(!new_basic_block);                              \
        if (!(new_basic_block = jit_cc_new_basic_block(cc, 0))) { \
            jit_set_last_error(cc, "create basic block failed");  \
            goto fail;                                            \
        }                                                         \
    } while (0)

#define CURR_BASIC_BLOCK() cc->cur_basic_block

#define BUILD_BR(target_block)                                     \
    do {                                                           \
        if (!GEN_INSN(JMP, jit_basic_block_label(target_block))) { \
            jit_set_last_error(cc, "generate jmp insn failed");    \
            goto fail;                                             \
        }                                                          \
    } while (0)

#define BUILD_COND_BR(value_if, block_then, block_else)                       \
    do {                                                                      \
        if (!GEN_INSN(CMP, cc->cmp_reg, value_if, NEW_CONST(cc, 0))           \
            || !GEN_INSN(BNE, cc->cmp_reg, jit_basic_block_label(block_then), \
                         jit_basic_block_label(block_else))) {                \
            jit_set_last_error(cc, "generate bne insn failed");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define SET_BUILDER_POS(basic_block)       \
    do {                                   \
        cc->cur_basic_block = basic_block; \
    } while (0)

#define SET_BB_BEGIN_BCIP(basic_block, bcip)                                   \
    do {                                                                       \
        *(jit_annl_begin_bcip(cc, jit_basic_block_label(basic_block))) = bcip; \
    } while (0)

#define SET_BB_END_BCIP(basic_block, bcip)                                   \
    do {                                                                     \
        *(jit_annl_end_bcip(cc, jit_basic_block_label(basic_block))) = bcip; \
    } while (0)

static JitBlock *
get_target_block(JitCompContext *cc, uint32 br_depth)
{
    uint32 i = br_depth;
    JitBlock *block = cc->block_stack.block_list_end;

    while (i-- > 0 && block) {
        block = block->prev;
    }

    if (!block) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return NULL;
    }
    return block;
}

static bool
load_block_params(JitCompContext *cc, JitBlock *block)
{
    JitFrame *jit_frame = cc->jit_frame;
    uint32 offset, i;
    JitReg value = 0;

    /* Clear jit frame's locals and stacks */
    clear_values(jit_frame);

    /* Restore jit frame's sp to block's sp begin */
    jit_frame->sp = block->frame_sp_begin;

    /* Load params to new block */
    offset = (uint32)(jit_frame->sp - jit_frame->lp);
    for (i = 0; i < block->param_count; i++) {
        switch (block->param_types[i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = gen_load_i32(jit_frame, offset);
                offset++;
                break;
            case VALUE_TYPE_I64:
                value = gen_load_i64(jit_frame, offset);
                offset += 2;
                break;
            case VALUE_TYPE_F32:
                value = gen_load_f32(jit_frame, offset);
                offset++;
                break;
            case VALUE_TYPE_F64:
                value = gen_load_f64(jit_frame, offset);
                offset += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
        PUSH(value, block->param_types[i]);
    }

    return true;
fail:
    return false;
}

static bool
load_block_results(JitCompContext *cc, JitBlock *block)
{
    JitFrame *jit_frame = cc->jit_frame;
    uint32 offset, i;
    JitReg value = 0;

    /* Restore jit frame's sp to block's sp begin */
    jit_frame->sp = block->frame_sp_begin;

    /* Load results to new block */
    offset = (uint32)(jit_frame->sp - jit_frame->lp);
    for (i = 0; i < block->result_count; i++) {
        switch (block->result_types[i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = gen_load_i32(jit_frame, offset);
                offset++;
                break;
            case VALUE_TYPE_I64:
                value = gen_load_i64(jit_frame, offset);
                offset += 2;
                break;
            case VALUE_TYPE_F32:
                value = gen_load_f32(jit_frame, offset);
                offset++;
                break;
            case VALUE_TYPE_F64:
                value = gen_load_f64(jit_frame, offset);
                offset += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
        PUSH(value, block->result_types[i]);
    }

    return true;
fail:
    return false;
}

static bool
push_jit_block_to_stack_and_pass_params(JitCompContext *cc, JitBlock *block,
                                        JitBasicBlock *basic_block, JitReg cond)
{
    JitFrame *jit_frame = cc->jit_frame;
    JitValue *value_list_head = NULL, *value_list_end = NULL, *jit_value;
    JitInsn *insn;
    JitReg value;
    uint32 i, param_index, cell_num;

    if (cc->cur_basic_block == basic_block) {
        /* Reuse the current basic block and no need to commit values,
           we just move param values from current block's value stack to
           the new block's value stack */
        for (i = 0; i < block->param_count; i++) {
            param_index = block->param_count - 1 - i;
            jit_value = jit_value_stack_pop(
                &cc->block_stack.block_list_end->value_stack);
            if (!value_list_head) {
                value_list_head = value_list_end = jit_value;
                jit_value->prev = jit_value->next = NULL;
            }
            else {
                jit_value->prev = NULL;
                jit_value->next = value_list_head;
                value_list_head->prev = jit_value;
                value_list_head = jit_value;
            }
        }
        block->value_stack.value_list_head = value_list_head;
        block->value_stack.value_list_end = value_list_end;

        /* Save block's begin frame sp */
        cell_num = wasm_get_cell_num(block->param_types, block->param_count);
        block->frame_sp_begin = jit_frame->sp - cell_num;

        /* Push the new block to block stack */
        jit_block_stack_push(&cc->block_stack, block);

        /* Continue to translate current block */
    }
    else {
        /* Commit register values to locals and stacks */
        gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);

        /* Pop param values from current block's value stack */
        for (i = 0; i < block->param_count; i++) {
            param_index = block->param_count - 1 - i;
            POP(value, block->param_types[param_index]);
        }

        /* Clear frame values */
        clear_values(jit_frame);
        /* Save block's begin frame sp */
        block->frame_sp_begin = jit_frame->sp;

        /* Push the new block to block stack */
        jit_block_stack_push(&cc->block_stack, block);

        if (block->label_type == LABEL_TYPE_LOOP) {
            BUILD_BR(basic_block);
        }
        else {
            /* IF block with condition br insn */
            if (!GEN_INSN(CMP, cc->cmp_reg, cond, NEW_CONST(I32, 0))
                || !(insn = GEN_INSN(BNE, cc->cmp_reg,
                                     jit_basic_block_label(basic_block), 0))) {
                jit_set_last_error(cc, "generate cond br failed");
                goto fail;
            }

            /* Don't create else basic block or end basic block now, just
               save its incoming BNE insn, and patch the insn's else label
               when the basic block is lazily created */
            if (block->wasm_code_else) {
                block->incoming_insn_for_else_bb = insn;
            }
            else {
                if (!jit_block_add_incoming_insn(block, insn)) {
                    jit_set_last_error(cc, "add incoming insn failed");
                    goto fail;
                }
            }
        }

        /* Start to translate the block */
        SET_BUILDER_POS(basic_block);

        /* Push the block parameters */
        if (!load_block_params(cc, block)) {
            goto fail;
        }
    }
    return true;
fail:
    return false;
}

static void
copy_block_arities(JitCompContext *cc, JitReg dst_frame_sp, uint8 *dst_types,
                   uint32 dst_type_count)
{
    JitFrame *jit_frame;
    uint32 offset_src, offset_dst, i;
    JitReg value;

    jit_frame = cc->jit_frame;
    offset_src = (uint32)(jit_frame->sp - jit_frame->lp)
                 - wasm_get_cell_num(dst_types, dst_type_count);
    offset_dst = 0;

    /* pop values from stack and store to dest frame */
    for (i = 0; i < dst_type_count; i++) {
        switch (dst_types[i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = gen_load_i32(jit_frame, offset_src);
                GEN_INSN(STI32, value, dst_frame_sp,
                         NEW_CONST(I32, offset_dst * 4));
                offset_src++;
                offset_dst++;
                break;
            case VALUE_TYPE_I64:
                value = gen_load_i64(jit_frame, offset_src);
                GEN_INSN(STI64, value, dst_frame_sp,
                         NEW_CONST(I32, offset_dst * 4));
                offset_src += 2;
                offset_dst += 2;
                break;
            case VALUE_TYPE_F32:
                value = gen_load_f32(jit_frame, offset_src);
                GEN_INSN(STF32, value, dst_frame_sp,
                         NEW_CONST(I32, offset_dst * 4));
                offset_src++;
                offset_dst++;
                break;
            case VALUE_TYPE_F64:
                value = gen_load_f64(jit_frame, offset_src);
                GEN_INSN(STI64, value, dst_frame_sp,
                         NEW_CONST(I32, offset_dst * 4));
                offset_src += 2;
                offset_dst += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
    }
}

static void
handle_func_return(JitCompContext *cc, JitBlock *block)
{
    JitReg prev_frame, prev_frame_sp;

#if UINTPTR_MAX == UINT64_MAX
    prev_frame = jit_cc_new_reg_I64(cc);
    prev_frame_sp = jit_cc_new_reg_I64(cc);

    /* prev_frame = cur_frame->prev_frame */
    GEN_INSN(LDI64, prev_frame, cc->fp_reg,
             NEW_CONST(I32, offsetof(WASMInterpFrame, prev_frame)));
    GEN_INSN(LDI64, prev_frame_sp, prev_frame,
             NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#else
    prev_frame = jit_cc_new_reg_I32(cc);
    prev_frame_sp = jit_cc_new_reg_I32(cc);

    /* prev_frame = cur_frame->prev_frame */
    GEN_INSN(LDI32, prev_frame, cc->fp_reg,
             NEW_CONST(I32, offsetof(WASMInterpFrame, prev_frame)));
    GEN_INSN(LDI32, prev_frame_sp, prev_frame,
             NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#endif

    if (block->result_count) {
        uint32 cell_num =
            wasm_get_cell_num(block->result_types, block->result_count);

        copy_block_arities(cc, prev_frame_sp, block->result_types,
                           block->result_count);
#if UINTPTR_MAX == UINT64_MAX
        /* prev_frame->sp += cell_num */
        GEN_INSN(ADD, prev_frame_sp, prev_frame_sp,
                 NEW_CONST(I64, cell_num * 4));
        GEN_INSN(STI64, prev_frame_sp, prev_frame,
                 NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#else
        /* prev_frame->sp += cell_num */
        GEN_INSN(ADD, prev_frame_sp, prev_frame_sp,
                 NEW_CONST(I32, cell_num * 4));
        GEN_INSN(STI32, prev_frame_sp, prev_frame,
                 NEW_CONST(I32, offsetof(WASMInterpFrame, sp)));
#endif
    }

    /* Free stack space of the current frame:
       exec_env->wasm_stack.s.top = cur_frame */
#if UINTPTR_MAX == UINT64_MAX
    GEN_INSN(STI64, cc->fp_reg, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
#else
    GEN_INSN(STI32, cc->fp_reg, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, wasm_stack.s.top)));
#endif
    /* Set the prev_frame as the current frame:
       exec_env->cur_frame = prev_frame */
#if UINTPTR_MAX == UINT64_MAX
    GEN_INSN(STI64, prev_frame, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, cur_frame)));
#else
    GEN_INSN(STI32, prev_frame, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, cur_frame)));
#endif
    /* fp_reg = prev_frame */
    GEN_INSN(MOV, cc->fp_reg, prev_frame);
    /* return 0 */
    GEN_INSN(RETURNBC, NEW_CONST(I32, JIT_INTERP_ACTION_NORMAL), 0, 0);
}

static bool
handle_op_end(JitCompContext *cc, uint8 **p_frame_ip, bool from_same_block)
{
    JitFrame *jit_frame = cc->jit_frame;
    JitBlock *block;
    JitIncomingInsn *incoming_insn;
    JitInsn *insn;

    /* Check block stack */
    if (!(block = cc->block_stack.block_list_end)) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }

    if (!block->incoming_insns_for_end_bb) {
        /* No other basic blocks jumping to this end, no need to
           create the end basic block, just continue to translate
           the following opcodes */
        if (block->label_type == LABEL_TYPE_FUNCTION) {
            handle_func_return(cc, block);
            SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
        }

        /* Pop block and destroy the block */
        block = jit_block_stack_pop(&cc->block_stack);
        jit_block_destroy(block);
        return true;
    }
    else {
        /* Commit register values to locals and stacks */
        gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);
        /* Clear frame values */
        clear_values(jit_frame);

        /* Create the end basic block */
        CREATE_BASIC_BLOCK(block->basic_block_end);
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
        SET_BB_BEGIN_BCIP(block->basic_block_end, *p_frame_ip);
        if (from_same_block)
            /* Jump to the end basic block */
            BUILD_BR(block->basic_block_end);

        /* Patch the INSNs which jump to this basic block */
        incoming_insn = block->incoming_insns_for_end_bb;
        while (incoming_insn) {
            insn = incoming_insn->insn;
            if (insn->opcode == JIT_OP_JMP) {
                *(jit_insn_opnd(insn, 0)) =
                    jit_basic_block_label(block->basic_block_end);
            }
            else if (insn->opcode == JIT_OP_BNE) {
                *(jit_insn_opnd(insn, 1)) =
                    jit_basic_block_label(block->basic_block_end);
            }
            else {
                bh_assert(0);
            }
            incoming_insn = incoming_insn->next;
        }

        SET_BUILDER_POS(block->basic_block_end);

        /* Pop block and load block results */
        block = jit_block_stack_pop(&cc->block_stack);
        if (!load_block_results(cc, block)) {
            jit_block_destroy(block);
            goto fail;
        }

        if (block->label_type == LABEL_TYPE_FUNCTION) {
            handle_func_return(cc, block);
            SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
        }

        jit_block_destroy(block);
        return true;
    }
    return true;
fail:
    return false;
}

static bool
handle_op_else(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitBlock *block = cc->block_stack.block_list_end;
    JitFrame *jit_frame = cc->jit_frame;
    JitInsn *insn;

    /* Check block */
    if (!block) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }
    if (block->label_type != LABEL_TYPE_IF) {
        jit_set_last_error(cc, "Invalid WASM block type");
        return false;
    }

    if (!block->incoming_insn_for_else_bb) {
        /* The if branch is handled like OP_BLOCK (cond is const and != 0),
           just skip the else branch and handle OP_END */
        *p_frame_ip = block->wasm_code_end + 1;
        return handle_op_end(cc, p_frame_ip, true);
    }
    else {
        /* Has else branch and need to translate else branch */

        /* Commit register values to locals and stacks */
        gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);
        /* Clear frame values */
        clear_values(jit_frame);

        /* Jump to end basic block */
        if (!(insn = GEN_INSN(JMP, 0))) {
            jit_set_last_error(cc, "generate jmp insn failed");
            return false;
        }
        if (!jit_block_add_incoming_insn(block, insn)) {
            jit_set_last_error(cc, "add incoming insn failed");
            return false;
        }

        /* Clear value stack, restore param values and
           start to translate the else branch. */
        jit_value_stack_destroy(&block->value_stack);

        /* Lazily create else basic block */
        CREATE_BASIC_BLOCK(block->basic_block_else);
        SET_BB_END_BCIP(block->basic_block_entry, *p_frame_ip - 1);
        SET_BB_BEGIN_BCIP(block->basic_block_else, *p_frame_ip);

        /* Patch the insn which conditionly jumps to the else basic block */
        insn = block->incoming_insn_for_else_bb;
        *(jit_insn_opnd(insn, 2)) =
            jit_basic_block_label(block->basic_block_else);

        SET_BUILDER_POS(block->basic_block_else);

        /* Reload block parameters */
        if (!load_block_params(cc, block)) {
            return false;
        }

        return true;
    }
    return true;
fail:
    return false;
}

static bool
handle_next_reachable_block(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitBlock *block = cc->block_stack.block_list_end, *block_prev;

    bh_assert(block);

    do {
        block_prev = block->prev;

        if (block->label_type == LABEL_TYPE_IF
            && block->incoming_insn_for_else_bb
            && *p_frame_ip <= block->wasm_code_else) {
            /* Else branch hasn't been translated,
               start to translate the else branch */
            *p_frame_ip = block->wasm_code_else + 1;
            return handle_op_else(cc, p_frame_ip);
        }
        else if (block->incoming_insns_for_end_bb) {
            *p_frame_ip = block->wasm_code_end + 1;
            return handle_op_end(cc, p_frame_ip, false);
        }
        else {
            jit_block_stack_pop(&cc->block_stack);
            jit_block_destroy(block);
            block = block_prev;
        }
    } while (block != NULL);

    return true;
}

bool
jit_compile_op_block(JitCompContext *cc, uint8 **p_frame_ip,
                     uint8 *frame_ip_end, uint32 label_type, uint32 param_count,
                     uint8 *param_types, uint32 result_count,
                     uint8 *result_types)
{
    BlockAddr block_addr_cache[BLOCK_ADDR_CACHE_SIZE][BLOCK_ADDR_CONFLICT_SIZE];
    JitBlock *block;
    JitReg value;
    uint8 *else_addr, *end_addr;

    /* Check block stack */
    if (!cc->block_stack.block_list_end) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }

    memset(block_addr_cache, 0, sizeof(block_addr_cache));

    /* Get block info */
    if (!(wasm_loader_find_block_addr(
            NULL, (BlockAddr *)block_addr_cache, *p_frame_ip, frame_ip_end,
            (uint8)label_type, &else_addr, &end_addr))) {
        jit_set_last_error(cc, "find block end addr failed");
        return false;
    }

    /* Allocate memory */
    if (!(block = jit_calloc(sizeof(JitBlock)))) {
        jit_set_last_error(cc, "allocate memory failed");
        return false;
    }

    if (param_count && !(block->param_types = jit_calloc(param_count))) {
        jit_set_last_error(cc, "allocate memory failed");
        goto fail;
    }
    if (result_count && !(block->result_types = jit_calloc(result_count))) {
        jit_set_last_error(cc, "allocate memory failed");
        goto fail;
    }

    /* Initialize block data */
    block->label_type = label_type;
    block->param_count = param_count;
    if (param_count) {
        bh_memcpy_s(block->param_types, param_count, param_types, param_count);
    }
    block->result_count = result_count;
    if (result_count) {
        bh_memcpy_s(block->result_types, result_count, result_types,
                    result_count);
    }
    block->wasm_code_else = else_addr;
    block->wasm_code_end = end_addr;

    if (label_type == LABEL_TYPE_BLOCK) {
        /* Push the new jit block to block stack and continue to
           translate current basic block */
        if (!push_jit_block_to_stack_and_pass_params(cc, block,
                                                     cc->cur_basic_block, 0))
            goto fail;
    }
    else if (label_type == LABEL_TYPE_LOOP) {
        CREATE_BASIC_BLOCK(block->basic_block_entry);
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
        SET_BB_BEGIN_BCIP(block->basic_block_entry, *p_frame_ip);
        /* Push the new jit block to block stack and continue to
           translate the new basic block */
        if (!push_jit_block_to_stack_and_pass_params(
                cc, block, block->basic_block_entry, 0))
            goto fail;
    }
    else if (label_type == LABEL_TYPE_IF) {
        POP_I32(value);

        if (!jit_reg_is_const_val(value)) {
            /* Compare value is not constant, create condition br IR */

            /* Create entry block */
            CREATE_BASIC_BLOCK(block->basic_block_entry);
            SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
            SET_BB_BEGIN_BCIP(block->basic_block_entry, *p_frame_ip);

            if (!push_jit_block_to_stack_and_pass_params(
                    cc, block, block->basic_block_entry, value))
                goto fail;
        }
        else {
            if (jit_cc_get_const_I32(cc, value) != 0) {
                /* Compare value is not 0, condition is true, else branch of
                   BASIC_BLOCK if cannot be reached, we treat it same as
                   LABEL_TYPE_BLOCK and start to translate if branch */
                if (!push_jit_block_to_stack_and_pass_params(
                        cc, block, cc->cur_basic_block, 0))
                    goto fail;
            }
            else {
                if (else_addr) {
                    /* Compare value is not 0, condition is false, if branch of
                       BASIC_BLOCK if cannot be reached, we treat it same as
                       LABEL_TYPE_BLOCK and start to translate else branch */
                    if (!push_jit_block_to_stack_and_pass_params(
                            cc, block, cc->cur_basic_block, 0))
                        goto fail;
                    *p_frame_ip = else_addr + 1;
                }
                else {
                    /* The whole if block cannot be reached, skip it */
                    jit_block_destroy(block);
                    *p_frame_ip = end_addr + 1;
                }
            }
        }
    }
    else {
        jit_set_last_error(cc, "Invalid block type");
        goto fail;
    }

    return true;
fail:
    jit_block_destroy(block);
    return false;
}

bool
jit_compile_op_else(JitCompContext *cc, uint8 **p_frame_ip)
{
    return handle_op_else(cc, p_frame_ip);
}

bool
jit_compile_op_end(JitCompContext *cc, uint8 **p_frame_ip)
{
    return handle_op_end(cc, p_frame_ip, true);
}

#if 0
#if WASM_ENABLE_THREAD_MGR != 0
bool
check_suspend_flags(JitCompContext *cc, JITFuncContext *func_ctx)
{
    LLVMValueRef terminate_addr, terminate_flags, flag, offset, res;
    JitBasicBlock *terminate_check_block, non_terminate_block;
    JITFuncType *jit_func_type = func_ctx->jit_func->func_type;
    JitBasicBlock *terminate_block;

    /* Offset of suspend_flags */
    offset = I32_FIVE;

    if (!(terminate_addr = LLVMBuildInBoundsGEP(
              cc->builder, func_ctx->exec_env, &offset, 1, "terminate_addr"))) {
        jit_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(terminate_addr =
              LLVMBuildBitCast(cc->builder, terminate_addr, INT32_PTR_TYPE,
                               "terminate_addr_ptr"))) {
        jit_set_last_error("llvm build bit cast failed");
        return false;
    }

    if (!(terminate_flags =
              LLVMBuildLoad(cc->builder, terminate_addr, "terminate_flags"))) {
        jit_set_last_error("llvm build bit cast failed");
        return false;
    }
    /* Set terminate_flags memory accecc to volatile, so that the value
        will always be loaded from memory rather than register */
    LLVMSetVolatile(terminate_flags, true);

    CREATE_BASIC_BLOCK(terminate_check_block, "terminate_check");
    MOVE_BASIC_BLOCK_AFTER_CURR(terminate_check_block);

    CREATE_BASIC_BLOCK(non_terminate_block, "non_terminate");
    MOVE_BASIC_BLOCK_AFTER_CURR(non_terminate_block);

    BUILD_ICMP(LLVMIntSGT, terminate_flags, I32_ZERO, res, "need_terminate");
    BUILD_COND_BR(res, terminate_check_block, non_terminate_block);

    /* Move builder to terminate check block */
    SET_BUILDER_POS(terminate_check_block);

    CREATE_BASIC_BLOCK(terminate_block, "terminate");
    MOVE_BASIC_BLOCK_AFTER_CURR(terminate_block);

    if (!(flag = LLVMBuildAnd(cc->builder, terminate_flags, I32_ONE,
                              "termination_flag"))) {
        jit_set_last_error("llvm build AND failed");
        return false;
    }

    BUILD_ICMP(LLVMIntSGT, flag, I32_ZERO, res, "need_terminate");
    BUILD_COND_BR(res, terminate_block, non_terminate_block);

    /* Move builder to terminate block */
    SET_BUILDER_POS(terminate_block);
    if (!jit_build_zero_function_ret(cc, func_ctx, jit_func_type)) {
        goto fail;
    }

    /* Move builder to terminate block */
    SET_BUILDER_POS(non_terminate_block);
    return true;

fail:
    return false;
}
#endif /* End of WASM_ENABLE_THREAD_MGR */
#endif

bool
jit_compile_op_br(JitCompContext *cc, uint32 br_depth, uint8 **p_frame_ip)
{
    JitFrame *jit_frame;
    JitBlock *block_dst, *block;
    JitReg frame_sp_dst;
    JitValueSlot *frame_sp_src = NULL;
    JitInsn *insn;
    bool copy_arities;
    uint32 offset;

#if 0 /* TODO */
#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (cc->enable_thread_mgr) {
        if (!check_suspend_flags(cc, func_ctx))
            return false;
    }
#endif
#endif

    /* Check block stack */
    if (!(block = cc->block_stack.block_list_end)) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }

    if (!(block_dst = get_target_block(cc, br_depth))) {
        return false;
    }

    jit_frame = cc->jit_frame;

    if (block_dst->label_type == LABEL_TYPE_LOOP) {
        frame_sp_src =
            jit_frame->sp
            - wasm_get_cell_num(block_dst->param_types, block_dst->param_count);
    }
    else {
        frame_sp_src = jit_frame->sp
                       - wasm_get_cell_num(block_dst->result_types,
                                           block_dst->result_count);
    }

    /* Only copy parameters or results when the src/dst addr are different */
    copy_arities = (block_dst->frame_sp_begin != frame_sp_src) ? true : false;

    if (copy_arities) {
#if UINTPTR_MAX == UINT64_MAX
        frame_sp_dst = jit_cc_new_reg_I64(cc);
#else
        frame_sp_dst = jit_cc_new_reg_I32(cc);
#endif
        offset = offsetof(WASMInterpFrame, lp)
                 + (block_dst->frame_sp_begin - jit_frame->lp) * 4;
#if UINTPTR_MAX == UINT64_MAX
        GEN_INSN(ADD, frame_sp_dst, cc->fp_reg, NEW_CONST(I64, offset));
#else
        GEN_INSN(ADD, frame_sp_dst, cc->fp_reg, NEW_CONST(I32, offset));
#endif
    }

    gen_commit_values(jit_frame, jit_frame->lp, block->frame_sp_begin);

    if (block_dst->label_type == LABEL_TYPE_LOOP) {
        if (copy_arities) {
            /* Dest block is Loop block, copy loop parameters */
            copy_block_arities(cc, frame_sp_dst, block_dst->param_types,
                               block_dst->param_count);
        }

        clear_values(jit_frame);

        /* Jump to the begin basic block */
        BUILD_BR(block_dst->basic_block_entry);
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
    }
    else {
        if (copy_arities) {
            /* Dest block is Block/If/Function block, copy block results */
            copy_block_arities(cc, frame_sp_dst, block_dst->result_types,
                               block_dst->result_count);
        }

        clear_values(jit_frame);

        /* Jump to the end basic block */
        if (!(insn = GEN_INSN(JMP, 0))) {
            jit_set_last_error(cc, "generate jmp insn failed");
            goto fail;
        }
        if (!jit_block_add_incoming_insn(block_dst, insn)) {
            jit_set_last_error(cc, "add incoming insn failed");
            goto fail;
        }
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
    }

    return handle_next_reachable_block(cc, p_frame_ip);
fail:
    return false;
}

bool
jit_compile_op_br_if(JitCompContext *cc, uint32 br_depth, uint8 **p_frame_ip)
{
    JitFrame *jit_frame;
    JitBlock *block_dst;
    JitReg frame_sp_dst, cond;
    JitBasicBlock *cur_basic_block, *if_basic_block = NULL;
    JitValueSlot *frame_sp_src = NULL;
    JitInsn *insn;
    bool copy_arities;
    uint32 offset;

#if 0 /* TODO */
#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (cc->enable_thread_mgr) {
        if (!check_suspend_flags(cc, func_ctx))
            return false;
    }
#endif
#endif

    if (!(block_dst = get_target_block(cc, br_depth))) {
        return false;
    }

    POP_I32(cond);

    jit_frame = cc->jit_frame;
    cur_basic_block = cc->cur_basic_block;
    gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);
    clear_values(jit_frame);

    CREATE_BASIC_BLOCK(if_basic_block);
    if (!GEN_INSN(CMP, cc->cmp_reg, cond, NEW_CONST(I32, 0))
        || !GEN_INSN(BNE, cc->cmp_reg, jit_basic_block_label(if_basic_block),
                     0)) {
        jit_set_last_error(cc, "generate bne insn failed");
        goto fail;
    }

    SET_BUILDER_POS(if_basic_block);
    SET_BB_BEGIN_BCIP(if_basic_block, *p_frame_ip - 1);

    if (block_dst->label_type == LABEL_TYPE_LOOP) {
        frame_sp_src =
            jit_frame->sp
            - wasm_get_cell_num(block_dst->param_types, block_dst->param_count);
    }
    else {
        frame_sp_src = jit_frame->sp
                       - wasm_get_cell_num(block_dst->result_types,
                                           block_dst->result_count);
    }

    /* Only copy parameters or results when the src/dst addr are different */
    copy_arities = (block_dst->frame_sp_begin != frame_sp_src) ? true : false;

    if (copy_arities) {
#if UINTPTR_MAX == UINT64_MAX
        frame_sp_dst = jit_cc_new_reg_I64(cc);
#else
        frame_sp_dst = jit_cc_new_reg_I32(cc);
#endif
        offset = offsetof(WASMInterpFrame, lp)
                 + (block_dst->frame_sp_begin - jit_frame->lp) * 4;
#if UINTPTR_MAX == UINT64_MAX
        GEN_INSN(ADD, frame_sp_dst, cc->fp_reg, NEW_CONST(I64, offset));
#else
        GEN_INSN(ADD, frame_sp_dst, cc->fp_reg, NEW_CONST(I32, offset));
#endif
    }

    if (block_dst->label_type == LABEL_TYPE_LOOP) {
        if (copy_arities) {
            /* Dest block is Loop block, copy loop parameters */
            copy_block_arities(cc, frame_sp_dst, block_dst->param_types,
                               block_dst->param_count);
        }
        /* Jump to the begin basic block */
        BUILD_BR(block_dst->basic_block_entry);
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
    }
    else {
        if (copy_arities) {
            /* Dest block is Block/If/Function block, copy block results */
            copy_block_arities(cc, frame_sp_dst, block_dst->result_types,
                               block_dst->result_count);
        }
        /* Jump to the end basic block */
        if (!(insn = GEN_INSN(JMP, 0))) {
            jit_set_last_error(cc, "generate jmp insn failed");
            goto fail;
        }
        if (!jit_block_add_incoming_insn(block_dst, insn)) {
            jit_set_last_error(cc, "add incoming insn failed");
            goto fail;
        }
        SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);
    }

    SET_BUILDER_POS(cur_basic_block);

    return true;
fail:
    return false;
}

bool
jit_compile_op_br_table(JitCompContext *cc, uint32 *br_depths, uint32 br_count,
                        uint8 **p_frame_ip)
{
    return false;
#if 0
    uint32 i, j;
    LLVMValueRef value_switch, value_cmp, value_case, value, *values = NULL;
    JitBasicBlock *default_basic_block = NULL, target_basic_block;
    JitBasicBlock *next_basic_block_end;
    JitBlock *target_block;
    uint32 br_depth, depth_idx;
    uint32 param_index, result_index;
    uint64 size;
    char name[32];

#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (cc->enable_thread_mgr) {
        if (!check_suspend_flags(cc, func_ctx))
            return false;
    }
#endif

    POP_I32(value_cmp);

    if (LLVMIsUndef(value_cmp)
#if LLVM_VERSION_NUMBER >= 12
        || LLVMIsPoison(value_cmp)
#endif
    ) {
        if (!(jit_emit_exception(cc, func_ctx, EXCE_INTEGER_OVERFLOW, false,
                                 NULL, NULL))) {
            goto fail;
        }
        return jit_handle_next_reachable_block(cc, func_ctx, p_frame_ip);
    }

    if (!LLVMIsConstant(value_cmp)) {
        /* Compare value is not constant, create switch IR */
        for (i = 0; i <= br_count; i++) {
            target_block = get_target_block(func_ctx, br_depths[i]);
            if (!target_block)
                return false;

            if (target_block->label_type != LABEL_TYPE_LOOP) {
                /* Dest block is Block/If/Function block */
                /* Create the end block */
                if (!target_block->basic_block_end) {
                    format_block_name(name, sizeof(name),
                                      target_block->block_index,
                                      target_block->label_type, LABEL_END);
                    CREATE_BASIC_BLOCK(target_block->basic_block_end, name);
                    if ((next_basic_block_end =
                             find_next_basic_block_end(target_block)))
                        MOVE_BASIC_BLOCK_BEFORE(target_block->basic_block_end,
                                                next_basic_block_end);
                }
                /* Handle result values */
                if (target_block->result_count) {
                    size = sizeof(LLVMValueRef)
                           * (uint64)target_block->result_count;
                    if (size >= UINT32_MAX
                        || !(values = jit_calloc((uint32)size))) {
                        jit_set_last_error(cc, "allocate memory failed");
                        goto fail;
                    }
                    CREATE_RESULT_VALUE_PHIS(target_block);
                    for (j = 0; j < target_block->result_count; j++) {
                        result_index = target_block->result_count - 1 - j;
                        POP(value, target_block->result_types[result_index]);
                        values[result_index] = value;
                        ADD_TO_RESULT_PHIS(target_block, value, result_index);
                    }
                    for (j = 0; j < target_block->result_count; j++) {
                        PUSH(values[j], target_block->result_types[j]);
                    }
                    jit_free(values);
                }
                target_block->is_reachable = true;
                if (i == br_count)
                    default_basic_block = target_block->basic_block_end;
            }
            else {
                /* Handle Loop parameters */
                if (target_block->param_count) {
                    size = sizeof(LLVMValueRef)
                           * (uint64)target_block->param_count;
                    if (size >= UINT32_MAX
                        || !(values = jit_calloc((uint32)size))) {
                        jit_set_last_error(cc, "allocate memory failed");
                        goto fail;
                    }
                    for (j = 0; j < target_block->param_count; j++) {
                        param_index = target_block->param_count - 1 - j;
                        POP(value, target_block->param_types[param_index]);
                        values[param_index] = value;
                        ADD_TO_PARAM_PHIS(target_block, value, param_index);
                    }
                    for (j = 0; j < target_block->param_count; j++) {
                        PUSH(values[j], target_block->param_types[j]);
                    }
                    jit_free(values);
                }
                if (i == br_count)
                    default_basic_block = target_block->basic_block_entry;
            }
        }

        /* Create switch IR */
        if (!(value_switch = LLVMBuildSwitch(cc->builder, value_cmp,
                                             default_basic_block, br_count))) {
            jit_set_last_error(cc, "llvm build switch failed");
            return false;
        }

        /* Add each case for switch IR */
        for (i = 0; i < br_count; i++) {
            value_case = I32_CONST(i);
            CHECK_LLVM_CONST(value_case);
            target_block = get_target_block(func_ctx, br_depths[i]);
            if (!target_block)
                return false;
            target_basic_block = target_block->label_type != LABEL_TYPE_LOOP
                                     ? target_block->basic_block_end
                                     : target_block->basic_block_entry;
            LLVMAddCase(value_switch, value_case, target_basic_block);
        }

        return handle_next_reachable_block(cc, func_ctx, p_frame_ip);
    }
    else {
        /* Compare value is constant, create br IR */
        depth_idx = (uint32)LLVMConstIntGetZExtValue(value_cmp);
        br_depth = br_depths[br_count];
        if (depth_idx < br_count) {
            br_depth = br_depths[depth_idx];
        }
        return jit_compile_op_br(cc, func_ctx, br_depth, p_frame_ip);
    }
fail:
    if (values)
        jit_free(values);
    return false;
#endif
}

bool
jit_compile_op_return(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitBlock *block_func = cc->block_stack.block_list_head;

    bh_assert(block_func);

    handle_func_return(cc, block_func);
    SET_BB_END_BCIP(cc->cur_basic_block, *p_frame_ip - 1);

    return handle_next_reachable_block(cc, p_frame_ip);
}

bool
jit_compile_op_unreachable(JitCompContext *cc, uint8 **p_frame_ip)
{
    if (!jit_emit_exception(cc, EXCE_UNREACHABLE, false, 0, NULL))
        return false;

    return handle_next_reachable_block(cc, p_frame_ip);
}

bool
jit_handle_next_reachable_block(JitCompContext *cc, uint8 **p_frame_ip)
{
    return handle_next_reachable_block(cc, p_frame_ip);
}
