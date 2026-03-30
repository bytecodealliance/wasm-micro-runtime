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

// Section 2: core:instance ::= ie:<core:instanceexpr> => (instance ie)
// core:instanceexpr ::= 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>) =>
// (instantiate m arg*) core:instantiatearg ::= n:<core:name> 0x12
// i:<instanceidx> => (with n (instance i))
bool
wasm_component_parse_core_instance_section(const uint8_t **payload,
                                           uint32_t payload_len,
                                           WASMComponentCoreInstSection *out,
                                           char *error_buf,
                                           uint32_t error_buf_size,
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

    uint64_t instance_count = 0;
    if (!read_leb((uint8_t **)&p, end, 32, false, &instance_count, error_buf,
                  error_buf_size)) {
        if (consumed_len)
            *consumed_len = (uint32_t)(p - *payload);
        return false;
    }
    out->count = (uint32_t)instance_count;

    if (instance_count > 0) {
        out->instances =
            wasm_runtime_malloc(sizeof(WASMComponentCoreInst) * instance_count);
        if (!out->instances) {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Failed to allocate memory for core instances");
            if (consumed_len)
                *consumed_len = (uint32_t)(p - *payload);
            return false;
        }

        // Initialize all instances to zero to avoid garbage data
        memset(out->instances, 0,
               sizeof(WASMComponentCoreInst) * instance_count);

        for (uint32_t i = 0; i < instance_count; ++i) {
            // Check bounds before reading tag
            if (p >= end) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Buffer overflow when reading instance tag");
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }

            uint8_t tag = *p++;
            out->instances[i].instance_expression_tag = tag;
            switch (tag) {
                case WASM_COMP_INSTANCE_EXPRESSION_WITH_ARGS:
                {
                    // 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
                    uint64_t module_idx = 0;
                    if (!read_leb((uint8_t **)&p, end, 32, false, &module_idx,
                                  error_buf, error_buf_size)) {
                        if (consumed_len)
                            *consumed_len = (uint32_t)(p - *payload);
                        return false;
                    }
                    out->instances[i].expression.with_args.idx =
                        (uint32_t)module_idx;

                    uint64_t arg_len = 0;
                    if (!read_leb((uint8_t **)&p, end, 32, false, &arg_len,
                                  error_buf, error_buf_size)) {
                        if (consumed_len)
                            *consumed_len = (uint32_t)(p - *payload);
                        return false;
                    }
                    out->instances[i].expression.with_args.arg_len =
                        (uint32_t)arg_len;

                    if (arg_len > 0) {
                        out->instances[i].expression.with_args.args =
                            wasm_runtime_malloc(sizeof(WASMComponentInstArg)
                                                * arg_len);
                        if (!out->instances[i].expression.with_args.args) {
                            set_error_buf_ex(error_buf, error_buf_size,
                                             "Failed to allocate memory for "
                                             "core instantiate args");
                            if (consumed_len)
                                *consumed_len = (uint32_t)(p - *payload);
                            return false;
                        }

                        // Initialize args to zero
                        memset(out->instances[i].expression.with_args.args, 0,
                               sizeof(WASMComponentInstArg) * arg_len);

                        for (uint32_t j = 0; j < arg_len; ++j) {
                            // core:instantiatearg ::= n:<core:name> 0x12
                            // i:<instanceidx> Parse core:name (LEB128 length +
                            // UTF-8 bytes)

                            // Check bounds before parsing name
                            if (p >= end) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Buffer overflow when parsing core name");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            WASMComponentCoreName *core_name = NULL;
                            if (!parse_core_name(&p, end, &core_name, error_buf,
                                                 error_buf_size)) {
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // Store the name in the instantiate arg structure
                            out->instances[i]
                                .expression.with_args.args[j]
                                .name = core_name;

                            // Check bounds before reading 0x12
                            if (p >= end) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Buffer overflow when reading 0x12 flag");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // Verify 0x12 for core:instantiatearg
                            if (*p++ != 0x12) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Failed to read 0x12 flag identifier for "
                                    "core instantiatearg field");
                                free_core_name(core_name);
                                wasm_runtime_free(core_name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // i:<instanceidx> - this is a core instance index
                            uint64_t instance_idx = 0;
                            if (!read_leb((uint8_t **)&p, end, 32, false,
                                          &instance_idx, error_buf,
                                          error_buf_size)) {
                                free_core_name(core_name);
                                wasm_runtime_free(core_name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }
                            out->instances[i]
                                .expression.with_args.args[j]
                                .idx.instance_idx = (uint32_t)instance_idx;
                        }
                    }
                    else {
                        out->instances[i].expression.with_args.args = NULL;
                    }
                    break;
                }
                case WASM_COMP_INSTANCE_EXPRESSION_WITHOUT_ARGS:
                {
                    // 0x01 e*:vec(<core:inlineexport>) => e*
                    uint64_t inline_expr_len = 0;
                    if (!read_leb((uint8_t **)&p, end, 32, false,
                                  &inline_expr_len, error_buf,
                                  error_buf_size)) {
                        if (consumed_len)
                            *consumed_len = (uint32_t)(p - *payload);
                        return false;
                    }

                    out->instances[i].expression.without_args.inline_expr_len =
                        (uint32_t)inline_expr_len;

                    if (inline_expr_len > 0) {
                        out->instances[i].expression.without_args.inline_expr =
                            wasm_runtime_malloc(
                                sizeof(WASMComponentInlineExport)
                                * inline_expr_len);
                        if (!out->instances[i]
                                 .expression.without_args.inline_expr) {
                            set_error_buf_ex(error_buf, error_buf_size,
                                             "Failed to allocate memory for "
                                             "core inline exports");
                            if (consumed_len)
                                *consumed_len = (uint32_t)(p - *payload);
                            return false;
                        }

                        // Initialize inline exports to zero
                        memset(out->instances[i]
                                   .expression.without_args.inline_expr,
                               0,
                               sizeof(WASMComponentInlineExport)
                                   * inline_expr_len);

                        for (uint32_t j = 0; j < inline_expr_len; j++) {
                            // core:inlineexport ::= n:<core:name>
                            // si:<core:sortidx>
                            WASMComponentCoreName *name = NULL;

                            // Debug: Check if we're about to go out of bounds
                            if (p >= end) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Buffer overflow in inline "
                                                 "exports parsing");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // Parse core:name using the existing
                            // parse_core_name function
                            bool name_parse_success = parse_core_name(
                                &p, end, &name, error_buf, error_buf_size);
                            if (!name_parse_success) {
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            out->instances[i]
                                .expression.without_args.inline_expr[j]
                                .name = name;

                            // Check bounds before parsing sort index
                            if (p >= end) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Buffer overflow when parsing "
                                                 "core sort idx");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // Parse core:sortidx (must use is_core=true for
                            // core instances)
                            WASMComponentSortIdx *sort_idx =
                                wasm_runtime_malloc(
                                    sizeof(WASMComponentSortIdx));
                            if (!sort_idx) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Failed to allocate memory "
                                                 "for core sort idx");
                                free_core_name(name);
                                wasm_runtime_free(name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }
                            // Zero-initialize sort_idx
                            memset(sort_idx, 0, sizeof(WASMComponentSortIdx));

                            bool status =
                                parse_sort_idx(&p, end, sort_idx, error_buf,
                                               error_buf_size, true);
                            if (!status) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Failed to parse core sort idx");
                                wasm_runtime_free(sort_idx);
                                free_core_name(name);
                                wasm_runtime_free(name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            out->instances[i]
                                .expression.without_args.inline_expr[j]
                                .sort_idx = sort_idx;
                        }
                    }
                    else {
                        out->instances[i].expression.without_args.inline_expr =
                            NULL;
                    }

                    break;
                }
                default:
                {
                    set_error_buf_ex(
                        error_buf, error_buf_size,
                        "Unknown core instance expression tag: 0x%02X", tag);
                    if (consumed_len)
                        *consumed_len = (uint32_t)(p - *payload);
                    return false;
                }
            }
        }
    }
    if (consumed_len)
        *consumed_len = payload_len;
    return true;
}

// Individual section free functions
void
wasm_component_free_core_instance_section(WASMComponentSection *section)
{
    if (!section || !section->parsed.core_instance_section)
        return;

    WASMComponentCoreInstSection *core_instance_sec =
        section->parsed.core_instance_section;
    if (core_instance_sec->instances) {
        for (uint32_t j = 0; j < core_instance_sec->count; ++j) {
            WASMComponentCoreInst *instance = &core_instance_sec->instances[j];

            switch (instance->instance_expression_tag) {
                case WASM_COMP_INSTANCE_EXPRESSION_WITH_ARGS:
                    if (instance->expression.with_args.args) {
                        for (uint32_t k = 0;
                             k < instance->expression.with_args.arg_len; ++k) {
                            WASMComponentInstArg *arg =
                                &instance->expression.with_args.args[k];

                            // Free core name
                            if (arg->name) {
                                free_core_name(arg->name);
                                wasm_runtime_free(arg->name);
                                arg->name = NULL;
                            }
                        }
                        wasm_runtime_free(instance->expression.with_args.args);
                        instance->expression.with_args.args = NULL;
                    }
                    break;

                case WASM_COMP_INSTANCE_EXPRESSION_WITHOUT_ARGS:
                    if (instance->expression.without_args.inline_expr) {
                        for (uint32_t k = 0;
                             k < instance->expression.without_args
                                     .inline_expr_len;
                             ++k) {
                            WASMComponentInlineExport *inline_export =
                                &instance->expression.without_args
                                     .inline_expr[k];

                            // Free core export name
                            if (inline_export->name) {
                                free_core_name(inline_export->name);
                                wasm_runtime_free(inline_export->name);
                                inline_export->name = NULL;
                            }

                            // Free core sort index
                            if (inline_export->sort_idx) {
                                if (inline_export->sort_idx->sort) {
                                    wasm_runtime_free(
                                        inline_export->sort_idx->sort);
                                    inline_export->sort_idx->sort = NULL;
                                }
                                wasm_runtime_free(inline_export->sort_idx);
                                inline_export->sort_idx = NULL;
                            }
                        }
                        wasm_runtime_free(
                            instance->expression.without_args.inline_expr);
                        instance->expression.without_args.inline_expr = NULL;
                    }
                    break;
            }
        }
        wasm_runtime_free(core_instance_sec->instances);
        core_instance_sec->instances = NULL;
    }
    wasm_runtime_free(core_instance_sec);
    section->parsed.core_instance_section = NULL;
}
