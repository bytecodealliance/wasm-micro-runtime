/*
 * Copyright (C) 2025 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_reloc.h"

#define R_LARCH_B26 66
#define R_LARCH_PCALA_HI20 71
#define R_LARCH_PCALA_LO12 72
#define R_LARCH_PCALA64_LO20 73
#define R_LARCH_PCALA64_HI12 74
#define R_LARCH_GOT_PC_HI20 75
#define R_LARCH_GOT_PC_LO12 76
#define R_LARCH_GOT64_PC_LO20 77
#define R_LARCH_GOT64_PC_HI12 78
#define R_LARCH_CALL36 110

static SymbolMap target_sym_map[] = {
    /* clang-format off */
    REG_COMMON_SYMBOLS
    /* clang-format on */
};

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

void
get_current_target(char *target_buf, uint32 target_buf_size)
{
    snprintf(target_buf, target_buf_size, "loongarch");
}

uint32
get_plt_item_size(void)
{
#if __loongarch_grlen == 64
    /* 4*4 bytes instructions and 8 bytes symbol address */
    return 24;
#else
    /* TODO */
    return 0;
#endif
}

SymbolMap *
get_target_symbol_map(uint32 *sym_num)
{
    if (sym_num != NULL)
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
#if __loongarch_grlen == 64
    uint32 i, num = sizeof(target_sym_map) / sizeof(SymbolMap);

    for (i = 0; i < num; i++) {
        uint32 *p = (uint32 *)plt;
        *p++ = 0x1c00000d; /* pcaddu12i $t1, 0       */
        *p++ = 0x28c041ad; /* ld.d      $t1, $t1, 16 */
        *p++ = 0x4c0001a0; /* jr        $t1          */
        *p++ = 0x03400000; /* nop                    */
        /* symbol addr */
        *(uint64 *)p = (uint64)(uintptr_t)target_sym_map[i].symbol_addr;
        plt += get_plt_item_size();
    }
#else
    /* TODO */
#endif
}

typedef struct RelocTypeStrMap {
    uint32 reloc_type;
    char *reloc_str;
} RelocTypeStrMap;

#define RELOC_TYPE_MAP(reloc_type) \
    {                              \
        reloc_type, #reloc_type    \
    }

static RelocTypeStrMap reloc_type_str_maps[] = {
    RELOC_TYPE_MAP(R_LARCH_B26),
    RELOC_TYPE_MAP(R_LARCH_PCALA_HI20),
    RELOC_TYPE_MAP(R_LARCH_PCALA_LO12),
    RELOC_TYPE_MAP(R_LARCH_CALL36),
};

static const char *
reloc_type_to_str(uint32 reloc_type)
{
    uint32 i;

    for (i = 0; i < sizeof(reloc_type_str_maps) / sizeof(RelocTypeStrMap);
         i++) {
        if (reloc_type_str_maps[i].reloc_type == reloc_type)
            return reloc_type_str_maps[i].reloc_str;
    }

    return "Unknown_Reloc_Type";
}

static bool
check_reloc_offset(uint32 target_section_size, uint64 reloc_offset,
                   uint32 reloc_data_size, char *error_buf,
                   uint32 error_buf_size)
{
    if (!(reloc_offset < (uint64)target_section_size
          && reloc_offset + reloc_data_size <= (uint64)target_section_size)) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid relocation offset.");
        return false;
    }
    return true;
}

#define Page(expr) ((expr) & ~0xFFF)

/* Calculate the adjusted page delta between dest and PC. */
uint64
getLoongArchPageDelta(uint64 dest, uint64 pc, uint32 reloc_type)
{
    uint64 pcalau12i_pc;
    switch (reloc_type) {
        case R_LARCH_PCALA64_LO20:
        case R_LARCH_GOT64_PC_LO20:
            pcalau12i_pc = pc - 8;
            break;
        case R_LARCH_PCALA64_HI12:
        case R_LARCH_GOT64_PC_HI12:
            pcalau12i_pc = pc - 12;
            break;
        default:
            pcalau12i_pc = pc;
            break;
    }
    uint64 result = Page(dest) - Page(pcalau12i_pc);
    if (dest & 0x800)
        result += 0x1000 - 0x100000000;
    if (result & 0x80000000)
        result += 0x100000000;
    return result;
}

/* Extract bits v[begin:end], where range is inclusive. */
static uint32
extractBits(uint64 v, uint32 begin, uint32 end)
{
    return begin == 63 ? v >> end : (v & ((1ULL << (begin + 1)) - 1)) >> end;
}

static uint32
setD10k16(uint32 insn, uint32 imm)
{
    uint32 immLo = extractBits(imm, 15, 0);
    uint32 immHi = extractBits(imm, 25, 16);
    return (insn & 0xfc000000) | (immLo << 10) | immHi;
}

static uint32_t
setJ20(uint32 insn, uint32 imm)
{
    return (insn & 0xfe00001f) | (extractBits(imm, 19, 0) << 5);
}

static uint32_t
setK12(uint32 insn, uint32 imm)
{
    return (insn & 0xffc003ff) | (extractBits(imm, 11, 0) << 10);
}

static uint32_t
setK16(uint32_t insn, uint32_t imm)
{
    return (insn & 0xfc0003ff) | (extractBits(imm, 15, 0) << 10);
}

bool
apply_relocation(AOTModule *module, uint8 *target_section_addr,
                 uint32 target_section_size, uint64 reloc_offset,
                 int64 reloc_addend, uint32 reloc_type, void *symbol_addr,
                 int32 symbol_index, char *error_buf, uint32 error_buf_size)
{
    void *S = symbol_addr;
    int64 A = reloc_addend;
    uint8 *P = target_section_addr + reloc_offset;
    int32 insn = *(int32 *)P;
    char buf[128];
    int64 X;

    switch (reloc_type) {
        case R_LARCH_B26:
        case R_LARCH_CALL36: /* S + A - P */
        {
            if (reloc_type == R_LARCH_B26) {
                CHECK_RELOC_OFFSET(sizeof(int32));
            }
            else if (reloc_type == R_LARCH_CALL36) {
                CHECK_RELOC_OFFSET(sizeof(int64));
            }
            /* Negative symbol index means the symbol is an AOT function and we
             * suppose R_LARCH_{B26,CALL36} is able to address it, so apply the
             * relocation with the symbol directly. Otherwise, the symbol is a
             * runtime function or a native function whose address is probably
             * beyond of R_LARCH_{B26,CALL36}'s addressing range, so apply the
             * relocation with PLT.
             */
            if (symbol_index >= 0) {
                if (reloc_addend > 0) {
                    set_error_buf(
                        error_buf, error_buf_size,
                        "AOT module load failed: relocate to plt table "
                        "with reloc addend larger than 0 is unsupported.");
                    return false;
                }
                S = (uint8 *)module->code + module->code_size
                    - get_plt_table_size() + get_plt_item_size() * symbol_index;
            }
            if (reloc_type == R_LARCH_B26) {
                X = (int64)S + A - (int64)P;
                if (!(X >= (-128 * BH_MB) && X <= (128 * BH_MB - 4))) {
                    goto fail_addr_out_of_range;
                }
                if ((X & 3) != 0) {
                    goto fail_addr_not_4bytes_aligned;
                }
                *(int32 *)P = setD10k16(insn, X >> 2);
            }
            else if (reloc_type == R_LARCH_CALL36) {
                int32 jirl = *(int32 *)(P + 4);
                X = (int64)S + A - (int64)P;
                if (!(X >= (-128LL * BH_GB - 0x20000)
                      && X <= (128LL * BH_GB - 0x20000 - 4))) {
                    goto fail_addr_out_of_range;
                }
                if ((X & 3) != 0) {
                    goto fail_addr_not_4bytes_aligned;
                }
                uint32 hi20 = extractBits(X + (1 << 17), 37, 18);
                uint32 lo16 = extractBits(X, 17, 2);
                *(int32 *)P = setJ20(insn, hi20);
                *(int32 *)(P + 4) = setK16(jirl, lo16);
            }
            break;
        }
        case R_LARCH_PCALA_HI20:
        case R_LARCH_GOT_PC_HI20: /* GOT + G has been calculated as symbol_addr
                                   */
        {
            CHECK_RELOC_OFFSET(sizeof(int32));
            X = getLoongArchPageDelta((int64)S + A, (int64)P, reloc_type);
            /* Note: Like ld and lld, no overflow check. */
            *(int32 *)P = setJ20(insn, extractBits(X, 31, 12));
            break;
        }
        case R_LARCH_PCALA_LO12:
        case R_LARCH_GOT_PC_LO12: /* GOT + G has been calculated as symbol_addr
                                   */
        {
            CHECK_RELOC_OFFSET(sizeof(int32));
            *(int32 *)P = setK12(insn, extractBits((int64)S + A, 11, 0));
            break;
        }
        case R_LARCH_PCALA64_LO20:
        case R_LARCH_GOT64_PC_LO20: /* GOT + G has been calculated as
                                       symbol_addr */
        {
            CHECK_RELOC_OFFSET(sizeof(int32));
            X = getLoongArchPageDelta((int64)S + A, (int64)P, reloc_type);
            *(int32 *)P = setJ20(insn, extractBits(X, 51, 32));
            break;
        }
        case R_LARCH_PCALA64_HI12:
        case R_LARCH_GOT64_PC_HI12: /* GOT + G has been calculated as
                                       symbol_addr */
        {
            CHECK_RELOC_OFFSET(sizeof(int32));
            X = getLoongArchPageDelta((int64)S + A, (int64)P, reloc_type);
            *(int32 *)P = setK12(insn, extractBits(X, 63, 52));
            break;
        }

        default:
            if (error_buf != NULL)
                snprintf(error_buf, error_buf_size,
                         "Load relocation section failed: "
                         "invalid relocation type %" PRIu32 ".",
                         reloc_type);
            return false;
    }

    /* clear icache */
    __asm__ volatile("ibar 0");

    return true;

fail_addr_out_of_range:
    snprintf(buf, sizeof(buf),
             "AOT module load failed: "
             "relocation truncated to fit %s failed.",
             reloc_type_to_str(reloc_type));
    set_error_buf(error_buf, error_buf_size, buf);
    return false;

fail_addr_not_4bytes_aligned:
    snprintf(buf, sizeof(buf),
             "AOT module load failed: "
             "target address is not 4-bytes aligned.");
    set_error_buf(error_buf, error_buf_size, buf);
    return false;
}
