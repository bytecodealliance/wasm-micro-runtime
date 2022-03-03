/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_table.h"
#include "../jit_frontend.h"

bool
jit_compile_op_elem_drop(JitCompContext *cc, uint32 tbl_seg_idx)
{
    return false;
}

bool
jit_compile_op_table_get(JitCompContext *cc, uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_table_set(JitCompContext *cc, uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_table_init(JitCompContext *cc, uint32 tbl_idx,
                          uint32 tbl_seg_idx)
{
    return false;
}

bool
jit_compile_op_table_copy(JitCompContext *cc, uint32 src_tbl_idx,
                          uint32 dst_tbl_idx)
{
    return false;
}

bool
jit_compile_op_table_size(JitCompContext *cc, uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_table_grow(JitCompContext *cc, uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_table_fill(JitCompContext *cc, uint32 tbl_idx)
{
    return false;
}
