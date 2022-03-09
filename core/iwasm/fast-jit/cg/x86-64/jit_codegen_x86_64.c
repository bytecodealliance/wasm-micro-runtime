/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_codegen.h"

bool
jit_codegen_init()
{
    return true;
}

void
jit_codegen_destroy()
{}

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
      1, 1, 0, 0, 0, 0, 0, 0 }, /* caller_saved_native */
    { 0, 0, 0, 0, 0, 0, 0, 0,
      1, 1, 0, 0, 0, 0, 0, 0 }, /* caller_saved_jitted */
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
jit_codegen_dump_native(void *begin_addr, void *end_addr)
{}

bool
jit_codegen_call_func_jitted(void *exec_env, void *frame, void *func_inst,
                             void *target)
{
    return false;
}
