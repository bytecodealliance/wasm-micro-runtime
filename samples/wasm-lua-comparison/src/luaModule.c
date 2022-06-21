

#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define THREAD_NUM 10

void init_lua()
{
    printf("%s\n", __FUNCTION__);
    lua_State* L = luaL_newstate();
}


void deInit_Lua(lua_State* L)
{
    printf("%s\n", __FUNCTION__);
    lua_close(L);
}


/**
 * @brief 
 * Note this function will need input
 */
void call_lua_function(lua_State* L)
{
    printf("%s\n", __FUNCTION__);
    luaL_openlibs(L);
    luaL_dofile(L, "wasm-apps/sum.c");
}