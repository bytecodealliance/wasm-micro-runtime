/*
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "dynlink_types.h"
#include "bh_assert.h"

void
dynlink_sections_deinit(DynLinkSections *sections)
{
    bh_assert(sections);

    if (sections->needed.entries) {
        wasm_runtime_free(sections->needed.entries);
    }
    if (sections->export_info.entries) {
        wasm_runtime_free(sections->export_info.entries);
    }

    memset(sections, 0, sizeof(*sections));
}