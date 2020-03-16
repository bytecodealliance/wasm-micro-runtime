/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_control.h"
#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"
#include "../interpreter/wasm_loader.h"

static char *block_name_prefix[] = { "block", "loop", "if" };
static char *block_name_suffix[] = { "begin", "else", "end" };

enum {
    LABEL_BEGIN = 0,
    LABEL_ELSE,
    LABEL_END
};

static void
format_block_name(char *name, uint32 name_size,
                  uint32 block_index, uint32 block_type,
                  uint32 label_type)
{
    if (block_type != BLOCK_TYPE_FUNCTION)
        snprintf(name, name_size, "%s%d%s%s",
                 block_name_prefix[block_type], block_index,
                 "_", block_name_suffix[label_type]);
    else
        snprintf(name, name_size, "%s", "func_end");
}

#define CREATE_BLOCK(new_llvm_block, name) do {         \
    if (!(new_llvm_block =                              \
      LLVMAppendBasicBlockInContext(comp_ctx->context,  \
                                    func_ctx->func,     \
                                    name))) {           \
      aot_set_last_error("add LLVM basic block failed.");\
      goto fail;                                        \
    }                                                   \
  } while (0)

#define CURR_BLOCK() LLVMGetInsertBlock(comp_ctx->builder)

#define MOVE_BLOCK_AFTER(llvm_block, llvm_block_after) \
    LLVMMoveBasicBlockAfter(llvm_block, llvm_block_after)

#define MOVE_BLOCK_AFTER_CURR(llvm_block) \
    LLVMMoveBasicBlockAfter(llvm_block, CURR_BLOCK())

#define MOVE_BLOCK_BEFORE(llvm_block, llvm_block_before) \
    LLVMMoveBasicBlockBefore(llvm_block, llvm_block_before)

#define BUILD_BR(llvm_block) do {                       \
    if (!LLVMBuildBr(comp_ctx->builder, llvm_block)) {  \
      aot_set_last_error("llvm build br failed.");      \
      goto fail;                                        \
    }                                                   \
  } while (0)

#define BUILD_COND_BR(value_if, block_then, block_else) do {\
    if (!LLVMBuildCondBr(comp_ctx->builder, value_if,       \
                         block_then, block_else)) {         \
      aot_set_last_error("llvm build cond br failed.");     \
      goto fail;                                            \
    }                                                       \
  } while (0)

#define SET_BUILDER_POS(llvm_block) \
    LLVMPositionBuilderAtEnd(comp_ctx->builder, llvm_block)

#define CREATE_RETURN_VALUE_PHI(block) do {                 \
    if (block->return_type != VALUE_TYPE_VOID               \
        && !block->return_value_phi) {                      \
      LLVMBasicBlockRef block_curr = CURR_BLOCK();          \
      SET_BUILDER_POS(block->llvm_end_block);               \
      if (!(block->return_value_phi =                       \
              LLVMBuildPhi(comp_ctx->builder,               \
                           TO_LLVM_TYPE(block->return_type),\
                           "phi"))) {                       \
        aot_set_last_error("llvm build phi failed.");       \
        goto fail;                                          \
      }                                                     \
      SET_BUILDER_POS(block_curr);                          \
    }                                                       \
  } while (0)

#define ADD_TO_RETURN_PHI(block, value) do {                \
    LLVMBasicBlockRef block_curr = CURR_BLOCK();            \
    LLVMAddIncoming(block->return_value_phi,                \
                    &value, &block_curr, 1);                \
  } while (0)


static LLVMBasicBlockRef
find_next_llvm_end_block(AOTBlock *block)
{
    block = block->prev;
    while (block && !block->llvm_end_block)
        block = block->prev;
    return block ? block->llvm_end_block : NULL;
}

static AOTBlock*
get_target_block(AOTFuncContext *func_ctx, uint32 br_depth)
{
    uint32 i = br_depth;
    AOTBlock *block = func_ctx->block_stack.block_list_end;

    while (i-- > 0 && block) {
        block = block->prev;
    }

    if (!block) {
        aot_set_last_error("WASM block stack underflow.");
        return NULL;
    }
    return block;
}

static bool
handle_next_reachable_block(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx,
                            uint8 **p_frame_ip)
{
    AOTBlock *block = func_ctx->block_stack.block_list_end;
    AOTBlock *block_prev;
    uint8 *frame_ip;

    if (block->block_type == BLOCK_TYPE_IF
        && block->llvm_else_block
        && !block->skip_wasm_code_else
        && *p_frame_ip <= block->wasm_code_else) {
        /* Clear value stack and start to translate else branch */
        aot_value_stack_destroy(&block->value_stack);
        SET_BUILDER_POS(block->llvm_else_block);
        *p_frame_ip = block->wasm_code_else + 1;
        return true;
    }

    while (block && !block->is_reachable) {
        block_prev = block->prev;
        block = aot_block_stack_pop(&func_ctx->block_stack);

        if (block->block_type == BLOCK_TYPE_IF
            && block->llvm_end_block) {
            LLVMDeleteBasicBlock(block->llvm_end_block);
            block->llvm_end_block = NULL;
        }

        frame_ip = block->wasm_code_end;
        aot_block_destroy(block);
        block = block_prev;
    }

    if (!block) {
        *p_frame_ip = frame_ip + 1;
        return true;
    }

    *p_frame_ip = block->wasm_code_end + 1;
    SET_BUILDER_POS(block->llvm_end_block);

    /* Pop block, push its return value, and destroy the block */
    block = aot_block_stack_pop(&func_ctx->block_stack);
    if (block->return_type != VALUE_TYPE_VOID) {
        bh_assert(block->return_value_phi);
        if (block->block_type != BLOCK_TYPE_FUNCTION)
            PUSH(block->return_value_phi, block->return_type);
        else
            LLVMBuildRet(comp_ctx->builder, block->return_value_phi);
    }
    else if (block->block_type == BLOCK_TYPE_FUNCTION) {
        LLVMBuildRetVoid(comp_ctx->builder);
    }
    aot_block_destroy(block);
    return true;
fail:
    return false;
}

bool
aot_compile_op_block(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                     uint8 **p_frame_ip, uint8 *frame_ip_end,
                     uint32 block_type, uint32 block_ret_type)
{
    BlockAddr block_addr_cache[BLOCK_ADDR_CACHE_SIZE][BLOCK_ADDR_CONFLICT_SIZE];
    AOTBlock *block;
    uint8 *else_addr, *end_addr;
    LLVMValueRef value;
    char name[32];

    /* Check block stack */
    if (!func_ctx->block_stack.block_list_end) {
        aot_set_last_error("WASM block stack underflow.");
        return false;
    }

    memset(block_addr_cache, 0, sizeof(block_addr_cache));

    /* Get block info */
    if (!(wasm_loader_find_block_addr((BlockAddr*)block_addr_cache,
                                      *p_frame_ip, frame_ip_end, (uint8)block_type,
                                      &else_addr, &end_addr, NULL, 0))) {
        aot_set_last_error("find block end addr failed.");
        return false;
    }

    /* Allocate memory */
    if (!(block = wasm_runtime_malloc(sizeof(AOTBlock)))) {
        aot_set_last_error("allocate memory failed.");
        return false;
    }

    /* Init aot block data */
    memset(block, 0, sizeof(AOTBlock));
    block->block_type = block_type;
    block->return_type = (uint8)block_ret_type;
    block->wasm_code_else = else_addr;
    block->wasm_code_end = end_addr;
    block->block_index = func_ctx->block_stack.block_index[block_type];
    func_ctx->block_stack.block_index[block_type]++;

    if (block_type == BLOCK_TYPE_BLOCK
        || block_type == BLOCK_TYPE_LOOP) {
        /* Create block */
        format_block_name(name, sizeof(name),
                          block->block_index, block_type, LABEL_BEGIN);
        CREATE_BLOCK(block->llvm_entry_block, name);
        MOVE_BLOCK_AFTER_CURR(block->llvm_entry_block);
        /* Jump to the entry block */
        BUILD_BR(block->llvm_entry_block);
        /* Start to translate the block */
        SET_BUILDER_POS(block->llvm_entry_block);
        aot_block_stack_push(&func_ctx->block_stack, block);
    }
    else if (block_type == BLOCK_TYPE_IF) {
        POP_COND(value);
        if (!LLVMIsConstant(value)) {
            /* Compare value is not constant, create condition br IR */
            /* Create entry block */
            format_block_name(name, sizeof(name),
                              block->block_index, block_type, LABEL_BEGIN);
            CREATE_BLOCK(block->llvm_entry_block, name);
            MOVE_BLOCK_AFTER_CURR(block->llvm_entry_block);

            /* Create end block */
            format_block_name(name, sizeof(name),
                              block->block_index, block_type, LABEL_END);
            CREATE_BLOCK(block->llvm_end_block, name);
            MOVE_BLOCK_AFTER(block->llvm_end_block, block->llvm_entry_block);

            if (else_addr) {
                /* Create else block */
                format_block_name(name, sizeof(name),
                                  block->block_index, block_type, LABEL_ELSE);
                CREATE_BLOCK(block->llvm_else_block, name);
                MOVE_BLOCK_AFTER(block->llvm_else_block, block->llvm_entry_block);
                /* Create condition br IR */
                BUILD_COND_BR(value, block->llvm_entry_block,
                              block->llvm_else_block);
            }
            else {
                /* Create condition br IR */
                BUILD_COND_BR(value, block->llvm_entry_block,
                              block->llvm_end_block);
                block->is_reachable = true;
            }
            /* Start to translate if branch of BLOCK if */
            SET_BUILDER_POS(block->llvm_entry_block);
            aot_block_stack_push(&func_ctx->block_stack, block);
        }
        else {
            if ((int32)LLVMConstIntGetZExtValue(value) != 0) {
                /* Compare value is not 0, condtion is true, else branch of
                   BLOCK if cannot be reached */
                block->skip_wasm_code_else = true;
                /* Create entry block */
                format_block_name(name, sizeof(name),
                                  block->block_index, block_type, LABEL_BEGIN);
                CREATE_BLOCK(block->llvm_entry_block, name);
                MOVE_BLOCK_AFTER_CURR(block->llvm_entry_block);
                /* Jump to the entry block */
                BUILD_BR(block->llvm_entry_block);
                /* Start to translate the if branch */
                SET_BUILDER_POS(block->llvm_entry_block);
                aot_block_stack_push(&func_ctx->block_stack, block);
            }
            else {
                /* Compare value is not 0, condtion is false, if branch of
                   BLOCK if cannot be reached */
                if (else_addr) {
                    /* Create else block */
                    format_block_name(name, sizeof(name),
                                      block->block_index, block_type, LABEL_ELSE);
                    CREATE_BLOCK(block->llvm_else_block, name);
                    MOVE_BLOCK_AFTER_CURR(block->llvm_else_block);
                    /* Jump to the else block */
                    BUILD_BR(block->llvm_else_block);
                    /* Start to translate the else branch */
                    SET_BUILDER_POS(block->llvm_else_block);
                    *p_frame_ip = else_addr + 1;
                    aot_block_stack_push(&func_ctx->block_stack, block);
                }
                else {
                    if (block->return_type != VALUE_TYPE_VOID) {
                        aot_set_last_error("WASM value stack underflow.");
                        goto fail;
                    }
                    /* skip the block */
                    wasm_runtime_free(block);
                    *p_frame_ip = end_addr + 1;
                }
            }
        }
    }
    else {
        aot_set_last_error("Invalid block type.");
        goto fail;
    }

    return true;
fail:
    wasm_runtime_free(block);
    return false;
}

bool
aot_compile_op_else(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                    uint8 **p_frame_ip)
{
    AOTBlock *block = func_ctx->block_stack.block_list_end;
    LLVMValueRef value;
    char name[32];

    /* Check block */
    if (!block) {
        aot_set_last_error("WASM block stack underflow.");
        return false;
    }
    if (block->block_type != BLOCK_TYPE_IF
        || (!block->skip_wasm_code_else
            && !block->llvm_else_block)) {
        aot_set_last_error("Invalid WASM block type.");
        return false;
    }

    /* Create end block if needed */
    if (!block->llvm_end_block) {
        format_block_name(name, sizeof(name),
                          block->block_index, block->block_type, LABEL_END);
        CREATE_BLOCK(block->llvm_end_block, name);
        if (block->llvm_else_block)
            MOVE_BLOCK_AFTER(block->llvm_end_block, block->llvm_else_block);
        else
            MOVE_BLOCK_AFTER_CURR(block->llvm_end_block);
    }

    block->is_reachable = true;

    /* Comes from the if branch of BLOCK if */
    if (block->return_type != VALUE_TYPE_VOID) {
        POP(value, block->return_type);
        CREATE_RETURN_VALUE_PHI(block);
        ADD_TO_RETURN_PHI(block, value);
    }

    /* Jump to end block */
    BUILD_BR(block->llvm_end_block);

    if (!block->skip_wasm_code_else
        && block->llvm_else_block) {
        /* Clear value stack and start to translate else branch */
        aot_value_stack_destroy(&block->value_stack);
        SET_BUILDER_POS(block->llvm_else_block);
        return true;
    }

    /* No else branch or no need to translate else branch */
    block->is_reachable = true;
    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
fail:
    return false;
}

bool
aot_compile_op_end(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                   uint8 **p_frame_ip)
{
    AOTBlock *block;
    LLVMValueRef value;
    LLVMBasicBlockRef next_llvm_end_block;
    char name[32];

    /* Check block stack */
    if (!(block = func_ctx->block_stack.block_list_end)) {
        aot_set_last_error("WASM block stack underflow.");
        return false;
    }

    /* Create the end block */
    if (!block->llvm_end_block) {
        format_block_name(name, sizeof(name),
                          block->block_index, block->block_type, LABEL_END);
        CREATE_BLOCK(block->llvm_end_block, name);
        if ((next_llvm_end_block = find_next_llvm_end_block(block)))
            MOVE_BLOCK_BEFORE(block->llvm_end_block, next_llvm_end_block);
    }

    /* Handle block return value */
    if (block->return_type != VALUE_TYPE_VOID) {
        POP(value, block->return_type);
        CREATE_RETURN_VALUE_PHI(block);
        ADD_TO_RETURN_PHI(block, value);
    }

    /* Jump to the end block */
    BUILD_BR(block->llvm_end_block);

    block->is_reachable = true;
    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
fail:
    return false;
}

bool
aot_compile_op_br(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                  uint32 br_depth, uint8 **p_frame_ip)
{
    AOTBlock *block_dst;
    LLVMValueRef value_ret;
    LLVMBasicBlockRef next_llvm_end_block;
    char name[32];

    if (!(block_dst = get_target_block(func_ctx, br_depth))) {
        return false;
    }

    if (block_dst->block_type == BLOCK_TYPE_LOOP) {
        /* Dest block is Loop block */
        BUILD_BR(block_dst->llvm_entry_block);
    }
    else {
        /* Dest block is Block/If/Function block */
        /* Create the end block */
        if (!block_dst->llvm_end_block) {
            format_block_name(name, sizeof(name),
                              block_dst->block_index, block_dst->block_type,
                              LABEL_END);
            CREATE_BLOCK(block_dst->llvm_end_block, name);
            if ((next_llvm_end_block = find_next_llvm_end_block(block_dst)))
                MOVE_BLOCK_BEFORE(block_dst->llvm_end_block,
                                  next_llvm_end_block);
        }

        block_dst->is_reachable = true;

        /* Handle return value */
        if (block_dst->return_type != VALUE_TYPE_VOID) {
            POP(value_ret, block_dst->return_type);
            CREATE_RETURN_VALUE_PHI(block_dst);
            ADD_TO_RETURN_PHI(block_dst, value_ret);
        }

        /* Jump to the end block */
        BUILD_BR(block_dst->llvm_end_block);
    }

    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
fail:
    return false;
}

bool
aot_compile_op_br_if(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                     uint32 br_depth, uint8 **p_frame_ip)
{
    AOTBlock *block_dst;
    LLVMValueRef value_cmp, value_ret;
    LLVMBasicBlockRef llvm_else_block, next_llvm_end_block;
    char name[32];

    POP_COND(value_cmp);
    if (!LLVMIsConstant(value_cmp)) {
        /* Compare value is not constant, create condition br IR */
        if (!(block_dst = get_target_block(func_ctx, br_depth))) {
            return false;
        }

        /* Create llvm else block */
        CREATE_BLOCK(llvm_else_block, "br_if_else");
        MOVE_BLOCK_AFTER_CURR(llvm_else_block);

        if (block_dst->block_type == BLOCK_TYPE_LOOP) {
            /* Dest block is Loop block */
            BUILD_COND_BR(value_cmp, block_dst->llvm_entry_block,
                          llvm_else_block);

            /* Move builder to else block */
            SET_BUILDER_POS(llvm_else_block);
        }
        else {
            /* Dest block is Block/If/Function block */
            /* Create the end block */
            if (!block_dst->llvm_end_block) {
                format_block_name(name, sizeof(name),
                                  block_dst->block_index, block_dst->block_type,
                                  LABEL_END);
                CREATE_BLOCK(block_dst->llvm_end_block, name);
                if ((next_llvm_end_block = find_next_llvm_end_block(block_dst)))
                    MOVE_BLOCK_BEFORE(block_dst->llvm_end_block,
                                      next_llvm_end_block);
            }

            /* Set reachable flag and create condtion br IR */
            block_dst->is_reachable = true;

            /* Handle return value */
            if (block_dst->return_type != VALUE_TYPE_VOID) {
                POP(value_ret, block_dst->return_type);
                CREATE_RETURN_VALUE_PHI(block_dst);
                ADD_TO_RETURN_PHI(block_dst, value_ret);
                PUSH(value_ret, block_dst->return_type);
            }

            /* Condition jump to end block */
            BUILD_COND_BR(value_cmp, block_dst->llvm_end_block,
                          llvm_else_block);

            /* Move builder to else block */
            SET_BUILDER_POS(llvm_else_block);
        }
    }
    else {
        if ((int32)LLVMConstIntGetZExtValue(value_cmp) != 0) {
            /* Compare value is not 0, condtion is true, same as op_br */
            return aot_compile_op_br(comp_ctx, func_ctx, br_depth, p_frame_ip);
        }
        else {
            /* Compare value is not 0, condtion is false, skip br_if */
            return true;
        }
    }
    return true;
fail:
    return false;
}

bool
aot_compile_op_br_table(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 *br_depths, uint32 br_count,
                        uint8 **p_frame_ip)
{
    uint32 i;
    LLVMValueRef value_switch, value_cmp, value_case, value_ret = NULL;
    LLVMBasicBlockRef default_llvm_block = NULL, target_llvm_block;
    LLVMBasicBlockRef next_llvm_end_block;
    AOTBlock *target_block;
    uint32 br_depth, depth_idx;
    char name[32];

    POP_I32(value_cmp);
    if (!LLVMIsConstant(value_cmp)) {
        /* Compare value is not constant, create switch IR */
        for (i = 0; i <= br_count; i++) {
            target_block = get_target_block(func_ctx, br_depths[i]);
            if (!target_block)
                return false;

            if (target_block->block_type != BLOCK_TYPE_LOOP) {
                /* Dest block is Block/If/Function block */
                /* Create the end block */
                if (!target_block->llvm_end_block) {
                    format_block_name(name, sizeof(name),
                                      target_block->block_index,
                                      target_block->block_type,
                                      LABEL_END);
                    CREATE_BLOCK(target_block->llvm_end_block, name);
                    if ((next_llvm_end_block =
                                find_next_llvm_end_block(target_block)))
                        MOVE_BLOCK_BEFORE(target_block->llvm_end_block,
                                          next_llvm_end_block);
                }
                /* Handle return value */
                if (target_block->return_type != VALUE_TYPE_VOID) {
                    POP(value_ret, target_block->return_type);
                    CREATE_RETURN_VALUE_PHI(target_block);
                    ADD_TO_RETURN_PHI(target_block, value_ret);
                    PUSH(value_ret, target_block->return_type);
                }
                target_block->is_reachable = true;
                if (i == br_count)
                    default_llvm_block = target_block->llvm_end_block;
            }
            else {
                if (i == br_count)
                    default_llvm_block = target_block->llvm_entry_block;
            }
        }

        /* Create switch IR */
        if (!(value_switch = LLVMBuildSwitch(comp_ctx->builder, value_cmp,
                                             default_llvm_block, br_count))) {
            aot_set_last_error("llvm build switch failed.");
            return false;
        }

        /* Add each case for switch IR */
        for (i = 0; i < br_count; i++) {
            value_case = I32_CONST(i);
            CHECK_LLVM_CONST(value_case);
            target_block = get_target_block(func_ctx, br_depths[i]);
            if (!target_block)
                return false;
            target_llvm_block = target_block->block_type != BLOCK_TYPE_LOOP
                                ? target_block->llvm_end_block
                                : target_block->llvm_entry_block;
            LLVMAddCase(value_switch, value_case, target_llvm_block);
        }

        return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
    }
    else {
        /* Compare value is constant, create br IR */
        depth_idx = (uint32)LLVMConstIntGetZExtValue(value_cmp);
        br_depth = br_depths[br_count];
        if (depth_idx < br_count) {
            br_depth = br_depths[depth_idx];
        }
        return aot_compile_op_br(comp_ctx, func_ctx, br_depth, p_frame_ip);
    }
fail:
    return false;
}

bool
aot_compile_op_return(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      uint8 **p_frame_ip)
{
    AOTBlock *block_func = func_ctx->block_stack.block_list_head;
    LLVMValueRef value;

    bh_assert(block_func);
    if (block_func->return_type != VALUE_TYPE_VOID) {
        POP(value, block_func->return_type);
        LLVMBuildRet(comp_ctx->builder, value);
    }
    else
        LLVMBuildRetVoid(comp_ctx->builder);

    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
fail:
    return false;
}

bool
aot_compile_op_unreachable(AOTCompContext *comp_ctx,
                           AOTFuncContext *func_ctx,
                           uint8 **p_frame_ip)
{
    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_UNREACHABLE,
                            false, NULL, NULL))
        return false;

    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
}

bool
aot_handle_next_reachable_block(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx,
                                uint8 **p_frame_ip)
{
    return handle_next_reachable_block(comp_ctx, func_ctx, p_frame_ip);
}
