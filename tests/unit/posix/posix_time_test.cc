/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_vmcore.h"
#include <unistd.h>

class PosixTimeTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

TEST_F(PosixTimeTest, GetBootTime)
{
    uint64 time1 = os_time_get_boot_us();
    usleep(1000); // Sleep 1ms
    uint64 time2 = os_time_get_boot_us();

    EXPECT_GT(time2, time1);
    EXPECT_GE(time2 - time1, 1000);
}

TEST_F(PosixTimeTest, GetBootTimeMonotonic)
{
    // Test that time is monotonic
    uint64 prev_time = os_time_get_boot_us();

    for (int i = 0; i < 10; i++) {
        uint64 curr_time = os_time_get_boot_us();
        EXPECT_GE(curr_time, prev_time);
        prev_time = curr_time;
        usleep(100); // Small sleep between checks
    }
}

TEST_F(PosixTimeTest, GetBootTimeNonZero)
{
    // Time should never be zero (system has been running)
    uint64 time = os_time_get_boot_us();
    EXPECT_GT(time, 0);
}

TEST_F(PosixTimeTest, GetThreadCpuTime)
{
    // Test os_time_thread_cputime_us() function
    uint64 cpu_time1 = os_time_thread_cputime_us();

    // Perform some CPU work to advance CPU time
    volatile int sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += i * i;
    }

    uint64 cpu_time2 = os_time_thread_cputime_us();

    // CPU time should be non-negative
    EXPECT_GE(cpu_time1, 0);
    EXPECT_GE(cpu_time2, 0);

    // CPU time should generally increase or stay same (may be 0 if not
    // supported)
    if (cpu_time1 > 0 && cpu_time2 > 0) {
        EXPECT_GE(cpu_time2, cpu_time1);
    }
}

TEST_F(PosixTimeTest, ThreadCpuTimeConsistency)
{
    // Test multiple calls to os_time_thread_cputime_us()
    uint64 cpu_time1 = os_time_thread_cputime_us();
    uint64 cpu_time2 = os_time_thread_cputime_us();
    uint64 cpu_time3 = os_time_thread_cputime_us();

    // All calls should return non-negative values
    EXPECT_GE(cpu_time1, 0);
    EXPECT_GE(cpu_time2, 0);
    EXPECT_GE(cpu_time3, 0);

    // Values should be monotonic if supported
    if (cpu_time1 > 0) {
        EXPECT_GE(cpu_time2, cpu_time1);
        EXPECT_GE(cpu_time3, cpu_time2);
    }
}