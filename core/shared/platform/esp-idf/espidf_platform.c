/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"


int errno = 0;

int
os_thread_sys_init();

void
os_thread_sys_destroy();

int
bh_platform_init()
{
    return os_thread_sys_init();
}

void
bh_platform_destroy()
{
    os_thread_sys_destroy();
}

int os_printf(const char *format, ...)
{
    int ret = 0;
    va_list ap;

    va_start(ap, format);
    ret += vprintf(format, ap);
    va_end(ap);

    return ret;
}

int
os_vprintf(const char *format, va_list ap)
{
    return vprintf(format, ap);
}

void *
os_mmap(void *hint, size_t size, int prot, int flags)
{

    return BH_MALLOC(size);
}

void
os_munmap(void *addr, size_t size)
{
    BH_FREE(addr);
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{
}

int
atoi(const char *nptr)
{
    bool is_negative = false;
    int total = 0;
    const char *p = nptr;
    char temp = '0';

    if (NULL == p) {
        os_printf("invlaid atoi input\n");
        return 0;
    }

    if (*p == '-') {
        is_negative = true;
        p++;
    }

    while ((temp = *p++) != '\0') {
        if (temp > '9' || temp < '0') {
            continue;
        }

        total = total * 10 + (int)(temp - '0');
    }

    if (is_negative)
        total = 0 - total;

    return total;
}

void *
memmove(void *dest, const void *src, size_t n)
{
    char *d = dest;
    const char *s = src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    }
    else {
        const char *lasts = s + (n-1);
        char *lastd = d + (n-1);
        while (n--)
            *lastd-- = *lasts--;
    }
    return dest;
}

