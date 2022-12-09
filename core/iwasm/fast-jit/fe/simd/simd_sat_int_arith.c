/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_sat_int_arith.h"

bool
jit_compile_simd_i8x16_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed)
{
    return false;
}

bool
jit_compile_simd_i16x8_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed)
{
    return false;
}

bool
jit_compile_simd_i32x4_saturate(JitCompContext *cc, V128Arithmetic arith_op,
                                bool is_signed)
{
    return false;
}
