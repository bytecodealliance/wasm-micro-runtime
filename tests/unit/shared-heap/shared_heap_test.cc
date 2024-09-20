/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "bh_read_file.h"
#include "wasm_runtime_common.h"

class shared_heap_test : public testing::Test
{
  protected:
    virtual void SetUp() {}
    static void SetUpTestCase() {}
    virtual void TearDown() {}
    WAMRRuntimeRAII<512 * 1024> runtime;
};

struct ret_env {
    wasm_exec_env_t exec_env;
    wasm_module_t wasm_module;
    wasm_module_inst_t wasm_module_inst;
    unsigned char *wasm_file_buf;
    char error_buf[128];
};

struct ret_env
load_wasm(char *wasm_file_tested, unsigned int app_heap_size)
{
    std::string wasm_mem_page = wasm_file_tested;
    const char *wasm_file = strdup(wasm_mem_page.c_str());
    wasm_module_inst_t wasm_module_inst = nullptr;
    wasm_module_t wasm_module = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    unsigned char *wasm_file_buf = nullptr;
    unsigned int wasm_file_size = 0;
    unsigned int stack_size = 16 * 1024, heap_size = app_heap_size;
    char error_buf[128] = { 0 };
    struct ret_env ret_module_env;

    memset(ret_module_env.error_buf, 0, 128);
    wasm_file_buf =
        (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    if (!wasm_file_buf) {
        goto fail;
    }

    wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf,
                                    sizeof(error_buf));
    if (!wasm_module) {
        memcpy(ret_module_env.error_buf, error_buf, 128);
        goto fail;
    }

    wasm_module_inst = wasm_runtime_instantiate(
        wasm_module, stack_size, heap_size, error_buf, sizeof(error_buf));
    if (!wasm_module_inst) {
        memcpy(ret_module_env.error_buf, error_buf, 128);
        goto fail;
    }

    exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size);

fail:
    ret_module_env.exec_env = exec_env;
    ret_module_env.wasm_module = wasm_module;
    ret_module_env.wasm_module_inst = wasm_module_inst;
    ret_module_env.wasm_file_buf = wasm_file_buf;

    return ret_module_env;
}

void
destroy_module_env(struct ret_env module_env)
{
    if (module_env.exec_env) {
        wasm_runtime_destroy_exec_env(module_env.exec_env);
    }

    if (module_env.wasm_module_inst) {
        wasm_runtime_deinstantiate(module_env.wasm_module_inst);
    }

    if (module_env.wasm_module) {
        wasm_runtime_unload(module_env.wasm_module);
    }

    if (module_env.wasm_file_buf) {
        wasm_runtime_free(module_env.wasm_file_buf);
    }
}

TEST_F(shared_heap_test, test_shared_heap)
{
    struct ret_env tmp_module_env;
    WASMFunctionInstanceCommon *func_test = nullptr;
    bool ret = false;
    uint32 argv[1] = { 65535 };
    const char *exception = nullptr;
    SharedHeapInitArgs args;
    WASMSharedHeap *shared_heap = nullptr;

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    tmp_module_env = load_wasm((char *)"test.wasm", 0);

    if (!shared_heap) {
        printf("Failed to create shared heap\n");
        goto test_failed;
    }
    if (!wasm_runtime_attach_shared_heap(tmp_module_env.wasm_module_inst, shared_heap)) {
        printf("Failed to attach shared heap\n");
        goto test_failed;
    }
    func_test = wasm_runtime_lookup_function(
        tmp_module_env.wasm_module_inst, "test");
    if (!func_test) {
        printf("\nFailed to wasm_runtime_lookup_function!\n");
        goto test_failed;
    }

    ret =
        wasm_runtime_call_wasm(tmp_module_env.exec_env, func_test, 1, argv);
    if (!ret) {
        printf("\nFailed to wasm_runtime_call_wasm!\n");
        const char *s = wasm_runtime_get_exception(tmp_module_env.wasm_module_inst);
        printf("exception: %s\n", s);
        goto test_failed;
    }

    wasm_runtime_detach_shared_heap(tmp_module_env.wasm_module_inst);

    EXPECT_EQ(10, argv[0]);

    destroy_module_env(tmp_module_env);
    return;
test_failed:
    destroy_module_env(tmp_module_env);
    EXPECT_EQ(1, 0);
}
