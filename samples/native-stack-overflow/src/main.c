/*
 * Copyright (C) 2024 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include "bh_read_file.h"

uint32_t
host_consume_stack_and_call_indirect(wasm_exec_env_t exec_env, uint32_t funcidx,
                                     uint32_t x, uint32_t stack);
uint32_t
host_consume_stack(wasm_exec_env_t exec_env, uint32_t stack);

extern unsigned int nest;

static NativeSymbol native_symbols[] = {
    { "host_consume_stack_and_call_indirect",
      host_consume_stack_and_call_indirect, "(iii)i", NULL },
    { "host_consume_stack", host_consume_stack, "(i)i", NULL },
};

struct record {
    bool failed;
    bool leaked;
    char exception[128]; /* EXCEPTION_BUF_LEN */
};

void
print_record(unsigned int start, unsigned int end, const struct record *rec)
{
    printf("%5u - %5u | %6s | %6s | %s\n", start, end,
           rec->failed ? "failed" : "ok", rec->leaked ? "leaked" : "ok",
           rec->exception);
}

int
main(int argc, char **argv)
{
    char *buffer;
    char error_buf[128];

    if (argc != 2) {
        return 2;
    }
    char *module_path = argv[1];

    wasm_module_t module = NULL;
    uint32 buf_size;
    uint32 stack_size = 4096;
    /*
     * disable app heap.
     * - we use wasi
     * - https://github.com/bytecodealliance/wasm-micro-runtime/issues/2275
     */
    uint32 heap_size = 0;

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;
    if (!wasm_runtime_full_init(&init_args)) {
        printf("wasm_runtime_full_init failed.\n");
        return -1;
    }

    buffer = bh_read_file_to_buffer(module_path, &buf_size);
    if (!buffer) {
        printf("bh_read_file_to_buffer failed\n");
        goto fail;
    }

    module = wasm_runtime_load((uint8 *)buffer, buf_size, error_buf,
                               sizeof(error_buf));
    if (!module) {
        printf("wasm_runtime_load failed: %s\n", error_buf);
        goto fail;
    }

    /* header */
    printf(" stack size   | fail?  | leak?  | exception\n");
    printf("-------------------------------------------------------------------"
           "--------\n");

    unsigned int stack;
    unsigned int prevstack;
    unsigned int stack_range_start = 0;
    unsigned int stack_range_end = 4096 * 6;
    unsigned int step = 16;
    struct record rec0;
    struct record rec1;
    struct record *rec = &rec0;
    struct record *prevrec = &rec1;
    bool have_prevrec = false;
    for (stack = stack_range_start; stack < stack_range_end; stack += step) {
        wasm_module_inst_t module_inst = NULL;
        wasm_exec_env_t exec_env = NULL;
        bool failed = true;
        const char *exception = NULL;
        nest = 0;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        if (!module_inst) {
            printf("wasm_runtime_instantiate failed: %s\n", error_buf);
            goto fail2;
        }

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        if (!exec_env) {
            printf("wasm_runtime_create_exec_env failed\n");
            goto fail2;
        }

        const char *funcname = "test";
        wasm_function_inst_t func =
            wasm_runtime_lookup_function(module_inst, funcname);
        if (!func) {
            printf("wasm_runtime_lookup_function failed for %s\n", funcname);
            goto fail2;
        }

        /* note: the function type is (ii)i */
        uint32_t wasm_argv[] = {
            stack,
            30,
        };
        uint32_t wasm_argc = 2;
        if (!wasm_runtime_call_wasm(exec_env, func, wasm_argc, wasm_argv)) {
            exception = wasm_runtime_get_exception(module_inst);
            goto fail2;
        }
        failed = false;
    fail2:
        /*
         * note: non-zero "nest" here demonstrates resource leak on longjmp
         * from signal handler.
         * cf.
         * https://github.com/bytecodealliance/wasm-micro-runtime/issues/3320
         */
        memset(rec, 0, sizeof(*rec));
        rec->failed = failed;
        rec->leaked = nest != 0;
        strncpy(rec->exception, exception ? exception : "",
                sizeof(rec->exception));
        if (have_prevrec && memcmp(prevrec, rec, sizeof(*rec))) {
            print_record(prevstack, stack, prevrec);
            have_prevrec = false;
        }
        if (!have_prevrec) {
            prevstack = stack;
            struct record *tmp = prevrec;
            prevrec = rec;
            rec = tmp;
            have_prevrec = true;
        }
        if (exec_env) {
            wasm_runtime_destroy_exec_env(exec_env);
        }
        if (module_inst) {
            wasm_runtime_deinstantiate(module_inst);
        }
    }
    if (have_prevrec) {
        print_record(prevstack, stack, prevrec);
    }

fail:
    if (module) {
        wasm_runtime_unload(module);
    }
    if (buffer) {
        BH_FREE(buffer);
    }
    wasm_runtime_destroy();
}
