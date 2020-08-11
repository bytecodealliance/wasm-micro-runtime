/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_reloc.h"

#define R_386_32        1   /* Direct 32 bit  */
#define R_386_PC32      2   /* PC relative 32 bit */

void __divdi3();
void __udivdi3();
void __moddi3();
void __umoddi3();

static SymbolMap target_sym_map[] = {
    REG_COMMON_SYMBOLS
#if !defined(_WIN32) && !defined(_WIN32_)
    /* compiler-rt symbols that come from compiler(e.g. gcc) */
    REG_SYM(__divdi3),
    REG_SYM(__udivdi3),
    REG_SYM(__moddi3),
    REG_SYM(__umoddi3)
#endif
};

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

SymbolMap *
get_target_symbol_map(uint32 *sym_num)
{
    *sym_num = sizeof(target_sym_map) / sizeof(SymbolMap);
    return target_sym_map;
}

void
get_current_target(char *target_buf, uint32 target_buf_size)
{
    snprintf(target_buf, target_buf_size, "i386");
}

uint32
get_plt_table_size()
{
    return 0;
}

void
init_plt_table(uint8 *plt)
{
    (void)plt;
}

static bool
check_reloc_offset(uint32 target_section_size,
                   uint64 reloc_offset, uint32 reloc_data_size,
                   char *error_buf, uint32 error_buf_size)
{
    if (!(reloc_offset < (uint64)target_section_size
          && reloc_offset + reloc_data_size <= (uint64)target_section_size)) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid relocation offset.");
        return false;
    }
    return true;
}

bool
apply_relocation(AOTModule *module,
                 uint8 *target_section_addr, uint32 target_section_size,
                 uint64 reloc_offset, uint64 reloc_addend,
                 uint32 reloc_type, void *symbol_addr, int32 symbol_index,
                 char *error_buf, uint32 error_buf_size)
{
    switch (reloc_type) {
        case R_386_32:
        {
            intptr_t value;

            CHECK_RELOC_OFFSET(sizeof(void*));
            value = *(intptr_t*)(target_section_addr + (uint32)reloc_offset);
            *(uint8**)(target_section_addr + reloc_offset)
                = (uint8*)symbol_addr + reloc_addend + value;   /* S + A */
            break;
        }

        case R_386_PC32:
        {
            int32 value;

            CHECK_RELOC_OFFSET(sizeof(void*));
            value = *(int32*)(target_section_addr + (uint32)reloc_offset);
            *(uint32*)(target_section_addr + (uint32)reloc_offset) = (uint32)
                ((uint8*)symbol_addr + (uint32)reloc_addend
                 - (uint8*)(target_section_addr + (uint32)reloc_offset)
                 + value);  /* S + A - P */
            break;
        }

        default:
            if (error_buf != NULL)
                snprintf(error_buf, error_buf_size,
                         "Load relocation section failed: "
                         "invalid relocation type %d.",
                         reloc_type);
            return false;
    }
    return true;
}

