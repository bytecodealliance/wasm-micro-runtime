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

#include "wasm_platform.h"

#ifndef CONFIG_AEE_ENABLE
static int
_stdout_hook_iwasm(int c)
{
    printk("%c", (char)c);
    return 1;
}

extern void __stdout_hook_install(int (*hook)(int));
#endif

bool is_little_endian = false;

bool __is_little_endian()
{
    union w
    {
        int a;
        char b;
    }c;

    c.a = 1;
    return (c.b == 1);
}

int wasm_platform_init()
{
    if (__is_little_endian())
        is_little_endian = true;

#ifndef CONFIG_AEE_ENABLE
    /* Enable printf() in Zephyr */
    __stdout_hook_install(_stdout_hook_iwasm);
#endif

    return 0;
}

