/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bh_definition.h"
#include "bh_platform.h"

int bh_return(int ret)
{
    return ret;
}

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

int b_strcat_s(char * s1, size_t s1max, const char * s2)
{
    if (NULL
            == s1|| NULL == s2 || s1max < (strlen(s1) + strlen(s2) + 1) || s1max > RSIZE_MAX) {
        return -1;
    }

    strcat(s1, s2);

    return 0;
}

int b_strcpy_s(char * s1, size_t s1max, const char * s2)
{
    if (NULL
            == s1|| NULL == s2 || s1max < (strlen(s2) + 1) || s1max > RSIZE_MAX) {
        return -1;
    }

    strcpy(s1, s2);

    return 0;
}
