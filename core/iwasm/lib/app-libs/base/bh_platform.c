/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *
 *
 */

static bool is_little_endian()
{
    long i = 0x01020304;
    unsigned char* c = (unsigned char*) &i;
    return (*c == 0x04) ? true : false;
}

static void swap32(uint8* pData)
{
    uint8 value = *pData;
    *pData = *(pData + 3);
    *(pData + 3) = value;

    value = *(pData + 1);
    *(pData + 1) = *(pData + 2);
    *(pData + 2) = value;
}

static void swap16(uint8* pData)
{
    uint8 value = *pData;
    *(pData) = *(pData + 1);
    *(pData + 1) = value;
}

uint32 htonl(uint32 value)
{
    uint32 ret;
    if (is_little_endian()) {
        ret = value;
        swap32((uint8*) &ret);
        return ret;
    }

    return value;
}

uint32 ntohl(uint32 value)
{
    return htonl(value);
}

uint16 htons(uint16 value)
{
    uint16 ret;
    if (is_little_endian()) {
        ret = value;
        swap16((uint8 *)&ret);
        return ret;
    }

    return value;
}

uint16 ntohs(uint16 value)
{
    return htons(value);
}

char *wa_strdup(const char *s)
{
    char *s1 = NULL;
    if (s && (s1 = wa_malloc(strlen(s) + 1)))
        memcpy(s1, s, strlen(s) + 1);
    return s1;
}

#define RSIZE_MAX 0x7FFFFFFF
int b_memcpy_s(void * s1, unsigned int s1max, const void * s2, unsigned int n)
{
    char *dest = (char*) s1;
    char *src = (char*) s2;
    if (n == 0) {
        return 0;
    }

    if (s1 == NULL || s1max > RSIZE_MAX) {
        return -1;
    }
    if (s2 == NULL || n > s1max) {
        memset(dest, 0, s1max);
        return -1;
    }
    memcpy(dest, src, n);
    return 0;
}
