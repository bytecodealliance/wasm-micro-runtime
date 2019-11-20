/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_platform.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FIXED_BUFFER_SIZE (1<<14)
static bh_print_function_t print_function = NULL;

char *bh_strdup(const char *s)
{
    uint32 size;
    char *s1 = NULL;

    if (s) {
        size = (uint32)(strlen(s) + 1);
        if ((s1 = bh_malloc(size)))
            bh_memcpy_s(s1, size, s, size);
    }
    return s1;
}

const unsigned short ** __ctype_b_loc(void)
{
    /* TODO */
    return NULL;
}

const int32_t ** __ctype_toupper_loc(void)
{
    /* TODO */
    return NULL;
}

const int32_t ** __ctype_tolower_loc(void)
{
    /* TODO */
    return NULL;
}

int bh_platform_init()
{
    return 0;
}

int putchar(int c)
{
    return 0;
}

int puts(const char *s)
{
    return 0;
}

void bh_set_print_function(bh_print_function_t pf)
{
    print_function = pf;
}

int bh_printf_sgx(const char *message, ...)
{
    if (print_function != NULL) {
        char msg[FIXED_BUFFER_SIZE] = { '\0' };
        va_list ap;
        va_start(ap, message);
        vsnprintf(msg, FIXED_BUFFER_SIZE, message, ap);
        va_end(ap);
        print_function(msg);
    }

    return 0;
}
