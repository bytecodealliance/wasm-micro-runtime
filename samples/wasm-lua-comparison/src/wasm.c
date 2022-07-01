#include "wasm.h" 
#include "wasm_export.h"
#include "bh_read_file.h"
#include <time.h>

#define THREAD_NUM 10

typedef struct ThreadArgs {
    wasm_exec_env_t exec_env;
    int start;
    int length;
} ThreadArgs;
char *wasm_file = "wasm-apps/test.wasm";
uint8 *wasm_file_buf = NULL;
uint32 wasm_file_size, wasm_argv[2], i, threads_created;
uint32 stack_size = 16 * 1024, heap_size = 16 * 1024;
wasm_module_t wasm_module = NULL;
wasm_module_inst_t wasm_module_inst = NULL;
wasm_exec_env_t exec_env = NULL;
RuntimeInitArgs init_args;
ThreadArgs thread_arg[THREAD_NUM];
pthread_t tid[THREAD_NUM];
wasm_thread_t wasm_tid;
uint32 result, sum;
wasm_function_inst_t func;
char error_buf[128] = { 0 };
pthread_t wasm_thread;

void init_wasm()
{
    printf("%s\n", __FUNCTION__);
    memset(thread_arg, 0, sizeof(ThreadArgs) * THREAD_NUM);
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;
    init_args.max_thread_num = THREAD_NUM;

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }
   
    /* load WASM byte buffer from WASM bin file */
    if (!(wasm_file_buf =
              (uint8 *)bh_read_file_to_buffer(wasm_file, &wasm_file_size)))
       return -1;
    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        return -1;
    }

    /* instantiate the module */
    if (!(wasm_module_inst =
              wasm_runtime_instantiate(wasm_module, stack_size, heap_size,
                                       error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        return -1;
    }

    /* Create the first exec_env */
    if (!(exec_env =
              wasm_runtime_create_exec_env(wasm_module_inst, stack_size))) {
        printf("failed to create exec_env\n");
        return -1;
    }
    func = wasm_runtime_lookup_function(wasm_module_inst, "power", NULL);
    if (!func) {
        printf("failed to lookup function sum");
        return -1;
    }
}


void deInit_wasm()
{
    printf("%s\n", __FUNCTION__);
}


/**
 * @brief 
 * Note this function will need input
 */
void call_wasm_function()
{
    clock_t start_t, stop_t;
    double total_t;
    printf("%s\n", __FUNCTION__);
    wasm_argv[0] = 1000000000;
    wasm_argv[1] = 3;

    /*
     * Execute the wasm function in current thread, get the expect result
     */
    start_t= clock();
    if (!wasm_runtime_call_wasm(exec_env, func, 2, wasm_argv)) {
        printf("%s\n", wasm_runtime_get_exception(wasm_module_inst));
    }
    stop_t= clock();
    printf("expect result: %d\n", wasm_argv[0]);
    total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
    printf("WASM Total time = %f\n", total_t);
    return NULL;
}

void wasm_thread_function()
{
    clock_t start_t, stop_t;
    double total_t;
    wasm_runtime_spawn_thread(exec_env,&wasm_tid, call_wasm_function, NULL);
    start_t= clock();
    wasm_runtime_join_thread(wasm_tid, NULL);
    stop_t= clock();
    total_t=(double)(stop_t-start_t)/ CLOCKS_PER_SEC;
    printf("WASM Thread Total time = %f\n", total_t);
}
