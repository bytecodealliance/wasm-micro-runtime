/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include <stdio.h>

void bh_log_emit(const char *fmt, va_list ap)
{
    bh_vprintf_sgx(fmt, ap);
}

int bh_fflush(void *stream)
{
    return 0;
}
