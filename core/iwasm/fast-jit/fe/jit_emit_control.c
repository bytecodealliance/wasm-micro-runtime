/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_control.h"
#include "jit_emit_exception.h"
#include "../jit_frontend.h"
#include "../interpreter/wasm_loader.h"

/* clang-format off */
enum {
    LABEL_BEGIN = 0,
    LABEL_ELSE,
    LABEL_END
};
/* clang-format on */

#define CREATE_BASIC_BLOCK(new_basic_block)                       \
    do {                                                          \
        if (!(new_basic_block = jit_cc_new_basic_block(cc, 0))) { \
            jit_set_last_error(cc, "add basic block failed");     \
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
        if (!GEN_INSN(CMP, cc->cmp_reg, value_if, NEW_CONST(I32, 0))          \
            || !GEN_INSN(BNE, cc->cmp_reg, jit_basic_block_label(block_then), \
                         jit_basic_block_label(block_else))) {                \
            jit_set_last_error(cc, "generate cond br failed");                \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define SET_BUILDER_POS(basic_block)       \
    do {                                   \
        cc->cur_basic_block = basic_block; \
    } while (0)

#define BUILD_ICMP(left, right, res)                            \
    do {                                                        \
        if (!(GEN_INSN(CMP, res, left, right))) {               \
            jit_set_last_error(cc, "generate cmp insn failed"); \
            goto fail;                                          \
        }                                                       \
    } while (0)

static JitBasicBlock *
find_next_basic_block_end(JitBlock *block)
{
    block = block->prev;
    while (block && !block->basic_block_end)
        block = block->prev;
    return block ? block->basic_block_end : NULL;
}

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
    JitReg value;

    /* Restore jit frame's sp to block's sp begin */
    jit_frame->sp = block->frame_sp_begin;
    offset = (uint32)(jit_frame->sp - jit_frame->lp) * 4;

    /* Push params to the new block */
    for (i = 0; i < block->param_count; i++) {
        switch (block->param_types[i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = gen_load_i32(jit_frame, offset);
                offset += 4;
                break;
            case VALUE_TYPE_I64:
                value = gen_load_i64(jit_frame, offset);
                offset += 8;
                break;
            case VALUE_TYPE_F32:
                value = gen_load_f32(jit_frame, offset);
                offset += 4;
                break;
            case VALUE_TYPE_F64:
                value = gen_load_f64(jit_frame, offset);
                offset += 8;
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
    JitReg value;

    /* Restore jit frame's sp to block's current sp */
    jit_frame->sp = block->frame_sp_begin;
    offset = (uint32)(jit_frame->sp - jit_frame->lp) * 4;

    /* Push block results to the previous block */
    for (i = 0; i < block->result_count; i++) {
        switch (block->result_types[i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = gen_load_i32(jit_frame, offset);
                offset += 4;
                break;
            case VALUE_TYPE_I64:
                value = gen_load_i64(jit_frame, offset);
                offset += 8;
                break;
            case VALUE_TYPE_F32:
                value = gen_load_f32(jit_frame, offset);
                offset += 4;
                break;
            case VALUE_TYPE_F64:
                value = gen_load_f64(jit_frame, offset);
                offset += 8;
                break;
        }
        PUSH(value, block->result_types[i]);
    }

    return true;
fail:
    return false;
}

static bool
handle_next_reachable_block(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitBlock *block = cc->block_stack.block_list_end, *block_prev;
    uint8 *frame_ip = NULL;

    bh_assert(block);

    if (block->label_type == LABEL_TYPE_IF && block->basic_block_else
        && *p_frame_ip <= block->wasm_code_else) {
        /* Clear value stack and start to translate else branch */
        jit_value_stack_destroy(&block->value_stack);
        SET_BUILDER_POS(block->basic_block_else);
        /* Push the block parameters */
        if (!load_block_params(cc, block)) {
            goto fail;
        }
        *p_frame_ip = block->wasm_code_else + 1;
        return true;
    }

    while (block && !block->is_reachable) {
        block_prev = block->prev;
        block = jit_block_stack_pop(&cc->block_stack);

        if (block->label_type == LABEL_TYPE_IF) {
            if (block->basic_block_else && !block->skip_wasm_code_else
                && *p_frame_ip <= block->wasm_code_else) {
                /* Clear value stack and start to translate else branch */
                jit_value_stack_destroy(&block->value_stack);
                SET_BUILDER_POS(block->basic_block_else);
                /* Push the block parameters */
                if (!load_block_params(cc, block)) {
                    goto fail;
                }
                *p_frame_ip = block->wasm_code_else + 1;
                /* Push back the block */
                jit_block_stack_push(&cc->block_stack, block);
                return true;
            }
            else if (block->basic_block_end) {
                /* Remove unreachable basic block */
                jit_basic_block_delete(block->basic_block_end);
                block->basic_block_end = NULL;
            }
        }

        frame_ip = block->wasm_code_end;
        jit_block_destroy(block);
        block = block_prev;
    }

    if (!block) {
        *p_frame_ip = frame_ip + 1;
        return true;
    }

    *p_frame_ip = block->wasm_code_end + 1;
    SET_BUILDER_POS(block->basic_block_end);

    /* Pop block, push its return value, and destroy the block */
    block = jit_block_stack_pop(&cc->block_stack);
    if (block->label_type != LABEL_TYPE_FUNCTION
        && !load_block_results(cc, block)) {
        goto fail;
    }

    if (block->label_type == LABEL_TYPE_FUNCTION) {
        GEN_INSN(RETURN, NEW_CONST(I32, 0));
    }
    jit_block_destroy(block);
    return true;
fail:
    return false;
}

static bool
push_jit_block_to_stack_and_pass_params(JitCompContext *cc, JitBlock *block,
                                        JitBasicBlock *basic_block_to_translate)
{
    JitFrame *jit_frame = cc->jit_frame;
    JitValueSlot *frame_sp;
    JitValue *value_list = NULL, *value_list_end = NULL, *jit_value;
    JitReg value;
    uint32 i, param_index, cell_num;

    if (block->label_type != LABEL_TYPE_BLOCK) {
        gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);
        /* Pop param values from current block's value stack */
        for (i = 0; i < block->param_count; i++) {
            param_index = block->param_count - 1 - i;
            POP(value, block->param_types[param_index]);
        }

        frame_sp = jit_frame->sp;
        /* Set block's begin frame sp */
        block->frame_sp_begin = frame_sp;

        /* Push the new block to block stack */
        jit_block_stack_push(&cc->block_stack, block);

        /* Start to translate the block */
        SET_BUILDER_POS(basic_block_to_translate);

        if (!load_block_params(cc, block))
            return false;
    }
    else {
        cell_num = wasm_get_cell_num(block->param_types, block->param_count);
        frame_sp = jit_frame->sp - cell_num;
        /* Set block's begin frame sp */
        block->frame_sp_begin = frame_sp;

        /* Move values from last block's value stack to
           new block's value stack */
        for (i = 0; i < block->param_count; i++) {
            param_index = block->param_count - 1 - i;
            jit_value = jit_value_stack_pop(
                &cc->block_stack.block_list_end->value_stack);
            if (!value_list) {
                value_list = value_list_end = jit_value;
                jit_value->prev = jit_value->next = NULL;
            }
            else {
                value_list_end->next = jit_value;
                jit_value->prev = value_list_end;
                jit_value->next = NULL;
            }
        }
        block->value_stack.value_list_head = value_list;
        block->value_stack.value_list_end = value_list;

        /* Push the new block to block stack */
        jit_block_stack_push(&cc->block_stack, block);

        /* Start to translate the block */
        SET_BUILDER_POS(basic_block_to_translate);
    }

    return true;
fail:
    return false;
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

    /* Init jit block data */
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
        /* Push the new block to block stack and continue to
           translate current block */
        if (!push_jit_block_to_stack_and_pass_params(cc, block,
                                                     cc->cur_basic_block))
            goto fail;
    }
    else if (label_type == LABEL_TYPE_LOOP) {
        CREATE_BASIC_BLOCK(block->basic_block_entry);
        /* Jump to the entry block */
        BUILD_BR(block->basic_block_entry);
        if (!push_jit_block_to_stack_and_pass_params(cc, block,
                                                     block->basic_block_entry))
            goto fail;
    }
    else if (label_type == LABEL_TYPE_IF) {
        POP_I32(value);

        if (!jit_reg_is_const_val(value)) {
            /* Compare value is not constant, create condition br IR */

            /* Create entry block */
            CREATE_BASIC_BLOCK(block->basic_block_entry);

            if (else_addr) {
                /* Create else block */
                CREATE_BASIC_BLOCK(block->basic_block_else);
                /* Create condition br IR */
                BUILD_COND_BR(value, block->basic_block_entry,
                              block->basic_block_else);
            }
            else {
                /* Create end block */
                CREATE_BASIC_BLOCK(block->basic_block_end);
                /* Create condition br IR */
                BUILD_COND_BR(value, block->basic_block_entry,
                              block->basic_block_end);
                block->is_reachable = true;
            }
            if (!push_jit_block_to_stack_and_pass_params(
                    cc, block, block->basic_block_entry))
                goto fail;
        }
        else {
            if (jit_cc_get_const_I32(cc, value) != 0) {
                /* Compare value is not 0, condition is true, else branch of
                   BASIC_BLOCK if cannot be reached */
                block->skip_wasm_code_else = true;
                /* Create entry block */
                CREATE_BASIC_BLOCK(block->basic_block_entry);
                /* Jump to the entry block */
                BUILD_BR(block->basic_block_entry);
                if (!push_jit_block_to_stack_and_pass_params(
                        cc, block, block->basic_block_entry))
                    goto fail;
            }
            else {
                /* Compare value is not 0, condition is false, if branch of
                   BASIC_BLOCK if cannot be reached */
                if (else_addr) {
                    /* Create else block */
                    CREATE_BASIC_BLOCK(block->basic_block_else);
                    /* Jump to the else block */
                    BUILD_BR(block->basic_block_else);
                    if (!push_jit_block_to_stack_and_pass_params(
                            cc, block, block->basic_block_else))
                        goto fail;
                    *p_frame_ip = else_addr + 1;
                }
                else {
                    /* skip the block */
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
    JitBlock *block = cc->block_stack.block_list_end;
    JitFrame *jit_frame = cc->jit_frame;

    /* Check block */
    if (!block) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }
    if (block->label_type != LABEL_TYPE_IF
        || (!block->skip_wasm_code_else && !block->basic_block_else)) {
        jit_set_last_error(cc, "Invalid WASM block type");
        return false;
    }

    /* Commit register values to stack and local */
    gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);

    /* Create end block if needed */
    if (!block->basic_block_end) {
        CREATE_BASIC_BLOCK(block->basic_block_end);
    }

    block->is_reachable = true;

    /* Jump to end block */
    BUILD_BR(block->basic_block_end);

    if (!block->skip_wasm_code_else && block->basic_block_else) {
        /* Clear value stack, recover param values and
           start to translate else branch. */
        jit_value_stack_destroy(&block->value_stack);
        SET_BUILDER_POS(block->basic_block_else);
        if (!load_block_params(cc, block))
            goto fail;
        return true;
    }

    /* No else branch or no need to translate else branch */
    block->is_reachable = true;
    return handle_next_reachable_block(cc, p_frame_ip);
fail:
    return false;
}

bool
jit_compile_op_end(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitFrame *jit_frame = cc->jit_frame;
    JitBlock *block;

    /* Check block stack */
    if (!(block = cc->block_stack.block_list_end)) {
        jit_set_last_error(cc, "WASM block stack underflow");
        return false;
    }

    /* Create the end block */
    if (!block->basic_block_end) {
        CREATE_BASIC_BLOCK(block->basic_block_end);
    }

    gen_commit_values(jit_frame, jit_frame->lp, jit_frame->sp);

    /* Jump to the end block */
    BUILD_BR(block->basic_block_end);

    block->is_reachable = true;
    return handle_next_reachable_block(cc, p_frame_ip);
fail:
    return false;
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
    return false;
#if 0
    JitBlock *block_dst;
    LLVMValueRef value_ret, value_param;
    JitBasicBlock *next_basic_block_end;
    char name[32];
    uint32 i, param_index, result_index;

#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (cc->enable_thread_mgr) {
        if (!check_suspend_flags(cc, func_ctx))
            return false;
    }
#endif

    if (!(block_dst = get_target_block(func_ctx, br_depth))) {
        return false;
    }

    if (block_dst->label_type == LABEL_TYPE_LOOP) {
        /* Dest block is Loop block */
        /* Handle Loop parameters */
        for (i = 0; i < block_dst->param_count; i++) {
            param_index = block_dst->param_count - 1 - i;
            POP(value_param, block_dst->param_types[param_index]);
            ADD_TO_PARAM_PHIS(block_dst, value_param, param_index);
        }
        BUILD_BR(block_dst->basic_block_entry);
    }
    else {
        /* Dest block is Block/If/Function block */
        /* Create the end block */
        if (!block_dst->basic_block_end) {
            format_block_name(name, sizeof(name), block_dst->block_index,
                              block_dst->label_type, LABEL_END);
            CREATE_BASIC_BLOCK(block_dst->basic_block_end, name);
            if ((next_basic_block_end = find_next_basic_block_end(block_dst)))
                MOVE_BASIC_BLOCK_BEFORE(block_dst->basic_block_end,
                                        next_basic_block_end);
        }

        block_dst->is_reachable = true;

        /* Handle result values */
        CREATE_RESULT_VALUE_PHIS(block_dst);
        for (i = 0; i < block_dst->result_count; i++) {
            result_index = block_dst->result_count - 1 - i;
            POP(value_ret, block_dst->result_types[result_index]);
            ADD_TO_RESULT_PHIS(block_dst, value_ret, result_index);
        }
        /* Jump to the end block */
        BUILD_BR(block_dst->basic_block_end);
    }

    return handle_next_reachable_block(cc, func_ctx, p_frame_ip);
fail:
    return false;
#endif
}

bool
jit_compile_op_br_if(JitCompContext *cc, uint32 br_depth, uint8 **p_frame_ip)
{
    return false;
#if 0
    JitBlock *block_dst;
    LLVMValueRef value_cmp, value, *values = NULL;
    JitBasicBlock *basic_block_else, next_basic_block_end;
    char name[32];
    uint32 i, param_index, result_index;
    uint64 size;

#if WASM_ENABLE_THREAD_MGR != 0
    /* Insert suspend check point */
    if (cc->enable_thread_mgr) {
        if (!check_suspend_flags(cc, func_ctx))
            return false;
    }
#endif

    POP_COND(value_cmp);

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
        /* Compare value is not constant, create condition br IR */
        if (!(block_dst = get_target_block(func_ctx, br_depth))) {
            return false;
        }

        /* Create llvm else block */
        CREATE_BASIC_BLOCK(basic_block_else, "br_if_else");
        MOVE_BASIC_BLOCK_AFTER_CURR(basic_block_else);

        if (block_dst->label_type == LABEL_TYPE_LOOP) {
            /* Dest block is Loop block */
            /* Handle Loop parameters */
            if (block_dst->param_count) {
                size = sizeof(LLVMValueRef) * (uint64)block_dst->param_count;
                if (size >= UINT32_MAX
                    || !(values = jit_calloc((uint32)size))) {
                    jit_set_last_error(cc, "allocate memory failed");
                    goto fail;
                }
                for (i = 0; i < block_dst->param_count; i++) {
                    param_index = block_dst->param_count - 1 - i;
                    POP(value, block_dst->param_types[param_index]);
                    ADD_TO_PARAM_PHIS(block_dst, value, param_index);
                    values[param_index] = value;
                }
                for (i = 0; i < block_dst->param_count; i++) {
                    PUSH(values[i], block_dst->param_types[i]);
                }
                jit_free(values);
                values = NULL;
            }

            BUILD_COND_BR(value_cmp, block_dst->basic_block_entry,
                          basic_block_else);

            /* Move builder to else block */
            SET_BUILDER_POS(basic_block_else);
        }
        else {
            /* Dest block is Block/If/Function block */
            /* Create the end block */
            if (!block_dst->basic_block_end) {
                format_block_name(name, sizeof(name), block_dst->block_index,
                                  block_dst->label_type, LABEL_END);
                CREATE_BASIC_BLOCK(block_dst->basic_block_end, name);
                if ((next_basic_block_end = find_next_basic_block_end(block_dst)))
                    MOVE_BASIC_BLOCK_BEFORE(block_dst->basic_block_end,
                                            next_basic_block_end);
            }

            /* Set reachable flag and create condition br IR */
            block_dst->is_reachable = true;

            /* Handle result values */
            if (block_dst->result_count) {
                size = sizeof(LLVMValueRef) * (uint64)block_dst->result_count;
                if (size >= UINT32_MAX
                    || !(values = jit_calloc((uint32)size))) {
                    jit_set_last_error(cc, "allocate memory failed");
                    goto fail;
                }
                CREATE_RESULT_VALUE_PHIS(block_dst);
                for (i = 0; i < block_dst->result_count; i++) {
                    result_index = block_dst->result_count - 1 - i;
                    POP(value, block_dst->result_types[result_index]);
                    values[result_index] = value;
                    ADD_TO_RESULT_PHIS(block_dst, value, result_index);
                }
                for (i = 0; i < block_dst->result_count; i++) {
                    PUSH(values[i], block_dst->result_types[i]);
                }
                jit_free(values);
                values = NULL;
            }

            /* Condition jump to end block */
            BUILD_COND_BR(value_cmp, block_dst->basic_block_end,
                          basic_block_else);

            /* Move builder to else block */
            SET_BUILDER_POS(basic_block_else);
        }
    }
    else {
        if ((int32)LLVMConstIntGetZExtValue(value_cmp) != 0) {
            /* Compare value is not 0, condition is true, same as op_br */
            return jit_compile_op_br(cc, func_ctx, br_depth, p_frame_ip);
        }
        else {
            /* Compare value is not 0, condition is false, skip br_if */
            return true;
        }
    }
    return true;
fail:
    if (values)
        jit_free(values);
    return false;
#endif
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
    return false;
#if 0
    JitBlock *block_func = func_ctx->block_stack.block_list_head;
    LLVMValueRef value;
    LLVMValueRef ret;
    JITFuncType *func_type;
    uint32 i, param_index, result_index;
#if WASM_ENABLE_DEBUG_JIT != 0
    LLVMMetadataRef return_location;
#endif

    bh_assert(block_func);
    func_type = func_ctx->jit_func->func_type;

#if WASM_ENABLE_DEBUG_JIT != 0
    return_location = dwarf_gen_location(
        cc, func_ctx, (*p_frame_ip - 1) - cc->comp_data->wasm_module->buf_code);
#endif
    if (block_func->result_count) {
        /* Store extra result values to function parameters */
        for (i = 0; i < block_func->result_count - 1; i++) {
            result_index = block_func->result_count - 1 - i;
            POP(value, block_func->result_types[result_index]);
            param_index = func_type->param_count + result_index;
            if (!LLVMBuildStore(cc->builder, value,
                                LLVMGetParam(func_ctx->func, param_index))) {
                jit_set_last_error(cc, "llvm build store failed");
                goto fail;
            }
        }
        /* Return the first result value */
        POP(value, block_func->result_types[0]);
        if (!(ret = LLVMBuildRet(cc->builder, value))) {
            jit_set_last_error(cc, "llvm build return failed");
            goto fail;
        }
#if WASM_ENABLE_DEBUG_JIT != 0
        LLVMInstructionSetDebugLoc(ret, return_location);
#endif
    }
    else {
        if (!(ret = LLVMBuildRetVoid(cc->builder))) {
            jit_set_last_error(cc, "llvm build return void failed");
            goto fail;
        }
#if WASM_ENABLE_DEBUG_JIT != 0
        LLVMInstructionSetDebugLoc(ret, return_location);
#endif
    }

    return handle_next_reachable_block(cc, func_ctx, p_frame_ip);
fail:
    return false;
#endif
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
