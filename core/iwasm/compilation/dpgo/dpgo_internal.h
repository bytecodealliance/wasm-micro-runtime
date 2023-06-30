/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _DPGO_INTERNAL_H_
#define _DPGO_INTERNAL_H_

#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>

#include "bh_platform.h"
#include "wasm.h"
#include "../../common/wasm_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#if WASM_ENABLE_DYNAMIC_PGO == 0
#error \
    "dpgo_internal.h should not be included if WASM_ENABLE_DYNAMIC_PGO is disable"
#endif

struct WASMProfCntInfo {
    bh_list_link l;
    uint32 func_idx;
    uint32 offset;
    uint32 opcode;
    uint32 counter_amount;
    uint32 first_counter_idx;
};

struct WASMProfCntInfo *
new_WasmProfCntInfo(uint32 cur_func_idx, uint32 offset, uint32 opcode,
                    uint32 counter_amount, uint32 first_counter_idx,
                    char *error_buf, uint32 error_buf_size);

static inline uint32 *
wasm_dpgo_get_func_entry_and_cond_br_cnts(WASMModule *module, uint32 func_idx,
                                          uint32 *capacity)
{
    uint32 *ent_and_br_cnts = module->ent_and_br_cnts[func_idx];
    if (capacity)
        *capacity = ent_and_br_cnts ? ent_and_br_cnts[0] : 0;

    return ent_and_br_cnts;
}

static inline uint32
wasm_dpgo_get_func_entry_count(WASMModule *module, uint32 func_idx)
{
    uint32 *ent_and_br_cnts =
        wasm_dpgo_get_func_entry_and_cond_br_cnts(module, func_idx, NULL);
    return ent_and_br_cnts ? ent_and_br_cnts[1] : 0;
}

static inline bh_list *
wasm_dpgo_get_func_prof_cnts_info(WASMModule *module, uint32 func_idx)
{
    return module->prof_cnts_info + func_idx;
}

static inline void
wasm_dpgo_prof_cnt_info_to_string(struct WASMProfCntInfo *cnt, char *buf,
                                  uint32 buf_size)
{
    snprintf(buf, buf_size, "  OP:0x%x,OFFSET:%u,CNT:%u,1ST_IDX:%u",
             cnt->opcode, cnt->offset, cnt->counter_amount,
             cnt->first_counter_idx);
}

struct WASMProfCntInfo *
wasm_dpgo_search_prof_cnt_info(WASMModule *module, uint32 func_idx,
                               uint32 offset);

bool
wasm_dpgo_get_cond_br_counts(WASMModule *module, uint32 func_idx, uint32 offset,
                             uint32 *true_cnt, uint32 *false_cnt);

bool
wasm_dpgo_get_select_counts(WASMModule *module, uint32 func_dix, uint32 offset,
                            uint32 *true_cnt, uint32 *false_cnt);

void
wasm_dpgo_dump_func_prof_cnts_info(WASMModule *module, uint32 func_idx);

static inline WASMDPGOFuncState
wasm_dpgo_get_func_hotness_state(WASMModule *wasm_module, uint32 func_idx)
{
    return wasm_module->func_opt_w_prof[func_idx];
}

static inline void
wasm_dpgo_set_func_hotness_state(WASMModule *wasm_module, uint32 func_idx,
                                 WASMDPGOFuncState state)
{
    wasm_module->func_opt_w_prof[func_idx] = state;
}

#ifndef NDEBUG
void
wasm_runtime_dump_pgo_info(WASMModule *module);
#endif

bool
verify_module_and_debug_info(LLVMModuleRef module);

void
wasm_dpgo_set_branch_weights(LLVMContextRef context, LLVMValueRef instruction,
                             uint32 *counts, uint32 counts_size);

void
wasm_dpgo_set_prof_meta(WASMModule *wasm_module, LLVMModuleRef module,
                        LLVMValueRef function, uint32 func_idx);

void
wasm_dpgo_unlike_true_branch(LLVMContextRef context, LLVMValueRef cond_br);

void
wasm_dpgo_unlike_false_branch(LLVMContextRef context, LLVMValueRef cond_br);

void
wasm_dpgo_extra_pass_pipeline(LLVMTargetMachineRef target_machine,
                              LLVMModuleRef module);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
