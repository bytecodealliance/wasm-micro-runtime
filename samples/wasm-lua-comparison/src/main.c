/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>

#include "wasm_export.h"
#include "bh_read_file.h"
#include "pthread.h"

// Include Lua Library
#include "lua.h"
#include <lualib.h>
#include <lauxlib.h>

#include "luaModule.h"
#include "wasm.h"

int
main(int argc, char *argv[])
{
    clock_t start_t, stop_t;
    double total_t;
    lua_State* L= luaL_newstate();

    init_wasm();

    init_lua();

    call_lua_function(L);

    call_wasm_function();

    start_t= clock();
    int test= sum(2,3);
    stop_t= clock();
    printf("C sum: %d\n", test);
    total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
    printf("Total time = %f\n", total_t);

    return 0;
}

int sum(int start, int length)
{
    int sum = 0, i, j;

    for(j=0; j<10000000; j++){
        for (i = start; i < start + length; i++) {
            sum += i;
        }
    }
    return sum;
}
