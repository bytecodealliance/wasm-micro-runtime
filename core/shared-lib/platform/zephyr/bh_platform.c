/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_memory.h"
#include "bh_common.h"
#include <stdlib.h>
#include <string.h>

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

void *
bh_mmap(void *hint, unsigned int size, int prot, int flags)
{
    return bh_malloc(size);
}

void
bh_munmap(void *addr, uint32 size)
{
    return bh_free(addr);
}

int
bh_mprotect(void *addr, uint32 size, int prot)
{
    return 0;
}
