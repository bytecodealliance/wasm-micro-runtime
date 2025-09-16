/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include "platform_api_extension.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

class PosixFileBasicTest : public ::testing::Test
{
  protected:
    const char *test_file = "/tmp/wamr_test_file.txt";
    const char *test_dir = "/tmp/wamr_test_dir";

    void SetUp() override
    {
        // Clean up any leftover files
        unlink(test_file);
        rmdir(test_dir);
    }

    void TearDown() override
    {
        // Clean up test files
        unlink(test_file);
        rmdir(test_dir);
    }
};

TEST_F(PosixFileBasicTest, OpenCloseFile)
{
    os_file_handle fd;
    __wasi_errno_t err;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Test file creation and open
    err = os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                    WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd);
    EXPECT_EQ(__WASI_ESUCCESS, err);
    EXPECT_GE(fd, 0);

    // Test file close
    err = os_close(fd, true);
    EXPECT_EQ(__WASI_ESUCCESS, err);
}

TEST_F(PosixFileBasicTest, ReadWriteFile)
{
    os_file_handle fd;
    const char *test_data = "Hello WAMR Testing!";
    char read_buf[32] = { 0 };
    size_t nwritten, nread;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Create and write
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd));

    // Write data
    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)test_data;
    iov.buf_len = strlen(test_data);
    EXPECT_EQ(__WASI_ESUCCESS, os_writev(fd, &iov, 1, &nwritten));
    EXPECT_EQ(strlen(test_data), nwritten);
    os_close(fd, true);

    // Read back
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, 0, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_READ_ONLY, &fd));

    __wasi_iovec_t riov;
    riov.buf = (uint8_t *)read_buf;
    riov.buf_len = sizeof(read_buf);
    EXPECT_EQ(__WASI_ESUCCESS, os_readv(fd, &riov, 1, &nread));
    EXPECT_EQ(strlen(test_data), nread);
    EXPECT_STREQ(test_data, read_buf);
    os_close(fd, true);
}

TEST_F(PosixFileBasicTest, FileSeek)
{
    os_file_handle fd;
    __wasi_filesize_t newoffset;
    const char *test_data = "0123456789ABCDEF";
    char read_buf[32] = { 0 };
    size_t nwritten, nread;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Create file with data
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd));

    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)test_data;
    iov.buf_len = strlen(test_data);
    os_writev(fd, &iov, 1, &nwritten);
    os_close(fd, true);

    // Open for reading and test seek
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, 0, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_READ_ONLY, &fd));

    // Seek to position 5
    EXPECT_EQ(__WASI_ESUCCESS, os_lseek(fd, 5, __WASI_WHENCE_SET, &newoffset));
    EXPECT_EQ(5, newoffset);

    // Read from position 5
    __wasi_iovec_t riov;
    riov.buf = (uint8_t *)read_buf;
    riov.buf_len = 5;
    os_readv(fd, &riov, 1, &nread);
    EXPECT_EQ(5, nread);
    EXPECT_EQ('5', read_buf[0]);

    os_close(fd, true);
}

TEST_F(PosixFileBasicTest, FileStat)
{
    os_file_handle fd;
    __wasi_filestat_t stat_buf;
    const char *test_data = "Test file for stat";
    size_t nwritten;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Create file with data
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd));

    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)test_data;
    iov.buf_len = strlen(test_data);
    os_writev(fd, &iov, 1, &nwritten);

    // Test fstat
    EXPECT_EQ(__WASI_ESUCCESS, os_fstat(fd, &stat_buf));
    EXPECT_EQ(strlen(test_data), stat_buf.st_size);
    EXPECT_EQ(__WASI_FILETYPE_REGULAR_FILE, stat_buf.st_filetype);

    os_close(fd, true);
}

TEST_F(PosixFileBasicTest, DirectoryOperations)
{
    os_file_handle fd;
    __wasi_errno_t err;

    // Create directory
    err = os_mkdirat(AT_FDCWD, test_dir);
    EXPECT_EQ(__WASI_ESUCCESS, err);

    // Open directory
    err = os_openat(AT_FDCWD, test_dir, 0, 0, 0,
                    WASI_LIBC_ACCESS_MODE_READ_ONLY, &fd);
    EXPECT_EQ(__WASI_ESUCCESS, err);
    EXPECT_GE(fd, 0);

    // Get directory stats
    __wasi_filestat_t stat_buf;
    EXPECT_EQ(__WASI_ESUCCESS, os_fstat(fd, &stat_buf));
    EXPECT_EQ(__WASI_FILETYPE_DIRECTORY, stat_buf.st_filetype);

    os_close(fd, true);

    // Remove directory
    rmdir(test_dir);
}

TEST_F(PosixFileBasicTest, FileAccess)
{
    os_file_handle fd;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Create a file
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd));
    os_close(fd, true);

    // Test file access using standard POSIX access()
    EXPECT_EQ(0, access(test_file, F_OK)); // File exists
    EXPECT_EQ(0, access(test_file, R_OK)); // Read permission
    EXPECT_EQ(0, access(test_file, W_OK)); // Write permission

    // Test non-existent file
    EXPECT_NE(0, access("/tmp/non_existent_file_12345", F_OK));
}

TEST_F(PosixFileBasicTest, FileTruncate)
{
    os_file_handle fd;
    __wasi_filestat_t stat_buf;
    const char *test_data = "This is a longer string for truncation test";
    size_t nwritten;
    __wasi_oflags_t oflags = __WASI_O_CREAT;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;

    // Create file with data
    ASSERT_EQ(__WASI_ESUCCESS,
              os_openat(AT_FDCWD, test_file, oflags, fdflags, lookup_flags,
                        WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd));

    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)test_data;
    iov.buf_len = strlen(test_data);
    os_writev(fd, &iov, 1, &nwritten);

    // Truncate file to 10 bytes
    EXPECT_EQ(__WASI_ESUCCESS, os_ftruncate(fd, 10));

    // Verify new size
    EXPECT_EQ(__WASI_ESUCCESS, os_fstat(fd, &stat_buf));
    EXPECT_EQ(10, stat_buf.st_size);

    os_close(fd, true);
}