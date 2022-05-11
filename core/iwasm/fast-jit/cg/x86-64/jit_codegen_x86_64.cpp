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
#define CODEGEN_DUMP 0

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

/* clang-format off */
x86::Gp regs_i8[] = {
    x86::bpl,  x86::al, x86::bl, x86::cl,
    x86::dl,   x86::dil,  x86::sil,  x86::spl,
    x86::r8b,  x86::r9b,  x86::r10b, x86::r11b,
    x86::r12b, x86::r13b, x86::r14b, x86::r15b
};

x86::Gp regs_i16[] = {
    x86::bp,   x86::ax,   x86::bx,   x86::cx,
    x86::dx,   x86::di,   x86::si,   x86::sp,
    x86::r8w,  x86::r9w,  x86::r10w, x86::r11w,
    x86::r12w, x86::r13w, x86::r14w, x86::r15w
};

x86::Gp regs_i32[] = {
    x86::ebp,  x86::eax,  x86::ebx,  x86::ecx,
    x86::edx,  x86::edi,  x86::esi,  x86::esp,
    x86::r8d,  x86::r9d,  x86::r10d, x86::r11d,
    x86::r12d, x86::r13d, x86::r14d, x86::r15d
};

x86::Gp regs_i64[] = {
    x86::rbp, x86::rax, x86::rbx, x86::rcx,
    x86::rdx, x86::rdi, x86::rsi, x86::rsp,
    x86::r8,  x86::r9,  x86::r10, x86::r11,
    x86::r12, x86::r13, x86::r14, x86::r15,
};

x86::Xmm regs_float[] = {
    x86::xmm0,
    x86::xmm1,
    x86::xmm2,
    x86::xmm3,
    x86::xmm4,
    x86::xmm5,
    x86::xmm6,
    x86::xmm7,
    x86::xmm8,
    x86::xmm9,
    x86::xmm10,
    x86::xmm11,
    x86::xmm12,
    x86::xmm13,
    x86::xmm14,
    x86::xmm15,
};
/* clang-format on */

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

#if CODEGEN_DUMP != 0
#define GOTO_FAIL     \
    do {              \
        PRINT_LINE(); \
        goto fail;    \
    } while (0)
#else
#define GOTO_FAIL goto fail
#endif

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
#define LOAD_1ARG() r0 = *jit_insn_opnd(insn, 0)

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

class JitErrorHandler : public ErrorHandler
{
  public:
    Error err;

    JitErrorHandler()
      : err(kErrorOk)
    {}

    void handleError(Error e, const char *msg, BaseEmitter *base) override
    {
        (void)msg;
        (void)base;
        this->err = e;
    }
};

/* Alu opcode */
typedef enum { ADD, SUB, MUL, DIV_S, REM_S, DIV_U, REM_U } ALU_OP;
/* Bit opcode */
typedef enum { OR, XOR, AND } BIT_OP;
/* Shift opcode */
typedef enum { SHL, SHRS, SHRU } SHIFT_OP;
/* Condition opcode */
typedef enum { EQ, NE, GTS, GES, LTS, LES, GTU, GEU, LTU, LEU } COND_OP;

/* Jmp type */
typedef enum JmpType {
    JMP_DST_LABEL,     /* jmp to dst label */
    JMP_END_OF_CALLBC, /* jmp to end of CALLBC */
} JmpType;

/**
 * Jmp info, save the info on first encoding pass,
 * and replace the offset with exact offset when the code cache
 * has been allocated actually.
 */
typedef struct JmpInfo {
    bh_list_link link;
    JmpType type;
    uint32 label_src;
    uint32 offset;
    union {
        uint32 label_dst;
    } dst_info;
} JmpInfo;

static bool
label_is_neighboring(JitCompContext *cc, int32 label_prev, int32 label_succ)
{
    return (label_prev == 0 && label_succ == 2)
           || (label_prev >= 2 && label_succ == label_prev + 1)
           || (label_prev == (int32)jit_cc_label_num(cc) - 1
               && label_succ == 1);
}

static bool
label_is_ahead(JitCompContext *cc, int32 label_dst, int32 label_src)
{
    return (label_dst == 0 && label_src != 0)
           || (label_dst != 1 && label_src == 1)
           || (2 <= label_dst && label_dst < label_src
               && label_src <= (int32)jit_cc_label_num(cc) - 1);
}

/**
 * Encode jumping from one label to the other label
 *
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_dst the index of dst label
 * @param label_src the index of src label
 *
 * @return true if success, false if failed
 */
static bool
jmp_from_label_to_label(x86::Assembler &a, bh_list *jmp_info_list,
                        int32 label_dst, int32 label_src)
{
    Imm imm(INT32_MAX);
    JmpInfo *node;

    node = (JmpInfo *)jit_calloc(sizeof(JmpInfo));
    if (!node)
        return false;

    node->type = JMP_DST_LABEL;
    node->label_src = label_src;
    node->dst_info.label_dst = label_dst;
    node->offset = a.code()->sectionById(0)->buffer().size() + 2;
    bh_list_insert(jmp_info_list, node);

    a.jmp(imm);
    return true;
}

/**
 * Encode detecting compare result register according to condition code
 * and then jumping to suitable label when the condtion is met
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_src the index of src label
 * @param op the opcode of condition operation
 * @param reg_no the no of register which contains the compare results
 * @param r1 the label info when condition is met
 * @param r2 the label info when condition is unmet, do nonthing if VOID
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
cmp_r_and_jmp_label(JitCompContext *cc, x86::Assembler &a,
                    bh_list *jmp_info_list, int32 label_src, COND_OP op,
                    int32 reg_no, JitReg r1, JitReg r2, bool is_last_insn)
{
    Imm imm(INT32_MAX);
    JmpInfo *node;

    node = (JmpInfo *)jit_malloc(sizeof(JmpInfo));
    if (!node)
        return false;

    node->type = JMP_DST_LABEL;
    node->label_src = label_src;
    node->dst_info.label_dst = jit_reg_no(r1);
    node->offset = a.code()->sectionById(0)->buffer().size() + 2;
    bh_list_insert(jmp_info_list, node);

    switch (op) {
        case EQ:
            a.je(imm);
            break;
        case NE:
            a.jne(imm);
            break;
        case GTS:
            a.jg(imm);
            break;
        case LES:
            a.jng(imm);
            break;
        case GES:
            a.jnl(imm);
            break;
        case LTS:
            a.jl(imm);
            break;
        case GTU:
            a.ja(imm);
            break;
        case LEU:
            a.jna(imm);
            break;
        case GEU:
            a.jnb(imm);
            break;
        case LTU:
            a.jb(imm);
            break;
        default:
            bh_assert(0);
            break;
    }

    if (r2) {
        int32 label_dst = jit_reg_no(r2);
        if (!(is_last_insn && label_is_neighboring(cc, label_src, label_dst)))
            if (!jmp_from_label_to_label(a, jmp_info_list, label_dst,
                                         label_src))
                return false;
    }

    return true;
}

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
        os_printf("%012" PRIX64 "  ", runtime_address);

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
    if (is_signed) {
        a.movsx(regs_i32[reg_no_dst], regs_i8[reg_no_src]);
    }
    else {
        a.movzx(regs_i32[reg_no_dst], regs_i8[reg_no_src]);
    }
    return true;
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
    if (is_signed) {
        a.movsx(regs_i32[reg_no_dst], regs_i16[reg_no_src]);
    }
    else {
        a.movzx(regs_i32[reg_no_dst], regs_i16[reg_no_src]);
    }
    return true;
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
    if (is_signed) {
        a.movsx(regs_i64[reg_no_dst], regs_i8[reg_no_src]);
    }
    else {
        a.movzx(regs_i64[reg_no_dst], regs_i8[reg_no_src]);
    }
    return true;
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
    if (is_signed) {
        a.movsx(regs_i64[reg_no_dst], regs_i16[reg_no_src]);
    }
    else {
        a.movzx(regs_i64[reg_no_dst], regs_i16[reg_no_src]);
    }
    return true;
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
    if (is_signed) {
        a.movsxd(regs_i64[reg_no_dst], regs_i32[reg_no_src]);
    }
    else {
        a.xor_(regs_i64[reg_no_dst], regs_i64[reg_no_dst]);
        a.mov(regs_i32[reg_no_dst], regs_i32[reg_no_src]);
    }
    return true;
}

static bool
mov_r_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src);

static bool
mov_r_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src);

static void
mov_r_to_r(x86::Assembler &a, uint32 kind_dst, int32 reg_no_dst,
           int32 reg_no_src)
{
    if (kind_dst == JIT_REG_KIND_I32)
        mov_r_to_r_i32(a, reg_no_dst, reg_no_src);
    else if (kind_dst == JIT_REG_KIND_I64)
        mov_r_to_r_i64(a, reg_no_dst, reg_no_src);
    else if (kind_dst == JIT_REG_KIND_F32) {
        /* TODO */
        bh_assert(0);
    }
    else if (kind_dst == JIT_REG_KIND_F64) {
        /* TODO */
        bh_assert(0);
    }
    else {
        bh_assert(0);
    }
}

/**
 * Encode moving memory to a register
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64),
 *        skipped by float and double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed whether the data is signed or unsigned
 * @param reg_no_dst the index of dest register
 * @param m_src the memory operand which contains the source data
 *
 * @return true if success, false otherwise
 */
static bool
mov_m_to_r(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst, bool is_signed,
           int32 reg_no_dst, x86::Mem &m_src)
{
    if (kind_dst == JIT_REG_KIND_I32) {
        switch (bytes_dst) {
            case 1:
            case 2:
                if (is_signed)
                    a.movsx(regs_i32[reg_no_dst], m_src);
                else
                    a.movzx(regs_i32[reg_no_dst], m_src);
                break;
            case 4:
                a.mov(regs_i32[reg_no_dst], m_src);
                break;
            default:
                bh_assert(0);
                return false;
        }
    }
    else if (kind_dst == JIT_REG_KIND_I64) {
        switch (bytes_dst) {
            case 1:
            case 2:
                if (is_signed)
                    a.movsx(regs_i64[reg_no_dst], m_src);
                else
                    a.movzx(regs_i64[reg_no_dst], m_src);
                break;
            case 4:
                if (is_signed)
                    a.movsxd(regs_i64[reg_no_dst], m_src);
                else {
                    a.xor_(regs_i64[reg_no_dst], regs_i64[reg_no_dst]);
                    a.mov(regs_i64[reg_no_dst], m_src);
                }
                break;
            case 8:
                a.mov(regs_i64[reg_no_dst], m_src);
                break;
            default:
                bh_assert(0);
                return false;
        }
    }
    else if (kind_dst == JIT_REG_KIND_F32) {
        a.movss(regs_float[reg_no_dst], m_src);
    }
    else if (kind_dst == JIT_REG_KIND_F64) {
        a.movsd(regs_float[reg_no_dst], m_src);
    }
    return true;
}

/**
 * Encode moving register to memory
 *
 * @param a the assembler to emit the code
 * @param bytes_dst the bytes number of the data,
 *        could be 1(byte), 2(short), 4(int32), 8(int64),
 *        skipped by float and double
 * @param kind_dst the kind of data to move, could be I32, I64, F32 or F64
 * @param is_signed whether the data is signed or unsigned
 * @param m_dst the dest memory operand
 * @param reg_no_src the index of dest register
 *
 * @return true if success, false otherwise
 */
static bool
mov_r_to_m(x86::Assembler &a, uint32 bytes_dst, uint32 kind_dst,
           x86::Mem &m_dst, int32 reg_no_src)
{
    if (kind_dst == JIT_REG_KIND_I32) {
        bh_assert(reg_no_src < 16);
        switch (bytes_dst) {
            case 1:
                a.mov(m_dst, regs_i8[reg_no_src]);
                break;
            case 2:
                a.mov(m_dst, regs_i16[reg_no_src]);
                break;
            case 4:
                a.mov(m_dst, regs_i32[reg_no_src]);
                break;
            default:
                bh_assert(0);
                return false;
        }
    }
    else if (kind_dst == JIT_REG_KIND_I64) {
        bh_assert(reg_no_src < 16);
        switch (bytes_dst) {
            case 1:
                a.mov(m_dst, regs_i8[reg_no_src]);
                break;
            case 2:
                a.mov(m_dst, regs_i16[reg_no_src]);
                break;
            case 4:
                a.mov(m_dst, regs_i32[reg_no_src]);
                break;
            case 8:
                a.mov(m_dst, regs_i64[reg_no_src]);
                break;
            default:
                bh_assert(0);
                return false;
        }
    }
    else if (kind_dst == JIT_REG_KIND_F32) {
        a.movss(m_dst, regs_float[reg_no_src]);
    }
    else if (kind_dst == JIT_REG_KIND_F64) {
        a.movsd(m_dst, regs_float[reg_no_src]);
    }
    return true;
}

/**
 * Encode moving immediate data to memory
 *
 * @param m dst memory
 * @param imm src immediate data
 *
 * @return new stream
 */
static bool
mov_imm_to_m(x86::Assembler &a, x86::Mem &m_dst, Imm imm_src, uint32 bytes_dst)
{
    if (bytes_dst == 8) {
        /* As there is no instruction `MOV m64, imm64`, we use
           two instructions to implement it */
        a.mov(regs_i64[REG_I64_FREE_IDX], imm_src);
        a.mov(m_dst, regs_i64[REG_I64_FREE_IDX]);
    }
    else
        a.mov(m_dst, imm_src);
    return true;
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
    x86::Mem m((uintptr_t)(base + offset), bytes_dst);
    return mov_m_to_r(a, bytes_dst, kind_dst, is_signed, reg_no_dst, m);
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
    x86::Mem m(regs_i64[reg_no_dst], base, bytes_dst);
    return mov_m_to_r(a, bytes_dst, kind_dst, is_signed, reg_no_dst, m);
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
    x86::Mem m(regs_i64[reg_no_base], offset, bytes_dst);
    return mov_m_to_r(a, bytes_dst, kind_dst, is_signed, reg_no_dst, m);
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
    x86::Mem m(regs_i64[reg_no_base], regs_i64[reg_no_offset], 0, 0, bytes_dst);
    return mov_m_to_r(a, bytes_dst, kind_dst, is_signed, reg_no_dst, m);
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
    x86::Mem m((uintptr_t)(base + offset), bytes_dst);
    return mov_r_to_m(a, bytes_dst, kind_dst, m, reg_no_src);
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
    x86::Mem m(regs_i64[reg_no_offset], base, bytes_dst);
    return mov_r_to_m(a, bytes_dst, kind_dst, m, reg_no_src);
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
    x86::Mem m(regs_i64[reg_no_base], offset, bytes_dst);
    return mov_r_to_m(a, bytes_dst, kind_dst, m, reg_no_src);
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
    x86::Mem m(regs_i64[reg_no_base], regs_i64[reg_no_offset], 0, 0, bytes_dst);
    return mov_r_to_m(a, bytes_dst, kind_dst, m, reg_no_src);
}

static void
imm_set_value(Imm &imm, void *data, uint32 bytes)
{
    switch (bytes) {
        case 1:
            imm.setValue(*(uint8 *)data);
            break;
        case 2:
            imm.setValue(*(uint16 *)data);
            break;
        case 4:
            imm.setValue(*(uint32 *)data);
            break;
        case 8:
            imm.setValue(*(uint64 *)data);
            break;
        default:
            bh_assert(0);
    }
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
    x86::Mem m((uintptr_t)(base + offset), bytes_dst);
    Imm imm;
    imm_set_value(imm, data_src, bytes_dst);
    return mov_imm_to_m(a, m, imm, bytes_dst);
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
    x86::Mem m(regs_i64[reg_no_offset], base, bytes_dst);
    Imm imm;
    imm_set_value(imm, data_src, bytes_dst);
    return mov_imm_to_m(a, m, imm, bytes_dst);
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
    x86::Mem m(regs_i64[reg_no_base], offset, bytes_dst);
    Imm imm;
    imm_set_value(imm, data_src, bytes_dst);
    return mov_imm_to_m(a, m, imm, bytes_dst);
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
    x86::Mem m(regs_i64[reg_no_base], regs_i64[reg_no_offset], 0, 0, bytes_dst);
    Imm imm;
    imm_set_value(imm, data_src, bytes_dst);
    return mov_imm_to_m(a, m, imm, bytes_dst);
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
    Imm imm(data);
    a.mov(regs_i32[reg_no], imm);
    return true;
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
    if (reg_no_dst != reg_no_src)
        a.mov(regs_i32[reg_no_dst], regs_i32[reg_no_src]);
    return true;
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
mov_imm_to_r_i64(x86::Assembler &a, int32 reg_no, int64 data)
{
    Imm imm(data);
    a.mov(regs_i64[reg_no], imm);
    return true;
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
    if (reg_no_dst != reg_no_src)
        a.mov(regs_i64[reg_no_dst], regs_i64[reg_no_src]);
    return true;
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
convert_imm_i32_to_r_i8(x86::Assembler &a, int32 reg_no, int32 data)
{
    /* (int32)(int8)data will do sign-extension */
    /* (int32)(uint32)(int8)data is longer */
    return mov_imm_to_r_i32(a, reg_no, data & 0x000000FF);
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
convert_r_i32_to_r_i8(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    a.and_(regs_i32[reg_no_src], 0x000000FF);
    return mov_r_to_r_i32(a, reg_no_dst, reg_no_src);
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
convert_imm_i32_to_r_u8(x86::Assembler &a, int32 reg_no, int32 data)
{
    return mov_imm_to_r_i32(a, reg_no, (uint8)data);
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
convert_r_i32_to_r_u8(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return convert_r_i32_to_r_i8(a, reg_no_dst, reg_no_src);
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
convert_imm_i32_to_r_i16(x86::Assembler &a, int32 reg_no, int32 data)
{
    /* (int32)(int16)data will do sign-extension */
    /* (int32)(uint32)(int16)data is longer */
    return mov_imm_to_r_i32(a, reg_no, data & 0x0000FFFF);
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
convert_r_i32_to_r_i16(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    a.and_(regs_i32[reg_no_src], 0x0000FFFF);
    return mov_r_to_r_i32(a, reg_no_dst, reg_no_src);
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
convert_imm_i32_to_r_u16(x86::Assembler &a, int32 reg_no, int32 data)
{
    return mov_imm_to_r_i32(a, reg_no, (uint16)data);
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
convert_r_i32_to_r_u16(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return convert_r_i32_to_r_i16(a, reg_no_dst, reg_no_src);
}

/**
 * Encoding convert int32 immediate data to int64 register
 *
 * @param a the assembler to emit the code
 * @param reg_no the dst register, need to be converted to uint64
 * @param data the src int32 immediate data
 *
 * @return  true if success, false otherwise
 */
static bool
convert_imm_i32_to_r_i64(x86::Assembler &a, int32 reg_no, int32 data)
{
    /* let compiler do sign-extending */
    return mov_imm_to_r_i64(a, reg_no, (int64)data);
}

/**
 * Encoding convert int32 register data to int64 register with signed extension
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the dst register, need to be converted to uint64
 * @param reg_no_src the src register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i32_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return extend_r32_to_r64(a, reg_no_dst, reg_no_src, true);
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
 * Encode converting uint32 immediate data to int64 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst int64 register
 * @param data the src immediate uint32 data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_u32_to_r_i64(x86::Assembler &a, int32 reg_no, uint32 data)
{
    return mov_imm_to_r_i64(a, reg_no, (uint64)data);
}

/**
 * Encode converting uint32 register data to int64 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst uint32 register
 * @param reg_no_src the no of src int64 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_u32_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return extend_r32_to_r64(a, reg_no_dst, reg_no_src, false);
}

/**
 * Encode converting int64 immediate data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst int32 register
 * @param data the src immediate int64 data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_i64_to_r_i32(x86::Assembler &a, int32 reg_no, int64 data)
{
    return mov_imm_to_r_i32(a, reg_no, (int32)data);
}

/**
 * Encode converting int64 register data to int32 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst int32 register
 * @param reg_no_src the no of src int64 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i64_to_r_i32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    a.and_(regs_i64[reg_no_src], 0x00000000FFFFFFFFLL);
    return mov_r_to_r_i32(a, reg_no_dst, reg_no_src);
}

/**
 * Encode converting int64 immediate data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst float register
 * @param data the src immediate int64 data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_i64_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int64 data)
{
    return false;
}

/**
 * Encode converting int64 register data to float register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst float register
 * @param reg_no_src the no of src int64 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i64_to_r_f32(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
{
    return false;
}

/**
 * Encode converting int64 immediate data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst double register
 * @param data the src immediate int64 data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_i64_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int64 data)
{
    return false;
}

/**
 * Encode converting int64 register data to double register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst double register
 * @param reg_no_src the no of src int64 register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_i64_to_r_f64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
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
 * Encode converting double immediate data to int64 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of dst int64 register
 * @param data the src immediate double data
 *
 * @return true if success, false otherwise
 */
static bool
convert_imm_f64_to_r_i64(x86::Assembler &a, int32 reg_no, double data)
{
    return false;
}

/**
 * Encode converting double register data to int64 register data
 *
 * @param a the assembler to emit the code
 * @param reg_no_dst the no of dst int64 register
 * @param reg_no_src the no of src double register
 *
 * @return true if success, false otherwise
 */
static bool
convert_r_f64_to_r_i64(x86::Assembler &a, int32 reg_no_dst, int32 reg_no_src)
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
    Imm imm(data);

    switch (op) {
        case ADD:
            mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
            if (data == 1)
                a.inc(regs_i32[reg_no_dst]);
            else if (data == -1)
                a.dec(regs_i32[reg_no_dst]);
            else if (data != 0)
                a.add(regs_i32[reg_no_dst], imm);
            break;
        case SUB:
            mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
            if (data == -1)
                a.inc(regs_i32[reg_no_dst]);
            else if (data == 1)
                a.dec(regs_i32[reg_no_dst]);
            else if (data != 0)
                a.sub(regs_i32[reg_no_dst], imm);
            break;
        case MUL:
            if (data == 0)
                a.xor_(regs_i32[reg_no_dst], regs_i32[reg_no_dst]);
            else if (data == -1) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
                a.neg(regs_i32[reg_no_dst]);
            }
            else if (data == 1) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
            }
            else if (data == 2) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
                imm.setValue(1);
                a.shl(regs_i32[reg_no_dst], imm);
            }
            else if (data == 4) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
                imm.setValue(2);
                a.shl(regs_i32[reg_no_dst], imm);
            }
            else if (data == 8) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no_src);
                imm.setValue(3);
                a.shl(regs_i32[reg_no_dst], imm);
            }
            else {
                a.imul(regs_i32[reg_no_dst], regs_i32[reg_no_src], imm);
            }
            break;
        case DIV_S:
        case REM_S:
            bh_assert(reg_no_src == REG_EAX_IDX);
            if (op == DIV_S) {
                bh_assert(reg_no_dst == REG_EAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_EDX_IDX);
            }
            a.mov(regs_i32[REG_I32_FREE_IDX], imm);
            /* signed extend eax to edx:eax */
            a.cdq();
            a.idiv(regs_i32[REG_I32_FREE_IDX]);
            break;
        case DIV_U:
        case REM_U:
            bh_assert(reg_no_src == REG_EAX_IDX);
            if (op == DIV_U) {
                bh_assert(reg_no_dst == REG_EAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_EDX_IDX);
            }
            a.mov(regs_i32[REG_I32_FREE_IDX], imm);
            /* unsigned extend eax to edx:eax */
            a.xor_(regs_i32[REG_EDX_IDX], regs_i32[REG_EDX_IDX]);
            a.div(regs_i32[REG_I32_FREE_IDX]);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
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
    switch (op) {
        case ADD:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no1_src);
                a.add(regs_i32[reg_no_dst], regs_i32[reg_no2_src]);
            }
            else
                a.add(regs_i32[reg_no2_src], regs_i32[reg_no1_src]);
            break;
        case SUB:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no1_src);
                a.sub(regs_i32[reg_no_dst], regs_i32[reg_no2_src]);
            }
            else {
                a.sub(regs_i32[reg_no2_src], regs_i32[reg_no1_src]);
                a.neg(regs_i32[reg_no2_src]);
            }
            break;
        case MUL:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I32, reg_no_dst, reg_no1_src);
                a.imul(regs_i32[reg_no_dst], regs_i32[reg_no2_src]);
            }
            else
                a.imul(regs_i32[reg_no2_src], regs_i32[reg_no1_src]);
            break;
        case DIV_S:
        case REM_S:
            bh_assert(reg_no1_src == REG_EAX_IDX);
            if (op == DIV_S) {
                bh_assert(reg_no_dst == REG_EAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_EDX_IDX);
                if (reg_no2_src == REG_EDX_IDX) {
                    /* convert `REM_S edx, eax, edx` into
                       `mov esi, edx` and `REM_S edx eax, rsi` to
                       avoid overwritting edx when a.cdq() */
                    a.mov(regs_i32[REG_I32_FREE_IDX], regs_i32[REG_EDX_IDX]);
                    reg_no2_src = REG_I32_FREE_IDX;
                }
            }
            /* signed extend eax to edx:eax */
            a.cdq();
            a.idiv(regs_i32[reg_no2_src]);
            break;
        case DIV_U:
        case REM_U:
            bh_assert(reg_no1_src == REG_EAX_IDX);
            if (op == DIV_U) {
                bh_assert(reg_no_dst == REG_EAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_EDX_IDX);
                if (reg_no2_src == REG_EDX_IDX) {
                    /* convert `REM_U edx, eax, edx` into
                       `mov esi, edx` and `REM_U edx eax, rsi` to
                       avoid overwritting edx when unsigned extend
                       eax to edx:eax */
                    a.mov(regs_i32[REG_I32_FREE_IDX], regs_i32[REG_EDX_IDX]);
                    reg_no2_src = REG_I32_FREE_IDX;
                }
            }
            /* unsigned extend eax to edx:eax */
            a.xor_(regs_i32[REG_EDX_IDX], regs_i32[REG_EDX_IDX]);
            a.div(regs_i32[reg_no2_src]);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
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
    Imm imm;
    int32 data = 0;

    switch (op) {
        case ADD:
            data = data1_src + data2_src;
            break;
        case SUB:
            data = data1_src - data2_src;
            break;
        case MUL:
            data = data1_src * data2_src;
            break;
        case DIV_S:
            data = data1_src / data2_src;
            break;
        case REM_S:
            data = data1_src % data2_src;
            break;
        case DIV_U:
            data = (uint32)data1_src / (uint32)data2_src;
            break;
        case REM_U:
            data = (uint32)data1_src % (uint32)data2_src;
            break;
        default:
            bh_assert(0);
            break;
    }

    imm.setValue(data);
    a.mov(regs_i32[reg_no_dst], imm);
    return true;
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
    if (op == ADD || op == MUL)
        return alu_r_r_imm_i32(a, op, reg_no_dst, reg_no2_src, data1_src);
    else if (op == SUB) {
        if (!alu_r_r_imm_i32(a, op, reg_no_dst, reg_no2_src, data1_src))
            return false;
        a.neg(regs_i32[reg_no_dst]);
        return true;
    }
    else {
        if (reg_no_dst != reg_no2_src) {
            if (!mov_imm_to_r_i32(a, reg_no_dst, data1_src)
                || !alu_r_r_r_i32(a, op, reg_no_dst, reg_no_dst, reg_no2_src))
                return false;
            return true;
        }
        else {
            if (!mov_imm_to_r_i32(a, REG_I32_FREE_IDX, data1_src)
                || !alu_r_r_r_i32(a, op, reg_no_dst, REG_I32_FREE_IDX,
                                  reg_no2_src))
                return false;
            return true;
        }
    }

    return true;
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
    return alu_r_r_imm_i32(a, op, reg_no_dst, reg_no1_src, data2_src);
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
    return alu_r_r_r_i32(a, op, reg_no_dst, reg_no1_src, reg_no2_src);
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
    Imm imm(data);

    switch (op) {
        case ADD:
            mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
            if (data == 1)
                a.inc(regs_i64[reg_no_dst]);
            else if (data == -1)
                a.dec(regs_i64[reg_no_dst]);
            else if (data != 0)
                a.add(regs_i64[reg_no_dst], imm);
            break;
        case SUB:
            mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
            if (data == -1)
                a.inc(regs_i64[reg_no_dst]);
            else if (data == 1)
                a.dec(regs_i64[reg_no_dst]);
            else if (data != 0)
                a.sub(regs_i64[reg_no_dst], imm);
            break;
        case MUL:
            if (data == 0)
                a.xor_(regs_i64[reg_no_dst], regs_i64[reg_no_dst]);
            else if (data == -1) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
                a.neg(regs_i64[reg_no_dst]);
            }
            else if (data == 1) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
            }
            else if (data == 2) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
                imm.setValue(1);
                a.shl(regs_i64[reg_no_dst], imm);
            }
            else if (data == 4) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
                imm.setValue(2);
                a.shl(regs_i64[reg_no_dst], imm);
            }
            else if (data == 8) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no_src);
                imm.setValue(3);
                a.shl(regs_i64[reg_no_dst], imm);
            }
            else {
                a.imul(regs_i64[reg_no_dst], regs_i64[reg_no_src], imm);
            }
            break;
        case DIV_S:
        case REM_S:
            bh_assert(reg_no_src == REG_RAX_IDX);
            if (op == DIV_S) {
                bh_assert(reg_no_dst == REG_RAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_RDX_IDX);
            }
            a.mov(regs_i64[REG_I64_FREE_IDX], imm);
            /* signed extend rax to rdx:rax */
            a.cqo();
            a.idiv(regs_i64[REG_I64_FREE_IDX]);
            break;
        case DIV_U:
        case REM_U:
            bh_assert(reg_no_src == REG_RAX_IDX);
            if (op == DIV_U) {
                bh_assert(reg_no_dst == REG_RAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_RDX_IDX);
            }
            a.mov(regs_i64[REG_I64_FREE_IDX], imm);
            /* unsigned extend rax to rdx:rax */
            a.xor_(regs_i64[REG_RDX_IDX], regs_i64[REG_RDX_IDX]);
            a.div(regs_i64[REG_I64_FREE_IDX]);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
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
    switch (op) {
        case ADD:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no1_src);
                a.add(regs_i64[reg_no_dst], regs_i64[reg_no2_src]);
            }
            else
                a.add(regs_i64[reg_no2_src], regs_i64[reg_no1_src]);
            break;
        case SUB:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no1_src);
                a.sub(regs_i64[reg_no_dst], regs_i64[reg_no2_src]);
            }
            else {
                a.sub(regs_i64[reg_no2_src], regs_i64[reg_no1_src]);
                a.neg(regs_i64[reg_no2_src]);
            }
            break;
        case MUL:
            if (reg_no_dst != reg_no2_src) {
                mov_r_to_r(a, JIT_REG_KIND_I64, reg_no_dst, reg_no1_src);
                a.imul(regs_i64[reg_no_dst], regs_i64[reg_no2_src]);
            }
            else
                a.imul(regs_i64[reg_no2_src], regs_i64[reg_no1_src]);
            break;
        case DIV_S:
        case REM_S:
            bh_assert(reg_no1_src == REG_RAX_IDX);
            if (op == DIV_S) {
                bh_assert(reg_no_dst == REG_RAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_RDX_IDX);
            }
            /* signed extend rax to rdx:rax */
            a.cqo();
            a.idiv(regs_i64[reg_no2_src]);
            break;
        case DIV_U:
        case REM_U:
            bh_assert(reg_no1_src == REG_RAX_IDX);
            if (op == DIV_U) {
                bh_assert(reg_no_dst == REG_RAX_IDX);
            }
            else {
                bh_assert(reg_no_dst == REG_RDX_IDX);
            }
            /* unsigned extend rax to rdx:rax */
            a.xor_(regs_i64[REG_RDX_IDX], regs_i64[REG_RDX_IDX]);
            a.div(regs_i64[reg_no2_src]);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
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
    Imm imm;
    int64 data = 0;

    switch (op) {
        case ADD:
            data = data1_src + data2_src;
            break;
        case SUB:
            data = data1_src - data2_src;
            break;
        case MUL:
            data = data1_src * data2_src;
            break;
        case DIV_S:
            data = data1_src / data2_src;
            break;
        case REM_S:
            data = data1_src % data2_src;
            break;
        case DIV_U:
            data = (uint64)data1_src / (uint64)data2_src;
            break;
        case REM_U:
            data = (uint64)data1_src % (uint64)data2_src;
            break;
        default:
            bh_assert(0);
            break;
    }

    imm.setValue(data);
    a.mov(regs_i64[reg_no_dst], imm);
    return true;
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
    if (op == ADD || op == MUL)
        return alu_r_r_imm_i64(a, op, reg_no_dst, reg_no2_src, data1_src);
    else if (op == SUB) {
        if (!alu_r_r_imm_i64(a, op, reg_no_dst, reg_no2_src, data1_src))
            return false;
        a.neg(regs_i64[reg_no_dst]);
        return true;
    }
    else {
        if (reg_no_dst != reg_no2_src) {
            if (!mov_imm_to_r_i64(a, reg_no_dst, data1_src)
                || !alu_r_r_r_i64(a, op, reg_no_dst, reg_no_dst, reg_no2_src))
                return false;
            return true;
        }
        else {
            if (!mov_imm_to_r_i64(a, REG_I64_FREE_IDX, data1_src)
                || !alu_r_r_r_i64(a, op, reg_no_dst, REG_I64_FREE_IDX,
                                  reg_no2_src))
                return false;
            return true;
        }
    }

    return true;
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
    return alu_r_r_imm_i64(a, op, reg_no_dst, reg_no1_src, data2_src);
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
    return alu_r_r_r_i64(a, op, reg_no_dst, reg_no1_src, reg_no2_src);
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
    Imm imm(data);

    switch (op) {
        case OR:
            if (data != 0)
                a.or_(regs_i32[reg_no], imm);
            break;
        case XOR:
            if (data == -1)
                a.not_(regs_i32[reg_no]);
            else if (data != 0)
                a.xor_(regs_i32[reg_no], imm);
            break;
        case AND:
            if (data != -1)
                a.and_(regs_i32[reg_no], imm);
            break;
        default:
            bh_assert(0);
            break;
    }
    return true;
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
    switch (op) {
        case OR:
            a.or_(regs_i32[reg_no_dst], regs_i32[reg_no_src]);
            break;
        case XOR:
            a.xor_(regs_i32[reg_no_dst], regs_i32[reg_no_src]);
            break;
        case AND:
            a.and_(regs_i32[reg_no_dst], regs_i32[reg_no_src]);
            break;
        default:
            bh_assert(0);
            break;
    }
    return true;
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
    Imm imm;

    switch (op) {
        case OR:
            imm.setValue(data1_src | data2_src);
            break;
        case XOR:
            imm.setValue(data1_src ^ data2_src);
            break;
        case AND:
            imm.setValue(data1_src & data2_src);
            break;
        default:
            bh_assert(0);
            break;
    }

    a.mov(regs_i32[reg_no_dst], imm);
    return true;
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
    if (op == AND && data1_src == 0)
        a.xor_(regs_i32[reg_no_dst], regs_i32[reg_no_dst]);
    else if (op == OR && data1_src == -1) {
        Imm imm(-1);
        a.mov(regs_i32[reg_no_dst], imm);
    }
    else {
        mov_r_to_r_i32(a, reg_no_dst, reg_no2_src);
        return bit_r_imm_i32(a, op, reg_no_dst, data1_src);
    }
    return true;
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
    return bit_imm_r_to_r_i32(a, op, reg_no_dst, data2_src, reg_no1_src);
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
    if (reg_no_dst != reg_no2_src) {
        mov_r_to_r_i32(a, reg_no_dst, reg_no1_src);
        return bit_r_r_i32(a, op, reg_no_dst, reg_no2_src);
    }
    else
        return bit_r_r_i32(a, op, reg_no_dst, reg_no1_src);
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
    int32 data;
    switch (op) {
        case SHL:
        {
            data = data1_src << data2_src;
            break;
        }
        case SHRS:
        {
            data = data1_src >> data2_src;
            break;
        }
        case SHRU:
        {
            data = ((uint32)data1_src) >> data2_src;
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_imm_to_r_i32(a, reg_no_dst, data);
fail:
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
    /* SHL/SHA/SHR r/m32, imm8 */
    Imm imm((uint8)data2_src);

    switch (op) {
        case SHL:
        {
            a.shl(regs_i32[reg_no1_src], imm);
            break;
        }
        case SHRS:
        {
            a.sar(regs_i32[reg_no1_src], imm);
            break;
        }
        case SHRU:
        {
            a.shr(regs_i32[reg_no1_src], imm);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_r_to_r_i32(a, reg_no_dst, reg_no1_src);
fail:
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
    /* should be CL */
    bh_assert(reg_no2_src == REG_ECX_IDX);

    switch (op) {
        case SHL:
        {
            a.shl(regs_i32[reg_no1_src], x86::cl);
            break;
        }
        case SHRS:
        {
            a.sar(regs_i32[reg_no1_src], x86::cl);
            break;
        }
        case SHRU:
        {
            a.shr(regs_i32[reg_no1_src], x86::cl);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_r_to_r_i32(a, reg_no_dst, reg_no1_src);
fail:
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
    int64 data;

    switch (op) {
        case SHL:
        {
            data = data1_src << data2_src;
            break;
        }
        case SHRS:
        {
            data = data1_src >> data2_src;
            break;
        }
        case SHRU:
        {
            data = ((uint64)data1_src) >> data2_src;
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_imm_to_r_i64(a, reg_no_dst, data);
fail:
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
    /* SHL/SHA/SHR r/m64, imm8 */
    Imm imm((uint8)data2_src);

    switch (op) {
        case SHL:
        {
            a.shl(regs_i64[reg_no1_src], imm);
            break;
        }
        case SHRS:
        {
            a.sar(regs_i64[reg_no1_src], imm);
            break;
        }
        case SHRU:
        {
            a.shr(regs_i64[reg_no1_src], imm);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_r_to_r_i64(a, reg_no_dst, reg_no1_src);
fail:
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
    /* should be CL */
    bh_assert(reg_no2_src == REG_ECX_IDX);

    switch (op) {
        case SHL:
        {
            a.shl(regs_i64[reg_no1_src], x86::cl);
            break;
        }
        case SHRS:
        {
            a.sar(regs_i64[reg_no1_src], x86::cl);
            break;
        }
        case SHRU:
        {
            a.shr(regs_i64[reg_no1_src], x86::cl);
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }

    return mov_r_to_r_i64(a, reg_no_dst, reg_no1_src);
fail:
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
    Imm imm(data);
    a.cmp(regs_i32[reg_no_src], imm);
    return true;
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
    a.cmp(regs_i32[reg_no1_src], regs_i32[reg_no2_src]);
    return true;
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
    Imm imm(data1_src);
    a.mov(regs_i32[REG_I32_FREE_IDX], imm);
    imm.setValue(data2_src);
    a.cmp(regs_i32[REG_I32_FREE_IDX], imm);
    return true;
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
    Imm imm(data1_src);
    a.mov(regs_i32[REG_I32_FREE_IDX], imm);
    a.cmp(regs_i32[REG_I32_FREE_IDX], regs_i32[reg_no2_src]);
    return true;
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
    Imm imm(data2_src);
    a.cmp(regs_i32[reg_no1_src], imm);
    return true;
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
    a.cmp(regs_i32[reg_no1_src], regs_i32[reg_no2_src]);
    return true;
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
    Imm imm(data);
    a.cmp(regs_i64[reg_no_src], imm);
    return true;
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
    a.cmp(regs_i64[reg_no1_src], regs_i64[reg_no2_src]);
    return true;
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
    Imm imm(data1_src);
    a.mov(regs_i64[REG_I64_FREE_IDX], imm);
    imm.setValue(data2_src);
    a.cmp(regs_i64[REG_I64_FREE_IDX], imm);
    return true;
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
    Imm imm(data1_src);
    a.mov(regs_i64[REG_I64_FREE_IDX], imm);
    a.cmp(regs_i64[REG_I64_FREE_IDX], regs_i64[reg_no2_src]);
    return true;
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
    Imm imm(data2_src);
    a.cmp(regs_i64[reg_no1_src], imm);
    return true;
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
    a.cmp(regs_i64[reg_no1_src], regs_i64[reg_no2_src]);
    return true;
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
        int32 base = 0, offset = 0;                                           \
        bool _ret = false;                                                    \
                                                                              \
        if (jit_reg_is_const(r1)) {                                           \
            CHECK_KIND(r1, JIT_REG_KIND_I32);                                 \
        }                                                                     \
        else {                                                                \
            CHECK_KIND(r1, JIT_REG_KIND_I64);                                 \
        }                                                                     \
        if (jit_reg_is_const(r2)) {                                           \
            CHECK_KIND(r2, JIT_REG_KIND_I32);                                 \
        }                                                                     \
        else {                                                                \
            CHECK_KIND(r2, JIT_REG_KIND_I64);                                 \
        }                                                                     \
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
        if (!_ret)                                                            \
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
        int32 base = 0, offset = 0;                                            \
        bool _ret = false;                                                     \
                                                                               \
        if (jit_reg_is_const(r1)) {                                            \
            CHECK_KIND(r1, JIT_REG_KIND_I32);                                  \
        }                                                                      \
        else {                                                                 \
            CHECK_KIND(r1, JIT_REG_KIND_I64);                                  \
        }                                                                      \
        if (jit_reg_is_const(r2)) {                                            \
            CHECK_KIND(r2, JIT_REG_KIND_I32);                                  \
        }                                                                      \
        else {                                                                 \
            CHECK_KIND(r2, JIT_REG_KIND_I64);                                  \
        }                                                                      \
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
 * Encode insn convert: I32TOI8 r0, r1, or I32TOI16, I32TOF32, F32TOF64, etc.
 * @param kind0 the dst JIT_REG_KIND, such as I32, I64, F32 and F64
 * @param kind1 the src JIT_REG_KIND, such as I32, I64, F32 and F64
 * @param type0 the dst data type, such as i8, u8, i16, u16, i32, f32, i64, f32,
 * f64
 * @param type1 the src data type, such as i8, u8, i16, u16, i32, f32, i64, f32,
 * f64
 */
#define CONVERT_R_R(kind0, kind1, type0, type1, Type1)                       \
    do {                                                                     \
        bool _ret = false;                                                   \
        CHECK_KIND(r0, JIT_REG_KIND_##kind0);                                \
        CHECK_KIND(r1, JIT_REG_KIND_##kind1);                                \
        if (jit_reg_is_const(r1)) {                                          \
            Type1 data = jit_cc_get_const_##kind1(cc, r1);                   \
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
        CHECK_KIND(r2, JIT_REG_KIND_##kind);                                  \
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
 * @param r1 condition jit register
 * @param r2 src jit register that contains the first src operand info
 * @param r3 src jit register that contains the second src operand info
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
 * Encode detecting the cmp flags in reg, and jmp to the relative address
 * according to the condition opcode
 *
 * @param a the assembler to emit the code
 * @param reg_no the no of register which contains cmp flags of cmp result
 * @param op the condition opcode to jmp
 * @param offset the relative offset to jmp when the contidtion meeted
 *
 * @return return the next address of native code after encoded
 */
static bool
cmp_r_and_jmp_relative(x86::Assembler &a, int32 reg_no, COND_OP op,
                       int32 offset)
{
    Imm target(INT32_MAX);
    char *stream = (char *)a.code()->sectionById(0)->buffer().data()
                   + a.code()->sectionById(0)->buffer().size();

    switch (op) {
        case EQ:
            a.je(target);
            break;
        case NE:
            a.jne(target);
            break;
        case GTS:
            a.jg(target);
            break;
        case LES:
            a.jng(target);
            break;
        case GES:
            a.jge(target);
            break;
        case LTS:
            a.jl(target);
            break;
        case GTU:
            a.ja(target);
            break;
        case LEU:
            a.jna(target);
            break;
        case GEU:
            a.jae(target);
            break;
        case LTU:
            a.jb(target);
            break;
        default:
            bh_assert(0);
            break;
    }

    /* The offset written by asmjit is always 0, we patch it again */
    *(int32 *)(stream + 2) = offset;
    return true;
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
    JitErrorHandler err_handler;
    Environment env(Arch::kX64);
    CodeHolder code1, code2;
    char *stream_mov1, *stream_mov2;
    uint32 size_mov1, size_mov2;

    code1.init(env);
    code1.setErrorHandler(&err_handler);
    x86::Assembler a1(&code1);

    code2.init(env);
    code2.setErrorHandler(&err_handler);
    x86::Assembler a2(&code2);

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

    if (!lower_mov(cc, a1, r0, r2))
        GOTO_FAIL;

    if (!lower_mov(cc, a2, r0, r3))
        GOTO_FAIL;

    stream_mov1 = (char *)a1.code()->sectionById(0)->buffer().data();
    size_mov1 = a1.code()->sectionById(0)->buffer().size();
    stream_mov2 = (char *)a2.code()->sectionById(0)->buffer().data();
    size_mov2 = a2.code()->sectionById(0)->buffer().size();

    if (r0 != r2) {
        a.embedDataArray(TypeId::kInt8, stream_mov1, size_mov1);
    }

    if (r3 && r0 != r3) {
        if (!cmp_r_and_jmp_relative(a, jit_reg_no(r1), op, (int32)size_mov2))
            return false;
        a.embedDataArray(TypeId::kInt8, stream_mov2, size_mov2);
    }

    return true;
fail:
    return false;
}

/* jmp to dst label */
#define JMP_TO_LABEL(label_dst, label_src)                               \
    do {                                                                 \
        if (label_is_ahead(cc, label_dst, label_src)) {                  \
            int32 _offset = label_offsets[label_dst]                     \
                            - a.code()->sectionById(0)->buffer().size(); \
            Imm imm(_offset);                                            \
            a.jmp(imm);                                                  \
        }                                                                \
        else {                                                           \
            if (!jmp_from_label_to_label(a, jmp_info_list, label_dst,    \
                                         label_src))                     \
                GOTO_FAIL;                                               \
        }                                                                \
    } while (0)

/**
 * Encode branch insn, BEQ/BNE/../BLTU r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param r0 dst jit register that contains the dst operand info
 * @param r1 src jit register that contains the first src operand info
 * @param r2 src jit register that contains the second src operand info
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lower_branch(JitCompContext *cc, x86::Assembler &a, bh_list *jmp_info_list,
             int32 label_src, COND_OP op, JitReg r0, JitReg r1, JitReg r2,
             bool is_last_insn)
{
    int32 reg_no, label_dst;

    CHECK_NCONST(r0);
    CHECK_KIND(r0, JIT_REG_KIND_I32);
    CHECK_KIND(r1, JIT_REG_KIND_L32);

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
    if (!cmp_r_and_jmp_label(cc, a, jmp_info_list, label_src, op, reg_no, r1,
                             r2, is_last_insn))
        GOTO_FAIL;

    return true;
fail:
    return false;
}

/**
 * Encode lookupswitch with key of immediate data
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_offsets the offsets of each label
 * @param label_src the index of src label
 * @param key the entry key
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lookupswitch_imm(JitCompContext *cc, x86::Assembler &a, bh_list *jmp_info_list,
                 uint32 *label_offsets, int32 label_src, int32 key,
                 const JitOpndLookupSwitch *opnd, bool is_last_insn)
{
    uint32 i;
    int32 label_dst;

    for (i = 0; i < opnd->match_pairs_num; i++)
        if (key == opnd->match_pairs[i].value) {
            label_dst = jit_reg_no(opnd->match_pairs[i].target);
            if (!(is_last_insn
                  && label_is_neighboring(cc, label_src, label_dst))) {
                JMP_TO_LABEL(label_dst, label_src);
            }
            return true;
        }

    if (opnd->default_target) {
        label_dst = jit_reg_no(opnd->default_target);
        if (!(is_last_insn && label_is_neighboring(cc, label_src, label_dst))) {
            JMP_TO_LABEL(label_dst, label_src);
        }
    }

    return true;
fail:
    return false;
}

/**
 * Encode detecting lookupswitch entry register and jumping to matched label
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_offsets the offsets of each label
 * @param label_src the index of src label
 * @param reg_no the no of entry register
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lookupswitch_r(JitCompContext *cc, x86::Assembler &a, bh_list *jmp_info_list,
               uint32 *label_offsets, int32 label_src, int32 reg_no,
               const JitOpndLookupSwitch *opnd, bool is_last_insn)
{
    JmpInfo *node;
    Imm imm;
    uint32 i;
    int32 label_dst;

    for (i = 0; i < opnd->match_pairs_num; i++) {
        imm.setValue(opnd->match_pairs[i].value);
        a.cmp(regs_i32[reg_no], imm);

        label_dst = jit_reg_no(opnd->match_pairs[i].target);
        imm.setValue(label_dst);

        node = (JmpInfo *)jit_malloc(sizeof(JmpInfo));
        if (!node)
            GOTO_FAIL;

        node->type = JMP_DST_LABEL;
        node->label_src = label_src;
        node->dst_info.label_dst = label_dst;
        node->offset = a.code()->sectionById(0)->buffer().size() + 2;
        bh_list_insert(jmp_info_list, node);

        a.je(imm);
    }

    if (opnd->default_target) {
        label_dst = jit_reg_no(opnd->default_target);
        if (!(is_last_insn && label_is_neighboring(cc, label_src, label_dst)))
            JMP_TO_LABEL(label_dst, label_src);
    }

    return true;
fail:
    return false;
}

/**
 * Encode lookupswitch insn, LOOKUPSWITCH opnd
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_offsets the offsets of each label
 * @param label_src the index of src label
 * @param opnd the lookup switch operand
 * @param is_last_insn if current insn is the last insn of current block
 *
 * @return true if success, false if failed
 */
static bool
lower_lookupswitch(JitCompContext *cc, x86::Assembler &a,
                   bh_list *jmp_info_list, uint32 *label_offsets,
                   int32 label_src, const JitOpndLookupSwitch *opnd,
                   bool is_last_insn)
{
    JitReg r0 = opnd->value;
    int32 key, reg_no;

    CHECK_KIND(r0, JIT_REG_KIND_I32);
    CHECK_KIND(opnd->default_target, JIT_REG_KIND_L32);

    if (jit_reg_is_const(r0)) {
        key = jit_cc_get_const_I32(cc, r0);
        if (!lookupswitch_imm(cc, a, jmp_info_list, label_offsets, label_src,
                              key, opnd, is_last_insn))
            GOTO_FAIL;
    }
    else {
        reg_no = jit_reg_no(r0);
        if (!lookupswitch_r(cc, a, jmp_info_list, label_offsets, label_src,
                            reg_no, opnd, is_last_insn))
            GOTO_FAIL;
    }

    return true;
fail:
    return false;
}

/**
 * Encode callnative insn, CALLNATIVE r0, r1, ...
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_src the index of src label
 * @param insn current insn info
 *
 * @return true if success, false if failed
 */
static bool
lower_callnative(JitCompContext *cc, x86::Assembler &a, bh_list *jmp_info_list,
                 int32 label_src, JitInsn *insn)
{
    void (*func_ptr)(void);
    JitReg ret_reg, func_reg, arg_reg;
    x86::Gp regs_arg[] = { x86::rdi, x86::rsi, x86::rdx,
                           x86::rcx, x86::r8,  x86::r9 };
    Imm imm;
    JmpInfo *node;
    uint32 i, opnd_num;
    int32 i32;
    int64 i64;

    ret_reg = *(jit_insn_opndv(insn, 0));
    func_reg = *(jit_insn_opndv(insn, 1));
    CHECK_KIND(func_reg, JIT_REG_KIND_I64);
    CHECK_CONST(func_reg);

    func_ptr = (void (*)(void))jit_cc_get_const_I64(cc, func_reg);

    opnd_num = jit_insn_opndv_num(insn);
    bh_assert(opnd_num <= (uint32)sizeof(regs_arg) / sizeof(JitReg));
    for (i = 0; i < opnd_num - 2; i++) {
        arg_reg = *(jit_insn_opndv(insn, i + 2));
        switch (jit_reg_kind(arg_reg)) {
            case JIT_REG_KIND_I32:
                if (jit_reg_is_const(arg_reg)) {
                    i32 = jit_cc_get_const_I32(cc, arg_reg);
                    imm.setValue((int64)i32);
                    a.mov(regs_arg[i], imm);
                }
                else {
                    a.movsxd(regs_arg[i], regs_i32[jit_reg_no(arg_reg)]);
                }
                break;
            case JIT_REG_KIND_I64:
                if (jit_reg_is_const(arg_reg)) {
                    i64 = jit_cc_get_const_I64(cc, arg_reg);
                    imm.setValue(i64);
                    a.mov(regs_arg[i], imm);
                }
                else {
                    if (regs_arg[i] != regs_i64[jit_reg_no(arg_reg)])
                        a.mov(regs_arg[i], regs_i64[jit_reg_no(arg_reg)]);
                }
                break;
            default:
                bh_assert(0);
                return false;
        }
    }

    imm.setValue((uint64)func_ptr);
    a.mov(regs_i64[REG_RAX_IDX], imm);
    a.call(regs_i64[REG_RAX_IDX]);

    if (ret_reg) {
        bh_assert(jit_reg_kind(ret_reg) == JIT_REG_KIND_I32);
        bh_assert(jit_reg_no(ret_reg) == REG_EAX_IDX);
    }

    return true;
fail:
    return false;
}

/**
 * Encode callbc insn, CALLBC r0, r1, r2
 *
 * @param cc the compiler context
 * @param a the assembler to emit the code
 * @param jmp_info_list the jmp info list
 * @param label_src the index of src label
 * @param insn current insn info
 *
 * @return true if success, false if failed
 */
static bool
lower_callbc(JitCompContext *cc, x86::Assembler &a, bh_list *jmp_info_list,
             int32 label_src, JitInsn *insn)
{
    JmpInfo *node;
    Imm imm;
    JitReg func_reg = *(jit_insn_opnd(insn, 2));

    /* Load return_jitted_addr from stack */
    x86::Mem m(x86::rbp, cc->jitted_return_address_offset);

    CHECK_KIND(func_reg, JIT_REG_KIND_I64);

    node = (JmpInfo *)jit_malloc(sizeof(JmpInfo));
    if (!node)
        GOTO_FAIL;

    node->type = JMP_END_OF_CALLBC;
    node->label_src = label_src;
    node->offset = a.code()->sectionById(0)->buffer().size() + 2;
    bh_list_insert(jmp_info_list, node);

    /* Set next jited addr to glue_ret_jited_addr, 0 will be replaced with
       actual offset after actual code cache is allocated */
    imm.setValue(INT64_MAX);
    a.mov(regs_i64[REG_I64_FREE_IDX], imm);
    a.mov(m, regs_i64[REG_I64_FREE_IDX]);
    a.jmp(regs_i64[jit_reg_no(func_reg)]);
    return true;
fail:
    return false;
}

static bool
lower_returnbc(JitCompContext *cc, x86::Assembler &a, int32 label_src,
               JitInsn *insn)
{
    JitReg ecx_hreg = jit_reg_new(JIT_REG_KIND_I32, REG_ECX_IDX);
    JitReg rcx_hreg = jit_reg_new(JIT_REG_KIND_I64, REG_RCX_IDX);
    JitReg act_reg = *(jit_insn_opnd(insn, 0));
    JitReg ret_reg = *(jit_insn_opnd(insn, 1));
    int32 act;

    CHECK_CONST(act_reg);
    CHECK_KIND(act_reg, JIT_REG_KIND_I32);

    act = jit_cc_get_const_I32(cc, act_reg);

    if (ret_reg) {
        if (jit_reg_is_kind(I32, ret_reg)) {
            if (!lower_mov(cc, a, ecx_hreg, ret_reg))
                return false;
        }
        else if (jit_reg_is_kind(I64, ret_reg)) {
            if (!lower_mov(cc, a, rcx_hreg, ret_reg))
                return false;
        }
        else if (jit_reg_is_kind(F32, ret_reg)) {
            /* TODO */
            return false;
        }
        else if (jit_reg_is_kind(F64, ret_reg)) {
            /* TODO */
            return false;
        }
        else {
            return false;
        }
    }

    {
        /* eax = act */
        Imm imm(act);
        a.mov(x86::eax, imm);

        x86::Mem m(x86::rbp, cc->jitted_return_address_offset);
        a.jmp(m);
    }
    return true;
fail:
    return false;
}

static bool
lower_return(JitCompContext *cc, x86::Assembler &a, JitInsn *insn)
{
    JitReg act_reg = *(jit_insn_opnd(insn, 0));
    int32 act;

    CHECK_CONST(act_reg);
    CHECK_KIND(act_reg, JIT_REG_KIND_I32);

    act = jit_cc_get_const_I32(cc, act_reg);
    {
        /* eax = act */
        Imm imm(act);
        a.mov(x86::eax, imm);

        imm.setValue((uintptr_t)code_block_return_to_interp_from_jitted);
        a.mov(regs_i64[REG_I64_FREE_IDX], imm);
        a.jmp(regs_i64[REG_I64_FREE_IDX]);
    }
    return true;
fail:
    return false;
}

/**
 * Replace all the jmp address pre-saved when the code cache hasn't been
 * allocated with actual address after code cache allocated
 *
 * @param cc compiler context containting the allocated code cacha info
 * @param jmp_info_list the jmp info list
 */
static void
patch_jmp_info_list(JitCompContext *cc, bh_list *jmp_info_list)
{
    JmpInfo *jmp_info, *jmp_info_next;
    JitReg reg_src, reg_dst;
    char *stream;

    jmp_info = (JmpInfo *)bh_list_first_elem(jmp_info_list);

    while (jmp_info) {
        jmp_info_next = (JmpInfo *)bh_list_elem_next(jmp_info);

        reg_src = jit_reg_new(JIT_REG_KIND_L32, jmp_info->label_src);
        stream = (char *)cc->jitted_addr_begin + jmp_info->offset;

        if (jmp_info->type == JMP_DST_LABEL) {
            reg_dst =
                jit_reg_new(JIT_REG_KIND_L32, jmp_info->dst_info.label_dst);
            *(int32 *)stream =
                (int32)((uintptr_t)*jit_annl_jitted_addr(cc, reg_dst)
                        - (uintptr_t)stream)
                - 4;
        }
        else if (jmp_info->type == JMP_END_OF_CALLBC) {
            /* 7 is the size of mov and jmp instruction */
            *(uintptr_t *)stream = (uintptr_t)stream + sizeof(uintptr_t) + 7;
        }

        jmp_info = jmp_info_next;
    }
}

/* Free the jmp info list */
static void
free_jmp_info_list(bh_list *jmp_info_list)
{
    void *cur_node = bh_list_first_elem(jmp_info_list);

    while (cur_node) {
        void *next_node = bh_list_elem_next(cur_node);

        bh_list_remove(jmp_info_list, cur_node);
        jit_free(cur_node);
        cur_node = next_node;
    }
}

bool
jit_codegen_gen_native(JitCompContext *cc)
{
    JitBasicBlock *block;
    JitInsn *insn;
    JitReg r0, r1, r2, r3;
    JmpInfo jmp_info_head;
    bh_list *jmp_info_list = (bh_list *)&jmp_info_head;
    uint32 label_index, label_num, i, j;
    uint32 *label_offsets = NULL, code_size;
#if CODEGEN_DUMP != 0
    uint32 code_offset = 0;
#endif
    bool return_value = false, is_last_insn;
    void **jitted_addr;
    char *code_buf, *stream;

    JitErrorHandler err_handler;
    Environment env(Arch::kX64);
    CodeHolder code;
    code.init(env);
    code.setErrorHandler(&err_handler);
    x86::Assembler a(&code);

    if (BH_LIST_SUCCESS != bh_list_init(jmp_info_list)) {
        jit_set_last_error(cc, "init jmp info list failed");
        return false;
    }

    label_num = jit_cc_label_num(cc);

    if (!(label_offsets =
              (uint32 *)jit_calloc(((uint32)sizeof(uint32)) * label_num))) {
        jit_set_last_error(cc, "allocate memory failed");
        goto fail;
    }

    for (i = 0; i < label_num; i++) {
        if (i == 0)
            label_index = 0;
        else if (i == label_num - 1)
            label_index = 1;
        else
            label_index = i + 1;

        label_offsets[label_index] = code.sectionById(0)->buffer().size();

        block = *jit_annl_basic_block(
            cc, jit_reg_new(JIT_REG_KIND_L32, label_index));

#if CODEGEN_DUMP != 0
        os_printf("\nL%d:\n\n", label_index);
#endif

        JIT_FOREACH_INSN(block, insn)
        {
            is_last_insn = (insn->next == block) ? true : false;

#if CODEGEN_DUMP != 0
            os_printf("\n");
            jit_dump_insn(cc, insn);
#endif
            switch (insn->opcode) {
                case JIT_OP_MOV:
                    LOAD_2ARGS();
                    if (!lower_mov(cc, a, r0, r1))
                        GOTO_FAIL;
                    break;

                case JIT_OP_I32TOI8:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, I32, i8, i32, int32);
                    break;

                case JIT_OP_I32TOU8:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, I32, u8, i32, int32);
                    break;

                case JIT_OP_I32TOI16:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, I32, i16, i32, int32);
                    break;

                case JIT_OP_I32TOU16:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, I32, u16, i32, int32);
                    break;

                case JIT_OP_I32TOI64:
                    LOAD_2ARGS();
                    CONVERT_R_R(I64, I32, i64, i32, int32);
                    break;

                case JIT_OP_U32TOI64:
                    LOAD_2ARGS();
                    CONVERT_R_R(I64, I32, i64, u32, int32);
                    break;

                case JIT_OP_I32TOF32:
                case JIT_OP_U32TOF32:
                    LOAD_2ARGS();
                    CONVERT_R_R(F32, I32, f32, i32, int32);
                    break;

                case JIT_OP_I32TOF64:
                case JIT_OP_U32TOF64:
                    LOAD_2ARGS();
                    CONVERT_R_R(F64, I32, f64, i32, int32);
                    break;

                case JIT_OP_I64TOI32:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, I64, i32, i64, int64);
                    break;

                case JIT_OP_I64TOF32:
                    LOAD_2ARGS();
                    CONVERT_R_R(F32, I64, f32, i64, int64);
                    break;

                case JIT_OP_I64TOF64:
                    LOAD_2ARGS();
                    CONVERT_R_R(F64, I64, f64, i64, int64);
                    break;

                case JIT_OP_F32TOI32:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, F32, i32, f32, int32);
                    break;

                case JIT_OP_F32TOF64:
                    LOAD_2ARGS();
                    CONVERT_R_R(F64, F32, f64, f32, float32);
                    break;

                case JIT_OP_F64TOI32:
                    LOAD_2ARGS();
                    CONVERT_R_R(I32, F64, i32, f64, float64);
                    break;

                case JIT_OP_F64TOI64:
                    LOAD_2ARGS();
                    CONVERT_R_R(I64, F64, i64, f64, float64);
                    break;

                case JIT_OP_F64TOF32:
                    LOAD_2ARGS();
                    CONVERT_R_R(F32, F64, f32, f64, float64);
                    break;

                case JIT_OP_NEG:
                    LOAD_2ARGS();
                    if (!lower_neg(cc, a, r0, r1))
                        GOTO_FAIL;
                    break;

                case JIT_OP_ADD:
                case JIT_OP_SUB:
                case JIT_OP_MUL:
                case JIT_OP_DIV_S:
                case JIT_OP_REM_S:
                case JIT_OP_DIV_U:
                case JIT_OP_REM_U:
                    LOAD_3ARGS();
                    if (!lower_alu(cc, a,
                                   (ALU_OP)(ADD + (insn->opcode - JIT_OP_ADD)),
                                   r0, r1, r2))
                        GOTO_FAIL;
                    break;

                case JIT_OP_SHL:
                case JIT_OP_SHRS:
                case JIT_OP_SHRU:
                    LOAD_3ARGS();
                    if (!lower_shift(
                            cc, a,
                            (SHIFT_OP)(SHL + (insn->opcode - JIT_OP_SHL)), r0,
                            r1, r2))
                        GOTO_FAIL;
                    break;

                case JIT_OP_OR:
                case JIT_OP_XOR:
                case JIT_OP_AND:
                    LOAD_3ARGS();
                    if (!lower_bit(cc, a,
                                   (BIT_OP)(OR + (insn->opcode - JIT_OP_OR)),
                                   r0, r1, r2))
                        GOTO_FAIL;
                    break;

                case JIT_OP_CMP:
                    LOAD_3ARGS();
                    if (!lower_cmp(cc, a, r0, r1, r2))
                        GOTO_FAIL;
                    break;

                case JIT_OP_SELECTEQ:
                case JIT_OP_SELECTNE:
                case JIT_OP_SELECTGTS:
                case JIT_OP_SELECTGES:
                case JIT_OP_SELECTLTS:
                case JIT_OP_SELECTLES:
                case JIT_OP_SELECTGTU:
                case JIT_OP_SELECTGEU:
                case JIT_OP_SELECTLTU:
                case JIT_OP_SELECTLEU:
                    LOAD_4ARGS();
                    if (!lower_select(
                            cc, a,
                            (COND_OP)(EQ + (insn->opcode - JIT_OP_SELECTEQ)),
                            r0, r1, r2, r3))
                        GOTO_FAIL;
                    break;

                case JIT_OP_LDEXECENV:
                    LOAD_1ARG();
                    CHECK_KIND(r0, JIT_REG_KIND_I32);
                    /* TODO */
                    break;

                case JIT_OP_LDJITINFO:
                    LOAD_1ARG();
                    CHECK_KIND(r0, JIT_REG_KIND_I32);
                    /* TODO */
                    break;

                case JIT_OP_LDI8:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 1, true);
                    break;

                case JIT_OP_LDU8:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 1, false);
                    break;

                case JIT_OP_LDI16:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 2, true);
                    break;

                case JIT_OP_LDU16:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 2, false);
                    break;

                case JIT_OP_LDI32:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 4, true);
                    break;

                case JIT_OP_LDU32:
                    LOAD_3ARGS();
                    LD_R_R_R(I32, 4, false);
                    break;

                case JIT_OP_LDI64:
                case JIT_OP_LDU64:
                case JIT_OP_LDPTR:
                    LOAD_3ARGS();
                    LD_R_R_R(I64, 8, false);
                    break;

                case JIT_OP_LDF32:
                    LOAD_3ARGS();
                    LD_R_R_R(F32, 4, false);
                    break;

                case JIT_OP_LDF64:
                    LOAD_3ARGS();
                    LD_R_R_R(F64, 8, false);
                    break;

                case JIT_OP_STI8:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(I32, int32, 1);
                    break;

                case JIT_OP_STI16:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(I32, int32, 2);
                    break;

                case JIT_OP_STI32:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(I32, int32, 4);
                    break;

                case JIT_OP_STI64:
                case JIT_OP_STPTR:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(I64, int64, 8);
                    break;

                case JIT_OP_STF32:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(F32, float32, 4);
                    break;

                case JIT_OP_STF64:
                    LOAD_3ARGS_NO_ASSIGN();
                    ST_R_R_R(F64, float64, 8);
                    break;

                case JIT_OP_JMP:
                    LOAD_1ARG();
                    CHECK_KIND(r0, JIT_REG_KIND_L32);
                    if (!(is_last_insn
                          && label_is_neighboring(cc, label_index,
                                                  jit_reg_no(r0))))
                        JMP_TO_LABEL(jit_reg_no(r0), label_index);
                    break;

                case JIT_OP_BEQ:
                case JIT_OP_BNE:
                case JIT_OP_BGTS:
                case JIT_OP_BGES:
                case JIT_OP_BLTS:
                case JIT_OP_BLES:
                case JIT_OP_BGTU:
                case JIT_OP_BGEU:
                case JIT_OP_BLTU:
                case JIT_OP_BLEU:
                    LOAD_3ARGS();
                    if (!lower_branch(
                            cc, a, jmp_info_list, label_index,
                            (COND_OP)(EQ + (insn->opcode - JIT_OP_BEQ)), r0, r1,
                            r2, is_last_insn))
                        GOTO_FAIL;
                    break;

                case JIT_OP_LOOKUPSWITCH:
                {
                    JitOpndLookupSwitch *opnd = jit_insn_opndls(insn);
                    if (!lower_lookupswitch(cc, a, jmp_info_list, label_offsets,
                                            label_index, opnd, is_last_insn))
                        GOTO_FAIL;
                    break;
                }

                case JIT_OP_CALLNATIVE:
                    if (!lower_callnative(cc, a, jmp_info_list, label_index,
                                          insn))
                        GOTO_FAIL;
                    break;

                case JIT_OP_CALLBC:
                    if (!lower_callbc(cc, a, jmp_info_list, label_index, insn))
                        GOTO_FAIL;
                    break;

                case JIT_OP_RETURNBC:
                    if (!lower_returnbc(cc, a, label_index, insn))
                        GOTO_FAIL;
                    break;

                case JIT_OP_RETURN:
                    if (!lower_return(cc, a, insn))
                        GOTO_FAIL;
                    break;

                default:
                    jit_set_last_error_v(cc, "unsupported JIT opcode 0x%2x",
                                         insn->opcode);
                    GOTO_FAIL;
            }

            if (err_handler.err) {
                jit_set_last_error_v(
                    cc, "failed to generate native code for JIT opcode 0x%02x",
                    insn->opcode);
                GOTO_FAIL;
            }

#if CODEGEN_DUMP != 0
            dump_native((char *)code.sectionById(0)->buffer().data()
                            + code_offset,
                        code.sectionById(0)->buffer().size() - code_offset);
            code_offset = code.sectionById(0)->buffer().size();
#endif
        }
    }

    code_buf = (char *)code.sectionById(0)->buffer().data();
    code_size = code.sectionById(0)->buffer().size();
    if (!(stream = (char *)jit_code_cache_alloc(code_size))) {
        jit_set_last_error(cc, "allocate memory failed");
        goto fail;
    }

    bh_memcpy_s(stream, code_size, code_buf, code_size);
    cc->jitted_addr_begin = stream;
    cc->jitted_addr_end = stream + code_size;

    for (i = 0; i < label_num; i++) {
        if (i == 0)
            label_index = 0;
        else if (i == label_num - 1)
            label_index = 1;
        else
            label_index = i + 1;

        jitted_addr = jit_annl_jitted_addr(
            cc, jit_reg_new(JIT_REG_KIND_L32, label_index));
        *jitted_addr = stream + label_offsets[label_index];
    }

    patch_jmp_info_list(cc, jmp_info_list);
    return_value = true;

fail:

    jit_free(label_offsets);
    free_jmp_info_list(jmp_info_list);
    return return_value;
}

bool
jit_codegen_lower(JitCompContext *cc)
{
    return true;
}

void
jit_codegen_free_native(JitCompContext *cc)
{}

void
jit_codegen_dump_native(void *begin_addr, void *end_addr)
{
#if WASM_ENABLE_FAST_JIT_DUMP != 0
    os_printf("\n");
    dump_native((char *)begin_addr, (char *)end_addr - (char *)begin_addr);
    os_printf("\n");
#endif
}

bool
jit_codegen_init()
{
    const JitHardRegInfo *hreg_info = jit_codegen_get_hreg_info();
    JitGlobals *jit_globals = jit_compiler_get_jit_globals();
    char *code_buf, *stream;
    uint32 code_size;

    JitErrorHandler err_handler;
    Environment env(Arch::kX64);
    CodeHolder code;
    code.init(env);
    code.setErrorHandler(&err_handler);
    x86::Assembler a(&code);

    /* push callee-save registers */
    a.push(x86::rbp);
    a.push(x86::rbx);
    a.push(x86::r12);
    a.push(x86::r13);
    a.push(x86::r14);
    a.push(x86::r15);
    /* push info */
    a.push(x86::rsi);

    /* Note: the number of register pushed must be odd, as the stack pointer
       %rsp must be aligned to a 16-byte boundary before making a call, so
       when a function (including this function) gets control, %rsp is not
       aligned. We push odd number registers here to make %rsp happy before
       calling native functions. */

    /* exec_env_reg = exec_env */
    a.mov(regs_i64[hreg_info->exec_env_hreg_index], x86::rdi);
    /* fp_reg = info->frame */
    a.mov(x86::rbp, x86::ptr(x86::rsi, 0));
    /* jmp target */
    a.jmp(x86::rdx);

    if (err_handler.err)
        return false;

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
    /* info->frame = fp_reg */
    {
        x86::Mem m(x86::rsi, 0);
        a.mov(m, x86::rbp);
    }
    /* info->out.ret.ival[0, 1] = rcx */
    {
        x86::Mem m(x86::rsi, 8);
        a.mov(m, x86::rcx);
    }

    /* pop callee-save registers */
    a.pop(x86::r15);
    a.pop(x86::r14);
    a.pop(x86::r13);
    a.pop(x86::r12);
    a.pop(x86::rbx);
    a.pop(x86::rbp);
    a.ret();

    if (err_handler.err)
        goto fail1;

    code_buf = (char *)code.sectionById(0)->buffer().data();
    code_size = code.sectionById(0)->buffer().size();
    stream = (char *)jit_code_cache_alloc(code_size);
    if (!stream)
        goto fail1;

    bh_memcpy_s(stream, code_size, code_buf, code_size);
    code_block_return_to_interp_from_jitted = stream;

    jit_globals->return_to_interp_from_jitted =
        code_block_return_to_interp_from_jitted;
    return true;

fail1:
    jit_code_cache_free(code_block_switch_to_jitted_from_interp);
    return false;
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
    { 0, 1, 0, 1, 1, 1, 0 }, /* caller_saved_native */
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
    /* xmm0 ~ xmm15 */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* TBD:caller_saved_native */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* TBD:caller_saved_jitted */
};

static uint8 hreg_info_F64[3][16] = {
    /* xmm0 ~ xmm15 */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* TBD:caller_saved_native */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* TBD:caller_saved_jitted */
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

JitReg
jit_codegen_get_hreg_by_name(const char *name)
{
    if (strcmp(name, "eax") == 0)
        return jit_reg_new(JIT_REG_KIND_I32, REG_EAX_IDX);
    else if (strcmp(name, "ecx") == 0)
        return jit_reg_new(JIT_REG_KIND_I32, REG_ECX_IDX);
    else if (strcmp(name, "edx") == 0)
        return jit_reg_new(JIT_REG_KIND_I32, REG_EDX_IDX);
    else if (strcmp(name, "rax") == 0)
        return jit_reg_new(JIT_REG_KIND_I64, REG_RAX_IDX);
    else if (strcmp(name, "rcx") == 0)
        return jit_reg_new(JIT_REG_KIND_I64, REG_RCX_IDX);
    else if (strcmp(name, "rdx") == 0)
        return jit_reg_new(JIT_REG_KIND_I64, REG_RDX_IDX);

    return 0;
}
