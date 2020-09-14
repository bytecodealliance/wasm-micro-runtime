/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_reloc.h"

#define R_X86_64_64     1   /* Direct 64 bit  */
#define R_X86_64_PC32   2   /* PC relative 32 bit signed */
#define R_X86_64_PLT32  4   /* 32 bit PLT address */
#define R_X86_64_32     10  /* Direct 32 bit zero extended */
#define R_X86_64_32S    11  /* Direct 32 bit sign extended */

#define IMAGE_REL_AMD64_REL32 4 /* The 32-bit relative address from
                                   the byte following the relocation */

void __divdi3();
void __udivdi3();
void __moddi3();
void __umoddi3();

static SymbolMap target_sym_map[] = {
    REG_COMMON_SYMBOLS
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
    snprintf(target_buf, target_buf_size, "x86_64");
}

static uint32
get_plt_item_size()
{
    /* size of mov instruction and jmp instruction */
    return 12;
}

uint32
get_plt_table_size()
{
    return get_plt_item_size() * (sizeof(target_sym_map) / sizeof(SymbolMap));
}

void
init_plt_table(uint8 *plt)
{
    uint32 i, num = sizeof(target_sym_map) / sizeof(SymbolMap);
    for (i = 0; i < num; i++) {
        uint8 *p = plt;
        /* mov symbol_addr, rax */
        *p++ = 0x48;
        *p++ = 0xB8;
        *(uint64*)p = (uint64)(uintptr_t)target_sym_map[i].symbol_addr;
        p += sizeof(uint64);
        /* jmp rax */
        *p++ = 0xFF;
        *p++ = 0xE0;
        plt += get_plt_item_size();
    }
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
        case R_X86_64_64:
        {
            intptr_t value;

            CHECK_RELOC_OFFSET(sizeof(void*));
            value = *(intptr_t*)(target_section_addr + (uint32)reloc_offset);
            *(uint8**)(target_section_addr + reloc_offset)
                = (uint8*)symbol_addr + reloc_addend + value;   /* S + A */
            break;
        }
        case R_X86_64_PC32:
        {
            intptr_t target_addr = (intptr_t)   /* S + A - P */
                                   ((uint8*)symbol_addr + reloc_addend
                                    - (target_section_addr + reloc_offset));

            CHECK_RELOC_OFFSET(sizeof(int32));
            if ((int32)target_addr != target_addr) {
                set_error_buf(error_buf, error_buf_size,
                              "AOT module load failed: "
                              "relocation truncated to fit R_X86_64_PC32 failed. "
                              "Try using wamrc with --size-level=1 option.");
                return false;
            }

            *(int32*)(target_section_addr + reloc_offset) = (int32)target_addr;
            break;
        }
        case R_X86_64_32:
        case R_X86_64_32S:
        {
            char buf[128];
            uintptr_t target_addr = (uintptr_t) /* S + A */
                                    ((uint8*)symbol_addr + reloc_addend);

            CHECK_RELOC_OFFSET(sizeof(int32));

            if ((reloc_type == R_X86_64_32
                 && (uint32)target_addr != (uint64)target_addr)
                || (reloc_type == R_X86_64_32S
                    && (int32)target_addr != (int64)target_addr)) {
                snprintf(buf, sizeof(buf),
                        "AOT module load failed: "
                        "relocation truncated to fit %s failed. "
                        "Try using wamrc with --size-level=1 option.",
                        reloc_type == R_X86_64_32
                        ? "R_X86_64_32" : "R_X86_64_32S");
                set_error_buf(error_buf, error_buf_size, buf);
                return false;
            }

            *(int32*)(target_section_addr + reloc_offset) = (int32)target_addr;
            break;
        }
        case R_X86_64_PLT32:
        {
            uint8 *plt;
            intptr_t target_addr = 0;

            CHECK_RELOC_OFFSET(sizeof(int32));

            if (symbol_index >= 0) {
                plt = (uint8*)module->code + module->code_size - get_plt_table_size()
                      + get_plt_item_size() * symbol_index;
                target_addr = (intptr_t)   /* L + A - P */
                              (plt + reloc_addend
                               - (target_section_addr + reloc_offset));
            }
            else {
                target_addr = (intptr_t)   /* L + A - P */
                              ((uint8*)symbol_addr + reloc_addend
                               - (target_section_addr + reloc_offset));
            }

            if ((int32)target_addr != target_addr) {
                set_error_buf(error_buf, error_buf_size,
                              "AOT module load failed: "
                              "relocation truncated to fit R_X86_64_PC32 failed. "
                              "Try using wamrc with --size-level=1 option.");
                return false;
            }
#ifdef BH_PLATFORM_WINDOWS
            target_addr -= sizeof(int32);
#endif
            *(int32*)(target_section_addr + reloc_offset) = (int32)target_addr;
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

