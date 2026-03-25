/*
 * Copyright (C) 2026 Airbus Defence and Space Romania SRL. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_component.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "wasm_loader_common.h"
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include <stdio.h>

// Local helpers to free nested allocations safely
static void
free_canon_opts_struct(WASMComponentCanonOpts *opts)
{
    if (!opts)
        return;
    if (opts->canon_opts) {
        wasm_runtime_free(opts->canon_opts);
        opts->canon_opts = NULL;
        opts->canon_opts_count = 0;
    }
    wasm_runtime_free(opts);
}

static void
free_result_list_struct(WASMComponentResultList *rl)
{
    if (!rl)
        return;
    if (rl->tag == WASM_COMP_RESULT_LIST_WITH_TYPE && rl->results) {
        wasm_runtime_free(rl->results);
        rl->results = NULL;
    }
    wasm_runtime_free(rl);
}

static void
free_single_canon_allocs(WASMComponentCanon *canon)
{
    if (!canon)
        return;
    switch (canon->tag) {
        case WASM_COMP_CANON_LIFT:
            if (canon->canon_data.lift.canon_opts) {
                free_canon_opts_struct(canon->canon_data.lift.canon_opts);
                canon->canon_data.lift.canon_opts = NULL;
            }
            break;
        case WASM_COMP_CANON_LOWER:
            if (canon->canon_data.lower.canon_opts) {
                free_canon_opts_struct(canon->canon_data.lower.canon_opts);
                canon->canon_data.lower.canon_opts = NULL;
            }
            break;
        case WASM_COMP_CANON_TASK_RETURN:
            if (canon->canon_data.task_return.result_list) {
                free_result_list_struct(
                    canon->canon_data.task_return.result_list);
                canon->canon_data.task_return.result_list = NULL;
            }
            if (canon->canon_data.task_return.canon_opts) {
                free_canon_opts_struct(
                    canon->canon_data.task_return.canon_opts);
                canon->canon_data.task_return.canon_opts = NULL;
            }
            break;
        case WASM_COMP_CANON_STREAM_READ:
        case WASM_COMP_CANON_STREAM_WRITE:
            if (canon->canon_data.stream_read_write.canon_opts) {
                free_canon_opts_struct(
                    canon->canon_data.stream_read_write.canon_opts);
                canon->canon_data.stream_read_write.canon_opts = NULL;
            }
            break;
        case WASM_COMP_CANON_FUTURE_READ:
        case WASM_COMP_CANON_FUTURE_WRITE:
            if (canon->canon_data.future_read_write.canon_opts) {
                free_canon_opts_struct(
                    canon->canon_data.future_read_write.canon_opts);
                canon->canon_data.future_read_write.canon_opts = NULL;
            }
            break;
        case WASM_COMP_CANON_ERROR_CONTEXT_NEW:
        case WASM_COMP_CANON_ERROR_CONTEXT_DEBUG:
            if (canon->canon_data.error_context_new_debug.canon_opts) {
                free_canon_opts_struct(
                    canon->canon_data.error_context_new_debug.canon_opts);
                canon->canon_data.error_context_new_debug.canon_opts = NULL;
            }
            break;
        default:
            break;
    }
}

// Parse single canon option
static bool
parse_canon_opt(const uint8_t **payload, const uint8_t *end,
                WASMComponentCanonOpt *out, char *error_buf,
                uint32_t error_buf_size)
{
    const uint8_t *p = *payload;

    uint8_t tag = *p++;
    out->tag = tag;
    switch (tag) {
        case WASM_COMP_CANON_OPT_STRING_UTF8:  // string-encoding=utf8 - 0x00
        case WASM_COMP_CANON_OPT_STRING_UTF16: // string-encoding=utf16 - 0x01
        case WASM_COMP_CANON_OPT_STRING_LATIN1_UTF16: // string-encoding=latin1+utf16
                                                      // - 0x02
            break;
        case WASM_COMP_CANON_OPT_MEMORY:
        { // (memory m) - 0x03
            uint64_t core_mem_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_mem_idx,
                          error_buf, error_buf_size)) {
                return false;
            }
            out->payload.memory.mem_idx = (uint32_t)core_mem_idx;
            break;
        }
        case WASM_COMP_CANON_OPT_REALLOC:
        { // (realloc f) - 0x04
            uint64_t core_func_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_func_idx,
                          error_buf, error_buf_size)) {
                return false;
            }
            out->payload.realloc_opt.func_idx = (uint32_t)core_func_idx;
            break;
        }
        case WASM_COMP_CANON_OPT_POST_RETURN:
        { // (post-return f) - 0x05
            uint64_t core_func_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_func_idx,
                          error_buf, error_buf_size)) {
                return false;
            }
            out->payload.post_return.func_idx = (uint32_t)core_func_idx;
            break;
        }
        case WASM_COMP_CANON_OPT_ASYNC:
        { // async - 0x06
            break;
        }
        case WASM_COMP_CANON_OPT_CALLBACK:
        { // (callback f) - 0x07
            uint64_t core_func_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_func_idx,
                          error_buf, error_buf_size)) {
                return false;
            }
            out->payload.callback.func_idx = (uint32_t)core_func_idx;
            break;
        }
        default:
        {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Invalid canon opt tag: %02x", tag);
            return false;
        }
    }

    *payload = p;
    return true;
}

// Parsing canon options
static bool
parse_canon_opts(const uint8_t **payload, const uint8_t *end,
                 WASMComponentCanonOpts **out, char *error_buf,
                 uint32_t error_buf_size)
{
    if (!payload || !*payload || !out || !end) {
        set_error_buf_ex(error_buf, error_buf_size,
                         "Invalid payload or output pointer");
        return false;
    }

    const uint8_t *p = *payload;

    *out = wasm_runtime_malloc(sizeof(WASMComponentCanonOpts));
    if (!*out) {
        set_error_buf_ex(error_buf, error_buf_size,
                         "Failed to allocate memory for canon opts");
        return false;
    }

    uint64_t canon_opts_count = 0;
    if (!read_leb((uint8_t **)&p, end, 32, false, &canon_opts_count, error_buf,
                  error_buf_size)) {
        free_canon_opts_struct(*out);
        *out = NULL;
        return false;
    }
    (*out)->canon_opts_count = (uint32_t)canon_opts_count;

    if (canon_opts_count > 0) {
        (*out)->canon_opts = wasm_runtime_malloc(sizeof(WASMComponentCanonOpt)
                                                 * canon_opts_count);
        if (!(*out)->canon_opts) {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Failed to allocate memory for canon opts");
            free_canon_opts_struct(*out);
            *out = NULL;
            return false;
        }

        for (uint64_t i = 0; i < canon_opts_count; i++) {
            if (!parse_canon_opt(&p, end, &(*out)->canon_opts[i], error_buf,
                                 error_buf_size)) {
                free_canon_opts_struct(*out);
                *out = NULL;
                return false;
            }
        }
    }
    else {
        (*out)->canon_opts = NULL;
    }

    bool has_string_encoding = false;
    uint8_t seen_bits = 0; /* bits 0..4 for tags 0x03..0x07 */
    for (uint32_t i = 0; i < (uint32_t)canon_opts_count; i++) {
        uint8_t t = (*out)->canon_opts[i].tag;
        if (t == WASM_COMP_CANON_OPT_STRING_UTF8
            || t == WASM_COMP_CANON_OPT_STRING_UTF16
            || t == WASM_COMP_CANON_OPT_STRING_LATIN1_UTF16) {
            if (has_string_encoding) {
                set_error_buf_ex(
                    error_buf, error_buf_size,
                    "Conflicting or duplicate string-encoding canonopt");
                free_canon_opts_struct(*out);
                *out = NULL;
                return false;
            }
            has_string_encoding = true;
        }
        else {
            uint8_t bit = (uint8_t)(1u << (t - WASM_COMP_CANON_OPT_MEMORY));
            if (seen_bits & bit) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Duplicate canonopt: 0x%02x", t);
                free_canon_opts_struct(*out);
                *out = NULL;
                return false;
            }
            seen_bits |= bit;
        }
    }

    *payload = p;
    return true;
}

// Parsing single canon
static bool
parse_single_canon(const uint8_t **payload, const uint8_t *end,
                   WASMComponentCanon *out, char *error_buf,
                   uint32_t error_buf_size)
{
    const uint8_t *p = *payload;

    uint8_t tag = *p++;
    out->tag = tag;
    switch (tag) {
        case WASM_COMP_CANON_LIFT:
        { // 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
            if (*p != 0x00) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid canon tag: %02x", tag);
                return false;
            }
            p++;

            // Read core:funcidx
            uint64_t core_func_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_func_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read core func idx");
                return false;
            }
            out->canon_data.lift.core_func_idx = (uint32_t)core_func_idx;

            // Read canon opts
            if (!parse_canon_opts(&p, end, &out->canon_data.lift.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }

            // Read typeidx
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                // cleanup previously allocated opts
                if (out->canon_data.lift.canon_opts) {
                    free_canon_opts_struct(out->canon_data.lift.canon_opts);
                    out->canon_data.lift.canon_opts = NULL;
                }
                return false;
            }
            out->canon_data.lift.type_idx = (uint32_t)type_idx;

            break;
        }

        case WASM_COMP_CANON_LOWER:
        { // 0x01 0x00 f:<funcidx> opts:<opts>
            if (*p != 0x00) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid canon tag: %02x", tag);
                return false;
            }
            p++;
            // Read funcidx
            uint64_t func_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &func_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.lower.func_idx = (uint32_t)func_idx;

            // Read canon opts
            if (!parse_canon_opts(&p, end, &out->canon_data.lower.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_RESOURCE_NEW:
        { // 0x02 rt:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.resource_new.resource_type_idx = (uint32_t)type_idx;

            break;
        }

        case WASM_COMP_CANON_RESOURCE_DROP:
        { // 0x03 rt:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.resource_drop.resource_type_idx =
                (uint32_t)type_idx;
            out->canon_data.resource_drop.async = false;
            break;
        }

        case WASM_COMP_CANON_RESOURCE_DROP_ASYNC:
        { // 0x07 rt:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.resource_drop.resource_type_idx =
                (uint32_t)type_idx;
            out->canon_data.resource_drop.async = true;

            break;
        }

        case WASM_COMP_CANON_RESOURCE_REP:
        { // 0x04 rt:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.resource_rep.resource_type_idx = (uint32_t)type_idx;
            break;
        }

        case WASM_COMP_CANON_BACKPRESSURE_SET:
        { // 0x08
            break;
        }

        case WASM_COMP_CANON_TASK_RETURN:
        { // 0x09 rs:<result_list> opts:<opts>
            if (!parse_result_list(&p, end,
                                   &out->canon_data.task_return.result_list,
                                   error_buf, error_buf_size)) {
                // Best-effort cleanup if result_list was set
                if (out->canon_data.task_return.result_list) {
                    free_result_list_struct(
                        out->canon_data.task_return.result_list);
                    out->canon_data.task_return.result_list = NULL;
                }
                return false;
            }

            if (!parse_canon_opts(&p, end,
                                  &out->canon_data.task_return.canon_opts,
                                  error_buf, error_buf_size)) {
                // cleanup result_list allocated above before failing
                if (out->canon_data.task_return.result_list) {
                    free_result_list_struct(
                        out->canon_data.task_return.result_list);
                    out->canon_data.task_return.result_list = NULL;
                }
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_TASK_CANCEL:
        { // 0x05 (no parameters)
            break;
        }

        case WASM_COMP_CANON_CONTEXT_GET:
        { // 0x0a 0x7f i:<u32>
            if (*p != 0x7f) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid canon tag: %02x", tag);
                return false;
            }
            p++;

            uint64_t i = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &i, error_buf,
                          error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read context idx");
                return false;
            }
            out->canon_data.context_get_set.context_idx = (uint32_t)i;

            break;
        }

        case WASM_COMP_CANON_CONTEXT_SET:
        { // 0x0b 0x7f i:<u32>
            if (*p != 0x7f) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid canon tag: %02x", tag);
                return false;
            }
            p++;

            uint64_t i = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &i, error_buf,
                          error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read context idx");
                return false;
            }
            out->canon_data.context_get_set.context_idx = (uint32_t)i;

            break;
        }

        case WASM_COMP_CANON_YIELD:
        { // 0x0c cancel?:<cancel?>
            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.yield.cancellable = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.yield.cancellable = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid cancel? tag: %02x", b);
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_SUBTASK_CANCEL:
        { // 0x06 async?:<async?>
            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.subtask_cancel.async = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.subtask_cancel.async = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid async? tag: %02x", b);
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_SUBTASK_DROP:
        { // 0x0d
            break;
        }

        case WASM_COMP_CANON_STREAM_NEW:
        { // 0x0e t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_new.stream_type_idx = (uint32_t)type_idx;
            break;
        }

        case WASM_COMP_CANON_STREAM_READ:
        { // 0x0f t:<typeidx> opts:<opts>
            // Read stream type idx
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_read_write.stream_type_idx =
                (uint32_t)type_idx;

            // Read canon opts
            if (!parse_canon_opts(&p, end,
                                  &out->canon_data.stream_read_write.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_STREAM_WRITE:
        { // 0x10 t:<typeidx> opts:<opts>
            // Read stream type idx
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_read_write.stream_type_idx =
                (uint32_t)type_idx;

            // Read canon opts
            if (!parse_canon_opts(&p, end,
                                  &out->canon_data.stream_read_write.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_STREAM_CANCEL_READ:
        { // 0x11 t:<typeidx> async?:<async?>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_cancel_read_write.stream_type_idx =
                (uint32_t)type_idx;

            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.stream_cancel_read_write.async = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.stream_cancel_read_write.async = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid async? tag: %02x", b);
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_STREAM_CANCEL_WRITE:
        { // 0x12 t:<typeidx> async?:<async?>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_cancel_read_write.stream_type_idx =
                (uint32_t)type_idx;

            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.stream_cancel_read_write.async = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.stream_cancel_read_write.async = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid async? tag: %02x", b);
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_STREAM_DROP_READABLE:
        { // 0x13 t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_drop_readable_writable.stream_type_idx =
                (uint32_t)type_idx;

            break;
        }

        case WASM_COMP_CANON_STREAM_DROP_WRITABLE:
        { // 0x14 t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.stream_drop_readable_writable.stream_type_idx =
                (uint32_t)type_idx;
            break;
        }

        case WASM_COMP_CANON_FUTURE_NEW:
        { // 0x15 t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_new.future_type_idx = (uint32_t)type_idx;
            break;
        }

        case WASM_COMP_CANON_FUTURE_READ:
        { // 0x16 t:<typeidx> opts:<opts>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_read_write.future_type_idx =
                (uint32_t)type_idx;

            if (!parse_canon_opts(&p, end,
                                  &out->canon_data.future_read_write.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_FUTURE_WRITE:
        { // 0x17 t:<typeidx> opts:<opts>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_read_write.future_type_idx =
                (uint32_t)type_idx;

            if (!parse_canon_opts(&p, end,
                                  &out->canon_data.future_read_write.canon_opts,
                                  error_buf, error_buf_size)) {
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_FUTURE_CANCEL_READ:
        { // 0x18 t:<typeidx> async?:<async?>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_cancel_read_write.future_type_idx =
                (uint32_t)type_idx;

            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.future_cancel_read_write.async = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.future_cancel_read_write.async = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid async? tag: %02x", b);
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_FUTURE_CANCEL_WRITE:
        { // 0x19 t:<typeidx> async?:<async?>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_cancel_read_write.future_type_idx =
                (uint32_t)type_idx;

            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.future_cancel_read_write.async = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.future_cancel_read_write.async = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid async? tag: %02x", b);
                return false;
            }

            break;
        }

        case WASM_COMP_CANON_FUTURE_DROP_READABLE:
        { // 0x1a t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_drop_readable_writable.future_type_idx =
                (uint32_t)type_idx;

            break;
        }

        case WASM_COMP_CANON_FUTURE_DROP_WRITABLE:
        { // 0x1b t:<typeidx>
            uint64_t type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &type_idx, error_buf,
                          error_buf_size)) {
                return false;
            }
            out->canon_data.future_drop_readable_writable.future_type_idx =
                (uint32_t)type_idx;

            break;
        }

        case WASM_COMP_CANON_ERROR_CONTEXT_NEW: // 0x1c opts:<opts>
        case WASM_COMP_CANON_ERROR_CONTEXT_DEBUG:
        { // 0x1d opts:<opts>
            if (!parse_canon_opts(
                    &p, end,
                    &out->canon_data.error_context_new_debug.canon_opts,
                    error_buf, error_buf_size)) {
                return false;
            }
            break;
        }

        case WASM_COMP_CANON_ERROR_CONTEXT_DROP: // 0x1e (no parameters)
        case WASM_COMP_CANON_WAITABLE_SET_NEW:
        { // 0x1f (no parameters)
            break;
        }

        case WASM_COMP_CANON_WAITABLE_SET_WAIT:
        { // 0x20 cancel?:<cancel?> m:<core:memidx>
            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.waitable_set_wait_poll.cancellable = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.waitable_set_wait_poll.cancellable = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid cancel? tag: %02x", b);
                return false;
            }

            uint64_t core_mem_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_mem_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read core mem idx");
                return false;
            }
            out->canon_data.waitable_set_wait_poll.mem_idx =
                (uint32_t)core_mem_idx;

            break;
        }

        case WASM_COMP_CANON_WAITABLE_SET_POLL:
        { // 0x21 cancel?:<cancel?> m:<core:memidx>
            uint8_t b = *p++;
            if (b == WASM_COMP_OPTIONAL_TRUE) {
                out->canon_data.waitable_set_wait_poll.cancellable = true;
            }
            else if (b == WASM_COMP_OPTIONAL_FALSE) {
                out->canon_data.waitable_set_wait_poll.cancellable = false;
            }
            else {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Invalid cancel? tag: %02x", b);
                return false;
            }

            uint64_t core_mem_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_mem_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read core mem idx");
                return false;
            }
            out->canon_data.waitable_set_wait_poll.mem_idx =
                (uint32_t)core_mem_idx;
            break;
        }

        case WASM_COMP_CANON_WAITABLE_SET_DROP: // 0x22 (no parameters)
        case WASM_COMP_CANON_WAITABLE_JOIN:
        { // 0x23 (no parameters)
            break;
        }

        case WASM_COMP_CANON_THREAD_SPAWN_REF:
        { // 0x40 ft:<typeidx>
            uint64_t func_type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &func_type_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read func type idx");
                return false;
            }
            out->canon_data.thread_spawn_ref.func_type_idx =
                (uint32_t)func_type_idx;
            break;
        }

        case WASM_COMP_CANON_THREAD_SPAWN_INDIRECT:
        { // 0x41 ft:<typeidx> tbl:<core:tableidx>
            uint64_t func_type_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &func_type_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read func type idx");
                return false;
            }
            out->canon_data.thread_spawn_indirect.func_type_idx =
                (uint32_t)func_type_idx;

            uint64_t core_table_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_table_idx,
                          error_buf, error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to read core table idx");
                return false;
            }
            out->canon_data.thread_spawn_indirect.table_idx =
                (uint32_t)core_table_idx;
            break;
        }

        case WASM_COMP_CANON_THREAD_AVAILABLE_PAR:
        { // 0x42 (no parameters)
            break;
        }

        default:
        {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Invalid canon tag: %02x", tag);
            return false;
        }
    }

    *payload = p;
    return true;
}

// Section 8: canons (canonical function definitions) section
bool
wasm_component_parse_canons_section(const uint8_t **payload,
                                    uint32_t payload_len,
                                    WASMComponentCanonSection *out,
                                    char *error_buf, uint32_t error_buf_size,
                                    uint32_t *consumed_len)
{
    if (!payload || !*payload || payload_len == 0 || !out) {
        set_error_buf_ex(error_buf, error_buf_size,
                         "Invalid payload or output pointer");
        if (consumed_len)
            *consumed_len = 0;
        return false;
    }

    const uint8_t *p = *payload;
    const uint8_t *end = *payload + payload_len;

    uint64_t canon_count = 0;
    if (!read_leb((uint8_t **)&p, end, 32, false, &canon_count, error_buf,
                  error_buf_size)) {
        if (consumed_len)
            *consumed_len = (uint32_t)(p - *payload);
        return false;
    }
    out->count = (uint32_t)canon_count;

    if (canon_count > 0) {
        out->canons =
            wasm_runtime_malloc(sizeof(WASMComponentCanon) * canon_count);
        if (!out->canons) {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Failed to allocate memory for canons");
            if (consumed_len)
                *consumed_len = (uint32_t)(p - *payload);
            return false;
        }

        // Initialize all canons to zero to avoid garbage data
        memset(out->canons, 0, sizeof(WASMComponentCanon) * canon_count);

        for (uint32_t i = 0; i < canon_count; ++i) {
            // Check bounds before reading tag
            if (p >= end) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Buffer overflow when reading canon tag");
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }

            if (!parse_single_canon(&p, end, &out->canons[i], error_buf,
                                    error_buf_size)) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to parse canon");
                // Free previously parsed canons to avoid leaks
                for (uint32_t j = 0; j < i; ++j) {
                    free_single_canon_allocs(&out->canons[j]);
                }
                wasm_runtime_free(out->canons);
                out->canons = NULL;
                out->count = 0;
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }
        }
    }

    if (consumed_len)
        *consumed_len = (uint32_t)(p - *payload);
    return true;
}

// Individual section free functions
void
wasm_component_free_canons_section(WASMComponentSection *section)
{
    if (!section || !section->parsed.canon_section
        || !section->parsed.canon_section->canons) {
        return;
    }

    WASMComponentCanonSection *canons_section = section->parsed.canon_section;

    for (uint32_t i = 0; i < canons_section->count; i++) {
        WASMComponentCanon *canon = &canons_section->canons[i];

        // Free canon options for each canon that has them
        switch (canon->tag) {
            case WASM_COMP_CANON_LIFT:
                if (canon->canon_data.lift.canon_opts) {
                    if (canon->canon_data.lift.canon_opts->canon_opts) {
                        wasm_runtime_free(
                            canon->canon_data.lift.canon_opts->canon_opts);
                    }
                    wasm_runtime_free(canon->canon_data.lift.canon_opts);
                }
                break;

            case WASM_COMP_CANON_LOWER:
                if (canon->canon_data.lower.canon_opts) {
                    if (canon->canon_data.lower.canon_opts->canon_opts) {
                        wasm_runtime_free(
                            canon->canon_data.lower.canon_opts->canon_opts);
                    }
                    wasm_runtime_free(canon->canon_data.lower.canon_opts);
                }
                break;

            case WASM_COMP_CANON_TASK_RETURN:
                if (canon->canon_data.task_return.result_list) {
                    if (canon->canon_data.task_return.result_list->results) {
                        wasm_runtime_free(
                            canon->canon_data.task_return.result_list->results);
                    }
                    wasm_runtime_free(
                        canon->canon_data.task_return.result_list);
                }
                if (canon->canon_data.task_return.canon_opts) {
                    if (canon->canon_data.task_return.canon_opts->canon_opts) {
                        wasm_runtime_free(canon->canon_data.task_return
                                              .canon_opts->canon_opts);
                    }
                    wasm_runtime_free(canon->canon_data.task_return.canon_opts);
                }
                break;

            case WASM_COMP_CANON_STREAM_READ:
            case WASM_COMP_CANON_STREAM_WRITE:
                if (canon->canon_data.stream_read_write.canon_opts) {
                    if (canon->canon_data.stream_read_write.canon_opts
                            ->canon_opts) {
                        wasm_runtime_free(canon->canon_data.stream_read_write
                                              .canon_opts->canon_opts);
                    }
                    wasm_runtime_free(
                        canon->canon_data.stream_read_write.canon_opts);
                }
                break;

            case WASM_COMP_CANON_FUTURE_READ:
            case WASM_COMP_CANON_FUTURE_WRITE:
                if (canon->canon_data.future_read_write.canon_opts) {
                    if (canon->canon_data.future_read_write.canon_opts
                            ->canon_opts) {
                        wasm_runtime_free(canon->canon_data.future_read_write
                                              .canon_opts->canon_opts);
                    }
                    wasm_runtime_free(
                        canon->canon_data.future_read_write.canon_opts);
                }
                break;

            case WASM_COMP_CANON_ERROR_CONTEXT_NEW:
            case WASM_COMP_CANON_ERROR_CONTEXT_DEBUG:
                if (canon->canon_data.error_context_new_debug.canon_opts) {
                    if (canon->canon_data.error_context_new_debug.canon_opts
                            ->canon_opts) {
                        wasm_runtime_free(
                            canon->canon_data.error_context_new_debug
                                .canon_opts->canon_opts);
                    }
                    wasm_runtime_free(
                        canon->canon_data.error_context_new_debug.canon_opts);
                }
                break;

            default:
                // Other canon types don't have nested allocations
                break;
        }
    }

    // Free the canons array itself
    wasm_runtime_free(canons_section->canons);
    canons_section->canons = NULL;
    canons_section->count = 0;
    // Free the section struct and null the pointer for consistency with other
    // sections
    wasm_runtime_free(canons_section);
    section->parsed.canon_section = NULL;
}
