/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_extension.h"
#include "platform_api_vmcore.h"

class PosixSleepTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

TEST_F(PosixSleepTest, BasicSleep)
{
    // Test sleep for 10 microseconds
    uint64 start = os_time_get_boot_us();
    os_usleep(10);
    uint64 end = os_time_get_boot_us();

    // Should have slept at least 10 microseconds
    EXPECT_GE(end - start, 10);
}

TEST_F(PosixSleepTest, ZeroSleep)
{
    // Test zero sleep - should return immediately
    uint64 start = os_time_get_boot_us();
    os_usleep(0);
    uint64 end = os_time_get_boot_us();

    // Should return almost immediately (allow some tolerance)
    EXPECT_LT(end - start, 1000); // Less than 1ms
}

TEST_F(PosixSleepTest, LongerSleep)
{
    // Test sleep for 1000 microseconds (1ms)
    uint64 start = os_time_get_boot_us();
    os_usleep(1000);
    uint64 end = os_time_get_boot_us();

    // Should have slept at least 1000 microseconds
    EXPECT_GE(end - start, 1000);
    // But not too long (allow 50% tolerance for system scheduling)
    EXPECT_LT(end - start, 1500);
}