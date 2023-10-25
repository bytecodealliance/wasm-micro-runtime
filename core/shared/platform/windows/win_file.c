/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_extension.h"
#include "libc_errno.h"
#include "win_util.h"

#define CHECK_VALID_HANDLE_WITH_RETURN_VALUE(win_handle, ret)         \
    do {                                                              \
        if ((win_handle) == NULL                                      \
            || ((win_handle)->type == windows_handle_type_socket      \
                && (win_handle)->raw.socket == INVALID_SOCKET)        \
            || ((win_handle)->type == windows_handle_type_file        \
                && (win_handle)->raw.handle == INVALID_HANDLE_VALUE)) \
            return (ret);                                             \
                                                                      \
    } while (0)

#define CHECK_VALID_HANDLE(win_handle) \
    CHECK_VALID_HANDLE_WITH_RETURN_VALUE(win_handle, __WASI_EBADF)

#define CHECK_VALID_FILE_HANDLE(win_handle)                        \
    do {                                                           \
        if ((win_handle) == NULL)                                  \
            return __WASI_EBADF;                                   \
                                                                   \
        if ((win_handle)->type == windows_handle_type_socket)      \
            return __WASI_EINVAL;                                  \
                                                                   \
        if (((win_handle)->type == windows_handle_type_file        \
             && (win_handle)->raw.handle == INVALID_HANDLE_VALUE)) \
            return __WASI_EBADF;                                   \
                                                                   \
    } while (0)

#define CHECK_VALID_WIN_DIR_STREAM(win_dir_stream)         \
    do {                                                   \
        if ((win_dir_stream) == NULL)                      \
            return __WASI_EINVAL;                          \
        CHECK_VALID_FILE_HANDLE((win_dir_stream)->handle); \
    } while (0)

static __wasi_errno_t
convert_winsock_error_code(int error_code)
{
    switch (error_code) {
        case WSASYSNOTREADY:
        case WSAEWOULDBLOCK:
            return __WASI_EAGAIN;
        case WSAVERNOTSUPPORTED:
            return __WASI_ENOTSUP;
        case WSAEINPROGRESS:
            return __WASI_EINPROGRESS;
        case WSAEPROCLIM:
            return __WASI_EBUSY;
        case WSAEFAULT:
            return __WASI_EFAULT;
        case WSAENETDOWN:
            return __WASI_ENETDOWN;
        case WSAENOTSOCK:
            return __WASI_ENOTSOCK;
        case WSAEINTR:
            return __WASI_EINTR;
        case WSAEAFNOSUPPORT:
            return __WASI_EAFNOSUPPORT;
        case WSAEMFILE:
            return __WASI_ENFILE;
        case WSAEINVAL:
            return __WASI_EINVAL;
        case WSAENOBUFS:
            return __WASI_ENOBUFS;
        case WSAEPROTONOSUPPORT:
            return __WASI_EPROTONOSUPPORT;
        case WSAEPROTOTYPE:
            return __WASI_EPROTOTYPE;
        case WSAESOCKTNOSUPPORT:
            return __WASI_ENOTSUP;
        case WSAEINVALIDPROCTABLE:
        case WSAEINVALIDPROVIDER:
        case WSAEPROVIDERFAILEDINIT:
        case WSANOTINITIALISED:
        default:
            return __WASI_EINVAL;
    }
}

static __wasi_filetype_t
get_disk_filetype(DWORD attribute)
{
    if (attribute == INVALID_FILE_ATTRIBUTES)
        return __WASI_FILETYPE_UNKNOWN;
    if (attribute & FILE_ATTRIBUTE_REPARSE_POINT)
        return __WASI_FILETYPE_SYMBOLIC_LINK;
    if (attribute & FILE_ATTRIBUTE_DIRECTORY)
        return __WASI_FILETYPE_DIRECTORY;

    return __WASI_FILETYPE_REGULAR_FILE;
}

static __wasi_filetype_t
get_socket_filetype(SOCKET socket)
{
    char socket_type = 0;
    int size = sizeof(socket_type);

    if (getsockopt(socket, SOL_SOCKET, SO_TYPE, &socket_type, &size) == 0) {
        switch (socket_type) {
            case SOCK_STREAM:
                return __WASI_FILETYPE_SOCKET_STREAM;
            case SOCK_DGRAM:
                return __WASI_FILETYPE_SOCKET_DGRAM;
        }
    }
    return __WASI_FILETYPE_UNKNOWN;
}

static __wasi_errno_t
convert_windows_filetype(os_file_handle handle, DWORD filetype,
                         __wasi_filetype_t *out_filetype)
{
    __wasi_errno_t error = __WASI_ESUCCESS;

    switch (filetype) {
        case FILE_TYPE_DISK:
            FILE_ATTRIBUTE_TAG_INFO file_info;

            bool success = GetFileInformationByHandleEx(
                handle->raw.handle, FileAttributeTagInfo, &file_info,
                sizeof(file_info));

            if (!success
                || file_info.FileAttributes == INVALID_FILE_ATTRIBUTES) {
                error = convert_windows_error_code(GetLastError());
                break;
            }

            *out_filetype = get_disk_filetype(file_info.FileAttributes);
            break;
        case FILE_TYPE_CHAR:
            *out_filetype = __WASI_FILETYPE_CHARACTER_DEVICE;
            break;
        case FILE_TYPE_PIPE:
            if (handle->type == windows_handle_type_socket)
                *out_filetype = get_socket_filetype(handle->raw.socket);
            else
                *out_filetype = __WASI_FILETYPE_BLOCK_DEVICE;

            break;
        case FILE_TYPE_REMOTE:
        case FILE_TYPE_UNKNOWN:
        default:
            *out_filetype = __WASI_FILETYPE_UNKNOWN;
    }

    return error;
}

// Converts the input string to a wchar string.
static __wasi_errno_t
convert_to_wchar(const char *str, wchar_t *buf, size_t buf_size)
{
    int converted_chars =
        MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, (int)buf_size);

    if (converted_chars == 0)
        return convert_windows_error_code(GetLastError());

    return __WASI_ESUCCESS;
}

// Get the filepath for a handle. The size of the buffer should be specified in
// terms of wchar.
static __wasi_errno_t
get_handle_filepath(HANDLE handle, wchar_t *buf, DWORD buf_size)
{
    DWORD bufsize_in_chars = buf_size * (sizeof(wchar_t) / sizeof(char));
    DWORD size = GetFinalPathNameByHandleW(
        handle, buf, bufsize_in_chars, FILE_NAME_NORMALIZED | VOLUME_NAME_NONE);

    if (size > bufsize_in_chars)
        return __WASI_ENAMETOOLONG;

    if (size == 0)
        return convert_windows_error_code(GetLastError());

    return __WASI_ESUCCESS;
}

static void
init_dir_stream(os_dir_stream dir_stream, os_file_handle handle)
{
    dir_stream->cursor = 0;
    dir_stream->handle = handle;
    dir_stream->cookie = 0;
}

// Advances to the next directory entry and optionally reads into to the
// provided buffer if not NULL.
static __wasi_errno_t
read_next_dir_entry(os_dir_stream dir_stream, FILE_ID_BOTH_DIR_INFO **out_entry)
{
    FILE_INFO_BY_HANDLE_CLASS file_info_class;

    if (dir_stream->cookie == 0)
        file_info_class = FileIdBothDirectoryRestartInfo;
    else
        file_info_class = FileIdBothDirectoryInfo;

    if (dir_stream->cursor == 0
        && !GetFileInformationByHandleEx(dir_stream->handle->raw.handle,
                                         file_info_class, dir_stream->info_buf,
                                         sizeof(dir_stream->info_buf))) {
        if (out_entry != NULL)
            *out_entry = NULL;
        DWORD win_error = GetLastError();
        // We've reached the end of the directory - return success
        if (win_error == ERROR_NO_MORE_FILES) {
            dir_stream->cookie = 0;
            dir_stream->cursor = 0;
            return __WASI_ESUCCESS;
        }

        return convert_windows_error_code(win_error);
    }

    FILE_ID_BOTH_DIR_INFO *current_info =
        (FILE_ID_BOTH_DIR_INFO *)(dir_stream->info_buf + dir_stream->cursor);

    if (current_info->NextEntryOffset == 0)
        dir_stream->cursor = 0;
    else
        dir_stream->cursor += current_info->NextEntryOffset;

    ++dir_stream->cookie;

    if (out_entry != NULL)
        *out_entry = current_info;
    else
        return __WASI_ESUCCESS;

    // Convert and copy over the wchar filename into the entry_name buf
    int ret = WideCharToMultiByte(
        CP_UTF8, 0, current_info->FileName,
        current_info->FileNameLength / (sizeof(wchar_t) / sizeof(char)),
        dir_stream->current_entry_name, sizeof(dir_stream->current_entry_name),
        NULL, NULL);

    if (ret == 0)
        return convert_windows_error_code(GetLastError());

    return __WASI_ESUCCESS;
}

static HANDLE
create_handle(wchar_t *path, bool is_dir, bool follow_symlink, bool readonly)
{
    CREATEFILE2_EXTENDED_PARAMETERS create_params;

    create_params.dwSize = sizeof(create_params);
    create_params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    create_params.dwSecurityQosFlags = 0;
    create_params.dwFileFlags = 0;
    create_params.lpSecurityAttributes = NULL;
    create_params.hTemplateFile = NULL;

    if (is_dir) {
        create_params.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        create_params.dwFileFlags |= FILE_FLAG_BACKUP_SEMANTICS;
    }

    if (!follow_symlink)
        create_params.dwFileFlags |= FILE_FLAG_OPEN_REPARSE_POINT;

    DWORD desired_access = GENERIC_READ;

    if (!readonly) {
        desired_access |= GENERIC_WRITE;
        create_params.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    }

    return CreateFile2(path, desired_access,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       OPEN_EXISTING, &create_params);
}

#if WINAPI_PARTITION_DESKTOP
// Modifies the given path in place and replaces it with the filename component
// (including the extension) of the path.
static __wasi_errno_t
extract_filename_from_path(wchar_t *path, size_t buf_size)
{
    wchar_t extension[256];
    wchar_t filename[256];
    __wasi_errno_t error = __WASI_ESUCCESS;

    // Get the filename from the fullpath.
    errno_t ret =
        _wsplitpath_s(path, NULL, 0, NULL, 0, filename, 256, extension, 256);
    if (ret != 0) {
        error = convert_errno(ret);
        return error;
    }

    ret = wcscat_s(filename, 256, extension);

    if (ret != 0) {
        error = convert_errno(ret);
        return error;
    }

    ret = wcscpy_s(path, buf_size, filename);

    if (ret != 0)
        error = convert_errno(ret);

    return error;
}

static __wasi_errno_t
get_handle_to_parent_directory(HANDLE handle, HANDLE *out_dir_handle)
{
    wchar_t path[PATH_MAX];
    __wasi_errno_t error = get_handle_filepath(handle, path, PATH_MAX);

    if (error != __WASI_ESUCCESS)
        return error;

    wchar_t parent_dir_path[PATH_MAX];
    errno_t ret = wcscpy_s(parent_dir_path, PATH_MAX, path);

    if (ret != 0) {
        error = convert_errno(ret);
        return error;
    }

    ret = wcscat_s(parent_dir_path, PATH_MAX, L"/..");

    if (ret != 0) {
        error = convert_errno(ret);
        return error;
    }

    HANDLE dir_handle = create_handle(parent_dir_path, true, true, true);

    if (dir_handle == INVALID_HANDLE_VALUE) {
        error = convert_windows_error_code(GetLastError());
        return error;
    }

    *out_dir_handle = dir_handle;
    return error;
}

// The easiest way to get all the necessary file information for files is to
// open a handle to the parent directory and iterate through the entries via
// FileIdBothDirectoryInfo. Other file information classes are only
// available on desktop.
static __wasi_errno_t
get_disk_file_information(HANDLE handle, __wasi_filestat_t *buf)
{
    __wasi_errno_t error = __WASI_ESUCCESS;
    HANDLE raw_dir_handle = INVALID_HANDLE_VALUE;

    wchar_t path[PATH_MAX] = L".";

    if (buf->st_filetype != __WASI_FILETYPE_DIRECTORY) {
        error = get_handle_filepath(handle, path, PATH_MAX);

        if (error != __WASI_ESUCCESS)
            goto fail;

        error = get_handle_to_parent_directory(handle, &raw_dir_handle);

        if (error != __WASI_ESUCCESS)
            goto fail;

        error = extract_filename_from_path(path, PATH_MAX);

        if (error != __WASI_ESUCCESS)
            goto fail;
    }
    else {
        raw_dir_handle = handle;
    }

    windows_handle dir_handle = { .access_mode = windows_access_mode_read,
                                  .raw = { .handle = raw_dir_handle },
                                  .type = windows_handle_type_file };
    windows_dir_stream dir_stream;
    init_dir_stream(&dir_stream, &dir_handle);

    do {
        FILE_ID_BOTH_DIR_INFO *file_id_both_dir_info = NULL;
        __wasi_errno_t error =
            read_next_dir_entry(&dir_stream, &file_id_both_dir_info);

        if (error != __WASI_ESUCCESS || file_id_both_dir_info == NULL)
            goto fail;

        const DWORD filename_length = file_id_both_dir_info->FileNameLength
                                      / (sizeof(wchar_t) / sizeof(char));

        if (wcsncmp(file_id_both_dir_info->FileName, path, filename_length)
            == 0) {
            buf->st_ino =
                (__wasi_inode_t)(file_id_both_dir_info->FileId.QuadPart);
            buf->st_atim = convert_filetime_to_wasi_timestamp(
                (LPFILETIME)&file_id_both_dir_info->LastAccessTime.QuadPart);
            buf->st_mtim = convert_filetime_to_wasi_timestamp(
                (LPFILETIME)&file_id_both_dir_info->LastWriteTime.QuadPart);
            buf->st_ctim = convert_filetime_to_wasi_timestamp(
                (LPFILETIME)&file_id_both_dir_info->ChangeTime.QuadPart);
            buf->st_size =
                (__wasi_filesize_t)(file_id_both_dir_info->EndOfFile.QuadPart);

            break;
        }
    } while (dir_stream.cookie != 0);

    FILE_STANDARD_INFO file_standard_info;

    bool success = GetFileInformationByHandleEx(handle, FileStandardInfo,
                                                &file_standard_info,
                                                sizeof(file_standard_info));

    if (!success) {
        error = convert_windows_error_code(GetLastError());
        goto fail;
    }

    buf->st_nlink = (__wasi_linkcount_t)file_standard_info.NumberOfLinks;
fail:
    if (buf->st_filetype != __WASI_FILETYPE_DIRECTORY
        && raw_dir_handle != INVALID_HANDLE_VALUE)
        CloseHandle(raw_dir_handle);

    return error;
}

#else

static __wasi_errno_t
get_disk_file_information(HANDLE handle, __wasi_filestat_t *buf)
{
    __wasi_errno_t error = __WASI_ESUCCESS;
    FILE_BASIC_INFO file_basic_info;

    int ret = GetFileInformationByHandleEx(
        handle, FileBasicInfo, &file_basic_info, sizeof(file_basic_info));

    if (ret == 0) {
        error = convert_windows_error_code(GetLastError());
        return error;
    }

    buf->st_atim = convert_filetime_to_wasi_timestamp(
        (LPFILETIME)&file_basic_info.LastAccessTime.QuadPart);
    buf->st_mtim = convert_filetime_to_wasi_timestamp(
        (LPFILETIME)&file_basic_info.LastWriteTime.QuadPart);
    buf->st_ctim = convert_filetime_to_wasi_timestamp(
        (LPFILETIME)&file_basic_info.ChangeTime.QuadPart);

    BY_HANDLE_FILE_INFORMATION file_info;
    ret = GetFileInformationByHandle(handle, &file_info);

    if (ret == 0) {
        error = convert_windows_error_code(GetLastError());
        return error;
    }

    ULARGE_INTEGER file_size = { .LowPart = file_info.nFileSizeLow,
                                 .HighPart = file_info.nFileSizeHigh };
    buf->st_size = (__wasi_filesize_t)(file_size.QuadPart);

    ULARGE_INTEGER file_id = { .LowPart = file_info.nFileIndexLow,
                               .HighPart = file_info.nFileIndexHigh };
    buf->st_ino = (__wasi_inode_t)(file_id.QuadPart);

    buf->st_dev = (__wasi_device_t)file_info.dwVolumeSerialNumber;
    buf->st_nlink = (__wasi_linkcount_t)file_info.nNumberOfLinks;

    return error;
}

#endif /* end of !WINAPI_PARTITION_DESKTOP */

static __wasi_errno_t
get_file_information(os_file_handle handle, __wasi_filestat_t *buf)
{
    __wasi_errno_t error = __WASI_ESUCCESS;

    DWORD windows_filetype = GetFileType(handle->raw.handle);
    error =
        convert_windows_filetype(handle, windows_filetype, &buf->st_filetype);

    if (error != __WASI_ESUCCESS)
        return error;

    buf->st_dev = 0;

    if (windows_filetype != FILE_TYPE_DISK) {
        buf->st_atim = 0;
        buf->st_ctim = 0;
        buf->st_mtim = 0;
        buf->st_nlink = 0;
        buf->st_size = 0;
        buf->st_ino = 0;

        return error;
    }

    return get_disk_file_information(handle->raw.handle, buf);
}

__wasi_errno_t
os_fstat(os_file_handle handle, struct __wasi_filestat_t *buf)
{
    CHECK_VALID_HANDLE(handle);

    return get_file_information(handle, buf);
}

__wasi_errno_t
os_fstatat(os_file_handle handle, const char *path,
           struct __wasi_filestat_t *buf, __wasi_lookupflags_t lookup_flags)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_fdflags(os_file_handle handle, __wasi_fdflags_t *flags)
{
    CHECK_VALID_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_set_fdflags(os_file_handle handle, __wasi_fdflags_t flags)
{
    CHECK_VALID_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_access_mode(os_file_handle handle,
                        wasi_libc_file_access_mode *access_mode)
{
    CHECK_VALID_HANDLE(handle);

    if ((handle->access_mode & windows_access_mode_read) != 0
        && (handle->access_mode & windows_access_mode_write) != 0)
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;
    else if ((handle->access_mode & windows_access_mode_write) != 0)
        *access_mode = WASI_LIBC_ACCESS_MODE_WRITE_ONLY;
    else
        *access_mode = WASI_LIBC_ACCESS_MODE_READ_ONLY;

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_fdatasync(os_file_handle handle)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fsync(os_file_handle handle)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_open_preopendir(const char *path, os_file_handle *out)
{
    *out = NULL;

    wchar_t wpath[PATH_MAX];
    __wasi_errno_t error = convert_to_wchar(path, wpath, PATH_MAX);

    if (error != __WASI_ESUCCESS)
        return error;

    HANDLE dir_handle = create_handle(wpath, true, true, true);

    if (dir_handle == INVALID_HANDLE_VALUE)
        return convert_windows_error_code(GetLastError());

    *out = BH_MALLOC(sizeof(windows_handle));

    if (*out == NULL) {
        CloseHandle(dir_handle);
        return __WASI_ENOMEM;
    }

    (*out)->type = windows_handle_type_file;
    (*out)->raw.handle = dir_handle;
    (*out)->access_mode = windows_access_mode_read;

    return error;
}

__wasi_errno_t
os_openat(os_file_handle handle, const char *path, __wasi_oflags_t oflags,
          __wasi_fdflags_t fs_flags, __wasi_lookupflags_t lookup_flags,
          wasi_libc_file_access_mode access_mode, os_file_handle *out)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_close(os_file_handle handle, bool is_stdio)
{
    CHECK_VALID_HANDLE(handle);

    // We don't own the underlying raw handle so just free the handle and return
    // success.
    if (is_stdio) {
        BH_FREE(handle);
        return __WASI_ESUCCESS;
    }

    switch (handle->type) {
        case windows_handle_type_file:
            bool success = CloseHandle(handle->raw.handle);

            if (!success)
                return convert_windows_error_code(GetLastError());

            break;
        case windows_handle_type_socket:
            int ret = closesocket(handle->raw.socket);

            if (ret != 0)
                return convert_winsock_error_code(WSAGetLastError());

            break;
        default:
            assert(false && "unreachable");
    }

    BH_FREE(handle);

    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_preadv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
          __wasi_filesize_t offset, size_t *nread)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
         size_t *nread)
{
    CHECK_VALID_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_pwritev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
           __wasi_filesize_t offset, size_t *nwritten)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_writev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
          size_t *nwritten)
{
    CHECK_VALID_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fallocate(os_file_handle handle, __wasi_filesize_t offset,
             __wasi_filesize_t length)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_ftruncate(os_file_handle handle, __wasi_filesize_t size)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_futimens(os_file_handle handle, __wasi_timestamp_t access_time,
            __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_utimensat(os_file_handle handle, const char *path,
             __wasi_timestamp_t access_time,
             __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags,
             __wasi_lookupflags_t lookup_flags)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readlinkat(os_file_handle handle, const char *path, char *buf,
              size_t bufsize, size_t *nread)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_linkat(os_file_handle from_handle, const char *from_path,
          os_file_handle to_handle, const char *to_path,
          __wasi_lookupflags_t lookup_flags)
{
    CHECK_VALID_FILE_HANDLE(from_handle);
    CHECK_VALID_FILE_HANDLE(to_handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_symlinkat(const char *old_path, os_file_handle handle, const char *new_path)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_mkdirat(os_file_handle handle, const char *path)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_renameat(os_file_handle old_handle, const char *old_path,
            os_file_handle new_handle, const char *new_path)
{
    CHECK_VALID_FILE_HANDLE(old_handle);
    CHECK_VALID_FILE_HANDLE(new_handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_unlinkat(os_file_handle handle, const char *path, bool is_dir)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_lseek(os_file_handle handle, __wasi_filedelta_t offset,
         __wasi_whence_t whence, __wasi_filesize_t *new_offset)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fadvise(os_file_handle handle, __wasi_filesize_t offset,
           __wasi_filesize_t length, __wasi_advice_t advice)
{
    CHECK_VALID_FILE_HANDLE(handle);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_isatty(os_file_handle handle)
{
    CHECK_VALID_HANDLE(handle);

    DWORD console_mode;
    return GetConsoleMode(handle->raw.handle, &console_mode) ? __WASI_ESUCCESS
                                                             : __WASI_ENOTTY;
}

static os_file_handle
create_stdio_handle(HANDLE raw_stdio_handle, DWORD stdio)
{
    os_file_handle stdio_handle = BH_MALLOC(sizeof(windows_handle));

    if (stdio_handle == NULL)
        return NULL;

    stdio_handle->type = windows_handle_type_file;
    stdio_handle->access_mode =
        windows_access_mode_read | windows_access_mode_write;

    if (raw_stdio_handle == INVALID_HANDLE_VALUE)
        raw_stdio_handle = GetStdHandle(stdio);

    stdio_handle->raw.handle = raw_stdio_handle;

    return stdio_handle;
}

os_file_handle
os_convert_stdin_handle(os_raw_file_handle raw_stdin)
{
    return create_stdio_handle(raw_stdin, STD_INPUT_HANDLE);
}

os_file_handle
os_convert_stdout_handle(os_raw_file_handle raw_stdout)
{
    return create_stdio_handle(raw_stdout, STD_OUTPUT_HANDLE);
}

os_file_handle
os_convert_stderr_handle(os_raw_file_handle raw_stderr)
{
    return create_stdio_handle(raw_stderr, STD_ERROR_HANDLE);
}

__wasi_errno_t
os_fdopendir(os_file_handle handle, os_dir_stream *dir_stream)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_rewinddir(os_dir_stream dir_stream)
{
    CHECK_VALID_WIN_DIR_STREAM(dir_stream);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_seekdir(os_dir_stream dir_stream, __wasi_dircookie_t position)
{
    CHECK_VALID_WIN_DIR_STREAM(dir_stream);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readdir(os_dir_stream dir_stream, __wasi_dirent_t *entry,
           const char **d_name)
{
    CHECK_VALID_WIN_DIR_STREAM(dir_stream);

    return __WASI_ENOSYS;
}

__wasi_errno_t
os_closedir(os_dir_stream dir_stream)
{
    CHECK_VALID_WIN_DIR_STREAM(dir_stream);

    return __WASI_ENOSYS;
}

os_dir_stream
os_get_invalid_dir_stream()
{
    return NULL;
}

bool
os_is_dir_stream_valid(os_dir_stream *dir_stream)
{
    assert(dir_stream != NULL);

    if (((*dir_stream) == NULL) || ((*dir_stream)->handle == NULL)
        || ((*dir_stream)->handle->type != windows_handle_type_file)
        || ((*dir_stream)->handle->raw.handle == INVALID_HANDLE_VALUE))
        return false;

    return true;
}

os_file_handle
os_get_invalid_handle()
{
    return NULL;
}

bool
os_is_handle_valid(os_file_handle *handle)
{
    assert(handle != NULL);

    CHECK_VALID_HANDLE_WITH_RETURN_VALUE(*handle, false);

    return true;
}

char *
os_realpath(const char *path, char *resolved_path)
{
    resolved_path = _fullpath(resolved_path, path, PATH_MAX);

    // Check the file/directory actually exists
    DWORD attributes = GetFileAttributesA(resolved_path);

    if (attributes == INVALID_FILE_ATTRIBUTES)
        return NULL;

    return resolved_path;
}