/*
 * Copyright (C) 2024 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdint.h>

uint32_t
host_consume_stack_and_call_indirect(int (*)(int), uint32_t, uint32_t);
uint32_t host_consume_stack(uint32_t);

int
cb(int x)
{
    return x * x;
}

int
consume_stack_cb(int x)
{
    /*
     * intentions:
     *
     * - consume native stack by making recursive calls
     *
     * - avoid tail-call optimization (either by the C compiler or
     *   aot-compiler)
     */
    if (x == 0) {
        return 0;
    }
    return consume_stack_cb(x - 1) + 1;
}

int
host_consume_stack_cb(int x)
{
    return host_consume_stack(x);
}

__attribute__((export_name("test"))) uint32_t
test(uint32_t native_stack, uint32_t recurse_count)
{
    uint32_t ret;
    ret = host_consume_stack_and_call_indirect(cb, 321, native_stack);
    ret = host_consume_stack_and_call_indirect(consume_stack_cb, recurse_count,
                                               native_stack);
#if 0 /* notyet */
    ret = host_consume_stack_and_call_indirect(host_consume_stack_cb, 1000000,
                                          native_stack);
#endif
    return 42;
}
