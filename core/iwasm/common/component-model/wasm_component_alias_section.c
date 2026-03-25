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

bool parse_single_alias(const uint8_t **payload, const uint8_t *end, WASMComponentAliasDefinition *out, char *error_buf, uint32_t error_buf_size) {
    const uint8_t *p = *payload;

    out->sort = wasm_runtime_malloc(sizeof(WASMComponentSort));
    if (!out->sort) {
        set_error_buf_ex(error_buf, error_buf_size, "Failed to allocate memory for alias sort");
        return false;
    }

    // Parse the sort using the reusable parse_sort method
    if (!parse_sort(&p, end, out->sort, error_buf, error_buf_size, false)) {
        return false;
    }

    // Read tag
    uint8_t tag = *p++;

    // Parse alias target using switch
    switch (tag) {
        case WASM_COMP_ALIAS_TARGET_EXPORT: {
            uint64_t instance_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &instance_idx, error_buf, error_buf_size)) {
                return false;
            }
            WASMComponentCoreName *name = NULL;
            if (!parse_core_name(&p, end, &name, error_buf, error_buf_size)) {
                return false;
            }
            out->alias_target_type = WASM_COMP_ALIAS_TARGET_EXPORT;
            out->target.exported.instance_idx = (uint32_t)instance_idx;
            out->target.exported.name = name;
            break;
        }
        case WASM_COMP_ALIAS_TARGET_CORE_EXPORT: {
            uint64_t core_instance_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &core_instance_idx, error_buf, error_buf_size)) {
                return false;
            }
            WASMComponentCoreName *core_name = NULL;
            if (!parse_core_name(&p, end, &core_name, error_buf, error_buf_size)) {
                return false;
            }
            out->alias_target_type = WASM_COMP_ALIAS_TARGET_CORE_EXPORT;
            out->target.core_exported.instance_idx = (uint32_t)core_instance_idx;
            out->target.core_exported.name = core_name;
            break;
        }
        case WASM_COMP_ALIAS_TARGET_OUTER: {
            uint64_t outer_ct = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &outer_ct, error_buf, error_buf_size)) {
                return false;
            }
            uint64_t outer_idx = 0;
            if (!read_leb((uint8_t **)&p, end, 32, false, &outer_idx, error_buf, error_buf_size)) {
                return false;
            }
            out->alias_target_type = WASM_COMP_ALIAS_TARGET_OUTER;
            out->target.outer.ct = (uint32_t)outer_ct;
            out->target.outer.idx = (uint32_t)outer_idx;

            bool valid_outer_sort =
                (out->sort->sort == WASM_COMP_SORT_TYPE)
                || (out->sort->sort == WASM_COMP_SORT_COMPONENT)
                || (out->sort->sort == WASM_COMP_SORT_CORE_SORT
                    && out->sort->core_sort == WASM_COMP_CORE_SORT_MODULE);
            if (!valid_outer_sort) {
                set_error_buf_ex(error_buf, error_buf_size, "Outer alias sort must be type, component, or core module");
                return false;
            }
            break;
        }
        default:
            snprintf(error_buf, error_buf_size, "Unknown alias target type: 0x%02X", tag);
            return false;
    }

    *payload = p;
    return true;
}

// Section 6: alias section
bool wasm_component_parse_alias_section(const uint8_t **payload, uint32_t payload_len, WASMComponentAliasSection *out, char *error_buf, uint32_t error_buf_size, uint32_t *consumed_len) {
    if (!payload || !*payload || payload_len == 0 || !out) {
        set_error_buf_ex(error_buf, error_buf_size, "Invalid payload or output pointer");
        if (consumed_len) *consumed_len = 0;
        return false;
    }

    const uint8_t *p = *payload;
    const uint8_t *end = *payload + payload_len;
    uint32_t alias_count = 0;

    // Read alias count
    uint64_t alias_count_leb = 0;
    if (!read_leb((uint8_t **)&p, end, 32, false, &alias_count_leb, error_buf, error_buf_size)) {
        if (consumed_len) *consumed_len = (uint32_t)(p - *payload);
        return false;
    }

    alias_count = (uint32_t)alias_count_leb;

    out->count = alias_count;
    if (alias_count > 0) {
        out->aliases = wasm_runtime_malloc(sizeof(WASMComponentAliasDefinition) * alias_count);
        if (!out->aliases) {
            if (consumed_len) *consumed_len = (uint32_t)(p - *payload);
            return false;
        }
        // Zero-initialize the aliases array
        memset(out->aliases, 0, sizeof(WASMComponentAliasDefinition) * alias_count);

        for (uint32_t i = 0; i < alias_count; ++i) {
            // Allocate memory for the sort field
            if (!parse_single_alias(&p, end, &out->aliases[i], error_buf, error_buf_size)) {
                if (consumed_len) *consumed_len = (uint32_t)(p - *payload);
                set_error_buf_ex(error_buf, error_buf_size, "Failed to parse alias %d", i);
                return false;
            }
        }
    }

    if (consumed_len) *consumed_len = (uint32_t)(p - *payload);

    // If binaries use alias ids, this parser will need to be extended.
    return true;
}

// Individual section free functions
void wasm_component_free_alias_section(WASMComponentSection *section) {
    if (!section || !section->parsed.alias_section) return;
    
    WASMComponentAliasSection *alias_sec = section->parsed.alias_section;
    if (alias_sec->aliases) {
        for (uint32_t j = 0; j < alias_sec->count; ++j) {
            WASMComponentAliasDefinition *alias = &alias_sec->aliases[j];
            
            // Free sort
            if (alias->sort) {
                wasm_runtime_free(alias->sort);
                alias->sort = NULL;
            }
            
            // Free target-specific data
            switch (alias->alias_target_type) {
                case WASM_COMP_ALIAS_TARGET_EXPORT:
                    if (alias->target.exported.name) {
                        free_core_name(alias->target.exported.name);
                        wasm_runtime_free(alias->target.exported.name);
                        alias->target.exported.name = NULL;
                    }
                    break;
                case WASM_COMP_ALIAS_TARGET_CORE_EXPORT:
                    if (alias->target.core_exported.name) {
                        free_core_name(alias->target.core_exported.name);
                        wasm_runtime_free(alias->target.core_exported.name);
                        alias->target.core_exported.name = NULL;
                    }
                    break;
                case WASM_COMP_ALIAS_TARGET_OUTER:
                    // No dynamic allocations for outer aliases
                    break;
            }
        }
        wasm_runtime_free(alias_sec->aliases);
        alias_sec->aliases = NULL;
    }
    wasm_runtime_free(alias_sec);
    section->parsed.alias_section = NULL;
} 
