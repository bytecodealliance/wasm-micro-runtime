/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_reloc.h"

static SymbolMap target_sym_map[] = {
    /* clang-format off */
    REG_COMMON_SYMBOLS
    /* clang-format on */
};

void
get_current_target(char *target_buf, uint32 target_buf_size)
{
    snprintf(target_buf, target_buf_size, "loongarch");
}

uint32
get_plt_item_size(void)
{
    /* TODO */
    return 0;
}

SymbolMap *
get_target_symbol_map(uint32 *sym_num)
{
    *sym_num = sizeof(target_sym_map) / sizeof(SymbolMap);
    return target_sym_map;
}

uint32
get_plt_table_size()
{
    return get_plt_item_size() * (sizeof(target_sym_map) / sizeof(SymbolMap));
}

void
init_plt_table(uint8 *plt)
{
    /* TODO */
}

bool
apply_relocation(AOTModule *module, uint8 *target_section_addr,
                 uint32 target_section_size, uint64 reloc_offset,
                 int64 reloc_addend, uint32 reloc_type, void *symbol_addr,
                 int32 symbol_index, char *error_buf, uint32 error_buf_size)
{
    /* TODO */
    return false;
}
