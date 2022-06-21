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
    lua_State* L= luaL_newstate();

    init_wasm();

    init_lua();

    call_lua_function(L);

    call_wasm_function();

    return 0;
}
