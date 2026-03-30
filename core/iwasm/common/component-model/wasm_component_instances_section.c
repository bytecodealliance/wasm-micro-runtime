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

// Section 5: instances section
// Binary.md: instance ::= ie:<instanceexpr> => (instance ie)
// instanceexpr ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>) =>
// (instantiate c arg*) instantiatearg ::= n:<name> si:<sortidx> => (with n si)
bool
wasm_component_parse_instances_section(const uint8_t **payload,
                                       uint32_t payload_len,
                                       WASMComponentInstSection *out,
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
            wasm_runtime_malloc(sizeof(WASMComponentInst) * instance_count);
        if (!out->instances) {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Failed to allocate memory for instances");
            if (consumed_len)
                *consumed_len = (uint32_t)(p - *payload);
            return false;
        }

        // Initialize all instances to zero to avoid garbage data
        memset(out->instances, 0, sizeof(WASMComponentInst) * instance_count);

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
                    // 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
                    uint64_t component_idx = 0;
                    if (!read_leb((uint8_t **)&p, end, 32, false,
                                  &component_idx, error_buf, error_buf_size)) {
                        if (consumed_len)
                            *consumed_len = (uint32_t)(p - *payload);
                        return false;
                    }
                    out->instances[i].expression.with_args.idx =
                        (uint32_t)component_idx;

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
                                             "component instantiate args");
                            if (consumed_len)
                                *consumed_len = (uint32_t)(p - *payload);
                            return false;
                        }

                        // Initialize args to zero
                        memset(out->instances[i].expression.with_args.args, 0,
                               sizeof(WASMComponentInstArg) * arg_len);

                        for (uint32_t j = 0; j < arg_len; ++j) {
                            // Parse core:name (LEB128 length + UTF-8 bytes)
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

                            // si:<sortidx> - this is a component-level sort
                            // index (non-core)
                            out->instances[i]
                                .expression.with_args.args[j]
                                .idx.sort_idx = wasm_runtime_malloc(
                                sizeof(WASMComponentSortIdx));
                            if (!out->instances[i]
                                     .expression.with_args.args[j]
                                     .idx.sort_idx) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Failed to allocate memory "
                                                 "for component arg sort idx");
                                free_core_name(core_name);
                                wasm_runtime_free(core_name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }
                            // Zero-initialize sort_idx
                            memset(out->instances[i]
                                       .expression.with_args.args[j]
                                       .idx.sort_idx,
                                   0, sizeof(WASMComponentSortIdx));

                            // Parse component sort index
                            bool status = parse_sort_idx(
                                &p, end,
                                out->instances[i]
                                    .expression.with_args.args[j]
                                    .idx.sort_idx,
                                error_buf, error_buf_size, false);
                            if (!status) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Failed to parse component arg sort idx");
                                free_core_name(core_name);
                                wasm_runtime_free(core_name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }
                        }
                    }
                    else {
                        out->instances[i].expression.with_args.args = NULL;
                    }
                    break;
                }
                case WASM_COMP_INSTANCE_EXPRESSION_WITHOUT_ARGS:
                {
                    // 0x01 e*:vec(<inlineexport>) => e*
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
                                             "component inline exports");
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
                            // inlineexport ::= n:<exportname> si:<sortidx>
                            WASMComponentCoreName *name = wasm_runtime_malloc(
                                sizeof(WASMComponentCoreName));
                            if (!name) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Failed to allocate memory "
                                                 "for component export name");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            // Parse export name (component-level name)
                            bool name_parse_success = parse_core_name(
                                &p, end, &name, error_buf, error_buf_size);
                            if (!name_parse_success) {
                                free_core_name(name);
                                wasm_runtime_free(name);
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }

                            out->instances[i]
                                .expression.without_args.inline_expr[j]
                                .name = name;

                            // Parse component sort index
                            WASMComponentSortIdx *sort_idx =
                                wasm_runtime_malloc(
                                    sizeof(WASMComponentSortIdx));
                            if (!sort_idx) {
                                set_error_buf_ex(error_buf, error_buf_size,
                                                 "Failed to allocate memory "
                                                 "for component sort idx");
                                if (consumed_len)
                                    *consumed_len = (uint32_t)(p - *payload);
                                return false;
                            }
                            // Zero-initialize sort_idx
                            memset(sort_idx, 0, sizeof(WASMComponentSortIdx));

                            bool status =
                                parse_sort_idx(&p, end, sort_idx, error_buf,
                                               error_buf_size, false);
                            if (!status) {
                                set_error_buf_ex(
                                    error_buf, error_buf_size,
                                    "Failed to parse component sort idx");
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
                    set_error_buf_ex(error_buf, error_buf_size,
                                     "Unknown instance expression tag: 0x%02X",
                                     tag);
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
wasm_component_free_instances_section(WASMComponentSection *section)
{
    if (!section || !section->parsed.instance_section)
        return;

    WASMComponentInstSection *instance_sec = section->parsed.instance_section;
    if (instance_sec->instances) {
        for (uint32_t j = 0; j < instance_sec->count; ++j) {
            WASMComponentInst *instance = &instance_sec->instances[j];

            switch (instance->instance_expression_tag) {
                case WASM_COMP_INSTANCE_EXPRESSION_WITH_ARGS:
                    if (instance->expression.with_args.args) {
                        for (uint32_t k = 0;
                             k < instance->expression.with_args.arg_len; ++k) {
                            WASMComponentInstArg *arg =
                                &instance->expression.with_args.args[k];

                            // Free component name
                            if (arg->name) {
                                free_core_name(arg->name);
                                wasm_runtime_free(arg->name);
                                arg->name = NULL;
                            }

                            // Free component sort index
                            if (arg->idx.sort_idx) {
                                if (arg->idx.sort_idx->sort) {
                                    wasm_runtime_free(arg->idx.sort_idx->sort);
                                    arg->idx.sort_idx->sort = NULL;
                                }
                                wasm_runtime_free(arg->idx.sort_idx);
                                arg->idx.sort_idx = NULL;
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

                            // Free component export name
                            if (inline_export->name) {
                                free_core_name(inline_export->name);
                                wasm_runtime_free(inline_export->name);
                                inline_export->name = NULL;
                            }

                            // Free component sort index
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
        wasm_runtime_free(instance_sec->instances);
        instance_sec->instances = NULL;
    }
    wasm_runtime_free(instance_sec);
    section->parsed.instance_section = NULL;
}
