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

struct arg_struct {
        int arg1;
        int arg2;
};

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
    int pow=power(1000000000);
    stop_t= clock();
    total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
    printf("Native total time = %f\n\n", total_t);
    printf("power native: %d\n", pow);
    printf("Starting thread example \n");
    pthread_t thread1, thread2, thread3;
    int  iret1, iret2, iret3;
    struct arg_struct args;
    args.arg1 = 2;
    args.arg2 = 3;
     
    /* Create independent threads each of which will execute function */

    iret1 = pthread_create( &thread1, NULL, call_wasm_function, NULL);
    iret2 = pthread_create( &thread2, NULL, call_lua_function, L);
    iret3 = pthread_create( &thread3, NULL, power, (void*) &args);
     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL); 
     start_t= clock();
     pthread_join( thread3, NULL);
     stop_t= clock();
     printf("C sum: %d\n", pow);
     total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
     printf("Native Thread Total time = %f\n", total_t);
     wasm_thread_function();
    exit(0);
}

int power(int n){
	int start=7;
	for(int i=0;i<n;i++){
		start*=start+1;
	}
	return start;
}