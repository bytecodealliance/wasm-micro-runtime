/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_assert.h"
#include <stdarg.h>

#ifdef BH_TEST
#include <setjmp.h>
#endif

#ifdef BH_TEST
/* for exception throwing */
jmp_buf bh_test_jb;
#endif

void bh_assert_internal(int v, const char *file_name, int line_number,
        const char *expr_string)
{
    if (v)
        return;

    if (!file_name)
        file_name = "NULL FILENAME";
    if (!expr_string)
        expr_string = "NULL EXPR_STRING";

    printf("\nASSERTION FAILED: %s, at FILE=%s, LINE=%d\n", expr_string,
            file_name, line_number);

#ifdef BH_TEST
    longjmp(bh_test_jb, 1);
#endif

    abort();
}

void bh_debug_internal(const char *file_name, int line_number, const char *fmt,
        ...)
{
#ifndef JEFF_TEST_VERIFIER
    va_list args;

    va_start(args, fmt);
    bh_assert(file_name);

    printf("\nDebug info FILE=%s, LINE=%d: ", file_name, line_number);
    vprintf(fmt, args);

    va_end(args);
    printf("\n");
#endif
}

