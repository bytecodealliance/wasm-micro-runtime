/*
 * Copyright (C) 2021 XiaoMi Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_reloc.h"

#define R_RISCV_CALL 18

static SymbolMap target_sym_map[] = { REG_COMMON_SYMBOLS };

void
get_current_target(char *target_buf, uint32 target_buf_size)
{
    snprintf(target_buf, target_buf_size, "riscv");
}

uint32
get_plt_item_size()
{
    return 16;
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
{}

bool
apply_relocation(AOTModule *module,
                 uint8 *target_section_addr,
                 uint32 target_section_size,
                 uint64 reloc_offset,
                 uint64 reloc_addend,
                 uint32 reloc_type,
                 void *symbol_addr,
                 int32 symbol_index,
                 char *error_buf,
                 uint32 error_buf_size)
{
    uint8 *P = target_section_addr + reloc_offset;
    uint32 hi20, lo12;

    hi20 = (uint32)((unsigned long)symbol_addr & 0xfffff000);
    lo12 = (uint32)(((unsigned long)symbol_addr & 0xfff) << 20);

    switch (reloc_type) {
        case R_RISCV_CALL:
            if ((uint32)(uintptr_t)symbol_addr != (uintptr_t)symbol_addr) {
                if (error_buf != NULL) {
                    snprintf(
                      error_buf, error_buf_size,
                      "Jump address exceeds 32-bit address space (0-4GB).");
                }
                return false;
            }

            /* lui  t0, hi20 */
            *(uint32 *)(P + 0) = (uint32)hi20 | 0x2b7;
            /* jalr ra, lo12(t0) */
            *(uint32 *)(P + 4) = (uint32)lo12 | 0x280e7;
            break;

        default:
            if (error_buf != NULL)
                snprintf(error_buf, error_buf_size,
                         "Load relocation section failed: "
                         "invalid relocation type %ld.",
                         reloc_type);
            return false;
    }

    return true;
}
