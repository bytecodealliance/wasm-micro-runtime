
/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "wasm_export.h"
#include "bh_read_file.h"
#include "bh_getopt.h"

void
print_usage(void)
{
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -f [path of wasm file] \n");
}

static void *
runner(void *vp)
{
    wasm_module_inst_t inst = vp;
    bool ok = wasm_runtime_init_thread_env();
    assert(ok);
    wasm_application_execute_main(inst, 0, NULL);
    return inst;
}

int
main(int argc, char *argv_main[])
{
    int exit_code = 1;
    static char global_heap_buf[512 * 1024];
    char *buffer;
    char error_buf[128];
    int opt;
    char *wasm_path = NULL;
    int ret;

    const unsigned int N = 4;
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst[N];
    pthread_t th[N];
    unsigned int i;
    uint32 buf_size, stack_size = 8092, heap_size = 8092;

    for (i = 0; i < N; i++) {
        module_inst[i] = NULL;
    }

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    while ((opt = getopt(argc, argv_main, "hf:")) != -1) {
        switch (opt) {
            case 'f':
                wasm_path = optarg;
                break;
            case 'h':
                print_usage();
                return 0;
            case '?':
                print_usage();
                return 0;
        }
    }
    if (optind == 1) {
        print_usage();
        return 0;
    }

    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    buffer = bh_read_file_to_buffer(wasm_path, &buf_size);

    if (!buffer) {
        printf("Open wasm app file [%s] failed.\n", wasm_path);
        goto fail;
    }

    module = wasm_runtime_load((uint8 *)buffer, buf_size, error_buf,
                               sizeof(error_buf));
    if (!module) {
        printf("Load wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    for (i = 0; i < N; i++) {
        module_inst[i] = wasm_runtime_instantiate(module, stack_size, heap_size,
                                                  error_buf, sizeof(error_buf));

        if (!module_inst[i]) {
            printf("Instantiate wasm module failed. error: %s\n", error_buf);
            goto fail;
        }

        /* Note: ensure that module inst has an exec env so that
         * it can receive the termination request.
         */
        wasm_runtime_get_exec_env_singleton(module_inst[i]);

        if ((i % 2) == 0) {
            printf("terminating thread %u before starting\n", i);
            wasm_runtime_terminate(module_inst[i]);
        }

        printf("starting thread %u\n", i);
        ret = pthread_create(&th[i], NULL, runner, module_inst[i]);
        if (ret != 0) {
            goto fail;
        }
    }

    printf("sleeping a bit to ensure that the threads actually started\n");
    sleep(1);

    for (i = 0; i < N; i++) {
        if ((i % 2) != 0) {
            printf("terminating thread %u\n", i);
            wasm_runtime_terminate(module_inst[i]);
        }
    }

    for (i = 0; i < N; i++) {
        printf("joining thread %u\n", i);
        void *status;
        ret = pthread_join(th[i], &status);
        if (ret != 0) {
            goto fail;
        }
    }

    for (i = 0; i < N; i++) {
        const char *exception = wasm_runtime_get_exception(module_inst[i]);
        if (exception != NULL) {
            if (!strstr(exception, "terminated by user")) {
                printf("thread %u got an exception: %s (unexpected)\n", i,
                       exception);
                goto fail;
            }
            printf("thread %u got an exception: %s (expected)\n", i, exception);
        }
        else {
            printf("thread %u got no exception (unexpected)\n", i);
            goto fail;
        }
    }

    exit_code = 0;
fail:
    for (i = 0; i < N; i++) {
        if (module_inst[i])
            wasm_runtime_deinstantiate(module_inst[i]);
    }
    if (module)
        wasm_runtime_unload(module);
    if (buffer)
        BH_FREE(buffer);
    wasm_runtime_destroy();
    return exit_code;
}
