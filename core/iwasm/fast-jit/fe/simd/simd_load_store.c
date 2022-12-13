/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_load_store.h"

bool
jit_compile_simd_v128_load(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_simd_load_extend(JitCompContext *cc, uint8 opcode, uint32 align,
                             uint32 offset)
{
    return false;
}

bool
jit_compile_simd_load_splat(JitCompContext *cc, uint8 opcode, uint32 align,
                            uint32 offset)
{
    return false;
}

bool
jit_compile_simd_load_lane(JitCompContext *cc, uint8 opcode, uint32 align,
                           uint32 offset, uint8 lane_id)
{
    return false;
}

bool
jit_compile_simd_load_zero(JitCompContext *cc, uint8 opcode, uint32 align,
                           uint32 offset)
{
    return false;
}

bool
jit_compile_simd_v128_store(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_simd_store_lane(JitCompContext *cc, uint8 opcode, uint32 align,
                            uint32 offset, uint8 lane_id)
{
    return false;
}
