/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "bh_read_file.h"
#include "wasm_runtime_common.h"

#include <gtest/gtest-spi.h>

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

static void
destroy_module_env(struct ret_env module_env);

static bool
load_wasm(char *wasm_file_tested, unsigned int app_heap_size,
          ret_env &ret_module_env)
{
    std::string wasm_mem_page = wasm_file_tested;
    const char *wasm_file = strdup(wasm_mem_page.c_str());
    unsigned int wasm_file_size = 0;
    unsigned int stack_size = 16 * 1024, heap_size = app_heap_size;
    char error_buf[128] = { 0 };

    ret_module_env.wasm_file_buf =
        (unsigned char *)bh_read_file_to_buffer(wasm_file, &wasm_file_size);
    if (!ret_module_env.wasm_file_buf) {
        goto fail;
    }

    ret_module_env.wasm_module =
        wasm_runtime_load(ret_module_env.wasm_file_buf, wasm_file_size,
                          error_buf, sizeof(error_buf));
    if (!ret_module_env.wasm_module) {
        memcpy(ret_module_env.error_buf, error_buf, 128);
        goto fail;
    }

    ret_module_env.wasm_module_inst =
        wasm_runtime_instantiate(ret_module_env.wasm_module, stack_size,
                                 heap_size, error_buf, sizeof(error_buf));
    if (!ret_module_env.wasm_module_inst) {
        memcpy(ret_module_env.error_buf, error_buf, 128);
        goto fail;
    }

    ret_module_env.exec_env = wasm_runtime_create_exec_env(
        ret_module_env.wasm_module_inst, stack_size);
    if (!ret_module_env.exec_env) {
        goto fail;
    }

    return true;
fail:
    destroy_module_env(ret_module_env);
    return false;
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

static void test_shared_heap(WASMSharedHeap *shared_heap, const char *file, const char *func_name, uint32 argc, uint32 argv[])
{
    struct ret_env tmp_module_env;
    WASMFunctionInstanceCommon *func_test = nullptr;
    bool ret = false;

    if (!load_wasm((char *)file, 0, tmp_module_env)) {
        ADD_FAILURE() << "Failed to load wasm file\n";
        goto fail0;
    }

    if (!wasm_runtime_attach_shared_heap(tmp_module_env.wasm_module_inst,
                                         shared_heap)) {
        ADD_FAILURE() << "Failed to attach shared heap\n";
        goto fail1;
    }
    func_test = wasm_runtime_lookup_function(tmp_module_env.wasm_module_inst,
                                             func_name);
    if (!func_test) {
        ADD_FAILURE() << "Failed to wasm_runtime_lookup_function!\n";
        goto fail2;
    }

    ret =
        wasm_runtime_call_wasm(tmp_module_env.exec_env, func_test, argc, argv);
    if (!ret) {
        const char *s =
            wasm_runtime_get_exception(tmp_module_env.wasm_module_inst);
        ADD_FAILURE() << "Failed to wasm_runtime_call_wasm with "
                      << "exception: " << s;
    }

fail2:
    wasm_runtime_detach_shared_heap(tmp_module_env.wasm_module_inst);
fail1:
    destroy_module_env(tmp_module_env);
fail0:
    return;
}

TEST_F(shared_heap_test, test_shared_heap_basic)
{
    SharedHeapInitArgs args;
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 };

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);

    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test.wasm", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);

    test_shared_heap(shared_heap, "test.aot", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);

}

TEST_F(shared_heap_test, test_shared_heap_malloc_fail)
{
    SharedHeapInitArgs args;
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 };

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);

    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test.wasm", "test_malloc_fail", 0, argv);
    EXPECT_EQ(1, argv[0]);

    test_shared_heap(shared_heap, "test.aot", "test_malloc_fail", 0, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_preallocated_shared_heap_malloc_fail)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE];

    /* create a preallocated shared heap */
    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    /* test wasm can't malloc with preallocated shared heap */
    argv[0] = 1024;
    test_shared_heap(shared_heap, "test.wasm", "my_shared_heap_malloc", 1,
                     argv);
    EXPECT_EQ(0, argv[0]);

    argv[0] = 1024;
    test_shared_heap(shared_heap, "test.aot", "my_shared_heap_malloc", 1, argv);
    EXPECT_EQ(0, argv[0]);
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[2] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = { 0 },
          preallocated_buf2[BUF_SIZE] = { 0 };
    uint32 start1, end1, start2, end2;

    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf2;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }

    /* app addr for shared heap */
    start1 = 0xFFFFFFFF - 2 * BUF_SIZE + 1;
    end1 = 0xFFFFFFFF - BUF_SIZE;
    start2 = 0xFFFFFFFF - BUF_SIZE + 1;
    end2 = 0xFFFFFFFF;

    /* shared heap 1 */
    argv[0] = end1;
    argv[1] = 101;
    test_shared_heap(shared_heap_chain, "test.wasm", "read_modify_write_8", 2,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[BUF_SIZE - 1], 101);

    /* shared heap 2 */
    argv[0] = start2;
    argv[1] = 129;
    test_shared_heap(shared_heap_chain, "test.wasm", "read_modify_write_8", 2,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf2[0], 129);

    /* TODO: test aot when chain is supported in AOT */
    /*
    argv[0] = start1;
    argv[1] = 98;
    test_shared_heap(shared_heap_chain, "test.aot", "read_modify_write_8", 2,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 98);

    argv[0] = end2;
    argv[1] = 81;
    test_shared_heap(shared_heap_chain, "test.aot", "read_modify_write_8", 2,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf2[BUF_SIZE - 1], 81);
    */
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw_oob)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[2] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE], preallocated_buf2[BUF_SIZE];
    uint32 start1, end1, start2, end2;

    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf2;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }

    /* app addr for shared heap */
    start1 = 0xFFFFFFFF - 2 * BUF_SIZE + 1;
    end1 = 0xFFFFFFFF - BUF_SIZE;
    start2 = 0xFFFFFFFF - BUF_SIZE + 1;
    end2 = 0xFFFFFFFF;

    /* try to rmw an u16, first u8 is in the first shared heap and second u8 is
     * in the second shared heap, will be seen as oob */
    argv[0] = end1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain, "test.wasm",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");

    /* TODO: test aot when chain is supported in AOT */
    /*argv[0] = end1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain, "test.wasm",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");*/
}

#ifndef native_function
#define native_function(func_name, signature) \
    { #func_name, (void *)glue_##func_name, signature, NULL }
#endif
#ifndef nitems
#define nitems(_a) (sizeof(_a) / sizeof(0 [(_a)]))
#endif /* nitems */
uintptr_t glue_test_addr_conv(wasm_exec_env_t env, uintptr_t addr)
{
    wasm_module_inst_t module_inst = get_module_inst(env);
    void *native_addr = (void *)addr;
    uintptr_t app_addr = addr_native_to_app(native_addr);

    native_addr = addr_app_to_native(app_addr);
    if (native_addr != (void *)addr) {
        ADD_FAILURE() << "address conversion incorrect";
        return 0;
    }
    return app_addr;
}

static NativeSymbol g_test_native_symbols[] =
{
  native_function(test_addr_conv,"(*)i"),
};

TEST_F(shared_heap_test, test_addr_conv)
{
    SharedHeapInitArgs args;
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 };
    bool ret = false;

    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test_addr_conv.wasm", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);

    test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_addr_conv_pre_allocated_oob)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 };
    bool ret = false;

    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test_addr_conv.wasm", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);

    test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_addr_conv_pre_allocated_oob)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = { 0 }, BUF_SIZE = os_getpagesize(),
           app_addr = 0xFFFFFFFF - BUF_SIZE;
    uint8 preallocated_buf[BUF_SIZE];
    bool ret = false;

    /* create a preallocated shared heap */
    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    argv[0] = app_addr;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap, "test_addr_conv.wasm",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");

    argv[0] = app_addr;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap, "test_addr_conv.aot",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");
}

TEST_F(shared_heap_test, test_shared_heap_chain)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE];
    bool ret = false;

    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    /* create a preallocated shared heap */
    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }

    test_shared_heap(shared_heap_chain, "test_addr_conv.wasm", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);

    /* TODO: test aot when chain is supported in AOT */
    /*test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 1, argv);
    EXPECT_EQ(1, argv[0]);*/
}

TEST_F(shared_heap_test, test_shared_heap_chain_create_fail)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    args.size = 4096;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);

    EXPECT_EQ(shared_heap_chain, nullptr);
}

TEST_F(shared_heap_test, test_shared_heap_chain_addr_conv)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE];
    bool ret = false;

    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.size = 4096;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    /* create a preallocated shared heap */
    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }

    argv[0] = 0xFFFFFFFF;
    test_shared_heap(shared_heap_chain, "test_addr_conv.wasm",
                     "test_preallocated", 1, argv);
    EXPECT_EQ(1, argv[0]);

    argv[0] = 0xFFFFF000;
    test_shared_heap(shared_heap_chain, "test_addr_conv.wasm",
                     "test_preallocated", 1, argv);
    EXPECT_EQ(1, argv[0]);

    /* TODO: test aot when chain is supported in AOT */
    /*argv[0] = 0xFFFFFFFF;
    test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 1, argv);
    EXPECT_EQ(1, argv[0]);

    argv[0] = 0xFFFFF000;
    test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 1, argv);
    EXPECT_EQ(1, argv[0]); */
}

TEST_F(shared_heap_test, test_shared_heap_chain_addr_conv_oob)
{
    SharedHeapInitArgs args = { 0 };
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = { 0 }, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE];
    bool ret = false;

    ret = wasm_native_register_natives("env", g_test_native_symbols,
                                       nitems(g_test_native_symbols));
    if (!ret) {
        FAIL() << "Failed to register natives";
    }

    args.size = 4096;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    /* create a preallocated shared heap */
    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }

    /* test wasm */
    argv[0] = 0xFFFFFFFF - BUF_SIZE - 4096;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_addr_conv.wasm",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");

    /* TODO: test aot when chain is supported in AOT */
    /*argv[0] = 0xFFFFFFFF - BUF_SIZE - 4096;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_addr_conv.aot",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");*/
}
