/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_codegen.h"
#include "jit_codecache.h"
#include "jit_compiler.h"
#include "jit_dump.h"

#include <asmjit/core.h>
#include <asmjit/x86.h>
#if WASM_ENABLE_FAST_JIT_DUMP != 0
#include <Zydis/Zydis.h>
#endif

#define CODEGEN_CHECK_ARGS 1

using namespace asmjit;

static char *code_block_switch_to_jitted_from_interp = NULL;
static char *code_block_return_to_interp_from_jitted = NULL;

typedef enum {
    REG_EBP_IDX = 0,
    REG_EAX_IDX,
    REG_EBX_IDX,
    REG_ECX_IDX,
    REG_EDX_IDX,
    REG_EDI_IDX,
    REG_ESI_IDX,
    REG_I32_FREE_IDX = REG_ESI_IDX
} RegIndexI32;

typedef enum {
    REG_RBP_IDX = 0,
    REG_RAX_IDX,
    REG_RBX_IDX,
    REG_RCX_IDX,
    REG_RDX_IDX,
    REG_RDI_IDX,
    REG_RSI_IDX,
    REG_RSP_IDX,
    REG_R8_IDX,
    REG_R9_IDX,
    REG_R10_IDX,
    REG_R11_IDX,
    REG_R12_IDX,
    REG_R13_IDX,
    REG_R14_IDX,
    REG_R15_IDX,
    REG_I64_FREE_IDX = REG_RSI_IDX
} RegIndexI64;

x86::Gp regs_i32[] = { x86::ebp, x86::eax, x86::ebx, x86::ecx,
                       x86::edx, x86::edi, x86::esi };

x86::Gp regs_i64[] = {
    x86::rbp, x86::rax, x86::rbx, x86::rcx, x86::rdx, x86::rdi,
    x86::rsi, x86::rsp, x86::r8,  x86::r9,  x86::r10, x86::r11,
    x86::r12, x86::r13, x86::r14, x86::r15,
};

int
jit_codegen_interp_jitted_glue(void *exec_env, JitInterpSwitchInfo *info,
                               void *target)
{
    typedef int32 (*F)(const void *exec_env, void *info, const void *target);
    union {
        F f;
        void *v;
    } u;

    u.v = code_block_switch_to_jitted_from_interp;
    return u.f(exec_env, info, target);
}

#define PRINT_LINE() LOG_VERBOSE("<Line:%d>\n", __LINE__)
#define GOTO_FAIL goto fail

#if CODEGEN_CHECK_ARGS == 0

#define CHECK_EQKIND(reg0, reg1) (void)0
#define CHECK_CONST(reg0) (void)0
#define CHECK_NCONST(reg0) (void)0
#define CHECK_KIND(reg0, type) (void)0

#else

/* Check if two register's kind is equal */
#define CHECK_EQKIND(reg0, reg1)                        \
    do {                                                \
        if (jit_reg_kind(reg0) != jit_reg_kind(reg1)) { \
            PRINT_LINE();                               \
            LOG_VERBOSE("reg type not equal:\n");       \
            jit_dump_reg(cc, reg0);                     \
            jit_dump_reg(cc, reg1);                     \
            GOTO_FAIL;                                  \
        }                                               \
    } while (0)

/* Check if a register is an const */
#define CHECK_CONST(reg0)                       \
    do {                                        \
        if (!jit_reg_is_const(reg0)) {          \
            PRINT_LINE();                       \
            LOG_VERBOSE("reg is not const:\n"); \
            jit_dump_reg(cc, reg0);             \
            GOTO_FAIL;                          \
        }                                       \
    } while (0)

/* Check if a register is not an const */
#define CHECK_NCONST(reg0)                  \
    do {                                    \
        if (jit_reg_is_const(reg0)) {       \
            PRINT_LINE();                   \
            LOG_VERBOSE("reg is const:\n"); \
            jit_dump_reg(cc, reg0);         \
            GOTO_FAIL;                      \
        }                                   \
    } while (0)

/* Check if a register is a special type */
#define CHECK_KIND(reg0, type)                                  \
    do {                                                        \
        if (jit_reg_kind(reg0) != type) {                       \
            PRINT_LINE();                                       \
            LOG_VERBOSE("invalid reg type %d, expected is: %d", \
                        jit_reg_kind(reg0), type);              \
            jit_dump_reg(cc, reg0);                             \
            GOTO_FAIL;                                          \
        }                                                       \
    } while (0)

#endif /* end of CODEGEN_CHECK_ARGS == 0 */

/* Load one operand from insn and check none */
#define LOAD_1ARG r0 = *jit_insn_opnd(insn, 0);

/* Load two operands from insn and check if r0 is non-const */
#define LOAD_2ARGS()              \
    r0 = *jit_insn_opnd(insn, 0); \
    r1 = *jit_insn_opnd(insn, 1); \
    CHECK_NCONST(r0)

/* Load three operands from insn and check if r0 is non-const */
#define LOAD_3ARGS()              \
    r0 = *jit_insn_opnd(insn, 0); \
    r1 = *jit_insn_opnd(insn, 1); \
    r2 = *jit_insn_opnd(insn, 2); \
    CHECK_NCONST(r0)

/* Load three operands from insn and check none */
#define LOAD_3ARGS_NO_ASSIGN()    \
    r0 = *jit_insn_opnd(insn, 0); \
    r1 = *jit_insn_opnd(insn, 1); \
    r2 = *jit_insn_opnd(insn, 2);

/* Load four operands from insn and check if r0 is non-const */
#define LOAD_4ARGS()              \
    r0 = *jit_insn_opnd(insn, 0); \
    r1 = *jit_insn_opnd(insn, 1); \
    r2 = *jit_insn_opnd(insn, 2); \
    r3 = *jit_insn_opnd(insn, 3); \
    CHECK_NCONST(r0)

/**
 * Encode extending register of byte to register of dword
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src tho no of src register
 * @param is_signed the data is signed or unsigned
 *
 * @return true if success, false otherwise
 */
static bool
extend_r8_to_r32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src,
                 bool is_signed)
{
    return false;
}
/**
 * Encode extending register of word to register of dword
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src tho no of src register
 * @param is_signed the data is signed or unsigned
 *
 * @return true if success, false otherwise
 */
static bool
extend_r16_to_r32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src,
                  bool is_signed)
{
    return false;
}

/**
 * Encode extending register of byte to register of qword
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src tho no of src register
 * @param is_signed the data is signed or unsigned
 *
 * @return true if success, false otherwise
 */
static bool
extend_r8_to_r64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src,
                 bool is_signed)
{
    return false;
}

/**
 * Encode extending register of word to register of qword
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src tho no of src register
 * @param is_signed the data is signed or unsigned
 *
 * @return true if success, false otherwise
 */
static bool
extend_r16_to_r64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src,
                  bool is_signed)
{
    return false;
}

/**
 * Encode extending register of dword to register of qword
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src tho no of src register
 * @param is_signed the data is signed or unsigned
 *
 * @return true if success, false otherwise
 */
static bool
extend_r32_to_r64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src,
                  bool is_signed)
{
    return false;
}

/**
 * Encode loading register data from memory with imm base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed the data is signed or unsigned
 * @param reg_no_dst the index of dest register
 * @param base the base address of the memory
 * @param offset the offset address of the memory
 *
 * @return true if success, false otherwise
 */
static bool
ld_r_from_base_imm_offset_imm(x86::Assembler &a, uint32 bytes_dst,
                              uint32 kind_dst, bool is_signed, int32 reg_no_dst,
                              int32 base, int32 offset)
{
    return false;
}

/**
 * Encode loading register data from memory with imm base and register offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed the data is signed or unsigned
 * @param reg_no_dst the index of dest register
 * @param base the base address of the memory
 * @param reg_no_offset the no of register which stores the offset of the memory
 *
 * @return true if success, false otherwise
 */
static bool
ld_r_from_base_imm_offset_r(x86::Assembler &a, uint32 bytes_dst,
                            uint32 kind_dst, bool is_signed, int32 reg_no_dst,
                            int32 base, int32 reg_no_offset)
{
    return false;
}

/**
 * Encode loading register data from memory with register base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed the data is signed or unsigned
 * @param reg_no_dst the index of dest register
 * @param reg_no_base the no of register which stores the base of the memory
 * @param offset the offset address of the memory
 *
 * @return true if success, false otherwise
 */
static bool
ld_r_from_base_r_offset_imm(x86::Assembler &a, uint32 bytes_dst,
                            uint32 kind_dst, bool is_signed, int32 reg_no_dst,
                            int32 reg_no_base, int32 offset)
{
    return false;
}

/**
 * Encode loading register data from memory with register base and register
 * offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed the data is signed or unsigned
 * @param reg_no_dst the index of dest register
 * @param reg_no_base the no of register which stores the base of the memory
 * @param reg_no_offset the no of register which stores the offset of the memory
 *
 * @return true if success, false otherwise
 */
static bool
ld_r_from_base_r_offset_r(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst,
                          bool is_signed, int32 reg_no_dst, int32 reg_no_base,
                          int32 reg_no_offset)
{
    return false;
}

/**
 * Encode storing register data to memory with imm base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param reg_no_src the index of src register
 * @param base the base address of the dst memory
 * @param offset the offset address of the dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_r_to_base_imm_offset_imm(x86::Assembler &a, uint32 bytes_dst,
                            uint32 kind_dst, int32 reg_no_src, int32 base,
                            int32 offset)
{
    return false;
}

/**
 * Encode storing register data to memory with imm base and register offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param reg_no_src the index of src register
 * @param base the base address of the dst memory
 * @param reg_no_offset the no of register which stores the offset of the dst
 * memory
 *
 * @return true if success, false otherwise
 */
static bool
st_r_to_base_imm_offset_r(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst,
                          int32 reg_no_src, int32 base, int32 reg_no_offset)
{
    return false;
}

/**
 * Encode storing register data to memory with register base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param reg_no_src the index of src register
 * @param reg_no_base the no of register which stores the base of the dst memory
 * @param offset the offset address of the dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_r_to_base_r_offset_imm(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst,
                          int32 reg_no_src, int32 reg_no_base, int32 offset)
{
    return false;
}

/**
 * Encode storing register data to memory with register base and register offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64), skipped by
 * float/double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param reg_no_src the index of src register
 * @param reg_no_base the no of register which stores the base of the dst memory
 * @param reg_no_offset the no of register which stores the offset of the dst
 * memory
 *
 * @return true if success, false otherwise
 */
static bool
st_r_to_base_r_offset_r(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst,
                        int32 reg_no_src, int32 reg_no_base,
                        int32 reg_no_offset)
{
    return false;
}

/**
 * Encode storing int32 imm data to memory with imm base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64)
 * @param data_src the src immediate data
 * @param base the base address of dst memory
 * @param offset the offset address of dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_imm_to_base_imm_offset_imm(x86::Assembler &a, uint32 bytes_dst,
                              void *data_src, int32 base, int32 offset)
{
    return false;
}

/**
 * Encode storing int32 imm data to memory with imm base and reg offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64)
 * @param data_src the src immediate data
 * @param base the base address of dst memory
 * @param reg_no_offset the no of register that stores the offset address
 *        of dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_imm_to_base_imm_offset_r(x86::Assembler &a, uint32 bytes_dst, void *data_src,
                            int32 base, int32 reg_no_offset)
{
    return false;
}

/**
 * Encode storing int32 imm data to memory with reg base and imm offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64)
 * @param data_src the src immediate data
 * @param reg_no_base the no of register that stores the base address
 *        of dst memory
 * @param offset the offset address of dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_imm_to_base_r_offset_imm(x86::Assembler &a, uint32 bytes_dst, void *data_src,
                            int32 reg_no_base, int32 offset)
{
    return false;
}

/**
 * Encode storing int32 imm data to memory with reg base and reg offset
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64)
 * @param data_src the src immediate data
 * @param reg_no_base the no of register that stores the base address
 *        of dst memory
 * @param reg_no_offset the no of register that stores the offset address
 *        of dst memory
 *
 * @return true if success, false otherwise
 */
static bool
st_imm_to_base_r_offset_r(x86::Assembler &a, uint32 bytes_dst, void *data_src,
                          int32 reg_no_base, int32 reg_no_offset)
{
    return false;
}

/**
 * Encode moving immediate int32 data to register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the immediate data to move
 *
 * @return true if success, false otherwise
 */
static bool
mov_imm_to_r_i32(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode moving int32 data from src register to dst register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
mov_r_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode moving immediate int64 data to register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the immediate data to move
 *
 * @return true if success, false otherwise
 */
static bool
mov_imm_to_r_i64(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode moving int64 data from src register to dst register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
mov_r_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode moving immediate float data to register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the immediate data to move
 *
 * @return true if success, false otherwise
 */
static bool
mov_imm_to_r_f32(x86::Assembler &a, int32 reg_no, float data)
{
    return false;
}

/**
 * Encode moving float data from src register to dst register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
mov_r_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode moving immediate double data to register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the immediate data to move
 *
 * @return true if success, false otherwise
 */
static bool
mov_imm_to_r_f64(x86::Assembler &a, int32 reg_no, double data)
{
    return false;
}

/**
 * Encode moving double data from src register to dst register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
mov_r_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to int8 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to int8
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_int8(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to int8 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to int8
 * @param reg_no_src the src register
 *
 * @return  true if success, false otherwise
 */
static bool
convert_r_i32_to_r_int8(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint8 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to uint8
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_uint8(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint8 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to uint8
 * @param reg_no_src the src register
 *
 * @return  true if success, false otherwise
 */
static bool
convert_r_i32_to_r_uint8(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to int16 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to int16
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_int16(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to int16 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to int16
 * @param reg_no_src the src register
 *
 * @return  true if success, false otherwise
 */
static bool
convert_r_i32_to_r_int16(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint16 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to uint16
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_uint16(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint16 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to uint16
 * @param reg_no_src the src register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i32_to_r_uint16(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint64 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to uint64
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_uint64(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encoding convert int32 immediate data to uint64 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to uint64
 * @param reg_no_src the src register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i32_to_r_uint64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting int32 immediate data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst float register
 * @param data the src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_f32(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode converting int32 register data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst float register
 * @param reg_no_src the no of src int32 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i32_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting int32 immediate data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst double register
 * @param data the src immediate int32 data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_f64(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode converting int32 register data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst double register
 * @param reg_no_src the no of src int32 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i32_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting float immediate data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst int32 register
 * @param data the src immediate float data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_f32_to_r_i32(x86::Assembler &a, int32 reg_no, float data)
{
    return false;
}

/**
 * Encode converting float register data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst int32 register
 * @param reg_no_src the no of src float register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_f32_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting float immediate data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst double register
 * @param data the src immediate float data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_f32_to_r_f64(x86::Assembler &a, int32 reg_no, float data)
{
    return false;
}

/**
 * Encode converting float register data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst double register
 * @param reg_no_src the no of src float register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_f32_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting double immediate data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst int32 register
 * @param data the src immediate double data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_f64_to_r_i32(x86::Assembler &a, int32 reg_no, double data)
{
    return false;
}

/**
 * Encode converting double register data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst int32 register
 * @param reg_no_src the no of src double register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_f64_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting double immediate data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst float register
 * @param data the src immediate double data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_f64_to_r_f32(x86::Assembler &a, int32 reg_no, double data)
{
    return false;
}

/**
 * Encode converting double register data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst float register
 * @param reg_no_src the no of src double register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_f64_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode making negative from int32 immediate data to int32 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the src int32 immediate data
 *
 * @return true if success, false otherwise
 */
static bool
neg_imm_to_r_i32(x86::Assembler &a, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode making negative from int32 register to int32 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
neg_r_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode making negative from int64 immediate data to int64 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst register
 * @param data the src int64 immediate data
 *
 * @return true if success, false otherwise
 */
static bool
neg_imm_to_r_i64(x86::Assembler &a, int32 reg_no, int64 data)
{
    return false;
}

/**
 * Encode making negative from int64 register to int64 register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
neg_r_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode making negative from float immediate data to float register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst float register
 * @param data the src float immediate data
 *
 * @return true if success, false otherwise
 */
static bool
neg_imm_to_r_f32(x86::Assembler &a, int32 reg_no, float data)
{
    return false;
}

/**
 * Encode making negative from float register to float register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register
 *
 * @return true if success, false otherwise
 */
static bool
neg_r_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode making negative from double immediate data to double register
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst double register
 * @param data the src double immediate data
 *
 * @return true if success, false otherwise
 */
static bool
neg_imm_to_r_f64(x86::Assembler &a, int32 reg_no, double data)
{
    return false;
}

/**
 * Encode making negative from double register to double register
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst double register
 * @param reg_no_src the no of src double register
 *
 * @return true if success, false otherwise
 */
static bool
neg_r_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/* Alu opcode */
typedef enum { ADD, SUB, MUL, DIV, REM } ALU_OP;
/* Bit opcode */
typedef enum { OR, XOR, AND } BIT_OP;
/* Shift opcode */
typedef enum { SHL, SHRS, SHRU } SHIFT_OP;
/* Condition opcode */
typedef enum { EQ, NE, GTS, GES, LTS, LES, GTU, GEU, LTU, LEU } COND_OP;

static COND_OP
not_cond(COND_OP op)
{
    COND_OP not_list[] = { NE, EQ, LES, LTS, GES, GTS, LEU, LTU, GEU, GTU };

    bh_assert(op <= LEU);
    return not_list[op];
}

/**
 * Encode int32 alu operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no the no of register, as first operand, and save result
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_imm_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                int32 reg_no_src, int32 data)
{
    return false;
}

/**
 * Encode int32 alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register, as first operand, and save result
 * @param reg_no_src the no of register, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_r_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst, int32 reg_no1_src,
              int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 alu operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_imm_to_r_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                     int32 data1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 alu operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_r_to_r_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int32 data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 alu operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_imm_to_r_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_to_r_i32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 alu operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no the no of register, as first operand, and save result
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_imm_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                int32 reg_no_src, int64 data)
{
    return false;
}

/**
 * Encode int64 alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register, as first operand, and save result
 * @param reg_no_src the no of register, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_r_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst, int32 reg_no1_src,
              int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 alu operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_imm_to_r_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                     int64 data1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 alu operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_r_to_r_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int64 data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 alu operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_imm_to_r_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_to_r_i64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode float alu operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_imm_to_r_f32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                     float data1_src, float data2_src)
{
    return false;
}

/**
 * Encode float alu operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_r_to_r_f32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   float data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode float alu operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_imm_to_r_f32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, float data2_src)
{
    return false;
}

/**
 * Encode float alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_to_r_f32(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode double alu operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_imm_to_r_f64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                     double data1_src, double data2_src)
{
    return false;
}

/**
 * Encode double alu operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_imm_r_to_r_f64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   double data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode double alu operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_imm_to_r_f64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, double data2_src)
{
    return false;
}

/**
 * Encode double alu operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of ALU operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
alu_r_r_to_r_f64(x86::Assembler &a, ALU_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 bit operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no the no of register, as first operand, and save result
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_imm_i32(x86::Assembler &a, BIT_OP op, int32 reg_no, int32 data)
{
    return false;
}

/**
 * Encode int32 bit operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register, as first operand, and save result
 * @param reg_no_src the no of register, as second operand
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_r_i32(x86::Assembler &a, BIT_OP op, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode int32 bit operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
bit_imm_imm_to_r_i32(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                     int32 data1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 bit operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
bit_imm_r_to_r_i32(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                   int32 data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 bit operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_imm_to_r_i32(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 bit operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_r_to_r_i32(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 bit operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no the no of register, as first operand, and save result
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_imm_i64(x86::Assembler &a, BIT_OP op, int32 reg_no, int64 data)
{
    return false;
}

/**
 * Encode int64 bit operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register, as first operand, and save result
 * @param reg_no_src the no of register, as second operand
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_r_i64(x86::Assembler &a, BIT_OP op, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode int64 bit operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
bit_imm_imm_to_r_i64(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                     int32 data1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 bit operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
bit_imm_r_to_r_i64(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                   int64 data1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 bit operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_imm_to_r_i64(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 bit operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of BIT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
bit_r_r_to_r_i64(x86::Assembler &a, BIT_OP op, int32 reg_no_dst,
                 int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 shift operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
shift_imm_imm_to_r_i32(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                       int32 data1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 shift operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
shift_imm_r_to_r_i32(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                     int32 data1_src, int32 reg_no2_src)
{
    /* Should have been optimized by previous lower */
    bh_assert(0);
    return false;
}

/**
 * Encode int32 shift operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
shift_r_imm_to_r_i32(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                     int32 reg_no1_src, int32 data2_src)
{
    return false;
}

/**
 * Encode int32 shift operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of shift operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
shift_r_r_to_r_i32(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 shift operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
shift_imm_imm_to_r_i64(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                       int64 data1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 shift operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
shift_imm_r_to_r_i64(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                     int64 data1_src, int32 reg_no2_src)
{
    /* Should have been optimized by previous lower */
    bh_assert(0);
    return false;
}

/**
 * Encode int64 shift operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of SHIFT operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
shift_r_imm_to_r_i64(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                     int32 reg_no1_src, int64 data2_src)
{
    return false;
}

/**
 * Encode int64 shift operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of shift operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
shift_r_r_to_r_i64(x86::Assembler &a, SHIFT_OP op, int32 reg_no_dst,
                   int32 reg_no1_src, int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 cmp operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register, as first operand
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src, int32 data)
{
    return false;
}

/**
 * Encode int32 cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of dst register
 * @param reg_no1_src the no of src register, as first operand
 * @param reg_no2_src the no of src register, as second operand
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
            int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 cmp operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_imm_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 data1_src,
                     int32 data2_src)
{
    return false;
}

/**
 * Encode int32 cmp operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_r_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 data1_src,
                   int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int32 cmp operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                   int32 data2_src)
{
    return false;
}

/**
 * Encode int32 cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                 int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 cmp operation of reg and data, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of dst register
 * @param reg_no_src the no of src register, as first operand
 * @param data the immediate data, as the second operand
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src, int64 data)
{
    return false;
}

/**
 * Encode int64 cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of dst register
 * @param reg_no1_src the no of src register, as first operand
 * @param reg_no2_src the no of src register, as second operand
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
            int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 cmp operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_imm_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 data1_src,
                     int32 data2_src)
{
    return false;
}

/**
 * Encode int64 cmp operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_r_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int64 data1_src,
                   int32 reg_no2_src)
{
    return false;
}

/**
 * Encode int64 cmp operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                   int64 data2_src)
{
    return false;
}

/**
 * Encode int64 cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                 int32 reg_no2_src)
{
    return false;
}

/**
 * Encode float cmp operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_imm_to_r_f32(x86::Assembler &a, int32 reg_no_dst, float data1_src,
                     float data2_src)
{
    return false;
}

/**
 * Encode float cmp operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_r_to_r_f32(x86::Assembler &a, int32 reg_no_dst, float data1_src,
                   int32 reg_no2_src)
{
    return false;
}

/**
 * Encode float cmp operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                   float data2_src)
{
    return false;
}

/**
 * Encode float cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                 int32 reg_no2_src)
{
    return false;
}

/**
 * Encode double cmp operation of imm and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_imm_to_r_f64(x86::Assembler &a, int32 reg_no_dst, double data1_src,
                     double data2_src)
{
    return false;
}

/**
 * Encode double cmp operation of imm and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param data1_src the first src immediate data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_imm_r_to_r_f64(x86::Assembler &a, int32 reg_no_dst, double data1_src,
                   int32 reg_no2_src)
{
    return false;
}

/**
 * Encode double cmp operation of reg and imm, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param data2_src the second src immediate data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_imm_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                   double data2_src)
{
    return false;
}

/**
 * Encode double cmp operation of reg and reg, and save result to reg
 *
 * @param a the assembler to emit the code
 * @param op the opcode of cmp operation
 * @param reg_no_dst the no of register
 * @param reg_no1_src the reg no of first src register data
 * @param reg_no2_src the reg no of second src register data
 *
 * @return true if success, false otherwise
 */
static bool
cmp_r_r_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no1_src,
                 int32 reg_no2_src)
{
    return false;
}

/**
 * Encode insn ld: LD_type r0, r1, r2
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param bytes_dst the byte number of dst data
 * @param is_signed the data is signed or unsigned
 */
#define LD_R_R_R(kind, bytes_dst, is_signed)                                  \
    do {                                                                      \
        int32 reg_no_dst;                                                     \
        int32 base, offset;                                                   \
        bool _ret = false;                                                    \
                                                                              \
        CHECK_KIND(r1, JIT_REG_KIND_I64);                                     \
        CHECK_KIND(r2, JIT_REG_KIND_I32);                                     \
        base = 0;                                                             \
        offset = 0;                                                           \
        real_opnd_to_reg[1] = r2;                                             \
                                                                              \
        reg_no_dst = jit_reg_no(r0);                                          \
        if (jit_reg_is_const(r1))                                             \
            base = jit_cc_get_const_I32(cc, r1);                              \
        if (jit_reg_is_const(r2))                                             \
            offset = jit_cc_get_const_I32(cc, r2);                            \
                                                                              \
        if (jit_reg_is_const(r1)) {                                           \
            if (jit_reg_is_const(r2))                                         \
                _ret = ld_r_from_base_imm_offset_imm(                         \
                    a, bytes_dst, JIT_REG_KIND_##kind, is_signed, reg_no_dst, \
                    base, offset);                                            \
            else                                                              \
                _ret = ld_r_from_base_imm_offset_r(                           \
                    a, bytes_dst, JIT_REG_KIND_##kind, is_signed, reg_no_dst, \
                    base, jit_reg_no(r2));                                    \
        }                                                                     \
        else if (jit_reg_is_const(r2))                                        \
            _ret = ld_r_from_base_r_offset_imm(                               \
                a, bytes_dst, JIT_REG_KIND_##kind, is_signed, reg_no_dst,     \
                jit_reg_no(r1), offset);                                      \
        else                                                                  \
            _ret = ld_r_from_base_r_offset_r(                                 \
                a, bytes_dst, JIT_REG_KIND_##kind, is_signed, reg_no_dst,     \
                jit_reg_no(r1), jit_reg_no(r2));                              \
        if (_ret)                                                             \
            GOTO_FAIL;                                                        \
    } while (0)

/**
 * Encode insn sd: ST_type r0, r1, r2
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param bytes_dst the byte number of dst data
 */
#define ST_R_R_R(kind, type, bytes_dst)                                        \
    do {                                                                       \
        type data_src = 0;                                                     \
        int32 reg_no_src = 0;                                                  \
        int32 base, offset;                                                    \
        bool _ret = false;                                                     \
                                                                               \
        CHECK_KIND(r1, JIT_REG_KIND_I64);                                      \
        CHECK_KIND(r2, JIT_REG_KIND_I32);                                      \
        base = 0;                                                              \
        offset = 0;                                                            \
        real_opnd_to_reg[0] = r2;                                              \
        real_opnd_to_reg[1] = r0;                                              \
                                                                               \
        if (jit_reg_is_const(r0))                                              \
            data_src = jit_cc_get_const_##kind(cc, r0);                        \
        else                                                                   \
            reg_no_src = jit_reg_no(r0);                                       \
        if (jit_reg_is_const(r1))                                              \
            base = jit_cc_get_const_I32(cc, r1);                               \
        if (jit_reg_is_const(r2))                                              \
            offset = jit_cc_get_const_I32(cc, r2);                             \
                                                                               \
        if (jit_reg_is_const(r0)) {                                            \
            if (jit_reg_is_const(r1)) {                                        \
                if (jit_reg_is_const(r2))                                      \
                    _ret = st_imm_to_base_imm_offset_imm(                      \
                        a, bytes_dst, &data_src, base, offset);                \
                else                                                           \
                    _ret = st_imm_to_base_imm_offset_r(                        \
                        a, bytes_dst, &data_src, base, jit_reg_no(r2));        \
            }                                                                  \
            else if (jit_reg_is_const(r2))                                     \
                _ret = st_imm_to_base_r_offset_imm(a, bytes_dst, &data_src,    \
                                                   jit_reg_no(r1), offset);    \
            else                                                               \
                _ret = st_imm_to_base_r_offset_r(                              \
                    a, bytes_dst, &data_src, jit_reg_no(r1), jit_reg_no(r2));  \
        }                                                                      \
        else if (jit_reg_is_const(r1)) {                                       \
            if (jit_reg_is_const(r2))                                          \
                _ret = st_r_to_base_imm_offset_imm(a, bytes_dst,               \
                                                   JIT_REG_KIND_##kind,        \
                                                   reg_no_src, base, offset);  \
            else                                                               \
                _ret = st_r_to_base_imm_offset_r(                              \
                    a, bytes_dst, JIT_REG_KIND_##kind, reg_no_src, base,       \
                    jit_reg_no(r2));                                           \
        }                                                                      \
        else if (jit_reg_is_const(r2))                                         \
            _ret =                                                             \
                st_r_to_base_r_offset_imm(a, bytes_dst, JIT_REG_KIND_##kind,   \
                                          reg_no_src, jit_reg_no(r1), offset); \
        else                                                                   \
            _ret = st_r_to_base_r_offset_r(a, bytes_dst, JIT_REG_KIND_##kind,  \
                                           reg_no_src, jit_reg_no(r1),         \
                                           jit_reg_no(r2));                    \
        if (!_ret)                                                             \
            GOTO_FAIL;                                                         \
    } while (0)

/**
 * Encode insn mov: MOV r0, r1
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param Type the data type, such as int32, int64, float32, and float64
 * @param type the abbreviation of data type, such as i32, i64, f32, and f64
 * @param bytes_dst the byte number of dst data
 */
#define MOV_R_R(kind, Type, type)                                        \
    do {                                                                 \
        bool _ret = false;                                               \
        CHECK_EQKIND(r0, r1);                                            \
        if (jit_reg_is_const(r1)) {                                      \
            Type data = jit_cc_get_const_##kind(cc, r1);                 \
            _ret = mov_imm_to_r_##type(a, jit_reg_no(r0), data);         \
        }                                                                \
        else                                                             \
            _ret = mov_r_to_r_##type(a, jit_reg_no(r0), jit_reg_no(r1)); \
        if (!_ret)                                                       \
            GOTO_FAIL;                                                   \
    } while (0)

/**
 * Encode mov insn, MOV r0, r1
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_mov(JitCompContext *cc, x86::Assembler &a, JitReg r0, JitReg r1)
{
    switch (jit_reg_kind(r0)) {
        case JIT_REG_KIND_I32:
            MOV_R_R(I32, int32, i32);
            break;
        case JIT_REG_KIND_I64:
            MOV_R_R(I64, int64, i64);
            break;
        case JIT_REG_KIND_F32:
            MOV_R_R(F32, float32, f32);
            break;
        case JIT_REG_KIND_F64:
            MOV_R_R(F64, float64, f64);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of mov: %d\n", jit_reg_kind(r0));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode insn neg: NEG r0, r1
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param Type the data type, such as int32, int64, float32, and float64
 * @param type the abbreviation of data type, such as i32, i64, f32, and f64
 */
#define NEG_R_R(kind, Type, type)                                        \
    do {                                                                 \
        bool _ret = false;                                               \
        CHECK_EQKIND(r0, r1);                                            \
        if (jit_reg_is_const(r1)) {                                      \
            Type data = jit_cc_get_const_##kind(cc, r1);                 \
            _ret = neg_imm_to_r_##type(a, jit_reg_no(r0), data);         \
        }                                                                \
        else                                                             \
            _ret = neg_r_to_r_##type(a, jit_reg_no(r0), jit_reg_no(r1)); \
        if (!_ret)                                                       \
            GOTO_FAIL;                                                   \
    } while (0)

/**
 * Encode neg insn, NEG r0, r1
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_neg(JitCompContext *cc, x86::Assembler &a, JitReg r0, JitReg r1)
{
    switch (jit_reg_kind(r0)) {
        case JIT_REG_KIND_I32:
            NEG_R_R(I32, int32, i32);
            break;
        case JIT_REG_KIND_I64:
            NEG_R_R(I64, int64, i64);
            break;
        case JIT_REG_KIND_F32:
            NEG_R_R(F32, float32, f32);
            break;
        case JIT_REG_KIND_F64:
            NEG_R_R(F64, float64, f64);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of neg: %d\n", jit_reg_kind(r0));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode insn convert: I32TOI1 r0, r1, or I32TOI2, I32TOF32, F32TOF64, etc.
 * @param kind0 the dst data kind, such as I32, I64, F32 and F64
 * @param kind1 the src data kind, such as I32, I64, F32 and F64
 * @param type0 the dst data type, such as int32, float and double
 * @param type1 the src data type, such as int32, float and double
 */
#define CONVERT_R_R(kind0, kind1, type0, type1)                              \
    do {                                                                     \
        bool _ret = false;                                                   \
        CHECK_KIND(r0, JIT_REG_KIND_##kind0);                                \
        CHECK_KIND(r1, JIT_REG_KIND_##kind1);                                \
        if (jit_reg_is_const(r1)) {                                          \
            type1 data = jit_cc_get_const_##kind1(cc, r1);                   \
            _ret =                                                           \
                convert_imm_##type1##_to_r_##type0(a, jit_reg_no(r0), data); \
        }                                                                    \
        else                                                                 \
            _ret = convert_r_##type1##_to_r_##type0(a, jit_reg_no(r0),       \
                                                    jit_reg_no(r1));         \
        if (!_ret)                                                           \
            GOTO_FAIL;                                                       \
    } while (0)

/**
 * Encode insn alu: ADD/SUB/MUL/DIV/REM r0, r1, r2
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param Type the data type, such as int32, int64, float32, and float64
 * @param type the abbreviation of data type, such as i32, i64, f32, and f64
 * @param op the opcode of alu
 */
#define ALU_R_R_R(kind, Type, type, op)                                       \
    do {                                                                      \
        Type data1, data2;                                                    \
        int32 reg_no_dst;                                                     \
        bool _ret = false;                                                    \
                                                                              \
        CHECK_EQKIND(r0, r1);                                                 \
        CHECK_EQKIND(r0, r2);                                                 \
        memset(&data1, 0, sizeof(Type));                                      \
        memset(&data2, 0, sizeof(Type));                                      \
                                                                              \
        reg_no_dst = jit_reg_no(r0);                                          \
        if (jit_reg_is_const(r1))                                             \
            data1 = jit_cc_get_const_##kind(cc, r1);                          \
        if (jit_reg_is_const(r2))                                             \
            data2 = jit_cc_get_const_##kind(cc, r2);                          \
                                                                              \
        if (jit_reg_is_const(r1)) {                                           \
            if (jit_reg_is_const(r2))                                         \
                _ret =                                                        \
                    alu_imm_imm_to_r_##type(a, op, reg_no_dst, data1, data2); \
            else                                                              \
                _ret = alu_imm_r_to_r_##type(a, op, reg_no_dst, data1,        \
                                             jit_reg_no(r2));                 \
        }                                                                     \
        else if (jit_reg_is_const(r2))                                        \
            _ret = alu_r_imm_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1),   \
                                         data2);                              \
        else                                                                  \
            _ret = alu_r_r_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1),     \
                                       jit_reg_no(r2));                       \
        if (!_ret)                                                            \
            GOTO_FAIL;                                                        \
    } while (0)

/**
 * Encode alu insn, ADD/SUB/MUL/DIV/REM r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param op the opcode of alu operations
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_alu(JitCompContext *cc, x86::Assembler &a, ALU_OP op, JitReg r0,
          JitReg r1, JitReg r2)
{
    switch (jit_reg_kind(r0)) {
        case JIT_REG_KIND_I32:
            ALU_R_R_R(I32, int32, i32, op);
            break;
        case JIT_REG_KIND_I64:
            ALU_R_R_R(I64, int64, i64, op);
            break;
        case JIT_REG_KIND_F32:
            ALU_R_R_R(F32, float32, f32, op);
            break;
        case JIT_REG_KIND_F64:
            ALU_R_R_R(F64, float64, f64, op);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of alu: %d\n", jit_reg_kind(r0));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode insn bit: AND/OR/XOR r0, r1, r2
 * @param kind the data kind, such as I32, I64
 * @param Type the data type, such as int32, int64
 * @param type the abbreviation of data type, such as i32, i64
 * @param op the opcode of bit operation
 */
#define BIT_R_R_R(kind, Type, type, op)                                       \
    do {                                                                      \
        Type data1, data2;                                                    \
        int32 reg_no_dst;                                                     \
        bool _ret = false;                                                    \
                                                                              \
        CHECK_EQKIND(r0, r1);                                                 \
        CHECK_EQKIND(r0, r2);                                                 \
        memset(&data1, 0, sizeof(Type));                                      \
        memset(&data2, 0, sizeof(Type));                                      \
                                                                              \
        reg_no_dst = jit_reg_no(r0);                                          \
        if (jit_reg_is_const(r1))                                             \
            data1 = jit_cc_get_const_##kind(cc, r1);                          \
        if (jit_reg_is_const(r2))                                             \
            data2 = jit_cc_get_const_##kind(cc, r2);                          \
                                                                              \
        if (jit_reg_is_const(r1)) {                                           \
            if (jit_reg_is_const(r2))                                         \
                _ret =                                                        \
                    bit_imm_imm_to_r_##type(a, op, reg_no_dst, data1, data2); \
            else                                                              \
                _ret = bit_imm_r_to_r_##type(a, op, reg_no_dst, data1,        \
                                             jit_reg_no(r2));                 \
        }                                                                     \
        else if (jit_reg_is_const(r2))                                        \
            _ret = bit_r_imm_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1),   \
                                         data2);                              \
        else                                                                  \
            _ret = bit_r_r_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1),     \
                                       jit_reg_no(r2));                       \
        if (!_ret)                                                            \
            GOTO_FAIL;                                                        \
    } while (0)

/**
 * Encode bit insn, AND/OR/XOR r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param op the opcode of bit operations
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_bit(JitCompContext *cc, x86::Assembler &a, BIT_OP op, JitReg r0,
          JitReg r1, JitReg r2)
{
    switch (jit_reg_kind(r0)) {
        case JIT_REG_KIND_I32:
            BIT_R_R_R(I32, int32, i32, op);
            break;
        case JIT_REG_KIND_I64:
            BIT_R_R_R(I64, int64, i64, op);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of bit: %d\n", jit_reg_kind(r0));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode insn shift: SHL/SHRS/SHRU r0, r1, r2
 * @param kind the data kind, such as I32, I64
 * @param Type the data type, such as int32, int64
 * @param type the abbreviation of data type, such as i32, i64
 * @param op the opcode of shift operation
 */
#define SHIFT_R_R_R(kind, Type, type, op)                                     \
    do {                                                                      \
        Type data1, data2;                                                    \
        int32 reg_no_dst;                                                     \
        bool _ret = false;                                                    \
                                                                              \
        CHECK_EQKIND(r0, r1);                                                 \
        CHECK_KIND(r2, JIT_REG_KIND_I32);                                     \
        memset(&data1, 0, sizeof(Type));                                      \
        memset(&data2, 0, sizeof(Type));                                      \
                                                                              \
        reg_no_dst = jit_reg_no(r0);                                          \
        if (jit_reg_is_const(r1))                                             \
            data1 = jit_cc_get_const_##kind(cc, r1);                          \
        if (jit_reg_is_const(r2))                                             \
            data2 = jit_cc_get_const_##kind(cc, r2);                          \
                                                                              \
        if (jit_reg_is_const(r1)) {                                           \
            if (jit_reg_is_const(r2))                                         \
                _ret = shift_imm_imm_to_r_##type(a, op, reg_no_dst, data1,    \
                                                 data2);                      \
            else                                                              \
                _ret = shift_imm_r_to_r_##type(a, op, reg_no_dst, data1,      \
                                               jit_reg_no(r2));               \
        }                                                                     \
        else if (jit_reg_is_const(r2))                                        \
            _ret = shift_r_imm_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1), \
                                           data2);                            \
        else                                                                  \
            _ret = shift_r_r_to_r_##type(a, op, reg_no_dst, jit_reg_no(r1),   \
                                         jit_reg_no(r2));                     \
        if (!_ret)                                                            \
            GOTO_FAIL;                                                        \
    } while (0)

/**
 * Encode shift insn, SHL/SHRS/SHRU r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param op the opcode of shift operations
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_shift(JitCompContext *cc, x86::Assembler &a, SHIFT_OP op, JitReg r0,
            JitReg r1, JitReg r2)
{
    switch (jit_reg_kind(r0)) {
        case JIT_REG_KIND_I32:
            SHIFT_R_R_R(I32, int32, i32, op);
            break;
        case JIT_REG_KIND_I64:
            SHIFT_R_R_R(I64, int64, i64, op);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of shift: %d\n", jit_reg_kind(r0));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode insn cmp: CMP r0, r1, r2
 * @param kind the data kind, such as I32, I64, F32 and F64
 * @param Type the data type, such as int32, int64, float32, and float64
 * @param type the abbreviation of data type, such as i32, i64, f32, and f64
 */
#define CMP_R_R_R(kind, Type, type)                                          \
    do {                                                                     \
        Type data1, data2;                                                   \
        int32 reg_no_dst;                                                    \
        bool _ret = false;                                                   \
                                                                             \
        CHECK_KIND(r0, JIT_REG_KIND_I32);                                    \
        CHECK_KIND(r1, JIT_REG_KIND_##kind);                                 \
        CHECK_EQKIND(r1, r2);                                                \
        memset(&data1, 0, sizeof(Type));                                     \
        memset(&data2, 0, sizeof(Type));                                     \
                                                                             \
        reg_no_dst = jit_reg_no(r0);                                         \
        if (jit_reg_is_const(r1))                                            \
            data1 = jit_cc_get_const_##kind(cc, r1);                         \
        if (jit_reg_is_const(r2))                                            \
            data2 = jit_cc_get_const_##kind(cc, r2);                         \
                                                                             \
        if (jit_reg_is_const(r1)) {                                          \
            if (jit_reg_is_const(r2))                                        \
                _ret = cmp_imm_imm_to_r_##type(a, reg_no_dst, data1, data2); \
            else                                                             \
                _ret = cmp_imm_r_to_r_##type(a, reg_no_dst, data1,           \
                                             jit_reg_no(r2));                \
        }                                                                    \
        else if (jit_reg_is_const(r2))                                       \
            _ret =                                                           \
                cmp_r_imm_to_r_##type(a, reg_no_dst, jit_reg_no(r1), data2); \
        else                                                                 \
            _ret = cmp_r_r_to_r_##type(a, reg_no_dst, jit_reg_no(r1),        \
                                       jit_reg_no(r2));                      \
        if (!_ret)                                                           \
            GOTO_FAIL;                                                       \
    } while (0)

/**
 * Encode cmp insn, CMP r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_cmp(JitCompContext *cc, x86::Assembler &a, JitReg r0, JitReg r1,
          JitReg r2)
{
    switch (jit_reg_kind(r1)) {
        case JIT_REG_KIND_I32:
            CMP_R_R_R(I32, int32, i32);
            break;
        case JIT_REG_KIND_I64:
            CMP_R_R_R(I64, int64, i64);
            break;
        case JIT_REG_KIND_F32:
            CMP_R_R_R(F32, float32, f32);
            break;
        case JIT_REG_KIND_F64:
            CMP_R_R_R(F64, float64, f64);
            break;
        default:
            LOG_VERBOSE("Invalid reg type of cmp: %d\n", jit_reg_kind(r1));
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode select insn, SELECT r0, r1, r2, r3
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 *
 * @return true if success, false if failed
 */
static bool
lower_select(JitCompContext *cc, x86::Assembler &a, COND_OP op, JitReg r0,
             JitReg r1, JitReg r2, JitReg r3)
{
#if 0
    char stream_mov1[128];
    char stream_mov2[128];
    char *stream1 = stream_mov1;
    char *stream2 = stream_mov2;

    CHECK_NCONST(r0);
    CHECK_NCONST(r1);
    CHECK_KIND(r1, JIT_REG_KIND_I32);

    if (r0 == r3 && r0 != r2) {
        JitReg r_tmp;

        /* Exchange r2, r3*/
        r_tmp = r2;
        r2 = r3;
        r3 = r_tmp;
        op = not_cond(op);
    }

    if (!lower_mov(cc, &stream1, r0, r2))
        GOTO_FAIL;
    if (!lower_mov(cc, &stream2, r0, r3))
        GOTO_FAIL;

    if (r0 != r2) {
        memcpy(stream, stream_mov1, (int32)(stream1 - stream_mov1));
        stream += (int32)(stream1 - stream_mov1);
    }

    if (r3 && r0 != r3) {
        stream = cmp_r_and_jmp_relative(stream, jit_reg_no(r1), op,
                                        (int32)(stream2 - stream_mov2));
        memcpy(stream, stream_mov2, (int32)(stream2 - stream_mov2));
        stream += (int32)(stream2 - stream_mov2);
    }

    return true;
fail:
    return false;
#endif
    return false;
}

/**
 * Encode branch insn, BEQ/BNE/../BLTU r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lower_branch(JitCompContext *cc, x86::Assembler &a, int32 label_src, COND_OP op,
             JitReg r0, JitReg r1, JitReg r2, bool is_last_insn)
{
#if 0
    int32 reg_no, label_dst;

    CHECK_NCONST(r0);
    CHECK_KIND(r0, JIT_REG_KIND_I32);
    CHECK_KIND(r1, JIT_REG_KIND_L4);

    label_dst = jit_reg_no(r1);
    if (label_dst < (int32)jit_cc_label_num(cc) - 1 && is_last_insn
        && label_is_neighboring(cc, label_src, label_dst)) {
        JitReg r_tmp;

        r_tmp = r1;
        r1 = r2;
        r2 = r_tmp;
        op = not_cond(op);
    }

    reg_no = jit_reg_no(r0);
    if (!cmp_r_and_jmp_label(cc, &stream, stream_offset, label_src, op, reg_no,
                             r1, r2, is_last_insn))
        GOTO_FAIL;

    return true;
fail:
    return false;
#endif
    return false;
}

/**
 * Encode lookupswitch with key of immediate data
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param label_src the index of src label
 * @param key the entry key
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lookupswitch_imm(JitCompContext *cc, x86::Assembler &a, int32 label_src,
                 int32 key, const JitOpndLookupSwitch *opnd, bool is_last_insn)
{
#if 0
    uint32 i;
    int32 label_dst;

    for (i = 0; i < opnd->match_pairs_num; i++)
        if (key == opnd->match_pairs[i].value) {
            label_dst = jit_reg_no(opnd->match_pairs[i].target);
            if (!(is_last_insn
                  && label_is_neighboring(cc, label_src, label_dst)))
                JMP_TO_LABEL(stream_offset, label_dst, label_src);

            return true;
        }

    if (opnd->default_target) {
        label_dst = jit_reg_no(opnd->default_target);
        if (!(is_last_insn && label_is_neighboring(cc, label_src, label_dst)))
            JMP_TO_LABEL(stream_offset, label_dst, label_src);
    }

    return true;
fail:
    return false;
#endif
    return false;
}

/**
 * Encode detecting lookupswitch entry register and jumping to matched label
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param label_src the index of src label
 * @param reg_no the no of entry register
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lookupswitch_r(JitCompContext *cc, x86::Assembler &a, int32 label_src,
               int32 reg_no, const JitOpndLookupSwitch *opnd, bool is_last_insn)
{
#if 0
    JmpInfo *node;
    Imm_Opnd imm;
    uint32 i, stream_offset_new;
    int32 label_dst;

    for (i = 0; i < opnd->match_pairs_num; i++) {
        imm_from_sz_v_s(imm, SZ32, opnd->match_pairs[i].value, true);
        alu_r_imm(cmp, regs_I32[reg_no], imm);

        label_dst = jit_reg_no(opnd->match_pairs[i].target);
        imm_from_sz_v_s(imm, SZ32, label_dst, true);

        node = jit_malloc(sizeof(JmpInfo));
        if (!node)
            GOTO_FAIL;

        node->type = JMP_DST_LABEL;
        node->label_src = label_src;
        node->dst_info.label_dst = label_dst;
        node->offset = (int32)(stream + 2 - (*stream_ptr - stream_offset));
        bh_list_insert(jmp_info_list, node);

        je(imm);
    }

    if (opnd->default_target) {
        label_dst = jit_reg_no(opnd->default_target);
        stream_offset_new = stream_offset + stream - *stream_ptr;
        if (!(is_last_insn && label_is_neighboring(cc, label_src, label_dst)))
            JMP_TO_LABEL(stream_offset_new, label_dst, label_src);
    }

    return true;
fail:
    return false;
#endif
    return false;
}

/**
 * Encode lookupswitch insn, LOOKUPSWITCH opnd
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param label_src the index of src label
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lower_lookupswitch(JitCompContext *cc, x86::Assembler &a, int32 label_src,
                   const JitOpndLookupSwitch *opnd, bool is_last_insn)
{
#if 0
    JitReg r0 = opnd->value;
    int32 key, reg_no;

    CHECK_KIND(r0, JIT_REG_KIND_I32);
    CHECK_KIND(opnd->default_target, JIT_REG_KIND_L4);

    if (jit_reg_is_const(r0)) {
        key = jit_cc_get_const_I32(cc, r0);
        if (!lookupswitch_imm(cc, &stream, stream_offset, label_src, key, opnd,
                              is_last_insn))
            GOTO_FAIL;
    }
    else {
        reg_no = jit_reg_no(r0);
        if (!lookupswitch_r(cc, &stream, stream_offset, label_src, reg_no, opnd,
                            is_last_insn))
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
#endif
    return false;
}

/**
 * Encode callnative insn, CALLNATIVE r0, r1, ...
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param label_src the index of src label
 * @param insn current insn info
 * @param global_offset_base the base for calculating global offset
 *
 * @return true if success, false if failed
 */
static bool
lower_callnative(JitCompContext *cc, x86::Assembler &a, int32 label_src,
                 JitInsn *insn, unsigned global_offset_base)
{
    return false;
}

/**
 * Encode callbc insn, CALLBC r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param label_src the index of src label
 * @param insn current insn info
 * @param global_offset_base the base for calculating global offset
 *
 * @return true if success, false if failed
 */
static bool
lower_callbc(JitCompContext *cc, x86::Assembler &a, int32 label_src,
             JitInsn *insn, unsigned global_offset_base)
{
    return false;
}

static bool
lower_returnbc(JitCompContext *cc, x86::Assembler &a, int32 label_src,
               JitInsn *insn)
{
    return false;
}

bool
jit_codegen_gen_native(JitCompContext *cc)
{
    jit_set_last_error(cc, "jit_codegen_gen_native failed");
    return false;
}

bool
jit_codegen_lower(JitCompContext *cc)
{
    return true;
}

void
jit_codegen_free_native(JitCompContext *cc)
{}

#if WASM_ENABLE_FAST_JIT_DUMP != 0
static void
dump_native(char *data, uint32 length)
{
    /* Initialize decoder context */
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                     ZYDIS_STACK_WIDTH_64);

    /* Initialize formatter */
    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    /* Loop over the instructions in our buffer */
    ZyanU64 runtime_address = (ZyanU64)(uintptr_t)data;
    ZyanUSize offset = 0;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(
        &decoder, data + offset, length - offset, &instruction, operands,
        ZYDIS_MAX_OPERAND_COUNT_VISIBLE, ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY))) {
        /* Print current instruction pointer */
        printf("%012" PRIX64 "  ", runtime_address);

        /* Format & print the binary instruction structure to
           human readable format */
        char buffer[256];
        ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
                                        instruction.operand_count_visible,
                                        buffer, sizeof(buffer),
                                        runtime_address);
        puts(buffer);

        offset += instruction.length;
        runtime_address += instruction.length;
    }
}
#endif

void
jit_codegen_dump_native(void *begin_addr, void *end_addr)
{
#if WASM_ENABLE_FAST_JIT_DUMP != 0
    dump_native((char *)begin_addr, (char *)end_addr - (char *)begin_addr);
#endif
}

bool
jit_codegen_init()
{
    const JitHardRegInfo *hreg_info = jit_codegen_get_hreg_info();
    char *code_buf, *stream;
    uint32 code_size;

    Environment env(Arch::kX64);
    CodeHolder code;
    code.init(env);
    x86::Assembler a(&code);

    /* push callee-save registers */
    a.push(x86::rbp);
    a.push(x86::rbx);
    a.push(x86::r12);
    a.push(x86::r13);
    a.push(x86::r14);
    a.push(x86::r15);
    /* push exec_env */
    a.push(x86::rdi);
    /* push info */
    a.push(x86::rsi);
    /* exec_env_reg = exec_env */
    a.mov(regs_i64[hreg_info->exec_env_hreg_index], x86::rdi);
    /* fp_reg = info.->frame */
    a.mov(x86::ebp, x86::ptr(x86::rsi, 0));
    /* jmp target */
    a.jmp(x86::rdx);

    code_buf = (char *)code.sectionById(0)->buffer().data();
    code_size = code.sectionById(0)->buffer().size();
    stream = (char *)jit_code_cache_alloc(code_size);
    if (!stream)
        return false;

    bh_memcpy_s(stream, code_size, code_buf, code_size);
    code_block_switch_to_jitted_from_interp = stream;

#if 0
    dump_native(stream, code_size);
#endif

    a.setOffset(0);
    /* pop info */
    a.pop(x86::rsi);
    /* pop exec_env */
    a.pop(x86::rdi);
    /* pop callee-save registers */
    a.pop(x86::r15);
    a.pop(x86::r14);
    a.pop(x86::r13);
    a.pop(x86::r12);
    a.pop(x86::rbx);
    a.pop(x86::rbp);

    code_buf = (char *)code.sectionById(0)->buffer().data();
    code_size = code.sectionById(0)->buffer().size();
    stream = (char *)jit_code_cache_alloc(code_size);
    if (!stream) {
        jit_code_cache_free(code_block_switch_to_jitted_from_interp);
        return false;
    }

    bh_memcpy_s(stream, code_size, code_buf, code_size);
    code_block_return_to_interp_from_jitted = stream;
    return true;
}

void
jit_codegen_destroy()
{
    jit_code_cache_free(code_block_switch_to_jitted_from_interp);
    jit_code_cache_free(code_block_return_to_interp_from_jitted);
}

/* clang-format off */
static const uint8 hreg_info_I32[3][7] = {
    /* ebp, eax, ebx, ecx, edx, edi, esi */
    { 1, 0, 0, 0, 0, 0, 1 }, /* fixed, esi is freely used */
    { 0, 1, 0, 1, 1, 0, 0 }, /* caller_saved_native */
    { 0, 1, 0, 1, 1, 1, 0 }  /* caller_saved_jitted */
};

static const uint8 hreg_info_I64[3][16] = {
    /* rbp, rax, rbx, rcx, rdx, rdi, rsi, rsp,
       r8,  r9,  r10, r11, r12, r13, r14, r15 */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 1 }, /* fixed, rsi is freely used */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 0, 0, 0, 0 }, /* caller_saved_native */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 0 }, /* caller_saved_jitted */
};

static uint8 hreg_info_F32[3][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* fixed, rsi is freely used */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_native */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_jitted */
};

static uint8 hreg_info_F64[3][16] = {
    { 1, 1, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0 }, /* fixed, rsi is freely used */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_native */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_jitted */
};

static const JitHardRegInfo hreg_info = {
    {
        { 0, NULL, NULL, NULL }, /* VOID */

        { sizeof(hreg_info_I32[0]), /* I32 */
          hreg_info_I32[0],
          hreg_info_I32[1],
          hreg_info_I32[2] },

        { sizeof(hreg_info_I64[0]), /* I64 */
          hreg_info_I64[0],
          hreg_info_I64[1],
          hreg_info_I64[2] },

        { sizeof(hreg_info_F32[0]), /* F32 */
          hreg_info_F32[0],
          hreg_info_F32[1],
          hreg_info_F32[2] },

        { sizeof(hreg_info_F64[0]), /* F64 */
          hreg_info_F64[0],
          hreg_info_F64[1],
          hreg_info_F64[2] },

        { 0, NULL, NULL, NULL }, /* V8 */
        { 0, NULL, NULL, NULL }, /* V16 */
        { 0, NULL, NULL, NULL }  /* V32 */
    },
    /* frame pointer hreg index: rbp */
    0,
    /* exec_env hreg index: r15 */
    15,
    /* cmp hreg index: esi */
    6
};
/* clang-format on */

const JitHardRegInfo *
jit_codegen_get_hreg_info()
{
    return &hreg_info;
}
