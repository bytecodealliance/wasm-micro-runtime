/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include <stdlib.h>
#include <string.h>

char *bh_strdup(const char *s)
{
    char *s1 = NULL;
    if (s && (s1 = bh_malloc(strlen(s) + 1)))
        memcpy(s1, s, strlen(s) + 1);
    return s1;
}

#ifndef CONFIG_AEE_ENABLE
static int
_stdout_hook_iwasm(int c)
{
    printk("%c", (char)c);
    return 1;
}

int bh_platform_init()
{
    extern void __stdout_hook_install(int (*hook)(int));
    /* Enable printf() in Zephyr */
    __stdout_hook_install(_stdout_hook_iwasm);
    return 0;
}
#else
int bh_platform_init()
{
    return 0;
}
#endif
