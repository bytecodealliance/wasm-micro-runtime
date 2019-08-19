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

#include "bh_common.h"
#include "bh_platform.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FIXED_BUFFER_SIZE (1<<14)
static bh_print_function_t print_function = NULL;

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

int bh_printf(const char *message, ...)
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
