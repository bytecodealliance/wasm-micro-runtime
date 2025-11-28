/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "bh_read_file.h"
#include "wasm_runtime.h"

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
load_wasm(const char *wasm_file_tested, unsigned int app_heap_size,
          ret_env &ret_module_env)
{
    char *wasm_file = strdup(wasm_file_tested);
    unsigned int wasm_file_size = 0;
    unsigned int stack_size = 16 * 1024, heap_size = app_heap_size;
    char error_buf[128] = {};

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

    free(wasm_file);
    return true;
fail:
    free(wasm_file);
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

static void
test_shared_heap(WASMSharedHeap *shared_heap, const char *file,
                 const char *func_name, uint32 argc, uint32 argv[])
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
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {};

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);

    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test.wasm", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);

    test_shared_heap(shared_heap, "test.aot", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);

    test_shared_heap(shared_heap, "test_chain.aot", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);
}

TEST_F(shared_heap_test, test_shared_heap_malloc_fail)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {};

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);

    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    test_shared_heap(shared_heap, "test.wasm", "test_malloc_fail", 0, argv);
    EXPECT_EQ(1, argv[0]);

    test_shared_heap(shared_heap, "test.aot", "test_malloc_fail", 0, argv);
    EXPECT_EQ(1, argv[0]);

    test_shared_heap(shared_heap, "test_chain.aot", "test_malloc_fail", 0,
                     argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_preallocated_shared_heap_malloc_fail)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {};

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

    argv[0] = 1024;
    test_shared_heap(shared_heap, "test_chain.aot", "my_shared_heap_malloc", 1,
                     argv);
    EXPECT_EQ(0, argv[0]);
}

TEST_F(shared_heap_test, test_preallocated_shared_runtime_api)
{
    struct ret_env tmp_module_env;
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {};
    void *native_ptr;
    uint64 offset, size;
    bool ret;

    args.size = 0x4000;
    shared_heap = wasm_runtime_create_shared_heap(&args);

    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    if (!load_wasm("test.wasm", 0, tmp_module_env)) {
        FAIL() << "Failed to load wasm file\n";
    }

    if (!wasm_runtime_attach_shared_heap(tmp_module_env.wasm_module_inst,
                                         shared_heap)) {
        ADD_FAILURE() << "Failed to attach shared heap\n";
        goto fail1;
    }

    offset = wasm_runtime_shared_heap_malloc(tmp_module_env.wasm_module_inst,
                                             32, &native_ptr);
    if (!offset) {
        ADD_FAILURE() << "Failed to attach shared heap\n";
        goto fail2;
    }

    size = (uint64_t)UINT32_MAX + 0x2000;
    printf("offset %lx size: %lx\n", offset, size);
    ASSERT_EQ(false, wasm_runtime_validate_app_addr(
                         tmp_module_env.wasm_module_inst, offset, size));

    ASSERT_EQ(NULL, wasm_runtime_addr_app_to_native(
                        tmp_module_env.wasm_module_inst, offset + size));

    size = (uint64_t)10;
    ASSERT_EQ(true, wasm_runtime_validate_app_addr(
                        tmp_module_env.wasm_module_inst, offset, size));

    ASSERT_EQ((char *)native_ptr + size,
              wasm_runtime_addr_app_to_native(tmp_module_env.wasm_module_inst,
                                              offset + size));

fail2:
    wasm_runtime_detach_shared_heap(tmp_module_env.wasm_module_inst);
fail1:
    destroy_module_env(tmp_module_env);
}

static void
create_test_shared_heap(uint8 *preallocated_buf, size_t size,
                        WASMSharedHeap **shared_heap_res)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    args.pre_allocated_addr = preallocated_buf;
    args.size = size;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    *shared_heap_res = shared_heap;
    if (!*shared_heap_res) {
        FAIL() << "Create shared heap chain failed.\n";
    }
}

static WASMSharedHeap *
create_preallocated_shared_heap(size_t size, uint8 *buffer)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = NULL;

    memset(buffer, 0, size);
    args.pre_allocated_addr = buffer;
    args.size = size;

    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        ADD_FAILURE() << "Create preallocated shared heap failed.\n";
        return NULL;
    }

    return shared_heap;
}

static WASMSharedHeap *
chain_shared_heaps(WASMSharedHeap *head, WASMSharedHeap *next)
{
    WASMSharedHeap *chain = wasm_runtime_chain_shared_heaps(head, next);
    if (!chain) {
        ADD_FAILURE() << "Create shared heap chain failed.\n";
        return NULL;
    }

    return chain;
}

static void
create_test_shared_heap_chain(uint8 *preallocated_buf, size_t size,
                              uint8 *preallocated_buf2, size_t size2,
                              WASMSharedHeap **shared_heap_chain)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr;
    args.pre_allocated_addr = preallocated_buf;
    args.size = size;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf2;
    args.size = size2;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    *shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    if (!*shared_heap_chain) {
        FAIL() << "Create shared heap chain failed.\n";
    }
}

TEST_F(shared_heap_test, test_destroy_shared_heap_head_only)
{
    WASMSharedHeap *shared_heap_chain = nullptr;
    WASMSharedHeap *second = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {};

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, NULL, BUF_SIZE,
                                  &shared_heap_chain);
    second = shared_heap_chain->chain_next;

    ASSERT_NE(nullptr, second);

    WASMSharedHeap *new_head = nullptr;
    ASSERT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain, false, &new_head));

    ASSERT_EQ(second, new_head);
    ASSERT_EQ(nullptr, new_head->chain_next);

    test_shared_heap(new_head, "test.wasm", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);
}

TEST_F(shared_heap_test, test_destroy_shared_heap_entire_chain)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap_chain = nullptr;

    args.size = 1024;
    shared_heap_chain = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap_chain) {
        FAIL() << "Failed to create shared heap";
    }

    WASMSharedHeap *new_head = nullptr;
    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain, true, &new_head));
    EXPECT_EQ(nullptr, new_head);
}

TEST_F(shared_heap_test, test_destroy_shared_heap_not_chain_head)
{
    WASMSharedHeap *shared_heap_chain = nullptr;
    WASMSharedHeap *body = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);
    body = shared_heap_chain->chain_next;

    ASSERT_NE(nullptr, body);

    WASMSharedHeap *new_head = nullptr;
    EXPECT_FALSE(wasm_runtime_destroy_shared_heap(body, true, &new_head));
    EXPECT_EQ(body, shared_heap_chain->chain_next);

    test_shared_heap(shared_heap_chain, "test.wasm", "test", 0, argv);
    EXPECT_EQ(10, argv[0]);

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain, true, &new_head));
    EXPECT_EQ(nullptr, new_head);
}

TEST_F(shared_heap_test, test_destroy_shared_heap_when_attached)
{
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {};
    struct ret_env module_env = {};

    create_test_shared_heap(preallocated_buf, BUF_SIZE, &shared_heap);

    ASSERT_TRUE(load_wasm((char *)"test.wasm", 0, module_env));
    ASSERT_TRUE(wasm_runtime_attach_shared_heap(module_env.wasm_module_inst,
                                                shared_heap));

    WASMSharedHeap *new_head = nullptr;
    EXPECT_FALSE(
        wasm_runtime_destroy_shared_heap(shared_heap, true, &new_head));

    wasm_runtime_detach_shared_heap(module_env.wasm_module_inst);
    destroy_module_env(module_env);

    EXPECT_TRUE(wasm_runtime_destroy_shared_heap(shared_heap, true, &new_head));
    EXPECT_EQ(nullptr, new_head);
}

TEST_F(shared_heap_test, test_destroy_shared_heap_three_nodes_chain)
{
    const uint32 BUF_SIZE = os_getpagesize();
    uint8 head_buf[BUF_SIZE], body_buf[BUF_SIZE], tail_buf[BUF_SIZE];
    WASMSharedHeap *head = nullptr;
    WASMSharedHeap *body = nullptr;
    WASMSharedHeap *tail = nullptr;
    WASMSharedHeap *new_head = nullptr;
    WASMSharedHeap *chain = nullptr;

    head = create_preallocated_shared_heap(BUF_SIZE, head_buf);
    body = create_preallocated_shared_heap(BUF_SIZE, body_buf);
    tail = create_preallocated_shared_heap(BUF_SIZE, tail_buf);

    ASSERT_NE(nullptr, head);
    ASSERT_NE(nullptr, body);
    ASSERT_NE(nullptr, tail);

    chain = chain_shared_heaps(body, tail);
    ASSERT_NE(nullptr, chain);

    chain = chain_shared_heaps(head, chain);
    ASSERT_NE(nullptr, chain);

    EXPECT_TRUE(wasm_runtime_destroy_shared_heap(chain, true, &new_head));
    EXPECT_EQ(nullptr, new_head);
}

TEST_F(shared_heap_test, test_destroy_shared_heap_cross_chains)
{
    const uint32 BUF_SIZE = os_getpagesize();
    uint8 chain1_bufs[3][BUF_SIZE];
    uint8 chain2_bufs[3][BUF_SIZE];
    WASMSharedHeap *chain1_nodes[3];
    WASMSharedHeap *chain2_nodes[3];
    WASMSharedHeap *new_head = nullptr;

    chain1_nodes[0] = create_preallocated_shared_heap(BUF_SIZE, chain1_bufs[0]);
    chain2_nodes[0] = create_preallocated_shared_heap(BUF_SIZE, chain2_bufs[0]);
    chain1_nodes[1] = create_preallocated_shared_heap(BUF_SIZE, chain1_bufs[1]);
    chain2_nodes[1] = create_preallocated_shared_heap(BUF_SIZE, chain2_bufs[1]);
    chain1_nodes[2] = create_preallocated_shared_heap(BUF_SIZE, chain1_bufs[2]);
    chain2_nodes[2] = create_preallocated_shared_heap(BUF_SIZE, chain2_bufs[2]);

    ASSERT_NE(nullptr, chain1_nodes[0]);
    ASSERT_NE(nullptr, chain2_nodes[0]);
    ASSERT_NE(nullptr, chain1_nodes[1]);
    ASSERT_NE(nullptr, chain2_nodes[1]);
    ASSERT_NE(nullptr, chain1_nodes[2]);
    ASSERT_NE(nullptr, chain2_nodes[2]);

    WASMSharedHeap *shared_heap_chain1 =
        chain_shared_heaps(chain1_nodes[1], chain1_nodes[2]);
    ASSERT_NE(nullptr, shared_heap_chain1);

    shared_heap_chain1 =
        chain_shared_heaps(chain1_nodes[0], shared_heap_chain1);
    ASSERT_NE(nullptr, shared_heap_chain1);

    WASMSharedHeap *shared_heap_chain2 =
        chain_shared_heaps(chain2_nodes[1], chain2_nodes[2]);
    ASSERT_NE(nullptr, shared_heap_chain2);

    shared_heap_chain2 =
        chain_shared_heaps(chain2_nodes[0], shared_heap_chain2);
    ASSERT_NE(nullptr, shared_heap_chain2);

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain1, false, &new_head));
    ASSERT_EQ(chain1_nodes[1], new_head);
    shared_heap_chain1 = new_head;

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain2, false, &new_head));
    ASSERT_EQ(chain2_nodes[1], new_head);
    shared_heap_chain2 = new_head;

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain1, false, &new_head));
    ASSERT_EQ(chain1_nodes[2], new_head);
    shared_heap_chain1 = new_head;

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain2, false, &new_head));
    ASSERT_EQ(chain2_nodes[2], new_head);
    shared_heap_chain2 = new_head;

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain1, false, &new_head));
    EXPECT_EQ(nullptr, new_head);

    EXPECT_TRUE(
        wasm_runtime_destroy_shared_heap(shared_heap_chain2, false, &new_head));
    EXPECT_EQ(nullptr, new_head);
}

TEST_F(shared_heap_test, test_shared_heap_rmw)
{
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[2] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {};
    uint32 start1, end1;

    create_test_shared_heap(preallocated_buf, BUF_SIZE, &shared_heap);

    /* app addr for shared heap */
    start1 = UINT32_MAX - BUF_SIZE + 1;
    end1 = UINT32_MAX;

    argv[0] = end1;
    argv[1] = 101;
    test_shared_heap(shared_heap, "test.wasm", "read_modify_write_8", 2, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[BUF_SIZE - 1], 101);

    argv[0] = start1;
    argv[1] = 37;
    test_shared_heap(shared_heap, "test.wasm", "read_modify_write_8", 2, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 37);

    argv[0] = end1;
    argv[1] = 81;
    test_shared_heap(shared_heap, "test.aot", "read_modify_write_8", 2, argv);
    EXPECT_EQ(101, argv[0]);
    EXPECT_EQ(preallocated_buf[BUF_SIZE - 1], 81);

    argv[0] = start1;
    argv[1] = 98;
    test_shared_heap(shared_heap, "test.aot", "read_modify_write_8", 2, argv);
    EXPECT_EQ(37, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 98);
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[2] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};
    uint32 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT32_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT32_MAX - BUF_SIZE;
    start2 = UINT32_MAX - BUF_SIZE + 1;
    end2 = UINT32_MAX;

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

    argv[0] = start1;
    argv[1] = 98;
    test_shared_heap(shared_heap_chain, "test_chain.aot", "read_modify_write_8",
                     2, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 98);

    argv[0] = end2;
    argv[1] = 81;
    test_shared_heap(shared_heap_chain, "test_chain.aot", "read_modify_write_8",
                     2, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf2[BUF_SIZE - 1], 81);
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw_bulk_memory)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[3] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};
    uint32 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT32_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT32_MAX - BUF_SIZE;
    start2 = UINT32_MAX - BUF_SIZE + 1;
    end2 = UINT32_MAX;

    argv[0] = end1;
    argv[1] = 101;
    argv[2] = 1;
    test_shared_heap(shared_heap_chain, "test_bulk_memory.wasm",
                     "memory_fill_test", 3, argv);
    /* no modification since no return value */
    EXPECT_EQ(end1, argv[0]);
    EXPECT_EQ(preallocated_buf[BUF_SIZE - 1], 101);

    argv[0] = start1;
    argv[1] = 14;
    argv[2] = 1;
    test_shared_heap(shared_heap_chain, "test_bulk_memory_chain.aot",
                     "memory_fill_test", 3, argv);
    /* no modification since no return value */
    EXPECT_EQ(start1, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 14);

    /* nothing happen when memory fill 0 byte */
    argv[0] = start2;
    argv[1] = 68;
    argv[2] = 0;
    test_shared_heap(shared_heap_chain, "test_bulk_memory_chain.aot",
                     "memory_fill_test", 3, argv);
    /* no modification since no return value */
    EXPECT_EQ(start2, argv[0]);
    EXPECT_EQ(preallocated_buf2[0], 0);

    argv[0] = end2;
    argv[1] = 98;
    argv[2] = 1;
    test_shared_heap(shared_heap_chain, "test_bulk_memory_chain.aot",
                     "memory_fill_test", 3, argv);
    /* no modification since no return value */
    EXPECT_EQ(end2, argv[0]);
    EXPECT_EQ(preallocated_buf2[BUF_SIZE - 1], 98);
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw_bulk_memory_oob)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[3] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};
    uint32 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT32_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT32_MAX - BUF_SIZE;
    start2 = UINT32_MAX - BUF_SIZE + 1;
    end2 = UINT32_MAX;

    /* shared heap 1 */
    argv[0] = end1;
    argv[1] = 101;
    argv[2] = 2;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory.wasm",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = end2;
    argv[1] = 98;
    argv[2] = 2;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory.wasm",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = start1;
    argv[1] = 98;
    argv[2] = BUF_SIZE + 1;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory.wasm",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = start2;
    argv[1] = 98;
    argv[2] = BUF_SIZE + 1;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory.wasm",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = end1;
    argv[1] = 101;
    argv[2] = 2;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory_chain.aot",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = end2;
    argv[1] = 98;
    argv[2] = 2;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory_chain.aot",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = start1;
    argv[1] = 98;
    argv[2] = BUF_SIZE + 1;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory_chain.aot",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");

    argv[0] = start2;
    argv[1] = 98;
    argv[2] = BUF_SIZE + 1;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_bulk_memory_chain.aot",
                                             "memory_fill_test", 3, argv),
                            "Exception: out of bounds memory access");
}

TEST_F(shared_heap_test, test_shared_heap_rmw_oob)
{
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[2] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE], preallocated_buf2[BUF_SIZE];
    uint32 start1, end1, start2, end2;

    create_test_shared_heap(preallocated_buf, BUF_SIZE, &shared_heap);

    /* app addr for shared heap */
    start1 = UINT32_MAX - BUF_SIZE + 1;
    end1 = UINT32_MAX;

    /* try to rmw an u16, first u8 is in the first shared heap and second u8 is
     * in the second shared heap, will be seen as oob */
    argv[0] = end1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap, "test.wasm",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");

    argv[0] = start1 - 1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap, "test.aot",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");

    argv[0] = end1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap, "test.aot",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");
}

TEST_F(shared_heap_test, test_shared_heap_chain_rmw_oob)
{
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[2] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};
    uint32 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT32_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT32_MAX - BUF_SIZE;
    start2 = UINT32_MAX - BUF_SIZE + 1;
    end2 = UINT32_MAX;

    /* try to rmw an u16, first u8 is in the first shared heap and second u8 is
     * in the second shared heap, will be seen as oob */
    argv[0] = end2;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain, "test.wasm",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");

    argv[0] = end1;
    argv[1] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_chain.aot",
                                             "read_modify_write_16", 2, argv),
                            "Exception: out of bounds memory access");
}

#if WASM_ENABLE_MEMORY64 != 0
TEST_F(shared_heap_test, test_shared_heap_chain_memory64_rmw)
{
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[3] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE] = {}, preallocated_buf2[BUF_SIZE] = {};
    uint64 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT64_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT64_MAX - BUF_SIZE;
    start2 = UINT64_MAX - BUF_SIZE + 1;
    end2 = UINT64_MAX;

    /* shared heap 1 */
    PUT_I64_TO_ADDR(argv, end1);
    argv[2] = 101;
    test_shared_heap(shared_heap_chain, "test64.wasm", "read_modify_write_8", 3,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[BUF_SIZE - 1], 101);

    /* shared heap 2 */
    PUT_I64_TO_ADDR(argv, start2);
    argv[2] = 129;
    test_shared_heap(shared_heap_chain, "test64.wasm", "read_modify_write_8", 3,
                     argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf2[0], 129);

    PUT_I64_TO_ADDR(argv, start1);
    argv[2] = 98;
    test_shared_heap(shared_heap_chain, "test64_chain.aot",
                     "read_modify_write_8", 3, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf[0], 98);

    PUT_I64_TO_ADDR(argv, end2);
    argv[2] = 81;
    test_shared_heap(shared_heap_chain, "test64_chain.aot",
                     "read_modify_write_8", 3, argv);
    EXPECT_EQ(0, argv[0]);
    EXPECT_EQ(preallocated_buf2[BUF_SIZE - 1], 81);
}

TEST_F(shared_heap_test, test_shared_heap_chain_memory64_rmw_oob)
{
    WASMSharedHeap *shared_heap_chain = nullptr;
    uint32 argv[3] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE], preallocated_buf2[BUF_SIZE];
    uint64 start1, end1, start2, end2;

    create_test_shared_heap_chain(preallocated_buf, BUF_SIZE, preallocated_buf2,
                                  BUF_SIZE, &shared_heap_chain);

    /* app addr for shared heap */
    start1 = UINT64_MAX - 2 * BUF_SIZE + 1;
    end1 = UINT64_MAX - BUF_SIZE;
    start2 = UINT64_MAX - BUF_SIZE + 1;
    end2 = UINT64_MAX;

    /* try to rmw an u16, first u8 is in the first shared heap and second u8 is
     * in the second shared heap, will be seen as oob */
    PUT_I64_TO_ADDR(argv, end1);
    argv[2] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain, "test64.wasm",
                                             "read_modify_write_16", 3, argv),
                            "Exception: out of bounds memory access");

    PUT_I64_TO_ADDR(argv, end1);
    argv[2] = 12025;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test64_chain.aot",
                                             "read_modify_write_16", 3, argv),
                            "Exception: out of bounds memory access");
}
#endif

#ifndef native_function
/* clang-format off */
#define native_function(func_name, signature) \
    { #func_name, (void *)glue_## func_name, signature, NULL }
/* clang-format on */
#endif
#ifndef nitems
#define nitems(_a) (sizeof(_a) / sizeof(0 [(_a)]))
#endif /* nitems */
uintptr_t
glue_test_addr_conv(wasm_exec_env_t env, uintptr_t addr)
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

static NativeSymbol g_test_native_symbols[] = {
    native_function(test_addr_conv, "(*)i"),
};

TEST_F(shared_heap_test, test_addr_conv)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {};
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

    test_shared_heap(shared_heap, "test_addr_conv_chain.aot", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_addr_conv_pre_allocated_oob)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize(),
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

    argv[0] = app_addr;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap,
                                             "test_addr_conv_chain.aot",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");
}

TEST_F(shared_heap_test, test_shared_heap_chain)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
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

    test_shared_heap(shared_heap, "test_addr_conv.aot", "test", 0, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_shared_heap_chain_create_fail)
{
    SharedHeapInitArgs args = {};
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

TEST_F(shared_heap_test, test_shared_heap_chain_create_fail2)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE];
    struct ret_env tmp_module_env;

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf;
    args.size = BUF_SIZE;
    shared_heap2 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap2) {
        FAIL() << "Create preallocated shared heap failed.\n";
    }

    if (!load_wasm((char *)"test.wasm", 0, tmp_module_env)) {
        FAIL() << "Failed to load wasm file\n";
    }

    if (!wasm_runtime_attach_shared_heap(tmp_module_env.wasm_module_inst,
                                         shared_heap)) {
        FAIL() << "Failed to attach shared heap\n";
    }

    /* can't create shared heap chain when shared heap is attached to a wasm
     * app */
    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap2);
    EXPECT_EQ(shared_heap_chain, nullptr);

    wasm_runtime_detach_shared_heap(tmp_module_env.wasm_module_inst);
    destroy_module_env(tmp_module_env);
}

TEST_F(shared_heap_test, test_shared_heap_chain_create_fail3)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap3 = nullptr, *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE], preallocated_buf2[BUF_SIZE];

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

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

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf2;
    args.size = BUF_SIZE;
    shared_heap3 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap3) {
        FAIL() << "Failed to create shared heap";
    }

    /* The head and body can't be already in other shared heap chain as body */
    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap3, shared_heap2);
    EXPECT_EQ(shared_heap_chain, nullptr);
    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap2, shared_heap);
    EXPECT_EQ(shared_heap_chain, nullptr);
}

TEST_F(shared_heap_test, test_shared_heap_chain_unchain)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap3 = nullptr, *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
    uint8 preallocated_buf[BUF_SIZE], preallocated_buf2[BUF_SIZE];

    args.size = 1024;
    shared_heap = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap) {
        FAIL() << "Failed to create shared heap";
    }

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

    memset(&args, 0, sizeof(args));
    args.pre_allocated_addr = preallocated_buf2;
    args.size = BUF_SIZE;
    shared_heap3 = wasm_runtime_create_shared_heap(&args);
    if (!shared_heap3) {
        FAIL() << "Failed to create shared heap";
    }

    /* unchain shared heap so that the 'body' can be another chain 'body'
     * again(1->2 to 1->3->2) */
    EXPECT_EQ(shared_heap2,
              wasm_runtime_unchain_shared_heaps(shared_heap_chain, false));
    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap3, shared_heap2);
    EXPECT_EQ(shared_heap_chain, shared_heap3);
    shared_heap_chain =
        wasm_runtime_chain_shared_heaps(shared_heap, shared_heap3);
    EXPECT_EQ(shared_heap, shared_heap_chain);

    /* break down the entire shared heap chain */
    EXPECT_EQ(shared_heap2,
              wasm_runtime_unchain_shared_heaps(shared_heap_chain, true));
}

TEST_F(shared_heap_test, test_shared_heap_chain_addr_conv)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
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

    argv[0] = 0xFFFFFFFF;
    test_shared_heap(shared_heap, "test_addr_conv_chain.aot",
                     "test_preallocated", 1, argv);
    EXPECT_EQ(1, argv[0]);

    argv[0] = 0xFFFFF000;
    test_shared_heap(shared_heap, "test_addr_conv_chain.aot",
                     "test_preallocated", 1, argv);
    EXPECT_EQ(1, argv[0]);
}

TEST_F(shared_heap_test, test_shared_heap_chain_addr_conv_oob)
{
    SharedHeapInitArgs args = {};
    WASMSharedHeap *shared_heap = nullptr, *shared_heap2 = nullptr,
                   *shared_heap_chain = nullptr;
    uint32 argv[1] = {}, BUF_SIZE = os_getpagesize();
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

    /* test aot */
    argv[0] = 0xFFFFFFFF - BUF_SIZE - 4096;
    EXPECT_NONFATAL_FAILURE(test_shared_heap(shared_heap_chain,
                                             "test_addr_conv_chain.aot",
                                             "test_preallocated", 1, argv),
                            "Exception: out of bounds memory access");
}
