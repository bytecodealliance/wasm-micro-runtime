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

int bh_platform_init()
{
    return 0;
}

