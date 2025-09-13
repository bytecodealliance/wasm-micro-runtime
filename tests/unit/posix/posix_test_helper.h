/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef POSIX_TEST_HELPER_H
#define POSIX_TEST_HELPER_H

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace posix_test {

// File system helpers
inline std::string
create_temp_file(const std::string &prefix)
{
    char temp_name[256];
    snprintf(temp_name, sizeof(temp_name), "/tmp/%s_XXXXXX", prefix.c_str());
    int fd = mkstemp(temp_name);
    if (fd >= 0) {
        close(fd);
        return std::string(temp_name);
    }
    return "";
}

inline std::string
create_temp_dir(const std::string &prefix)
{
    char temp_name[256];
    snprintf(temp_name, sizeof(temp_name), "/tmp/%s_XXXXXX", prefix.c_str());
    if (mkdtemp(temp_name)) {
        return std::string(temp_name);
    }
    return "";
}

inline void
cleanup_temp_file(const std::string &path)
{
    unlink(path.c_str());
}

inline void
cleanup_temp_dir(const std::string &path)
{
    rmdir(path.c_str());
}

// Time helpers
inline void
sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

// Memory helpers
inline bool
is_aligned(void *ptr, size_t alignment)
{
    return ((uintptr_t)ptr % alignment) == 0;
}

} // namespace posix_test

#endif // POSIX_TEST_HELPER_H