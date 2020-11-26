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

void *
os_malloc(unsigned size)
{
    return NULL;
}

void *
os_realloc(void *ptr, unsigned size)
{
    return NULL;
}

void
os_free(void *ptr)
{
}

int os_printf(const char *format, ...)
{
    /* TODO: implement os_printf */
    return 0;
}

int
os_vprintf(const char *format, va_list ap)
{
    /* TODO: implement os_vprintf */
    return 1;
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

int atoi(const char *nptr)
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

/**
 * TODO: implement these APIs which are needed by libc_builtin_wrapper.c
 *       and wasm_runtime_common.c
 */
int strncasecmp(const char *s1, const char *s2, size_t n)
{
    os_printf("### unimplemented function strncasecmp called!\n");
    return 0;
}

long int strtol(const char *str, char **endptr, int base)
{
    os_printf("### unimplemented function strtol called!\n");
    return 0;
}

unsigned long int strtoul(const char *str, char **endptr, int base)
{
    os_printf("### unimplemented function strtoul called!\n");
    return 0;
}

unsigned long long int strtoull(const char *nptr, char **endptr, int base)
{
    os_printf("### unimplemented function strtoull called!\n");
    return 0;
}

double strtod(const char *nptr, char **endptr)
{
    os_printf("### unimplemented function strtod called!\n");
    return 0;
}

float strtof(const char *nptr, char **endptr)
{
    os_printf("### unimplemented function strtof called!\n");
    return 0;
}

char *strstr(const char *haystack, const char *needle)
{
    os_printf("### unimplemented function strstr called!\n");
    return NULL;
}

size_t strspn(const char *s, const char *accept)
{
    os_printf("### unimplemented function strspn called!\n");
    return 0;
}

size_t strcspn(const char *s, const char *reject)
{
    os_printf("### unimplemented function strcspn called!\n");
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    os_printf("### unimplemented function memchr called!\n");
    return NULL;
}

int isalnum(int c)
{
    os_printf("### unimplemented function isalnum called!\n");
    return 0;
}

int isxdigit(int c)
{
    os_printf("### unimplemented function isxdigit called!\n");
    return 0;
}

int isdigit(int c)
{
    os_printf("### unimplemented function isdigit called!\n");
    return 0;
}

int isprint(int c)
{
    os_printf("### unimplemented function isprint called!\n");
    return 0;
}

int isgraph(int c)
{
    os_printf("### unimplemented function isgraph called!\n");
    return 0;
}

int isspace(int c)
{
    os_printf("### unimplemented function isspace called!\n");
    return 0;
}

int isalpha(int c)
{
    os_printf("### unimplemented function isalpha called!\n");
    return 0;
}

int isupper(int c)
{
    os_printf("### unimplemented function isupper called!\n");
    return 0;
}

int toupper(int c)
{
    os_printf("### unimplemented function toupper called!\n");
    return 0;
}

int tolower(int c)
{
    os_printf("### unimplemented function tolower called!\n");
    return 0;
}

void *memmove(void *dest, const void *src, size_t n)
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

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

static void swap32(uint8_t* pData)
{
    uint8_t value = *pData;
    *pData = *(pData + 3);
    *(pData + 3) = value;

    value = *(pData + 1);
    *(pData + 1) = *(pData + 2);
    *(pData + 2) = value;
}

static void swap16(uint8_t* pData)
{
    uint8_t value = *pData;
    *(pData) = *(pData + 1);
    *(pData + 1) = value;
}

uint32_t htonl(uint32_t value)
{
    uint32_t ret;
    if (is_little_endian()) {
        ret = value;
        swap32((uint8*) &ret);
        return ret;
    }

    return value;
}

uint32_t ntohl(uint32_t value)
{
    return htonl(value);
}

uint16_t htons(uint16_t value)
{
    uint16_t ret;
    if (is_little_endian()) {
        ret = value;
        swap16((uint8_t *)&ret);
        return ret;
    }

    return value;
}

uint16_t ntohs(uint16_t value)
{
    return htons(value);
}

