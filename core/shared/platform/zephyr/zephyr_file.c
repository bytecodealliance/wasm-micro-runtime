/*
 * Copyright (C) 2024 Grenoble INP - ESISAR.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_extension.h"
#include "libc_errno.h"

#include <string.h>
#include <stdlib.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/fs/littlefs.h>

/* Notes:
 * This is the implementation of a POSIX-like file system interface for Zephyr.
 * To manage our file descriptors, we created a struct `zephyr_fs_desc` that
 * represent a zephyr file descriptor and hold useful informations.
 * We also created a file descriptor table to keep track of all the file
 * descriptors.
 *
 * To pass the file descriptor reference to the higher level abstraction, we
 * pass the index of the fd table to an `os_file_handle` struct.
 * Then in the WASI implementation layer we can retrieve the file descriptor
 * reference.
 *
 * We also fake the stdin, stdout and stderr file descriptors.
 * We redirect the write operation on stdin, stdout and stderr to `os_printf`.
 * We do not handle write on stdin and read on stdin, stdout and stderr.
 */

// No OS API wrapper (Zephyr):
// file:
//     off_t fs_tell(struct fs_file_t *zfp)
// file system:
//     int fs_mount(struct fs_mount_t *mp)
//     int fs_unmount(struct fs_mount_t *mp
//     int fs_readmount(int *index, const char **name)
//     int fs_statvfs(const char *path, struct fs_statvfs *stat)
//     int fs_mkfs(int fs_type, uintptr_t dev_id, void *cfg, int flags)
//     int fs_register(int type, const struct fs_file_system_t *fs)
//     int fs_unregister(int type, const struct fs_file_system_t *fs)

// We will take the maximum number of open files
// from the Zephyr POSIX configuration
#define CONFIG_WASI_MAX_OPEN_FILES CONFIG_POSIX_MAX_FDS

// Macro to retrieve a file system descriptor and check it's validity.
#define GET_FILE_SYSTEM_DESCRIPTOR(fd, ptr)         \
    do {                                            \
        k_mutex_lock(&desc_array_mutex, K_FOREVER); \
        ptr = &desc_array[(int)fd];                 \
        if (!ptr) {                                 \
            k_mutex_unlock(&desc_array_mutex);      \
            return __WASI_EBADF;                    \
        }                                           \
        k_mutex_unlock(&desc_array_mutex);          \
    } while (0)

// Array to keep track of file system descriptors.
static struct zephyr_fs_desc desc_array[CONFIG_WASI_MAX_OPEN_FILES];

// mutex to protect the file descriptor array
K_MUTEX_DEFINE(desc_array_mutex);

static char prestat_dir[MAX_FILE_NAME + 1];

static struct zephyr_fs_desc *
zephyr_fs_alloc_obj(bool is_dir, const char *path, int *index)
{
    int i;
    struct zephyr_fs_desc *ptr = NULL;
    *index = -1; // give a default value to index in case table is full

    k_mutex_lock(&desc_array_mutex, K_FOREVER);
    for (i = 0; i < CONFIG_WASI_MAX_OPEN_FILES; i++) {
        if (desc_array[i].used == false) {
            ptr = &desc_array[i];
            ptr->used = true;
            ptr->is_dir = is_dir;
            ptr->path = strdup(path);
            if (ptr->path == NULL) {
                ptr->used = false;
                k_mutex_unlock(&desc_array_mutex);
                return NULL;
            }
            *index = i;
            break;
        }
    }

    k_mutex_unlock(&desc_array_mutex);

    return ptr;
}

static inline void
zephyr_fs_free_obj(struct zephyr_fs_desc *ptr)
{
    free(ptr->path);
    ptr->path = NULL;
    ptr->used = false;
}

void
debug_zephyr_fs_desc(const zephyr_fs_desc *desc)
{
    if (desc == NULL) {
        os_printf("Descriptor is NULL\n");
        return;
    }
    os_printf("Descriptor found at %p\n", desc);
    os_printf("    Path: %s\n", desc->path ? desc->path : "NULL");
    os_printf("    Is Directory: %s\n", desc->is_dir ? "Yes" : "No");
    os_printf("    Used: %s\n", desc->used ? "Yes" : "No");

    if (desc->is_dir) {
        os_printf("    Directory: %p\n", desc->dir);
    }
    else {
        os_printf("    File: %p\n", desc->file);
    }
}

/* /!\ Needed for socket to work */
__wasi_errno_t
os_fstat(os_file_handle handle, struct __wasi_filestat_t *buf)
{
    struct zephyr_fs_desc *ptr = NULL;
    int socktype, rc;

    if (!handle->is_sock) {
        GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);
        // debug_zephyr_fs_desc(ptr);

        /* We treat the case of std[in/out/err] */
        if (ptr->path != NULL
            && (!strcmp(ptr->path, "stdin") || !strcmp(ptr->path, "stdout")
                || !strcmp(ptr->path, "stderr"))) {
            buf->st_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
            buf->st_size = 0;
            buf->st_atim = 0;
            buf->st_mtim = 0;
            buf->st_ctim = 0;
            return __WASI_ESUCCESS;
        }

        return os_fstatat(handle, ptr->path, buf, 0);
    }
    else {
        // socklen_t socktypelen = sizeof(socktype);
        // rc = zsock_getsockopt(handle->fd, SOL_SOCKET, SO_TYPE, &socktype,
        // &socktypelen); Using `zsock_getsockopt` will add a dependency on the
        // network stack
        // TODO: may add a type to the `zephyr_handle`.
        rc = 1;
        socktype = SOCK_STREAM;
        if (rc < 0) {
            return convert_errno(-rc);
        }

        switch (socktype) {
            case SOCK_DGRAM:
                buf->st_filetype = __WASI_FILETYPE_SOCKET_DGRAM;
                break;
            case SOCK_STREAM:
                buf->st_filetype = __WASI_FILETYPE_SOCKET_STREAM;
                break;
            default:
                buf->st_filetype = __WASI_FILETYPE_UNKNOWN;
                break;
        }
        buf->st_size = 0;
        buf->st_atim = 0;
        buf->st_mtim = 0;
        buf->st_ctim = 0;
        return __WASI_ESUCCESS;
    }
}

__wasi_errno_t
os_fstatat(os_file_handle handle, const char *path,
           struct __wasi_filestat_t *buf, __wasi_lookupflags_t lookup_flags)
{
    struct fs_dirent entry;
    int rc;

    if (handle->fd < 0) {
        return __WASI_EBADF;
    }

    // Get file information using Zephyr's fs_stat function
    rc = fs_stat(path, &entry);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    // Fill in the __wasi_filestat_t structure
    buf->st_dev = 0; // Zephyr's fs_stat doesn't provide a device ID
    buf->st_ino = 0; // Zephyr's fs_stat doesn't provide an inode number
    buf->st_filetype = entry.type == FS_DIR_ENTRY_DIR
                           ? __WASI_FILETYPE_DIRECTORY
                           : __WASI_FILETYPE_REGULAR_FILE;
    buf->st_nlink = 1; // Zephyr's fs_stat doesn't provide a link count
    buf->st_size = entry.size;
    buf->st_atim = 0; // Zephyr's fs_stat doesn't provide timestamps
    buf->st_mtim = 0;
    buf->st_ctim = 0;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_file_get_fdflags(os_file_handle handle, __wasi_fdflags_t *flags)
{
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    if ((ptr->file.flags & FS_O_APPEND) != 0) {
        *flags |= __WASI_FDFLAG_APPEND;
    }
    /* Others flags:
     *     - __WASI_FDFLAG_DSYNC
     *     - __WASI_FDFLAG_RSYNC
     *     - __WASI_FDFLAG_SYNC
     *     - __WASI_FDFLAG_NONBLOCK
     * Have no equivalent in Zephyr.
     */
    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_file_set_fdflags(os_file_handle handle, __wasi_fdflags_t flags)
{
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    if ((flags & __WASI_FDFLAG_APPEND) != 0) {
        ptr->file.flags |= FS_O_APPEND;
    }
    /* Same as above */
    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_fdatasync(os_file_handle handle)
{
    return os_fsync(handle);
}

__wasi_errno_t
os_fsync(os_file_handle handle)
{
    struct zephyr_fs_desc *ptr = NULL;
    int rc = 0;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    if (ptr->is_dir) {
        return __WASI_EISDIR;
    }

    rc = fs_sync(&ptr->file);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_open_preopendir(const char *path, os_file_handle *out)
{
    int rc, index;
    struct zephyr_fs_desc *ptr;

    *out = BH_MALLOC(sizeof(struct zephyr_handle));
    if (*out == NULL) {
        return __WASI_ENOMEM;
    }

    ptr = zephyr_fs_alloc_obj(true, path, &index);
    if (ptr == NULL) {
        BH_FREE(*out);
        return __WASI_EMFILE;
    }

    fs_dir_t_init(&ptr->dir);

    rc = fs_opendir(&ptr->dir, path);
    if (rc < 0) {
        zephyr_fs_free_obj(ptr);
        BH_FREE(*out);
        return convert_errno(-rc);
    }

    (*out)->fd = index;
    (*out)->is_sock = false;

    strncpy(prestat_dir, path, MAX_FILE_NAME + 1);
    prestat_dir[MAX_FILE_NAME] = '\0';

    return __WASI_ESUCCESS;
}

static int
wasi_flags_to_zephyr(__wasi_oflags_t oflags, __wasi_fdflags_t fd_flags,
                     __wasi_lookupflags_t lookup_flags,
                     wasi_libc_file_access_mode access_mode)
{
    int mode = 0;

    // Convert open flags.
    if ((oflags & __WASI_O_CREAT) != 0) {
        mode |= FS_O_CREATE;
    }
    if (((oflags & __WASI_O_EXCL) != 0) || ((oflags & __WASI_O_TRUNC) != 0)
        || ((oflags & __WASI_O_DIRECTORY) != 0)) {
        /* Zephyr is not POSIX no equivalent for these flags */
        /* __WASI_O_DIRECTORY: Open shouldn't handle directories */
        // TODO: log warning
    }

    // Convert file descriptor flags.
    if ((fd_flags & __WASI_FDFLAG_APPEND) != 0) {
        mode |= FS_O_APPEND;
    }
    if (((fd_flags & __WASI_FDFLAG_DSYNC) != 0)
        || ((fd_flags & __WASI_FDFLAG_RSYNC) != 0)
        || ((fd_flags & __WASI_FDFLAG_SYNC) != 0)
        || ((fd_flags & __WASI_FDFLAG_NONBLOCK) != 0)) {
        /* Zephyr is not POSIX no equivalent for these flags */
        // TODO: log warning
    }

    // Convert lookup flag.
    if ((lookup_flags & __WASI_LOOKUP_SYMLINK_FOLLOW) == 0) {
        /* Zephyr is not POSIX no equivalent for these flags */
        // TODO: log warning
        return __WASI_ENOTSUP;
    }

    // Convert access mode.
    switch (access_mode) {
        case WASI_LIBC_ACCESS_MODE_READ_WRITE:
            mode |= FS_O_RDWR;
            break;
        case WASI_LIBC_ACCESS_MODE_READ_ONLY:
            mode |= FS_O_READ;
            break;
        case WASI_LIBC_ACCESS_MODE_WRITE_ONLY:
            mode |= FS_O_WRITE;
            break;
        default:
            // TODO: log warning
            break;
    }
    return mode;
}

__wasi_errno_t
os_openat(os_file_handle handle, const char *path, __wasi_oflags_t oflags,
          __wasi_fdflags_t fd_flags, __wasi_lookupflags_t lookup_flags,
          wasi_libc_file_access_mode access_mode, os_file_handle *out)
{
    /*
     * `handle` will be unused because zephyr doesn't expose an openat
     * function and don't seem to have the concept of relative path.
     * We fill `out` with a new file descriptor.
     */
    int rc, index;
    struct zephyr_fs_desc *ptr = NULL;
    char abs_path[MAX_FILE_NAME + 1];

    *out = BH_MALLOC(sizeof(struct zephyr_handle));
    if (*out == NULL) {
        return __WASI_ENOMEM;
    }

    snprintf(abs_path, MAX_FILE_NAME, "%s/%s", prestat_dir, path);
    int zmode =
        wasi_flags_to_zephyr(oflags, fd_flags, lookup_flags, access_mode);

    ptr = zephyr_fs_alloc_obj(false, abs_path, &index);
    if (!ptr && (index < 0)) {
        BH_FREE(*out);
        return __WASI_EMFILE;
    }

    fs_file_t_init(&ptr->file);
    rc = fs_open(&ptr->file, abs_path, zmode);
    if (rc < 0) {
        zephyr_fs_free_obj(ptr);
        BH_FREE(*out);
        return convert_errno(-rc);
    }

    (*out)->fd = index;
    (*out)->is_sock = false;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_file_get_access_mode(os_file_handle handle,
                        wasi_libc_file_access_mode *access_mode)
{
    struct zephyr_fs_desc *ptr = NULL;

    if (handle->is_sock) {
        // for socket we can use the following code
        // TODO: Need to determine better logic
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;
        return __WASI_ESUCCESS;
    }

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    if ((ptr->file.flags & FS_O_RDWR) != 0) {
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;
    }
    else if ((ptr->file.flags & FS_O_READ) != 0) {
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_ONLY;
    }
    else if ((ptr->file.flags & FS_O_WRITE) != 0) {
        *access_mode = WASI_LIBC_ACCESS_MODE_WRITE_ONLY;
    }
    else {
        // we return read/write by default
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;
    }
    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_close(os_file_handle handle, bool is_stdio)
{
    int rc;
    struct zephyr_fs_desc *ptr = NULL;

    if (is_stdio)
        return __WASI_ESUCCESS;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    rc = ptr->is_dir ? fs_closedir(&ptr->dir) : fs_close(&ptr->file);
    zephyr_fs_free_obj(ptr); // free in any case.
    BH_FREE(handle);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_preadv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
          __wasi_filesize_t offset, size_t *nread)
{
    struct zephyr_fs_desc *ptr = NULL;
    int rc;
    ssize_t total_read = 0;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    // Seek to the offset
    rc = fs_seek(&ptr->file, offset, FS_SEEK_SET);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    // Read data into each buffer
    for (int i = 0; i < iovcnt; i++) {
        ssize_t bytes_read = fs_read(&ptr->file, iov[i].buf, iov[i].buf_len);
        if (bytes_read < 0) {
            return convert_errno(-bytes_read);
        }

        total_read += bytes_read;

        // If we read less than we asked for, stop reading
        if (bytes_read < iov[i].buf_len) {
            break;
        }
    }

    *nread = total_read;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_pwritev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
           __wasi_filesize_t offset, size_t *nwritten)
{
    struct zephyr_fs_desc *ptr = NULL;
    ssize_t total_written = 0;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    // Seek to the offset
    int rc = fs_seek(&ptr->file, offset, FS_SEEK_SET);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    // Write data from each buffer
    for (int i = 0; i < iovcnt; i++) {
        ssize_t bytes_written =
            fs_write(&ptr->file, iov[i].buf, iov[i].buf_len);
        if (bytes_written < 0) {
            return convert_errno(-bytes_written);
        }

        total_written += bytes_written;

        // If we wrote less than we asked for, stop writing
        if (bytes_written < iov[i].buf_len) {
            break;
        }
    }

    *nwritten = total_written;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_readv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
         size_t *nread)
{
    struct zephyr_fs_desc *ptr = NULL;
    ssize_t total_read = 0;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    // Read data into each buffer
    for (int i = 0; i < iovcnt; i++) {
        ssize_t bytes_read = fs_read(&ptr->file, iov[i].buf, iov[i].buf_len);
        if (bytes_read < 0) {
            // If an error occurred, return it
            return convert_errno(-bytes_read);
        }

        total_read += bytes_read;

        // If we read less than we asked for, stop reading
        if (bytes_read < iov[i].buf_len) {
            break;
        }
    }

    *nread = total_read;

    return __WASI_ESUCCESS;
}

/* With wasi-libc we need to redirect write on stdout/err to printf */
// TODO: handle write on stdin
__wasi_errno_t
os_writev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
          size_t *nwritten)
{
    struct zephyr_fs_desc *ptr = NULL;
    ssize_t total_written = 0;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    if (strncmp(ptr->path, "std", 3) == 0) {
        // for std[in/out/err] we don't write because they are not real opened
        // files. Instead we emulate a write operation to make it work with
        // printf.
        for (int i = 0; i < iovcnt; i++) {
            if (iov[i].buf_len == 0)
                continue;
            os_printf("%s", (char *)iov[i].buf);
            total_written += iov[i].buf_len;

            // Clear the buffer after printing
            memset(iov[i].buf, 0, iov[i].buf_len);
        }
        *nwritten = total_written;

        return __WASI_ESUCCESS;
    }

    // Write data from each buffer
    for (int i = 0; i < iovcnt; i++) {
        ssize_t bytes_written =
            fs_write(&ptr->file, iov[i].buf, iov[i].buf_len);
        if (bytes_written < 0) {
            return convert_errno(-bytes_written);
        }

        total_written += bytes_written;

        // If we wrote less than we asked for, stop writing
        if (bytes_written < iov[i].buf_len) {
            break;
        }
    }

    *nwritten = total_written;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_fallocate(os_file_handle handle, __wasi_filesize_t offset,
             __wasi_filesize_t length)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_ftruncate(os_file_handle handle, __wasi_filesize_t size)
{
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    int rc = fs_truncate(&ptr->file, (off_t)size);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_futimens(os_file_handle handle, __wasi_timestamp_t access_time,
            __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_utimensat(os_file_handle handle, const char *path,
             __wasi_timestamp_t access_time,
             __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags,
             __wasi_lookupflags_t lookup_flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readlinkat(os_file_handle handle, const char *path, char *buf,
              size_t bufsize, size_t *nread)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_linkat(os_file_handle from_handle, const char *from_path,
          os_file_handle to_handle, const char *to_path,
          __wasi_lookupflags_t lookup_flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_symlinkat(const char *old_path, os_file_handle handle, const char *new_path)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_mkdirat(os_file_handle handle, const char *path)
{
    struct zephyr_fs_desc *ptr = NULL;
    int index, rc;
    char abs_path[MAX_FILE_NAME + 1];

    if (handle == NULL) {
        return __WASI_EINVAL; // Or another appropriate error code
    }

    snprintf(abs_path, MAX_FILE_NAME, "%s/%s", prestat_dir, path);

    rc = fs_mkdir(abs_path);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    ptr = zephyr_fs_alloc_obj(true, abs_path, &index);
    if (!ptr) {
        fs_unlink(abs_path);
        return __WASI_EMFILE;
    }
    fs_dir_t_init(&ptr->dir);

    handle->fd = index;
    handle->is_sock = false;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_renameat(os_file_handle old_handle, const char *old_path,
            os_file_handle new_handle, const char *new_path)
{
    /* `old_handle` need to be the the fd of the file to rename.
     * `new_handle` will not be used.
     * paths need to be absolute, no relative path will be accepted.
     */
    struct zephyr_fs_desc *ptr = NULL;
    (void)new_handle;

    char abs_old_path[MAX_FILE_NAME + 1];
    char abs_new_path[MAX_FILE_NAME + 1];

    GET_FILE_SYSTEM_DESCRIPTOR(old_handle->fd, ptr);

    char *path = strdup(new_path);
    if (path == NULL) {
        return __WASI_ENOMEM;
    }

    snprintf(abs_old_path, MAX_FILE_NAME, "%s/%s", prestat_dir, old_path);
    snprintf(abs_new_path, MAX_FILE_NAME, "%s/%s", prestat_dir, new_path);

    int rc = fs_rename(abs_old_path, abs_new_path);
    if (rc < 0) {
        free(path);
        return convert_errno(-rc);
    }

    free(ptr->path);
    ptr->path = path;
    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_unlinkat(os_file_handle handle, const char *path, bool is_dir)
{
    /* `old_handle` need to be the the fd of the file to unlink.
     * `path` need to be absolute, relative path will not be resolved.
     */
    char abs_path[MAX_FILE_NAME + 1];
    struct zephyr_fs_desc *ptr = NULL;

    snprintf(abs_path, MAX_FILE_NAME, "%s/%s", prestat_dir, path);

    if (is_dir) {
        return __WASI_ENOTDIR;
    }

    int rc = fs_unlink(path);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    zephyr_fs_free_obj(ptr);

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_lseek(os_file_handle handle, __wasi_filedelta_t offset,
         __wasi_whence_t whence, __wasi_filesize_t *new_offset)
{
    struct zephyr_fs_desc *ptr = NULL;
    int zwhence;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);

    // They have the same value but this is more explicit
    switch (whence) {
        case __WASI_WHENCE_SET:
            zwhence = FS_SEEK_SET;
            break;
        case __WASI_WHENCE_CUR:
            zwhence = FS_SEEK_CUR;
            break;
        case __WASI_WHENCE_END:
            zwhence = FS_SEEK_END;
            break;
        default:
            return __WASI_EINVAL;
    }

    off_t rc = fs_seek(&ptr->file, (off_t)offset, zwhence);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    *new_offset = (__wasi_filesize_t)rc;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_fadvise(os_file_handle handle, __wasi_filesize_t offset,
           __wasi_filesize_t length, __wasi_advice_t advice)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_isatty(os_file_handle handle)
{
    return __WASI_ENOSYS;
}

os_file_handle
os_convert_stdin_handle(os_raw_file_handle raw_stdin)
{
    os_file_handle handle = BH_MALLOC(sizeof(struct zephyr_handle));
    if (handle == NULL) {
        return NULL;
    }

    /* We allocate a fake stdin reference */
    if (zephyr_fs_alloc_obj(false, "stdin", &handle->fd) == NULL) {
        BH_FREE(handle);
        return NULL;
    }

    handle->is_sock = false;
    return handle;
}

os_file_handle
os_convert_stdout_handle(os_raw_file_handle raw_stdout)
{
    os_file_handle handle = BH_MALLOC(sizeof(struct zephyr_handle));
    if (handle == NULL) {
        return NULL;
    }

    /* We allocate a fake stdin reference */
    if (zephyr_fs_alloc_obj(false, "stdout", &handle->fd) == NULL) {
        BH_FREE(handle);
        return NULL;
    }

    handle->is_sock = false;
    return handle;
}

os_file_handle
os_convert_stderr_handle(os_raw_file_handle raw_stderr)
{
    os_file_handle handle = BH_MALLOC(sizeof(struct zephyr_handle));
    if (handle == NULL) {
        return NULL;
    }

    /* We allocate a fake stdin reference */
    if (zephyr_fs_alloc_obj(false, "stderr", &handle->fd) == NULL) {
        BH_FREE(handle);
        return NULL;
    }

    handle->is_sock = false;
    return handle;
}

__wasi_errno_t
os_fdopendir(os_file_handle handle, os_dir_stream *dir_stream)
{
    /* Here we assume that either mdkdir or preopendir was called
     * before otherwise function will fail.
     */
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(handle->fd, ptr);
    if (!ptr->is_dir) {
        return __WASI_ENOTDIR;
    }

    int rc = fs_opendir(&ptr->dir, ptr->path);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    /* we store the fd in the `os_dir_stream` to use other function */
    *dir_stream = handle->fd;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_rewinddir(os_dir_stream dir_stream)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_seekdir(os_dir_stream dir_stream, __wasi_dircookie_t position)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readdir(os_dir_stream dir_stream, __wasi_dirent_t *entry,
           const char **d_name)
{
    struct fs_dirent fs_entry;
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(dir_stream, ptr);
    if (!ptr->is_dir) {
        return __WASI_ENOTDIR;
    }

    int rc = fs_readdir(&ptr->dir, &fs_entry);
    if (rc < 0) {
        return convert_errno(-rc);
    }

    entry->d_next = 0; // default value to start of the directory.
    entry->d_ino = 0;  // no inode in zephyr
    entry->d_namlen = strlen(fs_entry.name);
    entry->d_type = fs_entry.type == FS_DIR_ENTRY_DIR
                        ? __WASI_FILETYPE_DIRECTORY
                        : __WASI_FILETYPE_REGULAR_FILE;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_closedir(os_dir_stream dir_stream)
{
    struct zephyr_fs_desc *ptr = NULL;

    GET_FILE_SYSTEM_DESCRIPTOR(dir_stream, ptr);
    if (!ptr->is_dir) {
        return __WASI_ENOTDIR;
    }

    int rc = fs_closedir(&ptr->dir);
    zephyr_fs_free_obj(ptr); // free in any case.
    if (rc < 0) {
        return convert_errno(-rc);
    }

    return __WASI_ESUCCESS;
}

os_dir_stream
os_get_invalid_dir_stream()
{
    return NULL;
}

bool
os_is_dir_stream_valid(os_dir_stream *dir_stream)
{
    return false;
}

bool
os_is_handle_valid(os_file_handle *handle)
{
    if (handle == NULL || *handle == NULL) {
        return false;
    }
    return (*handle)->fd > -1;
}

char *
os_realpath(const char *path, char *resolved_path)
{
    /* In fact we could implement a path resolving method, because every paths
     * are at one point put into memory.
     * We could then maintain a 'tree' to represent the file system.
     *    --> The file system root is easily accessable with:
     *            * (fs_dir_t) dir.mp->mnt_point
     *            * (fs_file_t) file.mp->mnt_point
     * But we will just use absolute path for now.
     */
    if (!path) {
        // Log error
        return NULL;
    }
    return (const char *)path;
}

bool
os_compare_file_handle(os_file_handle handle1, os_file_handle handle2)
{
    return handle1->fd == handle2->fd && handle1->is_sock == handle2->is_sock;
}

bool
os_is_stdin_handle(os_file_handle fd)
{
    return fd == stdin;
}

bool
os_is_stdout_handle(os_file_handle fd)
{
    return fd == stdout;
}

bool
os_is_stderr_handle(os_file_handle fd)
{
    return fd == stderr;
}