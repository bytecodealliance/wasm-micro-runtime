#include <stdio.h>
#include <stdlib.h>

#include "bh_platform.h"
#include "bh_read_file.h"
#include "wasm_export_checked.h"

#define VERIFY_API_RESULT(callee, result, fail_label)          \
    do {                                                       \
        if (result.error_code != 0) {                          \
            printf("%s failed with error code: %d\n", #callee, \
                   result.error_code);                         \
            goto fail_label;                                   \
        }                                                      \
    } while (0)

int
main(int argc, char *argv_main[])
{
    Result api_result;
    wasm_module_t module = NULL;
    uint32 buf_size, stack_size = 8092, heap_size = 8092;
    wasm_module_inst_t module_inst = NULL;
    wasm_function_inst_t func = NULL;
    wasm_exec_env_t exec_env = NULL;
    int ret = EXIT_FAILURE;

    RuntimeInitArgs init_args;
    // 512Kb
    static char global_heap_buf[512 * 1024];
    char *wasm_path = "fib.wasm";
    char *buffer;
    char error_buf[128];

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    api_result = wasm_runtime_full_init_checked(&init_args);
    VERIFY_API_RESULT(wasm_runtime_full_init_checked, api_result, fail);

    api_result = wasm_runtime_set_log_level_checked(WASM_LOG_LEVEL_VERBOSE);
    VERIFY_API_RESULT(wasm_runtime_set_log_level_checked, api_result,
                      release_runtime);

    buffer = bh_read_file_to_buffer(wasm_path, &buf_size);
    if (buffer == NULL) {
        printf("Open wasm app file [%s] failed.\n", wasm_path);
        goto release_runtime;
    }

    api_result = wasm_runtime_load_checked((uint8 *)buffer, buf_size, error_buf,
                                           sizeof(error_buf));
    VERIFY_API_RESULT(wasm_runtime_load_checked, api_result, release_file);
    module = api_result.value.wasm_module_t_value;

    api_result = wasm_runtime_instantiate_checked(module, stack_size, heap_size,
                                                  error_buf, sizeof(error_buf));
    VERIFY_API_RESULT(wasm_runtime_instantiate_checked, api_result,
                      release_module);
    module_inst = api_result.value.wasm_module_inst_t_value;

    api_result = wasm_runtime_create_exec_env_checked(module_inst, stack_size);
    VERIFY_API_RESULT(wasm_runtime_create_exec_env_checked, api_result,
                      release_instance);
    exec_env = api_result.value.wasm_exec_env_t_value;

    api_result = wasm_runtime_lookup_function_checked(module_inst, "fib");
    VERIFY_API_RESULT(wasm_runtime_lookup_function_checked, api_result,
                      release_exec_env);
    func = api_result.value.wasm_function_inst_t_value;

    wasm_val_t result[1] = { { .kind = WASM_I32 } };
    wasm_val_t arguments[1] = {
        { .kind = WASM_I32, .of.i32 = 6 },
    };

    api_result = wasm_runtime_call_wasm_a_checked(exec_env, func, 1, result, 1,
                                                  arguments);
    VERIFY_API_RESULT(wasm_runtime_call_wasm_a_checked, api_result,
                      release_runtime);
    printf("Native finished calling wasm function: fib(%d), returned: %d\n",
           arguments[0].of.i32, result[0].of.i32);
    bh_assert(result[0].of.i32 == 8);

    arguments[0].of.i32 = 2;
    api_result = wasm_runtime_call_wasm_a_checked(exec_env, func, 1, result, 1,
                                                  arguments);
    VERIFY_API_RESULT(wasm_runtime_call_wasm_a_checked, api_result,
                      release_runtime);
    printf("Native finished calling wasm function: fib(%d), returned: %d\n",
           arguments[0].of.i32, result[0].of.i32);
    bh_assert(result[0].of.i32 == 1);

    ret = EXIT_SUCCESS;

release_exec_env:
    wasm_runtime_destroy_exec_env_checked(exec_env);
release_instance:
    wasm_runtime_deinstantiate_checked(module_inst);
release_module:
    wasm_runtime_unload_checked(module);
release_file:
    wasm_runtime_free(buffer);
release_runtime:
    wasm_runtime_destroy_checked();
fail:
    return ret;
}
