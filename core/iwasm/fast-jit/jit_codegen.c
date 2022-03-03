/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_compiler.h"
#include "jit_codegen.h"

bool
jit_pass_lower_cg(JitCompContext *cc)
{
    return jit_codegen_lower(cc);
}

bool
jit_pass_codegen(JitCompContext *cc)
{
#if 0
    bh_assert(jit_annl_is_enabled_next_label(cc));

    if (!jit_annl_enable_jitted_addr(cc))
        return false;
#endif

    return jit_codegen_gen_native(cc);
}
