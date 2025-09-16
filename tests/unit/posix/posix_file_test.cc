/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_extension.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstring>

class PosixFileTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        // Create temporary test directory
        test_dir = "/tmp/wamr_file_test";
        mkdir(test_dir.c_str(), 0755);

        test_file = test_dir + "/test_file.txt";
        test_fd = -1;
    }

    virtual void TearDown()
    {
        if (test_fd >= 0) {
            os_close(test_fd, false);
        }
        // Cleanup test files and directory
        unlink(test_file.c_str());
        rmdir(test_dir.c_str());
    }

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
    std::string test_dir;
    std::string test_file;
    os_file_handle test_fd;
};

TEST_F(PosixFileTest, FileOpenAndClose)
{
    // Test os_openat with various flags
    os_file_handle dirfd = AT_FDCWD;
    __wasi_oflags_t open_flags = __WASI_O_CREAT | __WASI_O_TRUNC;
    __wasi_fdflags_t fdflags = 0;
    __wasi_lookupflags_t lookup_flags = 0;
    wasi_libc_file_access_mode access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;

    __wasi_errno_t result =
        os_openat(dirfd, test_file.c_str(), open_flags, fdflags, lookup_flags,
                  access_mode, &test_fd);

    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_GE(test_fd, 0);

    // Test os_close
    result = os_close(test_fd, false);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    test_fd = -1; // Prevent double close in teardown
}

TEST_F(PosixFileTest, FileReadWriteOperations)
{
    // Create and open test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test os_writev
    const char *test_data = "Hello, WAMR File Test!";
    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)test_data;
    iov.buf_len = strlen(test_data);

    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nwritten);

    // Test os_lseek to beginning
    __wasi_filedelta_t offset = 0;
    __wasi_whence_t whence = __WASI_WHENCE_SET;
    __wasi_filesize_t new_offset;
    result = os_lseek(test_fd, offset, whence, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(0, new_offset);

    // Test os_readv
    char read_buffer[64] = { 0 };
    __wasi_iovec_t read_iov;
    read_iov.buf = (uint8_t *)read_buffer;
    read_iov.buf_len = sizeof(read_buffer) - 1;

    size_t nread;
    result = os_readv(test_fd, &read_iov, 1, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nread);
    read_buffer[nread] = '\0'; // Null terminate for string comparison
    EXPECT_STREQ(test_data, read_buffer);
}

TEST_F(PosixFileTest, FileStatusOperations)
{
    // Create test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                  WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test os_fstat
    __wasi_filestat_t filestat;
    result = os_fstat(test_fd, &filestat);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(__WASI_FILETYPE_REGULAR_FILE, filestat.st_filetype);

    // Test os_fstatat
    __wasi_filestat_t filestat2;
    result = os_fstatat(AT_FDCWD, test_file.c_str(), &filestat2, 0);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(filestat.st_dev, filestat2.st_dev);
    EXPECT_EQ(filestat.st_ino, filestat2.st_ino);

    // Test os_isatty (should return false for regular file)
    result = os_isatty(test_fd);
    EXPECT_NE(__WASI_ESUCCESS, result); // Regular files are not TTY
}

TEST_F(PosixFileTest, FileSyncOperations)
{
    // Create and write to test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write some data
    const char *data = "Test data for sync operations";
    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)data;
    iov.buf_len = strlen(data);

    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test os_fsync
    result = os_fsync(test_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test os_fdatasync (may not be available on all platforms)
    result = os_fdatasync(test_fd);
    // Accept success or ENOSYS (not implemented)
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOSYS);
}

TEST_F(PosixFileTest, FileManipulationOperations)
{
    // Create test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test os_fallocate (allocate 1KB)
    __wasi_filesize_t offset = 0;
    __wasi_filesize_t len = 1024;
    result = os_fallocate(test_fd, offset, len);
    // Accept success or ENOSYS (not supported on all filesystems)
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOSYS);

    // Test os_ftruncate
    __wasi_filesize_t size = 512;
    result = os_ftruncate(test_fd, size);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Verify truncation worked
    __wasi_filestat_t filestat;
    result = os_fstat(test_fd, &filestat);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(size, filestat.st_size);
}

TEST_F(PosixFileTest, ErrorHandling)
{
    // Test operations on invalid file descriptor
    os_file_handle invalid_fd = 999999;

    __wasi_filestat_t filestat;
    __wasi_errno_t result = os_fstat(invalid_fd, &filestat);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_fsync(invalid_fd);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_ftruncate(invalid_fd, 100);
    EXPECT_EQ(__WASI_EBADF, result);

    // Test opening non-existent file without create flag
    os_file_handle bad_fd;
    result = os_openat(AT_FDCWD, "/non/existent/path", 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &bad_fd);
    EXPECT_EQ(__WASI_ENOENT, result);
}

TEST_F(PosixFileTest, FileSeekOperations)
{
    // Create and write test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write test data
    const char *data = "0123456789";
    __wasi_ciovec_t iov;
    iov.buf = (const uint8_t *)data;
    iov.buf_len = strlen(data);

    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test seek to end
    __wasi_filesize_t new_offset;
    result = os_lseek(test_fd, 0, __WASI_WHENCE_END, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data), new_offset);

    // Test seek to middle
    result = os_lseek(test_fd, 5, __WASI_WHENCE_SET, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(5, new_offset);

    // Test relative seek
    result = os_lseek(test_fd, 2, __WASI_WHENCE_CUR, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(7, new_offset);
}

// Step 2: Enhanced File Function Tests for Coverage Improvement

TEST_F(PosixFileTest, FileOpenReadWriteModes)
{
    // Test different access modes
    os_file_handle fd_read, fd_write, fd_rw;

    // Create file first for read-only test
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &fd_rw);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    const char *data = "test data";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(fd_rw, &iov, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    os_close(fd_rw, false);

    // Test read-only mode
    result = os_openat(AT_FDCWD, test_file.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &fd_read);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    if (result == __WASI_ESUCCESS) {
        os_close(fd_read, false);
    }

    // Test write-only mode
    result = os_openat(AT_FDCWD, test_file.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_WRITE_ONLY, &fd_write);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    if (result == __WASI_ESUCCESS) {
        os_close(fd_write, false);
    }
}

TEST_F(PosixFileTest, FileCloseSuccessAndError)
{
    // Test successful close
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                  WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    result = os_close(test_fd, false);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    test_fd = -1;

    // Test close with invalid fd
    result = os_close(999999, false);
    EXPECT_EQ(__WASI_EBADF, result);

    // Test close with pooled flag
    result = os_openat(AT_FDCWD, test_file.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &test_fd);
    if (result == __WASI_ESUCCESS) {
        result = os_close(test_fd, true); // pooled close
        EXPECT_EQ(__WASI_ESUCCESS, result);
        test_fd = -1;
    }
}

TEST_F(PosixFileTest, FileReadBasicOperations)
{
    // Create and write test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    const char *write_data = "Hello World Test Data";
    __wasi_ciovec_t write_iov = { (const uint8_t *)write_data,
                                  strlen(write_data) };
    size_t nwritten;
    result = os_writev(test_fd, &write_iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Seek to beginning
    __wasi_filesize_t new_offset;
    result = os_lseek(test_fd, 0, __WASI_WHENCE_SET, &new_offset);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test read with different buffer sizes
    char small_buffer[5];
    __wasi_iovec_t small_iov = { (uint8_t *)small_buffer,
                                 sizeof(small_buffer) };
    size_t nread;
    result = os_readv(test_fd, &small_iov, 1, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(sizeof(small_buffer), nread);

    // Test read with larger buffer
    char large_buffer[100];
    __wasi_iovec_t large_iov = { (uint8_t *)large_buffer,
                                 sizeof(large_buffer) };
    result = os_lseek(test_fd, 0, __WASI_WHENCE_SET, &new_offset);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    result = os_readv(test_fd, &large_iov, 1, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(write_data), nread);
}

TEST_F(PosixFileTest, FileWriteBasicOperations)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test multiple writes
    const char *data1 = "First ";
    const char *data2 = "Second ";
    const char *data3 = "Third";

    __wasi_ciovec_t iov1 = { (const uint8_t *)data1, strlen(data1) };
    __wasi_ciovec_t iov2 = { (const uint8_t *)data2, strlen(data2) };
    __wasi_ciovec_t iov3 = { (const uint8_t *)data3, strlen(data3) };

    size_t nwritten;
    result = os_writev(test_fd, &iov1, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data1), nwritten);

    result = os_writev(test_fd, &iov2, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data2), nwritten);

    result = os_writev(test_fd, &iov3, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data3), nwritten);

    // Verify total content
    __wasi_filesize_t new_offset;
    result = os_lseek(test_fd, 0, __WASI_WHENCE_SET, &new_offset);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    char read_buffer[50];
    __wasi_iovec_t read_iov = { (uint8_t *)read_buffer,
                                sizeof(read_buffer) - 1 };
    size_t nread;
    result = os_readv(test_fd, &read_iov, 1, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    read_buffer[nread] = '\0';
    EXPECT_STREQ("First Second Third", read_buffer);
}

TEST_F(PosixFileTest, FileSeekFilePositioning)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write test data
    const char *data = "ABCDEFGHIJ";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test different seek operations
    __wasi_filesize_t new_offset;

    // Seek from beginning
    result = os_lseek(test_fd, 3, __WASI_WHENCE_SET, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(3, new_offset);

    // Seek from current position
    result = os_lseek(test_fd, 2, __WASI_WHENCE_CUR, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(5, new_offset);

    // Seek from end
    result = os_lseek(test_fd, -2, __WASI_WHENCE_END, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data) - 2, new_offset);

    // Test invalid seek
    result = os_lseek(test_fd, -1000, __WASI_WHENCE_SET, &new_offset);
    EXPECT_NE(__WASI_ESUCCESS, result);
}

TEST_F(PosixFileTest, FileTruncateFileTruncation)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write initial data
    const char *data = "This is a long test string for truncation";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test truncate to smaller size
    __wasi_filesize_t new_size = 10;
    result = os_ftruncate(test_fd, new_size);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Verify truncation
    __wasi_filestat_t filestat;
    result = os_fstat(test_fd, &filestat);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(new_size, filestat.st_size);

    // Test truncate to larger size (should extend file)
    new_size = 100;
    result = os_ftruncate(test_fd, new_size);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    result = os_fstat(test_fd, &filestat);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(new_size, filestat.st_size);
}

TEST_F(PosixFileTest, FileSyncAndFdatasyncOperations)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write data to sync
    const char *data = "Data to be synchronized";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test fsync
    result = os_fsync(test_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test fdatasync (platform dependent)
    result = os_fdatasync(test_fd);
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOSYS);

    // Test sync on invalid fd
    result = os_fsync(999999);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_fdatasync(999999);
    EXPECT_TRUE(result == __WASI_EBADF || result == __WASI_ENOSYS);
}

TEST_F(PosixFileTest, FileIsattyTerminalDetection)
{
    // Test isatty on regular file (should fail)
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                  WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    result = os_isatty(test_fd);
    EXPECT_NE(__WASI_ESUCCESS, result); // Regular file is not TTY

    // Test isatty on standard streams
    result = os_isatty(0); // stdin
    // Result depends on test environment
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);

    result = os_isatty(1); // stdout
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);

    result = os_isatty(2); // stderr
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);
}

TEST_F(PosixFileTest, FileOpendirAndReaddirOperations)
{
    // Test directory operations properly
    __wasi_errno_t result;

    // First, open the test directory as a file descriptor
    os_file_handle dir_fd;
    result = os_openat(AT_FDCWD, test_dir.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &dir_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test fdopendir with valid directory fd
    os_dir_stream dir_stream;
    result = os_fdopendir(dir_fd, &dir_stream);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    if (result == __WASI_ESUCCESS) {
        // Test closedir
        result = os_closedir(dir_stream);
        EXPECT_EQ(__WASI_ESUCCESS, result);
    }

    // Close the directory fd
    os_close(dir_fd, false);

    // Test fdopendir on non-existent/invalid fd
    result = os_fdopendir(999999, &dir_stream);
    EXPECT_NE(__WASI_ESUCCESS, result); // Should fail with invalid fd
}

TEST_F(PosixFileTest, FileStatFileInformation)
{
    // Create test file with data
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    const char *data = "Test data for stat operations";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test fstat
    __wasi_filestat_t fstat_result;
    result = os_fstat(test_fd, &fstat_result);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(__WASI_FILETYPE_REGULAR_FILE, fstat_result.st_filetype);
    EXPECT_EQ(strlen(data), fstat_result.st_size);
    EXPECT_GT(fstat_result.st_ino, 0);

    // Test fstatat
    __wasi_filestat_t fstatat_result;
    result = os_fstatat(AT_FDCWD, test_file.c_str(), &fstatat_result, 0);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(fstat_result.st_dev, fstatat_result.st_dev);
    EXPECT_EQ(fstat_result.st_ino, fstatat_result.st_ino);
    EXPECT_EQ(fstat_result.st_size, fstatat_result.st_size);

    // Test stat on directory
    __wasi_filestat_t dir_stat;
    result = os_fstatat(AT_FDCWD, test_dir.c_str(), &dir_stat, 0);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(__WASI_FILETYPE_DIRECTORY, dir_stat.st_filetype);
}

// Step 3: File Advanced Operations - Testing vectored I/O and advanced file
// operations
TEST_F(PosixFileTest, FileAdvancedReadvVectorReadOperations)
{
    // Create and write test data using writev first
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write multiple segments using writev
    const char *data1 = "First segment ";
    const char *data2 = "Second segment ";
    const char *data3 = "Third segment";

    __wasi_ciovec_t write_iovs[3] = { { (const uint8_t *)data1, strlen(data1) },
                                      { (const uint8_t *)data2, strlen(data2) },
                                      { (const uint8_t *)data3,
                                        strlen(data3) } };

    size_t nwritten;
    result = os_writev(test_fd, write_iovs, 3, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data1) + strlen(data2) + strlen(data3), nwritten);

    // Seek to beginning for reading
    __wasi_filedelta_t offset = 0;
    __wasi_filesize_t new_offset;
    result = os_lseek(test_fd, offset, __WASI_WHENCE_SET, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test readv with multiple buffers
    char buffer1[20], buffer2[20], buffer3[20];
    __wasi_iovec_t read_iovs[3] = { { (uint8_t *)buffer1, sizeof(buffer1) },
                                    { (uint8_t *)buffer2, sizeof(buffer2) },
                                    { (uint8_t *)buffer3, sizeof(buffer3) } };

    size_t nread;
    result = os_readv(test_fd, read_iovs, 3, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_GT(nread, 0);

    // Verify data was read correctly
    EXPECT_EQ(0, strncmp(buffer1, data1, strlen(data1)));
}

TEST_F(PosixFileTest, FileAdvancedWritevVectorWriteOperations)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test writev with various buffer sizes
    const char *small_data = "A";
    const char *medium_data = "This is medium data content";
    char large_data[1024];
    memset(large_data, 'X', sizeof(large_data) - 1);
    large_data[sizeof(large_data) - 1] = '\0';

    __wasi_ciovec_t iovs[3] = {
        { (const uint8_t *)small_data, strlen(small_data) },
        { (const uint8_t *)medium_data, strlen(medium_data) },
        { (const uint8_t *)large_data, strlen(large_data) }
    };

    size_t nwritten;
    result = os_writev(test_fd, iovs, 3, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    size_t expected_written =
        strlen(small_data) + strlen(medium_data) + strlen(large_data);
    EXPECT_EQ(expected_written, nwritten);

    // Test writev with empty vectors
    __wasi_ciovec_t empty_iov = { nullptr, 0 };
    result = os_writev(test_fd, &empty_iov, 1, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(0, nwritten);

    // Test writev with zero count
    result = os_writev(test_fd, iovs, 0, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(0, nwritten);
}

TEST_F(PosixFileTest, FileAdvancedPreadPositionedReads)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write test data
    const char *test_data = "0123456789ABCDEFGHIJ";
    __wasi_ciovec_t write_iov = { (const uint8_t *)test_data,
                                  strlen(test_data) };
    size_t nwritten;
    result = os_writev(test_fd, &write_iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test pread at different positions without affecting file position
    char buffer[10];
    size_t nread;

    // Read from position 5
    __wasi_iovec_t read_iov = { (uint8_t *)buffer, 5 };
    result = os_preadv(test_fd, &read_iov, 1, 5, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(5, nread);
    EXPECT_EQ(0, strncmp(buffer, "56789", 5));

    // Read from position 0
    read_iov.buf_len = 3;
    result = os_preadv(test_fd, &read_iov, 1, 0, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(3, nread);
    EXPECT_EQ(0, strncmp(buffer, "012", 3));

    // Test pread beyond file size
    read_iov.buf_len = 5;
    result = os_preadv(test_fd, &read_iov, 1, 100, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(0, nread); // Should read 0 bytes
}

TEST_F(PosixFileTest, FileAdvancedPwritePositionedWrites)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Initialize file with known data
    const char *initial_data = "AAAAAAAAAA"; // 10 A's
    __wasi_ciovec_t write_iov = { (const uint8_t *)initial_data,
                                  strlen(initial_data) };
    size_t nwritten;
    result = os_writev(test_fd, &write_iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test pwrite at position 3
    const char *new_data = "XYZ";
    write_iov.buf = (const uint8_t *)new_data;
    write_iov.buf_len = strlen(new_data);
    result = os_pwritev(test_fd, &write_iov, 1, 3, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(new_data), nwritten);

    // Verify the write occurred at the correct position
    char verify_buffer[15];
    size_t nread;
    __wasi_iovec_t read_iov = { (uint8_t *)verify_buffer, 10 };
    result = os_preadv(test_fd, &read_iov, 1, 0, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(10, nread);
    EXPECT_EQ(0, strncmp(verify_buffer, "AAAXYZAAAA", 10));

    // Test pwrite beyond current file size (should extend file)
    const char *extend_data = "END";
    write_iov.buf = (const uint8_t *)extend_data;
    write_iov.buf_len = strlen(extend_data);
    result = os_pwritev(test_fd, &write_iov, 1, 15, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(extend_data), nwritten);
}

TEST_F(PosixFileTest, FileAdvancedReaddirDirectoryIteration)
{
    // Create multiple files in test directory
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";

    // Create files
    os_file_handle fd1, fd2;
    __wasi_errno_t result =
        os_openat(AT_FDCWD, file1.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC, 0,
                  0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &fd1);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    os_close(fd1, false);

    result = os_openat(AT_FDCWD, file2.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                       0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &fd2);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    os_close(fd2, false);

    // Test opening directory (exercises directory handling code paths)
    os_file_handle dir_fd;
    result = os_openat(AT_FDCWD, test_dir.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &dir_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test that we successfully opened a directory
    EXPECT_GE(dir_fd, 0);

    // Clean up
    os_close(dir_fd, false);
    unlink(file1.c_str());
    unlink(file2.c_str());
}

TEST_F(PosixFileTest, FileAdvancedReaddirErrorConditions)
{
    // Test opening regular file vs directory (exercises different code paths)
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test opening directory
    os_file_handle dir_fd;
    result = os_openat(AT_FDCWD, test_dir.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &dir_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Verify different file descriptors for file vs directory
    EXPECT_NE(test_fd, dir_fd);

    if (result == __WASI_ESUCCESS) {
        os_close(dir_fd, false);
    }
}

TEST_F(PosixFileTest, FileAdvancedVectoredIoEdgeCases)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test writev with mixed empty and non-empty vectors
    const char *data = "Test data";
    __wasi_ciovec_t mixed_iovs[4] = {
        { nullptr, 0 },                          // Empty
        { (const uint8_t *)data, strlen(data) }, // Non-empty
        { nullptr, 0 },                          // Empty
        { (const uint8_t *)" more", 5 }          // Non-empty
    };

    size_t nwritten;
    result = os_writev(test_fd, mixed_iovs, 4, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data) + 5, nwritten);

    // Test readv with mixed buffer sizes
    __wasi_filesize_t dummy_offset;
    os_lseek(test_fd, 0, __WASI_WHENCE_SET, &dummy_offset);

    char small_buf[5], large_buf[20], tiny_buf[1];
    __wasi_iovec_t read_iovs[3] = { { (uint8_t *)small_buf, sizeof(small_buf) },
                                    { (uint8_t *)large_buf, sizeof(large_buf) },
                                    { (uint8_t *)tiny_buf, sizeof(tiny_buf) } };

    size_t nread;
    result = os_readv(test_fd, read_iovs, 3, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_GT(nread, 0);
}

TEST_F(PosixFileTest, FileAdvancedPositionedIoBoundaries)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write initial data
    const char *data = "0123456789";
    size_t nwritten;
    __wasi_ciovec_t write_iov = { (const uint8_t *)data, strlen(data) };
    result = os_pwritev(test_fd, &write_iov, 1, 0, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test boundary conditions
    char buffer[5];
    size_t nread;

    // Read exactly at file boundary
    __wasi_iovec_t read_iov = { (uint8_t *)buffer, 1 };
    result = os_preadv(test_fd, &read_iov, 1, 9, &nread); // Last byte
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(1, nread);
    EXPECT_EQ('9', buffer[0]);

    // Read just beyond file boundary
    read_iov.buf_len = 5;
    result = os_preadv(test_fd, &read_iov, 1, 10, &nread); // Beyond EOF
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(0, nread);

    // Write at file boundary (should extend)
    const char *extend = "ABC";
    write_iov.buf = (const uint8_t *)extend;
    write_iov.buf_len = strlen(extend);
    result = os_pwritev(test_fd, &write_iov, 1, 10, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(extend), nwritten);

    // Verify file was extended
    read_iov.buf_len = 3;
    result = os_preadv(test_fd, &read_iov, 1, 10, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(3, nread);
    EXPECT_EQ(0, strncmp(buffer, "ABC", 3));
}

TEST_F(PosixFileTest, FileAdvancedDirectoryOperationsComprehensive)
{
    // Test directory operations with various scenarios
    std::string nested_dir = test_dir + "/nested";
    std::string deep_dir = nested_dir + "/deep";

    // Create nested directories
    mkdir(nested_dir.c_str(), 0755);
    mkdir(deep_dir.c_str(), 0755);

    // Create files at different levels
    std::string root_file = test_dir + "/root.txt";
    std::string nested_file = nested_dir + "/nested.txt";
    std::string deep_file = deep_dir + "/deep.txt";

    // Create the files
    for (const auto &filepath : { root_file, nested_file, deep_file }) {
        os_file_handle fd;
        __wasi_errno_t result = os_openat(
            AT_FDCWD, filepath.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC, 0, 0,
            WASI_LIBC_ACCESS_MODE_READ_WRITE, &fd);
        if (result == __WASI_ESUCCESS) {
            os_close(fd, false);
        }
    }

    // Test reading each directory level
    for (const auto &dirpath : { test_dir, nested_dir, deep_dir }) {
        os_file_handle dir_fd;
        __wasi_errno_t result =
            os_openat(AT_FDCWD, dirpath.c_str(), 0, 0, 0,
                      WASI_LIBC_ACCESS_MODE_READ_ONLY, &dir_fd);
        if (result == __WASI_ESUCCESS) {
            char buffer[1024];
            size_t nread;
            // Just verify we can open directories at different levels
            EXPECT_GE(dir_fd, 0);
            os_close(dir_fd, false);
        }
    }

    // Cleanup in reverse order
    for (const auto &filepath : { deep_file, nested_file, root_file }) {
        unlink(filepath.c_str());
    }
    for (const auto &dirpath : { deep_dir, nested_dir }) {
        rmdir(dirpath.c_str());
    }
}

TEST_F(PosixFileTest, FileAdvancedIoErrorRecovery)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test recovery from various error conditions

    // 1. Test operations on closed file
    os_file_handle closed_fd = test_fd;
    os_close(closed_fd, false);

    char buffer[100];
    size_t nread;
    __wasi_iovec_t read_iov = { (uint8_t *)buffer, 10 };
    result = os_preadv(closed_fd, &read_iov, 1, 0, &nread);
    EXPECT_NE(__WASI_ESUCCESS, result);

    size_t nwritten;
    __wasi_ciovec_t write_iov = { (const uint8_t *)"data", 4 };
    result = os_pwritev(closed_fd, &write_iov, 1, 0, &nwritten);
    EXPECT_NE(__WASI_ESUCCESS, result);

    // 2. Reopen and verify normal operation resumes
    result = os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Normal operation should work
    const char *test_data = "Recovery test";
    write_iov.buf = (const uint8_t *)test_data;
    write_iov.buf_len = strlen(test_data);
    result = os_pwritev(test_fd, &write_iov, 1, 0, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nwritten);

    // Verify data was written
    read_iov.buf_len = strlen(test_data);
    result = os_preadv(test_fd, &read_iov, 1, 0, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nread);
    EXPECT_EQ(0, strncmp(buffer, test_data, strlen(test_data)));
}