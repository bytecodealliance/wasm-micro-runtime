#include "platform_api_extension.h"
#include "libc_errno.h"


__wasi_errno_t
os_fstat(os_file_handle handle, struct __wasi_filestat_t *buf){
    // Zephyr does not support file system without enbling it. 
    // Zephyr do expose `fd_stat` in the file system API.
    // A POSIX wrapper `fstat` exists for the function.
    // We choose to go without it for now, because socket should work without file system.
    // In this cas the handle could be:
    //     * stdin, stdout or stderr
    //     * a socket

    // typedef struct __wasi_filestat_t {
    // __wasi_device_t dev;         // Device ID of device containing the file.
    // __wasi_inode_t ino;          // File serial number.
    // __wasi_filetype_t filetype;  // File type.
    // __wasi_linkcount_t nlink;    // Number of hard links to the file.
    // __wasi_filesize_t size;      // For regular files, the file size in bytes. For symbolic links, the length in bytes of the pathname contained in the symbolic link.
    // __wasi_timestamp_t atim;     // Last data access timestamp.
    // __wasi_timestamp_t mtim;     // Last data modification timestamp.
    // __wasi_timestamp_t ctim;     // Last file status change timestamp.
    // } __wasi_filestat_t;
    int socktype;
    socklen_t socktypelen = sizeof(socktype);

    if(handle == 0){
        return __WASI_EBADF;
    }
    
    if (handle == 1 || handle == 2 || handle == 3){
        buf->st_filetype = __WASI_FILETYPE_UNKNOWN;
        return __WASI_ESUCCESS;
    }
   
    if(zsock_getsockopt(handle, SOL_SOCKET, SO_TYPE, &socktype, socktypelen) > 0){
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
        return __WASI_ESUCCESS;
    } else {
        return __WASI_EBADF;

    }
}

__wasi_errno_t
os_fstatat(os_file_handle handle, const char *path,
           struct __wasi_filestat_t *buf, __wasi_lookupflags_t lookup_flags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_fdflags(os_file_handle handle, __wasi_fdflags_t *flags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_set_fdflags(os_file_handle handle, __wasi_fdflags_t flags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fdatasync(os_file_handle handle){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fsync(os_file_handle handle){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_open_preopendir(const char *path, os_file_handle *out){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_openat(os_file_handle handle, const char *path, __wasi_oflags_t oflags,
          __wasi_fdflags_t fd_flags, __wasi_lookupflags_t lookup_flags,
          wasi_libc_file_access_mode access_mode, os_file_handle *out){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_access_mode(os_file_handle handle,
                        wasi_libc_file_access_mode *access_mode){
    // No acces mode in Zephyr
    // As we are handling socket and stdin, stdout, stderr
    // We can set the access mode to read write
    *access_mode = WASI_LIBC_ACCESS_MODE_READ_WRITE;
    
    return __WASI_ESUCCESS;
}

__wasi_errno_t
os_close(os_file_handle handle, bool is_stdio){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_preadv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
          __wasi_filesize_t offset, size_t *nread){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_pwritev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
           __wasi_filesize_t offset, size_t *nwritten){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
         size_t *nread){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_writev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
          size_t *nwritten){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fallocate(os_file_handle handle, __wasi_filesize_t offset,
             __wasi_filesize_t length){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_ftruncate(os_file_handle handle, __wasi_filesize_t size){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_futimens(os_file_handle handle, __wasi_timestamp_t access_time,
            __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_utimensat(os_file_handle handle, const char *path,
             __wasi_timestamp_t access_time,
             __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags,
             __wasi_lookupflags_t lookup_flags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readlinkat(os_file_handle handle, const char *path, char *buf,
              size_t bufsize, size_t *nread){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_linkat(os_file_handle from_handle, const char *from_path,
          os_file_handle to_handle, const char *to_path,
          __wasi_lookupflags_t lookup_flags){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_symlinkat(const char *old_path, os_file_handle handle, const char *new_path){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_mkdirat(os_file_handle handle, const char *path){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_renameat(os_file_handle old_handle, const char *old_path,
            os_file_handle new_handle, const char *new_path){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_unlinkat(os_file_handle handle, const char *path, bool is_dir){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_lseek(os_file_handle handle, __wasi_filedelta_t offset,
         __wasi_whence_t whence, __wasi_filesize_t *new_offset){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fadvise(os_file_handle handle, __wasi_filesize_t offset,
           __wasi_filesize_t length, __wasi_advice_t advice){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_isatty(os_file_handle handle){
    return __WASI_ENOSYS;
}

os_file_handle
os_convert_stdin_handle(os_raw_file_handle raw_stdin){
#ifndef STDIN_FILENO
#define STDIN_FILENO 1
#endif
    return raw_stdin > 0 ? raw_stdin : STDIN_FILENO;
}

os_file_handle
os_convert_stdout_handle(os_raw_file_handle raw_stdout){
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 2
#endif
    return raw_stdout > 0 ? raw_stdout : STDOUT_FILENO;
}

os_file_handle
os_convert_stderr_handle(os_raw_file_handle raw_stderr){
#ifndef STDERR_FILENO
#define STDERR_FILENO 3
#endif
    return raw_stderr > 0 ? raw_stderr : STDERR_FILENO;
}

__wasi_errno_t
os_fdopendir(os_file_handle handle, os_dir_stream *dir_stream){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_rewinddir(os_dir_stream dir_stream){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_seekdir(os_dir_stream dir_stream, __wasi_dircookie_t position){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readdir(os_dir_stream dir_stream, __wasi_dirent_t *entry,
           const char **d_name){
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_closedir(os_dir_stream dir_stream){
    return __WASI_ENOSYS;
}

os_dir_stream
os_get_invalid_dir_stream(){
    return NULL;
}

bool
os_is_dir_stream_valid(os_dir_stream *dir_stream){
    return false;
}

bool
os_is_handle_valid(os_file_handle *handle){
    if(handle != NULL)
        return *handle > -1;

    return false;
}

char *
os_realpath(const char *path, char *resolved_path){
    return NULL;
}