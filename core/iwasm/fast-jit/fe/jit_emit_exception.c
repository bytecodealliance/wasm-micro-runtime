/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_exception.h"
#include "../jit_frontend.h"

bool
jit_emit_exception(JitCompContext *cc, int32 exception_id, bool is_cond_br,
                   JitReg cond_br_if, JitBasicBlock *cond_br_else_block)
{
    return false;
}
