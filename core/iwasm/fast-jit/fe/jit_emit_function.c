/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_function.h"
#include "../jit_frontend.h"

bool
jit_compile_op_call(JitCompContext *cc, uint32 func_idx, bool tail_call)
{
    return false;
}

bool
jit_compile_op_call_indirect(JitCompContext *cc, uint32 type_idx,
                             uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_ref_null(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_ref_is_null(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_ref_func(JitCompContext *cc, uint32 func_idx)
{
    return false;
}
