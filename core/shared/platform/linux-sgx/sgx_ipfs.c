/*
 * Copyright (C) 2022 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifdef WAMR_SGX_IPFS

#include "ssp_config.h"
#include "bh_platform.h"
#include "sgx_ipfs.h"

#include <errno.h>

#include "sgx_tprotected_fs.h"

#define SGX_ERROR_FILE_LOWEST_ERROR_ID SGX_ERROR_FILE_BAD_STATUS
#define SGX_ERROR_FILE_HIGHEST_ERROR_ID SGX_ERROR_FILE_CLOSE_FAILED

// Converts an SGX error code to a POSIX error code.
__wasi_errno_t
convert_sgx_errno(int error)
{
    if (error >= SGX_ERROR_FILE_LOWEST_ERROR_ID
        && error <= SGX_ERROR_FILE_HIGHEST_ERROR_ID) {
        switch (error) {
            /* The file is in bad status */
            case SGX_ERROR_FILE_BAD_STATUS:
                return ENOTRECOVERABLE;
            /* The Key ID field is all zeros, can't re-generate the encryption
             * key */
            case SGX_ERROR_FILE_NO_KEY_ID:
                return EKEYREJECTED;
            /* The current file name is different then the original file name
             * (not allowed, substitution attack) */
            case SGX_ERROR_FILE_NAME_MISMATCH:
                return EIO;
            /* The file is not an SGX file */
            case SGX_ERROR_FILE_NOT_SGX_FILE:
                return EEXIST;
            /* A recovery file can't be opened, so flush operation can't
             * continue (only used when no EXXX is returned)  */
            case SGX_ERROR_FILE_CANT_OPEN_RECOVERY_FILE:
                return EIO;
            /* A recovery file can't be written, so flush operation can't
             * continue (only used when no EXXX is returned)  */
            case SGX_ERROR_FILE_CANT_WRITE_RECOVERY_FILE:
                return EIO;
            /* When openeing the file, recovery is needed, but the recovery
             * process failed */
            case SGX_ERROR_FILE_RECOVERY_NEEDED:
                return EIO;
            /* fflush operation (to disk) failed (only used when no EXXX is
             * returned) */
            case SGX_ERROR_FILE_FLUSH_FAILED:
                return EIO;
            /* fclose operation (to disk) failed (only used when no EXXX is
             * returned) */
            case SGX_ERROR_FILE_CLOSE_FAILED:
                return EIO;
        }
    }

    return error;
}

int
ipfs_posix_fallocate(void *sgx_file, off_t offset, size_t len)
{
    // The wrapper for fseek takes care of extending the file if sought beyond
    // the end
    if (ipfs_lseek(sgx_file, offset + len, SEEK_CUR) == -1) {
        return errno;
    }

    // Make sure the file is allocated by flushing it
    if (sgx_fflush(sgx_file) != 0) {
        return errno;
    }

    return 0;
}

size_t
ipfs_readv(void *sgx_file, const struct iovec *iov, int iovcnt)
{
    int i;
    size_t read_result, number_of_read_bytes = 0;

    // For each element in the vector
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0)
            continue;

        read_result = sgx_fread(iov[i].iov_base, 1, iov[i].iov_len, sgx_file);
        number_of_read_bytes += read_result;

        if (read_result != iov[i].iov_len) {
            if (!sgx_feof(sgx_file)) {
                errno = convert_sgx_errno(sgx_ferror(sgx_file));
                return -1;
            }
        }
    }

    return number_of_read_bytes;
}

size_t
ipfs_pread(void *sgx_file, void *buffer, size_t size, off_t offset)
{
    // Save the current offset, to restore it after the read operation
    off_t original_offset = (off_t)sgx_ftell(sgx_file);

    if (original_offset == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Move to the desired location
    if (sgx_fseek(sgx_file, offset, SEEK_SET) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Store the content of into buffer
    size_t bytes_read = sgx_fread(buffer, 1, size, sgx_file);
    if (bytes_read == 0) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Restore the position of the cursor
    if (sgx_fseek(sgx_file, original_offset, SEEK_SET) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    return bytes_read;
}

size_t
ipfs_writev(void *sgx_file, const struct iovec *iov, int iovcnt)
{
    int i;
    size_t write_result, number_of_written_bytes = 0;

    // For each element in the vector
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0)
            continue;

        write_result = sgx_fwrite(iov[i].iov_base, 1, iov[i].iov_len, sgx_file);
        number_of_written_bytes += write_result;

        if (write_result != iov[i].iov_len) {
            errno = convert_sgx_errno(sgx_ferror(sgx_file));
            return -1;
        }
    }

    return number_of_written_bytes;
}

size_t
ipfs_pwrite(void *sgx_file, const void *buffer, size_t size, off_t offset)
{
    // Save the current offset, to restore it after the read operation
    off_t original_offset = (off_t)sgx_ftell(sgx_file);

    if (original_offset == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Move to the desired location
    if (sgx_fseek(sgx_file, offset, SEEK_SET) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Write the content of the buffer
    size_t bytes_written = sgx_fwrite(buffer, 1, size, sgx_file);
    if (bytes_written == 0) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Restore the position of the cursor
    if (sgx_fseek(sgx_file, original_offset, SEEK_SET) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    return bytes_written;
}

int
ipfs_close(void *sgx_file)
{
    if (!sgx_fclose(sgx_file)) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    return 0;
}

void *
ipfs_fopen(const char *filename, __wasi_rights_t fs_rights_base,
           __wasi_oflags_t oflags, __wasi_fdflags_t fs_flags)
{
    // Mapping back the mode
    // TODO Refine the rights, based on the POSIX implem.
    const char *mode;

    bool read = (fs_rights_base & __WASI_RIGHT_FD_READ) != 0;
    bool write = (fs_rights_base & __WASI_RIGHT_FD_WRITE) != 0;
    bool must_create = (oflags & __WASI_O_CREAT) != 0;
    bool must_truncate = (oflags & __WASI_O_TRUNC) != 0;
    bool must_append = (fs_flags & __WASI_FDFLAG_APPEND) != 0;
    bool read_only = (read & !write) != 0;
    bool write_only = (!read & write) != 0;
    bool read_write = (read & write) != 0;

    // The mapping of the mode are described in the table in the official
    // specifications:
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fopen.html
    if (read_only)
        mode = "r";
    else if (write_only && must_create && must_truncate)
        mode = "w";
    else if (write_only && must_create && must_append)
        mode = "a";
    else if (read_write && must_create && must_truncate)
        mode = "w+";
    else if (read_write && must_create && must_append)
        mode = "a+";
    else if (read_write && must_create)
        mode = "w+";
    else if (read_write)
        mode = "r+";
    else
        mode = NULL;

    // Cannot map the requested access to the SGX PFS
    if (mode == NULL) {
        errno = __WASI_ENOTCAPABLE;
        return NULL;
    }

    // Opening the file
    os_printf("Opening mode: %s\n", mode);
    void *sgx_file = sgx_fopen_auto_key(filename, mode);

    if (sgx_file == NULL) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return NULL;
    }

    return sgx_file;
}

int
ipfs_fflush(void *sgx_file)
{
    int ret = sgx_fflush(sgx_file);

    if (ret == 1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return EOF;
    }

    return ret;
}

off_t
ipfs_lseek(void *sgx_file, off_t offset, int nwhence)
{
    off_t new_offset;
    int fseek_result = sgx_fseek(sgx_file, offset, nwhence);

    if (fseek_result == 0) {
        new_offset = (__wasi_filesize_t)sgx_ftell(sgx_file);

        if (new_offset == -1) {
            errno = convert_sgx_errno(sgx_ferror(sgx_file));
            return -1;
        }

        return new_offset;
    }
    else {
        // In the case fseek returned an error
        int sgx_error = sgx_ferror(sgx_file);
        if (sgx_error != EINVAL) {
            errno = convert_sgx_errno(sgx_error);
            return -1;
        }

        // We must consider a difference in behavior of sgx_fseek and the POSIX
        // fseek. If the cursor is moved beyond the end of the file, sgx_fseek
        // returns an error, whereas POSIX fseek accepts the cursor move and
        // fill with zeroes the difference for the next write. This
        // implementation handle zeroes completion and moving the cursor forward
        // the end of the file, but does it now (during the fseek), which is
        // different compared to POSIX implementation, that writes zeroes on the
        // next write. This avoids the runtime to keep track of the cursor
        // manually.

        // Assume the error is raised because the cursor is moved beyond the end
        // of the file. Try to move the cursor at the end of the file.
        if (sgx_fseek(sgx_file, 0, SEEK_END) == -1) {
            errno = convert_sgx_errno(sgx_ferror(sgx_file));
            return -1;
        }

        // Write the missing zeroes
        char zero = 0;
        int64_t number_of_zeroes = offset - sgx_ftell(sgx_file);
        if (sgx_fwrite(&zero, 1, number_of_zeroes, sgx_file) == 0) {
            errno = convert_sgx_errno(sgx_ferror(sgx_file));
            return -1;
        }

        // Move again at the end of the file
        if (sgx_fseek(sgx_file, 0, SEEK_END) == -1) {
            errno = convert_sgx_errno(sgx_ferror(sgx_file));
            return -1;
        }

        return offset;
    }
}

off_t
ipfs_ftell(void *sgx_file)
{
    int64_t ftell_result = (off_t)sgx_ftell(sgx_file);

    if (ftell_result == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    return ftell_result;
}

// The official API does not provide a way to truncate files.
// Only files extension is supported.
int
ipfs_ftruncate(void *sgx_file, off_t len)
{
    off_t original_offset = sgx_ftell(sgx_file);

    // Optimization path: if the length is smaller than the offset,
    // IPFS does not support truncate to a smaller size.
    if (len < original_offset) {
        os_printf(
            "SGX IPFS does not support truncate files to smaller sizes.\n");
        return __WASI_ECANCELED;
    }

    // Move to the end of the file to determine whether this is
    // a file extension or reduction.
    if (sgx_fseek(sgx_file, 0, SEEK_END) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    off_t file_size = sgx_ftell(sgx_file);

    // Reducing the file space is not supported by IPFS.
    if (len < file_size) {
        os_printf(
            "SGX IPFS does not support truncate files to smaller sizes.\n");
        return __WASI_ECANCELED;
    }

    // Increasing the size is equal to writing from the end of the file
    // with null bytes.
    char null_byte = 0;
    if (sgx_fwrite(&null_byte, 1, len - file_size, sgx_file) == 0) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    // Restore the position of the cursor
    if (sgx_fseek(sgx_file, original_offset, SEEK_SET) == -1) {
        errno = convert_sgx_errno(sgx_ferror(sgx_file));
        return -1;
    }

    return 0;
}

#endif /* end of WAMR_SGX_IPFS */