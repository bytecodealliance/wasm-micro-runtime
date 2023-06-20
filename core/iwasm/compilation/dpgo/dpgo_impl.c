/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "./dpgo_internal.h"
#include "../common/wasm_runtime_common.h"

struct WASMProfCntInfo *
new_WasmProfCntInfo(uint32 cur_func_idx, uint32 offset, uint32 opcode,
                    uint32 counter_amount, uint32 first_counter_idx,
                    char *error_buf, uint32 error_buf_size)
{
    struct WASMProfCntInfo *opcode_cnt;

    opcode_cnt = (struct WASMProfCntInfo *)wasm_runtime_malloc(
        sizeof(struct WASMProfCntInfo));
    if (!opcode_cnt) {
        snprintf(error_buf, error_buf_size, "allocate memory failed");
        return NULL;
    }

    opcode_cnt->func_idx = cur_func_idx;
    opcode_cnt->offset = offset;
    opcode_cnt->opcode = opcode;
    opcode_cnt->counter_amount = counter_amount;
    opcode_cnt->first_counter_idx = first_counter_idx;

    return opcode_cnt;
}

/*
 * search in `prof_cnts_info[func_idx]` with the given `offset` and
 * return `struct WASMProfCntInfo`
 */
struct WASMProfCntInfo *
wasm_dpgo_search_prof_cnt_info(WASMModule *module, uint32 func_idx,
                               uint32 offset)
{
    bh_list *func_prof_cnts_info;
    void *elem;

    func_prof_cnts_info = wasm_dpgo_get_func_prof_cnts_info(module, func_idx);
    bh_assert(func_prof_cnts_info);

    elem = bh_list_first_elem(func_prof_cnts_info);
    while (elem) {
        struct WASMProfCntInfo *cnt_info = (struct WASMProfCntInfo *)elem;
        if (cnt_info->offset == offset) {
            return cnt_info;
        }
        elem = bh_list_elem_next(elem);
    }

    return NULL;
}

bool
wasm_dpgo_get_cond_br_counts(WASMModule *module, uint32 func_idx, uint32 offset,
                             uint32 *true_cnt, uint32 *false_cnt)
{
    struct WASMProfCntInfo *cnt_info;
    uint32 *cur_func_cnts;
    uint32 *cur_op_cnts;

    cnt_info = wasm_dpgo_search_prof_cnt_info(module, func_idx, offset);
    if (!cnt_info)
        return false;

    bh_assert(cnt_info->counter_amount == 2);

    cur_func_cnts =
        wasm_dpgo_get_func_entry_and_cond_br_cnts(module, func_idx, NULL);
    if (!cur_func_cnts)
        return false;

    cur_op_cnts = cur_func_cnts + cnt_info->first_counter_idx;

    /*
     * there will be two counters for a condBr. the first one is before
     * condBr. the second one represents true target.
     */
    *true_cnt = cur_op_cnts[1];
    *false_cnt = cur_op_cnts[0] - cur_op_cnts[1];
    return true;
}

bool
wasm_dpgo_get_select_counts(WASMModule *module, uint32 func_dix, uint32 offset,
                            uint32 *true_cnt, uint32 *false_cnt)
{
    return wasm_dpgo_get_cond_br_counts(module, func_dix, offset, true_cnt,
                                        false_cnt);
}

void
wasm_dpgo_dump_func_prof_cnts_info(WASMModule *module, uint32 func_idx)
{
    bh_list *func_cnts_info =
        wasm_dpgo_get_func_prof_cnts_info(module, func_idx);
    struct WASMProfCntInfo *cnt_info =
        (struct WASMProfCntInfo *)bh_list_first_elem(func_cnts_info);

    LOG_DEBUG("Dump Prof Cnt Info of Func.#%u, CAP.%u", func_idx,
              func_cnts_info->len);
    while (cnt_info) {
        char info[128] = { 0 };
        wasm_dpgo_prof_cnt_info_to_string(cnt_info, info, sizeof(info));
        LOG_DEBUG("  %s", info);
        cnt_info = (struct WASMProfCntInfo *)bh_list_elem_next(cnt_info);
    }
}

#ifndef NDEBUG
void
wasm_runtime_dump_pgo_info(WASMModule *module)
{
    for (unsigned i = 0;
         i < (module->import_function_count + module->function_count); i++) {
        uint32 ent_and_br_cnts_capacity = 0;
        uint32 *ent_and_br_cnts = wasm_dpgo_get_func_entry_and_cond_br_cnts(
            module, i, &ent_and_br_cnts_capacity);

        if (i < module->import_function_count) {
            bh_assert(!ent_and_br_cnts
                      && "should be no counter for an import func");
            continue;
        }

        if (ent_and_br_cnts_capacity == 0)
            continue;

        uint32 func_ent_cnt_value = wasm_dpgo_get_func_entry_count(module, i);
        if (func_ent_cnt_value < wasm_runtime_get_hot_func_threshold())
            continue;

        LOG_DEBUG("Counters of Func#%u: CAPACITY: %u", i,
                  ent_and_br_cnts_capacity);
        LOG_DEBUG("  [1] : %u", func_ent_cnt_value);

        /* too much to print. Enable it when necessary */
#if 0
        for (unsigned j = 2; j < ent_and_br_cnts_capacity; j++) {
            uint32 cnt_val = ent_and_br_cnts[j];
            if (cnt_val)
                LOG_WARNING("  [%u] : %u", j, cnt_val);
        }
#endif
    }
}
#endif