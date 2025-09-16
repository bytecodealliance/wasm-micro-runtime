/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_vmcore.h"
#include "platform_api_extension.h"
#include <cstring>

class PosixMallocTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

TEST_F(PosixMallocTest, BasicAllocation)
{
    // Test normal allocation
    void *ptr = os_malloc(1024);
    ASSERT_NE(nullptr, ptr);

    // Write to the allocated memory to ensure it's valid
    memset(ptr, 0xAA, 1024);

    os_free(ptr);
}

TEST_F(PosixMallocTest, ZeroAllocation)
{
    // Test zero allocation - behavior is implementation-defined
    void *ptr = os_malloc(0);
    // Just ensure it doesn't crash
    os_free(ptr); // Should handle NULL or valid pointer gracefully
}

TEST_F(PosixMallocTest, MultipleAllocations)
{
    // Test multiple allocations
    void *ptr1 = os_malloc(100);
    void *ptr2 = os_malloc(200);
    void *ptr3 = os_malloc(300);

    ASSERT_NE(nullptr, ptr1);
    ASSERT_NE(nullptr, ptr2);
    ASSERT_NE(nullptr, ptr3);

    // Ensure different allocations don't overlap
    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    EXPECT_NE(ptr1, ptr3);

    os_free(ptr1);
    os_free(ptr2);
    os_free(ptr3);
}

TEST_F(PosixMallocTest, Realloc)
{
    // Test realloc
    void *ptr = os_malloc(100);
    ASSERT_NE(nullptr, ptr);

    // Write pattern to original allocation
    memset(ptr, 0xBB, 100);

    // Realloc to larger size
    void *new_ptr = os_realloc(ptr, 200);
    ASSERT_NE(nullptr, new_ptr);

    // Verify first 100 bytes are preserved
    unsigned char *bytes = (unsigned char *)new_ptr;
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(0xBB, bytes[i]);
    }

    os_free(new_ptr);
}

TEST_F(PosixMallocTest, ReallocNull)
{
    // Realloc with NULL pointer should behave like malloc
    void *ptr = os_realloc(nullptr, 100);
    ASSERT_NE(nullptr, ptr);
    os_free(ptr);
}

TEST_F(PosixMallocTest, ReallocZeroSize)
{
    // Realloc to zero size
    void *ptr = os_malloc(100);
    ASSERT_NE(nullptr, ptr);

    void *new_ptr = os_realloc(ptr, 0);
    // Implementation may return NULL or a unique pointer
    // Just ensure it doesn't crash
    os_free(new_ptr);
}

TEST_F(PosixMallocTest, FreeNull)
{
    // Free NULL should not crash
    EXPECT_NO_THROW(os_free(nullptr));
}

TEST_F(PosixMallocTest, DumpsProcMemInfo)
{
    // Test os_dumps_proc_mem_info function
    char buffer[1024];

    // Test normal usage
    int result = os_dumps_proc_mem_info(buffer, sizeof(buffer));
    EXPECT_EQ(0, result); // Should succeed on Linux

    // Buffer should contain some memory info
    EXPECT_GT(strlen(buffer), 0);

    // Should contain RSS information
    EXPECT_NE(nullptr, strstr(buffer, "RSS"));
}

TEST_F(PosixMallocTest, DumpsProcMemInfoInvalidArgs)
{
    char buffer[100];

    // Test with NULL buffer
    int result = os_dumps_proc_mem_info(nullptr, 100);
    EXPECT_EQ(-1, result);

    // Test with zero size
    result = os_dumps_proc_mem_info(buffer, 0);
    EXPECT_EQ(-1, result);

    // Test with both NULL and zero
    result = os_dumps_proc_mem_info(nullptr, 0);
    EXPECT_EQ(-1, result);
}

TEST_F(PosixMallocTest, DumpsProcMemInfoSmallBuffer)
{
    char small_buffer[10];

    // Test with very small buffer - should handle gracefully
    int result = os_dumps_proc_mem_info(small_buffer, sizeof(small_buffer));
    // Should either succeed or fail gracefully
    EXPECT_TRUE(result >= -1 && result <= 0);
}