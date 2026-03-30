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

// Section 10: imports section
bool
wasm_component_parse_imports_section(const uint8_t **payload,
                                     uint32_t payload_len,
                                     WASMComponentImportSection *out,
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

    // import ::= in:<importname'> ed:<externdesc> => (import in ed)
    // Read the count of imports (LEB128-encoded)
    uint64_t import_count_leb = 0;
    if (!read_leb((uint8_t **)&p, end, 32, false, &import_count_leb, error_buf,
                  error_buf_size)) {
        if (consumed_len)
            *consumed_len = (uint32_t)(p - *payload);
        return false;
    }

    uint32_t import_count = (uint32_t)import_count_leb;
    out->count = import_count;

    if (import_count > 0) {
        out->imports =
            wasm_runtime_malloc(sizeof(WASMComponentImport) * import_count);
        if (!out->imports) {
            set_error_buf_ex(error_buf, error_buf_size,
                             "Failed to allocate memory for imports");
            if (consumed_len)
                *consumed_len = (uint32_t)(p - *payload);
            return false;
        }

        // Initialize all imports to zero to avoid garbage data
        memset(out->imports, 0, sizeof(WASMComponentImport) * import_count);

        for (uint32_t i = 0; i < import_count; ++i) {
            // importname' ::= 0x00 len:<u32> in:<importname> => in (if len =
            // |in|)
            //             | 0x01 len:<u32> in:<importname> vs:<versionsuffix'>
            //             => in vs (if len = |in|)
            // Parse import name (simple or versioned)
            WASMComponentImportName *import_name =
                wasm_runtime_malloc(sizeof(WASMComponentImportName));
            if (!import_name) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to allocate memory for import_name");
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }
            // Initialize the struct to zero to avoid garbage data
            memset(import_name, 0, sizeof(WASMComponentImportName));

            bool status = parse_component_import_name(
                &p, end, import_name, error_buf, error_buf_size);
            if (!status) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to parse component name for import %u",
                                 i);
                wasm_runtime_free(import_name);
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }

            // externdesc ::= 0x00 0x11 i:<core:typeidx> => (core module (type
            // i))
            //            | 0x01 i:<typeidx> => (func (type i))
            //            | 0x02 b:<valuebound> => (value b)
            //            | 0x03 b:<typebound> => (type b)
            //            | 0x04 i:<typeidx> => (component (type i))
            //            | 0x05 i:<typeidx> => (instance (type i))
            // Parse externdesc (core module, func, value, type, component,
            // instance)
            WASMComponentExternDesc *extern_desc =
                wasm_runtime_malloc(sizeof(WASMComponentExternDesc));
            if (!extern_desc) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to allocate memory for extern_desc");
                wasm_runtime_free(import_name);
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }
            // Initialize the struct to zero to avoid garbage data
            memset(extern_desc, 0, sizeof(WASMComponentExternDesc));

            status = parse_extern_desc(&p, end, extern_desc, error_buf,
                                       error_buf_size);
            if (!status) {
                set_error_buf_ex(error_buf, error_buf_size,
                                 "Failed to parse extern_desc for import %u",
                                 i);
                wasm_runtime_free(extern_desc);
                wasm_runtime_free(import_name);
                if (consumed_len)
                    *consumed_len = (uint32_t)(p - *payload);
                return false;
            }

            // Store the parsed import (importname' + externdesc)
            out->imports[i].import_name = import_name;
            out->imports[i].extern_desc = extern_desc;
        }
    }

    if (consumed_len)
        *consumed_len = (uint32_t)(p - *payload);
    return true;
}

// Individual section free functions
void
wasm_component_free_imports_section(WASMComponentSection *section)
{
    if (!section || !section->parsed.import_section)
        return;

    WASMComponentImportSection *import_sec = section->parsed.import_section;
    if (import_sec->imports) {
        for (uint32_t j = 0; j < import_sec->count; ++j) {
            WASMComponentImport *import = &import_sec->imports[j];

            // Free import name
            if (import->import_name) {
                free_component_import_name(import->import_name);
                wasm_runtime_free(import->import_name);
                import->import_name = NULL;
            }

            // Free extern desc
            if (import->extern_desc) {
                free_extern_desc(import->extern_desc);
                wasm_runtime_free(import->extern_desc);
                import->extern_desc = NULL;
            }
        }
        wasm_runtime_free(import_sec->imports);
        import_sec->imports = NULL;
    }
    wasm_runtime_free(import_sec);
    section->parsed.import_section = NULL;
}
