#include "wasm_export.h"
#include "pthread.h"

typedef struct ThreadArgs {
    wasm_exec_env_t exec_env;
    int start;
    int length;
} ThreadArgs;

void init(void *arg)
{
    ThreadArgs *thread_arg = (ThreadArgs *)arg;
    wasm_exec_env_t exec_env = thread_arg->exec_env;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_function_inst_t func;
    uint32 argv[2];
}

init_env()
{
    if (!wasm_runtime_init_thread_env()) {
        printf("failed to initialize thread environment");
        return NULL;
    }
}

call_func(void *arg)
{
    init(*arg);
    func = wasm_runtime_lookup_function(module_inst, "sum", NULL);
    if (!func) {
        printf("failed to lookup function sum");
        wasm_runtime_destroy_thread_env();
        return NULL;
    }
    argv[0] = thread_arg->start;
    argv[1] = thread_arg->length;

    /* call the WASM function */
    if (!wasm_runtime_call_wasm(exec_env, func, 2, argv)) {
        printf("%s\n", wasm_runtime_get_exception(module_inst));
        wasm_runtime_destroy_thread_env();
        return NULL;
    }
}


