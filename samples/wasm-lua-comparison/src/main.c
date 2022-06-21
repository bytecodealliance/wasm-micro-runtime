/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include "bh_read_file.h"
#include "pthread.h"
#include "lua.h"
#include "wasm.h"
//#include "/usr/local/include/lauxlib.h"
//#include "/usr/local/include/lualib.h"
#include "/usr/local/include/lua.hpp"

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
