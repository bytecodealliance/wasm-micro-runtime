/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_control.h"
//#include "jit_emit_exception.h"
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
            jit_set_last_error(cc, "add basic block failed.");    \
            goto fail;                                            \
        }                                                         \
    } while (0)

#define CURR_BASIC_BLOCK() cc->cur_basic_block

#define BUILD_BR(target_block)                                     \
    do {                                                           \
        if (!GEN_INSN(JMP, jit_basic_block_label(target_block))) { \
            jit_set_last_error(cc, "generate jmp insn failed.");   \
            goto fail;                                             \
        }                                                          \
    } while (0)

#define BUILD_COND_BR(value_if, block_then, block_else)                       \
    do {                                                                      \
        if (!GEN_INSN(CMP, cc->cmp_reg, value_if, NEW_CONST(I32, 0))          \
            || !GEN_INSN(BNE, cc->cmp_reg, jit_basic_block_label(block_then), \
                         jit_basic_block_label(block_else))) {                \
            jit_set_last_error(cc, "generate cond br failed.");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define SET_BUILDER_POS(basic_block)       \
    do {                                   \
        cc->cur_basic_block = basic_block; \
    } while (0)

#if 0
#define CREATE_RESULT_VALUE_PHIS(block)                                       \
    do {                                                                      \
        if (block->result_count && !block->result_phis) {                     \
            uint32 _i;                                                        \
            uint64 _size;                                                     \
            JitBasicBlock *_block_curr = CURR_BASIC_BLOCK();                  \
            /* Allocate memory */                                             \
            _size = sizeof(LLVMValueRef) * (uint64)block->result_count;       \
            if (_size >= UINT32_MAX                                           \
                || !(block->result_phis =                                     \
                         wasm_runtime_malloc((uint32)_size))) {               \
                jit_set_last_error("allocate memory failed.");                \
                goto fail;                                                    \
            }                                                                 \
            SET_BUILDER_POS(block->basic_block_end);                          \
            for (_i = 0; _i < block->result_count; _i++) {                    \
                if (!(block->result_phis[_i] = LLVMBuildPhi(                  \
                          cc->builder, TO_LLVM_TYPE(block->result_types[_i]), \
                          "phi"))) {                                          \
                    jit_set_last_error("llvm build phi failed.");             \
                    goto fail;                                                \
                }                                                             \
            }                                                                 \
            SET_BUILDER_POS(_block_curr);                                     \
        }                                                                     \
    } while (0)

#define ADD_TO_RESULT_PHIS(block, value, idx)                                  \
    do {                                                                       \
        JitBasicBlock *_block_curr = CURR_BASIC_BLOCK();                       \
        LLVMTypeRef phi_ty = LLVMTypeOf(block->result_phis[idx]);              \
        LLVMTypeRef value_ty = LLVMTypeOf(value);                              \
        bh_assert(LLVMGetTypeKind(phi_ty) == LLVMGetTypeKind(value_ty));       \
        bh_assert(LLVMGetTypeContext(phi_ty) == LLVMGetTypeContext(value_ty)); \
        LLVMAddIncoming(block->result_phis[idx], &value, &_block_curr, 1);     \
        (void)phi_ty;                                                          \
        (void)value_ty;                                                        \
    } while (0)
#endif

#define BUILD_ICMP(left, right, res)                             \
    do {                                                         \
        if (!(GEN_INSN(CMP, res, left, right))) {                \
            jit_set_last_error(cc, "generate cmp insn failed."); \
            goto fail;                                           \
        }                                                        \
    } while (0)

#if 0
#define ADD_TO_PARAM_PHIS(block, value, idx)                              \
    do {                                                                  \
        JitBasicBlock *_block_curr = CURR_BASIC_BLOCK();                  \
        LLVMAddIncoming(block->param_phis[idx], &value, &_block_curr, 1); \
    } while (0)
#endif

static JitBasicBlock *
find_next_jit_end_block(JitBlock *block)
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
        jit_set_last_error(cc, "WASM block stack underflow.");
        return NULL;
    }
    return block;
}

static bool
handle_next_reachable_block(JitCompContext *cc, uint8 **p_frame_ip)
{
    JitBlock *block = cc->block_stack.block_list_end;
    JitBlock *block_prev;
    uint8 *frame_ip = NULL;
    uint32 i;
    WASMType *func_type;
    JitReg ret;

    bh_assert(block);

    if (block->label_type == LABEL_TYPE_IF && block->basic_block_else
        && *p_frame_ip <= block->wasm_code_else) {
        /* Clear value stack and start to translate else branch */
        jit_value_stack_destroy(&block->value_stack);
#if 0
        /* Recover parameters of else branch */
        for (i = 0; i < block->param_count; i++)
            PUSH(block->else_param_phis[i], block->param_types[i]);
#endif
        SET_BUILDER_POS(block->basic_block_else);
        cc->cur_basic_block = block->basic_block_else;
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
#if 0
    func_type = cc->cur_wasm_func->func_type;
    for (i = 0; i < block->result_count; i++) {
        bh_assert(block->result_phis[i]);
        if (block->label_type != LABEL_TYPE_FUNCTION) {
            PUSH(block->result_phis[i], block->result_types[i]);
        }
        else {
            /* Store extra return values to function parameters */
            if (i != 0) {
                uint32 param_index = func_type->param_count + i;
                if (!LLVMBuildStore(
                        cc->builder, block->result_phis[i],
                        LLVMGetParam(func_ctx->func, param_index))) {
                    jit_set_last_error("llvm build store failed.");
                    goto fail;
                }
            }
        }
    }
#endif
#if 0
    if (block->label_type == LABEL_TYPE_FUNCTION) {
        if (block->result_count) {
            /* Return the first return value */
            if (!(ret = LLVMBuildRet(cc->builder, block->result_phis[0]))) {
                jit_set_last_error("llvm build return failed.");
                goto fail;
            }
        }
        else {
            if (!(ret = LLVMBuildRetVoid(cc->builder))) {
                jit_set_last_error("llvm build return void failed.");
                goto fail;
            }
        }
    }
#endif
    jit_block_destroy(block);
    return true;
fail:
    return false;
}

static bool
push_jit_block_to_stack_and_pass_params(JitCompContext *cc, JitBlock *block)
{
    uint32 i, param_index;
    JitReg value;
    uint64 size;
    JitBasicBlock *block_curr = CURR_BASIC_BLOCK();

#if 0
    if (block->param_count) {
        size = sizeof(LLVMValueRef) * (uint64)block->param_count;
        if (size >= UINT32_MAX
            || !(block->param_phis = wasm_runtime_malloc((uint32)size))) {
            jit_set_last_error("allocate memory failed.");
            return false;
        }

        if (block->label_type == LABEL_TYPE_IF && !block->skip_wasm_code_else
            && !(block->else_param_phis = wasm_runtime_malloc((uint32)size))) {
            wasm_runtime_free(block->param_phis);
            block->param_phis = NULL;
            jit_set_last_error("allocate memory failed.");
            return false;
        }

        /* Create param phis */
        for (i = 0; i < block->param_count; i++) {
            SET_BUILDER_POS(block->basic_block_entry);
            snprintf(name, sizeof(name), "%s%d_phi%d",
                     block_name_prefix[block->label_type], block->block_index,
                     i);
            if (!(block->param_phis[i] = LLVMBuildPhi(
                      cc->builder, TO_LLVM_TYPE(block->param_types[i]),
                      name))) {
                jit_set_last_error("llvm build phi failed.");
                goto fail;
            }

            if (block->label_type == LABEL_TYPE_IF
                && !block->skip_wasm_code_else && block->basic_block_else) {
                /* Build else param phis */
                SET_BUILDER_POS(block->basic_block_else);
                snprintf(name, sizeof(name), "else%d_phi%d", block->block_index,
                         i);
                if (!(block->else_param_phis[i] = LLVMBuildPhi(
                          cc->builder, TO_LLVM_TYPE(block->param_types[i]),
                          name))) {
                    jit_set_last_error("llvm build phi failed.");
                    goto fail;
                }
            }
        }
        SET_BUILDER_POS(block_curr);

        /* Pop param values from current block's
         * value stack and add to param phis.
         */
        for (i = 0; i < block->param_count; i++) {
            param_index = block->param_count - 1 - i;
            POP(value, block->param_types[param_index]);
            ADD_TO_PARAM_PHIS(block, value, param_index);
            if (block->label_type == LABEL_TYPE_IF
                && !block->skip_wasm_code_else) {
                if (block->basic_block_else) {
                    /* has else branch, add to else param phis */
                    LLVMAddIncoming(block->else_param_phis[param_index], &value,
                                    &block_curr, 1);
                }
                else {
                    /* no else branch, add to result phis */
                    CREATE_RESULT_VALUE_PHIS(block);
                    ADD_TO_RESULT_PHIS(block, value, param_index);
                }
            }
        }
    }
#endif

    /* Push the new block to block stack */
    jit_block_stack_push(&cc->block_stack, block);

#if 0
    /* Push param phis to the new block */
    for (i = 0; i < block->param_count; i++) {
        PUSH(block->param_phis[i], block->param_types[i]);
    }
#endif

    return true;

fail:
#if 0
    if (block->param_phis) {
        wasm_runtime_free(block->param_phis);
        block->param_phis = NULL;
    }
    if (block->else_param_phis) {
        wasm_runtime_free(block->else_param_phis);
        block->else_param_phis = NULL;
    }
#endif
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
    uint8 *else_addr, *end_addr;
    JitReg value;
    char name[32];

    /* Check block stack */
    if (!cc->block_stack.block_list_end) {
        jit_set_last_error(cc, "WASM block stack underflow.");
        return false;
    }

    memset(block_addr_cache, 0, sizeof(block_addr_cache));

    /* Get block info */
    if (!(wasm_loader_find_block_addr(
            NULL, (BlockAddr *)block_addr_cache, *p_frame_ip, frame_ip_end,
            (uint8)label_type, &else_addr, &end_addr))) {
        jit_set_last_error(cc, "find block end addr failed.");
        return false;
    }

    /* Allocate memory */
    if (!(block = wasm_runtime_malloc(sizeof(JitBlock)))) {
        jit_set_last_error(cc, "allocate memory failed.");
        return false;
    }
    memset(block, 0, sizeof(JitBlock));
    if (param_count
        && !(block->param_types = wasm_runtime_malloc(param_count))) {
        jit_set_last_error(cc, "allocate memory failed.");
        goto fail;
    }
    if (result_count) {
        if (!(block->result_types = wasm_runtime_malloc(result_count))) {
            jit_set_last_error(cc, "allocate memory failed.");
            goto fail;
        }
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
    block->block_index = cc->block_stack.block_index[label_type];
    cc->block_stack.block_index[label_type]++;

    if (label_type == LABEL_TYPE_BLOCK || label_type == LABEL_TYPE_LOOP) {
        CREATE_BASIC_BLOCK(block->basic_block_entry);
        /* Jump to the entry block */
        BUILD_BR(block->basic_block_entry);
        if (!push_jit_block_to_stack_and_pass_params(cc, block))
            goto fail;
        /* Start to translate the block */
        SET_BUILDER_POS(block->basic_block_entry);
    }
    else if (label_type == LABEL_TYPE_IF) {
        // POP_COND(value);

#if 0
        if (LLVMIsUndef(value)
#if LLVM_VERSION_NUMBER >= 12
            || LLVMIsPoison(value)
#endif
        ) {
            if (!(jit_emit_exception(cc, func_ctx, EXCE_INTEGER_OVERFLOW, false,
                                     NULL, NULL))) {
                goto fail;
            }
            return jit_handle_next_reachable_block(cc, func_ctx, p_frame_ip);
        }
#endif

        if (!jit_reg_is_const_val(value)) {
            /* Compare value is not constant, create condition br IR */

            /* Create entry block */
            CREATE_BASIC_BLOCK(block->basic_block_entry);
            if (else_addr)
                /* Create else block */
                CREATE_BASIC_BLOCK(block->basic_block_else);
            /* Create end block */
            CREATE_BASIC_BLOCK(block->basic_block_end);

            if (else_addr) {
                /* Create condition br IR */
                BUILD_COND_BR(value, block->basic_block_entry,
                              block->basic_block_else);
            }
            else {
                /* Create condition br IR */
                BUILD_COND_BR(value, block->basic_block_entry,
                              block->basic_block_end);
                block->is_reachable = true;
            }
            if (!push_jit_block_to_stack_and_pass_params(cc, block))
                goto fail;
            /* Start to translate if branch of BASIC_BLOCK if */
            SET_BUILDER_POS(block->basic_block_entry);
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
                if (!push_jit_block_to_stack_and_pass_params(cc, block))
                    goto fail;
                /* Start to translate the if branch */
                SET_BUILDER_POS(block->basic_block_entry);
            }
            else {
                /* Compare value is not 0, condition is false, if branch of
                   BASIC_BLOCK if cannot be reached */
                if (else_addr) {
                    /* Create else block */
                    CREATE_BASIC_BLOCK(block->basic_block_else);
                    /* Jump to the else block */
                    BUILD_BR(block->basic_block_else);
                    if (!push_jit_block_to_stack_and_pass_params(cc, block))
                        goto fail;
                    /* Start to translate the else branch */
                    SET_BUILDER_POS(block->basic_block_else);
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
        jit_set_last_error(cc, "Invalid block type.");
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
    JitReg value;
    uint32 i, result_index;

    /* Check block */
    if (!block) {
        jit_set_last_error(cc, "WASM block stack underflow.");
        return false;
    }
    if (block->label_type != LABEL_TYPE_IF
        || (!block->skip_wasm_code_else && !block->basic_block_else)) {
        jit_set_last_error(cc, "Invalid WASM block type.");
        return false;
    }

    /* Create end block if needed */
    if (!block->basic_block_end) {
        CREATE_BASIC_BLOCK(block->basic_block_end);
    }

    block->is_reachable = true;

#if 0
    /* Comes from the if branch of BASIC_BLOCK if */
    CREATE_RESULT_VALUE_PHIS(block);
    for (i = 0; i < block->result_count; i++) {
        result_index = block->result_count - 1 - i;
        POP(value, block->result_types[result_index]);
        ADD_TO_RESULT_PHIS(block, value, result_index);
    }
#endif

    /* Jump to end block */
    BUILD_BR(block->basic_block_end);

    if (!block->skip_wasm_code_else && block->basic_block_else) {
        /* Clear value stack, recover param values
         * and start to translate else branch.
         */
        jit_value_stack_destroy(&block->value_stack);
#if 0
        for (i = 0; i < block->param_count; i++)
            PUSH(block->else_param_phis[i], block->param_types[i]);
#endif
        SET_BUILDER_POS(block->basic_block_else);
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
    JitBlock *block;
    JitReg value;
    JitBasicBlock *next_basic_block_end;
    uint32 i, result_index;

    /* Check block stack */
    if (!(block = cc->block_stack.block_list_end)) {
        jit_set_last_error(cc, "WASM block stack underflow.");
        return false;
    }

    /* Create the end block */
    if (!block->basic_block_end) {
        CREATE_BASIC_BLOCK(block->basic_block_end);
    }

#if 0
    /* Handle block result values */
    CREATE_RESULT_VALUE_PHIS(block);
    for (i = 0; i < block->result_count; i++) {
        value = NULL;
        result_index = block->result_count - 1 - i;
        POP(value, block->result_types[result_index]);
        bh_assert(value);
        ADD_TO_RESULT_PHIS(block, value, result_index);
    }
#endif

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

bool
jit_compile_op_br(JitCompContext *cc, uint32 br_depth, uint8 **p_frame_ip)
{
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
}

bool
jit_compile_op_br_if(JitCompContext *cc, uint32 br_depth, uint8 **p_frame_ip)
{
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
                    || !(values = wasm_runtime_malloc((uint32)size))) {
                    jit_set_last_error(cc, "allocate memory failed.");
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
                wasm_runtime_free(values);
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
                    || !(values = wasm_runtime_malloc((uint32)size))) {
                    jit_set_last_error(cc, "allocate memory failed.");
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
                wasm_runtime_free(values);
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
        wasm_runtime_free(values);
    return false;
}

bool
jit_compile_op_br_table(JitCompContext *cc, uint32 *br_depths, uint32 br_count,
                        uint8 **p_frame_ip)
{
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
                        || !(values = wasm_runtime_malloc((uint32)size))) {
                        jit_set_last_error(cc, "allocate memory failed.");
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
                    wasm_runtime_free(values);
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
                        || !(values = wasm_runtime_malloc((uint32)size))) {
                        jit_set_last_error(cc, "allocate memory failed.");
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
                    wasm_runtime_free(values);
                }
                if (i == br_count)
                    default_basic_block = target_block->basic_block_entry;
            }
        }

        /* Create switch IR */
        if (!(value_switch = LLVMBuildSwitch(cc->builder, value_cmp,
                                             default_basic_block, br_count))) {
            jit_set_last_error(cc, "llvm build switch failed.");
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
        wasm_runtime_free(values);
    return false;
}

bool
jit_compile_op_return(JitCompContext *cc, uint8 **p_frame_ip)
{
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
                jit_set_last_error(cc, "llvm build store failed.");
                goto fail;
            }
        }
        /* Return the first result value */
        POP(value, block_func->result_types[0]);
        if (!(ret = LLVMBuildRet(cc->builder, value))) {
            jit_set_last_error(cc, "llvm build return failed.");
            goto fail;
        }
#if WASM_ENABLE_DEBUG_JIT != 0
        LLVMInstructionSetDebugLoc(ret, return_location);
#endif
    }
    else {
        if (!(ret = LLVMBuildRetVoid(cc->builder))) {
            jit_set_last_error(cc, "llvm build return void failed.");
            goto fail;
        }
#if WASM_ENABLE_DEBUG_JIT != 0
        LLVMInstructionSetDebugLoc(ret, return_location);
#endif
    }

    return handle_next_reachable_block(cc, func_ctx, p_frame_ip);
fail:
    return false;
}
#endif

bool
jit_compile_op_unreachable(JitCompContext *cc, uint8 **p_frame_ip)
{
    /*
    if (!jit_emit_exception(cc, func_ctx, EXCE_UNREACHABLE, false, NULL, NULL))
        return false;
    */

    return handle_next_reachable_block(cc, p_frame_ip);
}

bool
jit_handle_next_reachable_block(JitCompContext *cc, uint8 **p_frame_ip)
{
    return handle_next_reachable_block(cc, p_frame_ip);
}
