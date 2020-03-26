/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "wasm_export.h"
#include "math.h"

// The first parameter is not exec_env because it is invoked by native funtions
void reverse(char * str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// The first parameter exec_env must be defined using type wasm_exec_env_t
// which is the calling convention for exporting native API by WAMR.
//
// Converts a given integer x to string str[].
// digit is the number of digits required in the output.
// If digit is more than the number of digits in x,
// then 0s are added at the beginning.
int intToStr(wasm_exec_env_t exec_env, int x, char* str, int str_len, int digit)
{
    int i = 0;

    printf ("calling into native function: %s\n", __FUNCTION__);

    while (x) {
        // native is responsible for checking the str_len overflow
        if (i >= str_len) {
            return -1;
        }
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < digit) {
        if (i >= str_len) {
            return -1;
        }
        str[i++] = '0';
    }

    reverse(str, i);

    if (i >= str_len)
        return -1;
    str[i] = '\0';
    return i;
}

int get_pow(wasm_exec_env_t exec_env, int x, int y) {
    printf ("calling into native function: %s\n", __FUNCTION__);
    return (int)pow(x, y);
}
