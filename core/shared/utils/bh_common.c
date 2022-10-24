/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"


static char *
align_ptr(char *src, uint32 b)
{
    uintptr_t v = (uintptr_t)src;
    uintptr_t m = b - 1;
    return (char *)((v + m) & ~m);
}

int
b_memcpy_aw(void *s1, unsigned int s1max, const void *s2, unsigned int n)
{
    char *dest = (char *)s1;
    char *src  = (char *)s2;

    if(dest == NULL || src == NULL) {
        return -1;
    }

    if(s1max < n) {
        return -1;
    }

    char *pa = align_ptr(src, sizeof(unsigned int));
    char *pb = align_ptr((src + n), sizeof(unsigned int)) - sizeof(unsigned int);

    char *p_pre = pa, *p_suf =pa, *p_read=pa;

    if (pa > src) {
        p_pre = pa - sizeof(unsigned int);
    }
    if (pa < pb) {
        p_suf = pb;
    }

    unsigned int pre_offset = 0;
    unsigned int pre_size = 0;
    if (p_pre != p_read) {
        pre_offset = src - p_pre;
        if (src + n > p_read) {
            pre_size = p_read - src;
        }
        else {
            pre_size = n;
        }
    }

    unsigned int read_size = 0;
    unsigned int suf_offset = 0;
    unsigned int suf_size = 0;
    if (p_suf != p_read) {
        read_size = p_suf - p_read;
        suf_size = src + n - p_suf;
    }
    else {
        if (src + n > pa) {
            read_size = src + n - pa;
        }
    }
    if((pre_size + read_size + suf_size) != n) {
        return -1;
    }

    // copy pre segment
    unsigned int buff;
    char* pbuff = (char*)&buff;
    buff = (*(unsigned int*)p_pre);
    bh_memcpy_s(dest, pre_size, pbuff + pre_offset,
                pre_size);

    // copy segment
    if (read_size < 4) {
        buff = (*(unsigned int*)p_read);
        bh_memcpy_s(dest + pre_size, read_size, pbuff, read_size);
    }
    else {
        unsigned int* des = (unsigned int*)(dest + pre_size);
        unsigned int* src = (unsigned int*)p_read;
        for(int i = 0;i < read_size/4;i++)
            *des++ = *src++;
    }

    // copy suffix segment
    buff = (*(unsigned int*)p_suf);
    bh_memcpy_s(dest + pre_size + read_size, suf_size,
                pbuff, suf_size);

    return 0;
}

int
b_memcpy_s(void *s1, unsigned int s1max, const void *s2, unsigned int n)
{
    char *dest = (char *)s1;
    char *src = (char *)s2;
    if (n == 0) {
        return 0;
    }

    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL || n > s1max) {
        memset(dest, 0, s1max);
        return -1;
    }
    memcpy(dest, src, n);
    return 0;
}

int
b_memmove_s(void *s1, unsigned int s1max, const void *s2, unsigned int n)
{
    char *dest = (char *)s1;
    char *src = (char *)s2;
    if (n == 0) {
        return 0;
    }

    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL || n > s1max) {
        memset(dest, 0, s1max);
        return -1;
    }
    memmove(dest, src, n);
    return 0;
}

int
b_strcat_s(char *s1, unsigned int s1max, const char *s2)
{
    if (NULL == s1 || NULL == s2 || s1max < (strlen(s1) + strlen(s2) + 1)) {
        return -1;
    }

    memcpy(s1 + strlen(s1), s2, strlen(s2) + 1);
    return 0;
}

int
b_strcpy_s(char *s1, unsigned int s1max, const char *s2)
{
    if (NULL == s1 || NULL == s2 || s1max < (strlen(s2) + 1)) {
        return -1;
    }

    memcpy(s1, s2, strlen(s2) + 1);
    return 0;
}

char *
bh_strdup(const char *s)
{
    uint32 size;
    char *s1 = NULL;

    if (s) {
        size = (uint32)(strlen(s) + 1);
        if ((s1 = BH_MALLOC(size)))
            bh_memcpy_s(s1, size, s, size);
    }
    return s1;
}

char *
wa_strdup(const char *s)
{
    uint32 size;
    char *s1 = NULL;

    if (s) {
        size = (uint32)(strlen(s) + 1);
        if ((s1 = WA_MALLOC(size)))
            bh_memcpy_s(s1, size, s, size);
    }
    return s1;
}
