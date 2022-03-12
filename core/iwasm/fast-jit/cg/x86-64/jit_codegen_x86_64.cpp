/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_codegen.h"
#include "jit_codecache.h"
#include "jit_compiler.h"

#include <asmjit/core.h>
#include <asmjit/x86.h>

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

void
jit_codegen_dump_native(void *begin_addr, void *end_addr)
{}

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

    a.setOffset(0);
    a.pop(x86::rsi);
    a.pop(x86::rdi);
    a.pop(x86::r15);
    a.pop(x86::r14);
    a.pop(x86::r13);
    a.pop(x86::r12);
    a.push(x86::rbx);
    a.push(x86::rbp);

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
static const uint8 hreg_info_I4[3][7] = {
    /* ebp, eax, ebx, ecx, edx, edi, esi */
    { 1, 0, 0, 0, 0, 0, 1 }, /* fixed, esi is freely used */
    { 0, 1, 0, 1, 1, 0, 0 }, /* caller_saved_native */
    { 0, 1, 0, 1, 1, 1, 0 }  /* caller_saved_jitted */
};

static const uint8 hreg_info_I8[3][16] = {
    /* rbp, rax, rbx, rcx, rdx, rdi, rsi, rsp,
       r8,  r9,  r10, r11, r12, r13, r14, r15 */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 1 }, /* fixed, rsi is freely used */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 0, 0, 0, 0 }, /* caller_saved_native */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 0 }, /* caller_saved_jitted */
};

static uint8 hreg_info_F4[3][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* fixed, rsi is freely used */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_native */
    { 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1 }, /* caller_saved_jitted */
};

static uint8 hreg_info_F8[3][16] = {
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

        { sizeof(hreg_info_I4[0]), /* I4 */
          hreg_info_I4[0],
          hreg_info_I4[1],
          hreg_info_I4[2] },

        { sizeof(hreg_info_I8[0]), /* I8 */
          hreg_info_I8[0],
          hreg_info_I8[1],
          hreg_info_I8[2] },

        { sizeof(hreg_info_F4[0]), /* F4 */
          hreg_info_F4[0],
          hreg_info_F4[1],
          hreg_info_F4[2] },

        { sizeof(hreg_info_F8[0]), /* F8 */
          hreg_info_F8[0],
          hreg_info_F8[1],
          hreg_info_F8[2] },

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
