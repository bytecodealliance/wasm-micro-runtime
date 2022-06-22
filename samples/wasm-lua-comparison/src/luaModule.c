

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    clock_t start_t, stop_t;
    double total_t;
    printf("%s\n", __FUNCTION__);
    luaL_openlibs(L);
    luaL_dofile(L, "test.lua");
    lua_getglobal(L, "sum");
    lua_pushnumber(L,2);
    lua_pushnumber(L,3);
    start_t= clock();
    lua_call(L,2,1);
    stop_t= clock();
    int sum = (int)lua_tointeger(L,-1);
    lua_pop(L,1);
    printf("sum: %d\n", sum);
    total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
    printf("Total time = %f\n", total_t);
}