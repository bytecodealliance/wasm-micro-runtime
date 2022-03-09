/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_compare.h"
#include "../jit_frontend.h"

bool
jit_compile_op_i32_compare(JitCompContext *cc, IntCond cond)
{
    return false;
}

bool
jit_compile_op_i64_compare(JitCompContext *cc, IntCond cond)
{
    return false;
}

bool
jit_compile_op_f32_compare(JitCompContext *cc, FloatCond cond)
{
    return false;
}

bool
jit_compile_op_f64_compare(JitCompContext *cc, FloatCond cond)
{
    return false;
}
