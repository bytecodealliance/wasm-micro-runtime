/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * This file declares the WASI interface as well as optional filesystem
 * functions which platforms would need to implement to support WASI libc
 * filesystem operations. The definitions of types, macros and structures in
 * this file should be consistent with those in wasi-libc:
 * https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/headers/public/wasi/api.h
 */

#ifndef _PLATFORM_WASI_H
#define _PLATFORM_WASI_H

#include "platform_common.h"
#include "platform_internal.h"

/* clang-format off */

#ifdef __cplusplus
#ifndef _Static_assert
#define _Static_assert static_assert
#endif /* _Static_assert */

#ifndef _Alignof
#define _Alignof alignof
#endif /* _Alignof */

extern "C" {
#endif

/* There is no need to check the WASI layout if we're using uvwasi or libc-wasi
 * is not enabled at all. */
#if WASM_ENABLE_UVWASI != 0 || WASM_ENABLE_LIBC_WASI == 0
#define assert_wasi_layout(expr, message) /* nothing */
#else
#define assert_wasi_layout(expr, message) _Static_assert(expr, message)
#endif

assert_wasi_layout(_Alignof(int8_t) == 1, "non-wasi data layout");
assert_wasi_layout(_Alignof(uint8_t) == 1, "non-wasi data layout");
assert_wasi_layout(_Alignof(int16_t) == 2, "non-wasi data layout");
assert_wasi_layout(_Alignof(uint16_t) == 2, "non-wasi data layout");
assert_wasi_layout(_Alignof(int32_t) == 4, "non-wasi data layout");
assert_wasi_layout(_Alignof(uint32_t) == 4, "non-wasi data layout");
#if 0
assert_wasi_layout(_Alignof(int64_t) == 8, "non-wasi data layout");
assert_wasi_layout(_Alignof(uint64_t) == 8, "non-wasi data layout");
#endif

typedef uint32_t __wasi_size_t;
assert_wasi_layout(_Alignof(__wasi_size_t) == 4, "non-wasi data layout");

typedef uint8_t __wasi_advice_t;
#define __WASI_ADVICE_NORMAL     (0)
#define __WASI_ADVICE_SEQUENTIAL (1)
#define __WASI_ADVICE_RANDOM     (2)
#define __WASI_ADVICE_WILLNEED   (3)
#define __WASI_ADVICE_DONTNEED   (4)
#define __WASI_ADVICE_NOREUSE    (5)

typedef uint32_t __wasi_clockid_t;
#define __WASI_CLOCK_REALTIME           (0)
#define __WASI_CLOCK_MONOTONIC          (1)
#define __WASI_CLOCK_PROCESS_CPUTIME_ID (2)
#define __WASI_CLOCK_THREAD_CPUTIME_ID  (3)

typedef uint64_t __wasi_device_t;

typedef uint64_t __wasi_dircookie_t;
#define __WASI_DIRCOOKIE_START (0)

typedef uint32_t __wasi_dirnamlen_t;

typedef uint16_t __wasi_errno_t;
#define __WASI_ESUCCESS        (0)
#define __WASI_E2BIG           (1)
#define __WASI_EACCES          (2)
#define __WASI_EADDRINUSE      (3)
#define __WASI_EADDRNOTAVAIL   (4)
#define __WASI_EAFNOSUPPORT    (5)
#define __WASI_EAGAIN          (6)
#define __WASI_EALREADY        (7)
#define __WASI_EBADF           (8)
#define __WASI_EBADMSG         (9)
#define __WASI_EBUSY           (10)
#define __WASI_ECANCELED       (11)
#define __WASI_ECHILD          (12)
#define __WASI_ECONNABORTED    (13)
#define __WASI_ECONNREFUSED    (14)
#define __WASI_ECONNRESET      (15)
#define __WASI_EDEADLK         (16)
#define __WASI_EDESTADDRREQ    (17)
#define __WASI_EDOM            (18)
#define __WASI_EDQUOT          (19)
#define __WASI_EEXIST          (20)
#define __WASI_EFAULT          (21)
#define __WASI_EFBIG           (22)
#define __WASI_EHOSTUNREACH    (23)
#define __WASI_EIDRM           (24)
#define __WASI_EILSEQ          (25)
#define __WASI_EINPROGRESS     (26)
#define __WASI_EINTR           (27)
#define __WASI_EINVAL          (28)
#define __WASI_EIO             (29)
#define __WASI_EISCONN         (30)
#define __WASI_EISDIR          (31)
#define __WASI_ELOOP           (32)
#define __WASI_EMFILE          (33)
#define __WASI_EMLINK          (34)
#define __WASI_EMSGSIZE        (35)
#define __WASI_EMULTIHOP       (36)
#define __WASI_ENAMETOOLONG    (37)
#define __WASI_ENETDOWN        (38)
#define __WASI_ENETRESET       (39)
#define __WASI_ENETUNREACH     (40)
#define __WASI_ENFILE          (41)
#define __WASI_ENOBUFS         (42)
#define __WASI_ENODEV          (43)
#define __WASI_ENOENT          (44)
#define __WASI_ENOEXEC         (45)
#define __WASI_ENOLCK          (46)
#define __WASI_ENOLINK         (47)
#define __WASI_ENOMEM          (48)
#define __WASI_ENOMSG          (49)
#define __WASI_ENOPROTOOPT     (50)
#define __WASI_ENOSPC          (51)
#define __WASI_ENOSYS          (52)
#define __WASI_ENOTCONN        (53)
#define __WASI_ENOTDIR         (54)
#define __WASI_ENOTEMPTY       (55)
#define __WASI_ENOTRECOVERABLE (56)
#define __WASI_ENOTSOCK        (57)
#define __WASI_ENOTSUP         (58)
#define __WASI_ENOTTY          (59)
#define __WASI_ENXIO           (60)
#define __WASI_EOVERFLOW       (61)
#define __WASI_EOWNERDEAD      (62)
#define __WASI_EPERM           (63)
#define __WASI_EPIPE           (64)
#define __WASI_EPROTO          (65)
#define __WASI_EPROTONOSUPPORT (66)
#define __WASI_EPROTOTYPE      (67)
#define __WASI_ERANGE          (68)
#define __WASI_EROFS           (69)
#define __WASI_ESPIPE          (70)
#define __WASI_ESRCH           (71)
#define __WASI_ESTALE          (72)
#define __WASI_ETIMEDOUT       (73)
#define __WASI_ETXTBSY         (74)
#define __WASI_EXDEV           (75)
#define __WASI_ENOTCAPABLE     (76)

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#define WARN_UNUSED _Check_return_
#elif defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#define WARN_UNUSED __attribute__((__warn_unused_result__))
#endif

#define ALIGNED_TYPE(t,x) typedef t ALIGNED_(x)

typedef uint16_t __wasi_eventrwflags_t;
#define __WASI_EVENT_FD_READWRITE_HANGUP (0x0001)

typedef uint8_t __wasi_eventtype_t;
#define __WASI_EVENTTYPE_CLOCK          (0)
#define __WASI_EVENTTYPE_FD_READ        (1)
#define __WASI_EVENTTYPE_FD_WRITE       (2)

typedef uint32_t __wasi_exitcode_t;

typedef uint32_t __wasi_fd_t;

typedef uint16_t __wasi_fdflags_t;
#define __WASI_FDFLAG_APPEND   (0x0001)
#define __WASI_FDFLAG_DSYNC    (0x0002)
#define __WASI_FDFLAG_NONBLOCK (0x0004)
#define __WASI_FDFLAG_RSYNC    (0x0008)
#define __WASI_FDFLAG_SYNC     (0x0010)

typedef int64_t __wasi_filedelta_t;

typedef uint64_t __wasi_filesize_t;

typedef uint8_t __wasi_filetype_t;
#define __WASI_FILETYPE_UNKNOWN          (0)
#define __WASI_FILETYPE_BLOCK_DEVICE     (1)
#define __WASI_FILETYPE_CHARACTER_DEVICE (2)
#define __WASI_FILETYPE_DIRECTORY        (3)
#define __WASI_FILETYPE_REGULAR_FILE     (4)
#define __WASI_FILETYPE_SOCKET_DGRAM     (5)
#define __WASI_FILETYPE_SOCKET_STREAM    (6)
#define __WASI_FILETYPE_SYMBOLIC_LINK    (7)

typedef uint16_t __wasi_fstflags_t;
#define __WASI_FILESTAT_SET_ATIM     (0x0001)
#define __WASI_FILESTAT_SET_ATIM_NOW (0x0002)
#define __WASI_FILESTAT_SET_MTIM     (0x0004)
#define __WASI_FILESTAT_SET_MTIM_NOW (0x0008)

typedef uint64_t __wasi_inode_t;

ALIGNED_TYPE(uint64_t, 8) __wasi_linkcount_t;

typedef uint32_t __wasi_lookupflags_t;
#define __WASI_LOOKUP_SYMLINK_FOLLOW (0x00000001)

typedef uint16_t __wasi_oflags_t;
#define __WASI_O_CREAT     (0x0001)
#define __WASI_O_DIRECTORY (0x0002)
#define __WASI_O_EXCL      (0x0004)
#define __WASI_O_TRUNC     (0x0008)

typedef uint16_t __wasi_riflags_t;
#define __WASI_SOCK_RECV_PEEK    (0x0001)
#define __WASI_SOCK_RECV_WAITALL (0x0002)

typedef uint64_t __wasi_rights_t;

/**
 * Observe that WASI defines rights in the plural form
 * TODO: refactor to use RIGHTS instead of RIGHT
 */
#define __WASI_RIGHT_FD_DATASYNC ((__wasi_rights_t)(UINT64_C(1) << 0))
#define __WASI_RIGHT_FD_READ ((__wasi_rights_t)(UINT64_C(1) << 1))
#define __WASI_RIGHT_FD_SEEK ((__wasi_rights_t)(UINT64_C(1) << 2))
#define __WASI_RIGHT_FD_FDSTAT_SET_FLAGS ((__wasi_rights_t)(UINT64_C(1) << 3))
#define __WASI_RIGHT_FD_SYNC ((__wasi_rights_t)(UINT64_C(1) << 4))
#define __WASI_RIGHT_FD_TELL ((__wasi_rights_t)(UINT64_C(1) << 5))
#define __WASI_RIGHT_FD_WRITE ((__wasi_rights_t)(UINT64_C(1) << 6))
#define __WASI_RIGHT_FD_ADVISE ((__wasi_rights_t)(UINT64_C(1) << 7))
#define __WASI_RIGHT_FD_ALLOCATE ((__wasi_rights_t)(UINT64_C(1) << 8))
#define __WASI_RIGHT_PATH_CREATE_DIRECTORY ((__wasi_rights_t)(UINT64_C(1) << 9))
#define __WASI_RIGHT_PATH_CREATE_FILE ((__wasi_rights_t)(UINT64_C(1) << 10))
#define __WASI_RIGHT_PATH_LINK_SOURCE ((__wasi_rights_t)(UINT64_C(1) << 11))
#define __WASI_RIGHT_PATH_LINK_TARGET ((__wasi_rights_t)(UINT64_C(1) << 12))
#define __WASI_RIGHT_PATH_OPEN ((__wasi_rights_t)(UINT64_C(1) << 13))
#define __WASI_RIGHT_FD_READDIR ((__wasi_rights_t)(UINT64_C(1) << 14))
#define __WASI_RIGHT_PATH_READLINK ((__wasi_rights_t)(UINT64_C(1) << 15))
#define __WASI_RIGHT_PATH_RENAME_SOURCE ((__wasi_rights_t)(UINT64_C(1) << 16))
#define __WASI_RIGHT_PATH_RENAME_TARGET ((__wasi_rights_t)(UINT64_C(1) << 17))
#define __WASI_RIGHT_PATH_FILESTAT_GET ((__wasi_rights_t)(UINT64_C(1) << 18))
#define __WASI_RIGHT_PATH_FILESTAT_SET_SIZE ((__wasi_rights_t)(UINT64_C(1) << 19))
#define __WASI_RIGHT_PATH_FILESTAT_SET_TIMES ((__wasi_rights_t)(UINT64_C(1) << 20))
#define __WASI_RIGHT_FD_FILESTAT_GET ((__wasi_rights_t)(UINT64_C(1) << 21))
#define __WASI_RIGHT_FD_FILESTAT_SET_SIZE ((__wasi_rights_t)(UINT64_C(1) << 22))
#define __WASI_RIGHT_FD_FILESTAT_SET_TIMES ((__wasi_rights_t)(UINT64_C(1) << 23))
#define __WASI_RIGHT_PATH_SYMLINK ((__wasi_rights_t)(UINT64_C(1) << 24))
#define __WASI_RIGHT_PATH_REMOVE_DIRECTORY ((__wasi_rights_t)(UINT64_C(1) << 25))
#define __WASI_RIGHT_PATH_UNLINK_FILE ((__wasi_rights_t)(UINT64_C(1) << 26))
#define __WASI_RIGHT_POLL_FD_READWRITE ((__wasi_rights_t)(UINT64_C(1) << 27))
#define __WASI_RIGHT_SOCK_CONNECT ((__wasi_rights_t)(UINT64_C(1) << 28))
#define __WASI_RIGHT_SOCK_LISTEN ((__wasi_rights_t)(UINT64_C(1) << 29))
#define __WASI_RIGHT_SOCK_BIND ((__wasi_rights_t)(UINT64_C(1) << 30))
#define __WASI_RIGHT_SOCK_ACCEPT ((__wasi_rights_t)(UINT64_C(1) << 31))
#define __WASI_RIGHT_SOCK_RECV ((__wasi_rights_t)(UINT64_C(1) << 32))
#define __WASI_RIGHT_SOCK_SEND ((__wasi_rights_t)(UINT64_C(1) << 33))
#define __WASI_RIGHT_SOCK_ADDR_LOCAL ((__wasi_rights_t)(UINT64_C(1) << 34))
#define __WASI_RIGHT_SOCK_ADDR_REMOTE ((__wasi_rights_t)(UINT64_C(1) << 35))
#define __WASI_RIGHT_SOCK_RECV_FROM ((__wasi_rights_t)(UINT64_C(1) << 36))
#define __WASI_RIGHT_SOCK_SEND_TO ((__wasi_rights_t)(UINT64_C(1) << 37))

typedef uint16_t __wasi_roflags_t;
#define __WASI_SOCK_RECV_DATA_TRUNCATED (0x0001)

typedef uint8_t __wasi_sdflags_t;
#define __WASI_SHUT_RD (0x01)
#define __WASI_SHUT_WR (0x02)

typedef uint16_t __wasi_siflags_t;

typedef uint8_t __wasi_signal_t;

typedef uint16_t __wasi_subclockflags_t;
#define __WASI_SUBSCRIPTION_CLOCK_ABSTIME (0x0001)

typedef uint64_t __wasi_timestamp_t;

typedef uint64_t __wasi_userdata_t;

typedef uint8_t __wasi_whence_t;
#define __WASI_WHENCE_SET (0)
#define __WASI_WHENCE_CUR (1)
#define __WASI_WHENCE_END (2)

typedef uint8_t __wasi_preopentype_t;
#define __WASI_PREOPENTYPE_DIR              (0)

struct fd_table;
struct fd_prestats;
struct argv_environ_values;
struct addr_pool;

typedef struct ALIGNED_(8) __wasi_dirent_t {
    __wasi_dircookie_t d_next;
    __wasi_inode_t d_ino;
    __wasi_dirnamlen_t d_namlen;
    __wasi_filetype_t d_type;
} __wasi_dirent_t;
assert_wasi_layout(offsetof(__wasi_dirent_t, d_next) == 0, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_dirent_t, d_ino) == 8, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_dirent_t, d_namlen) == 16, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_dirent_t, d_type) == 20, "non-wasi data layout");
assert_wasi_layout(sizeof(__wasi_dirent_t) == 24, "non-wasi data layout");
assert_wasi_layout(_Alignof(__wasi_dirent_t) == 8, "non-wasi data layout");

typedef struct ALIGNED_(8) __wasi_event_t {
    __wasi_userdata_t userdata;
    __wasi_errno_t error;
    __wasi_eventtype_t type;
    uint8_t __paddings[5];
    union __wasi_event_u {
        struct __wasi_event_u_fd_readwrite_t {
            __wasi_filesize_t nbytes;
            __wasi_eventrwflags_t flags;
            uint8_t __paddings[6];
        } fd_readwrite;
    } u;
} __wasi_event_t;
assert_wasi_layout(offsetof(__wasi_event_t, userdata) == 0, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_event_t, error) == 8, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_event_t, type) == 10, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_event_t, u.fd_readwrite.nbytes) == 16, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_event_t, u.fd_readwrite.flags) == 24, "non-wasi data layout");
assert_wasi_layout(sizeof(__wasi_event_t) == 32, "non-wasi data layout");
assert_wasi_layout(_Alignof(__wasi_event_t) == 8, "non-wasi data layout");

typedef struct __wasi_prestat_t {
    __wasi_preopentype_t pr_type;
    union __wasi_prestat_u {
        struct __wasi_prestat_u_dir_t {
            size_t pr_name_len;
        } dir;
    } u;
} __wasi_prestat_t;
assert_wasi_layout(offsetof(__wasi_prestat_t, pr_type) == 0, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    offsetof(__wasi_prestat_t, u.dir.pr_name_len) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    offsetof(__wasi_prestat_t, u.dir.pr_name_len) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    sizeof(__wasi_prestat_t) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    sizeof(__wasi_prestat_t) == 16, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    _Alignof(__wasi_prestat_t) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    _Alignof(__wasi_prestat_t) == 8, "non-wasi data layout");

typedef struct ALIGNED_(8) __wasi_fdstat_t {
    __wasi_filetype_t fs_filetype;
    __wasi_fdflags_t fs_flags;
    uint8_t __paddings[4];
    __wasi_rights_t fs_rights_base;
    __wasi_rights_t fs_rights_inheriting;
} __wasi_fdstat_t;
assert_wasi_layout(
    offsetof(__wasi_fdstat_t, fs_filetype) == 0, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_fdstat_t, fs_flags) == 2, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_fdstat_t, fs_rights_base) == 8, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_fdstat_t, fs_rights_inheriting) == 16,
    "non-wasi data layout");
assert_wasi_layout(sizeof(__wasi_fdstat_t) == 24, "non-wasi data layout");
assert_wasi_layout(_Alignof(__wasi_fdstat_t) == 8, "non-wasi data layout");

typedef struct ALIGNED_(8) __wasi_filestat_t {
    __wasi_device_t st_dev;
    __wasi_inode_t st_ino;
    __wasi_filetype_t st_filetype;
    __wasi_linkcount_t st_nlink;
    __wasi_filesize_t st_size;
    __wasi_timestamp_t st_atim;
    __wasi_timestamp_t st_mtim;
    __wasi_timestamp_t st_ctim;
} __wasi_filestat_t;
assert_wasi_layout(offsetof(__wasi_filestat_t, st_dev) == 0, "non-wasi data layout");
assert_wasi_layout(offsetof(__wasi_filestat_t, st_ino) == 8, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_filetype) == 16, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_nlink) == 24, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_size) == 32, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_atim) == 40, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_mtim) == 48, "non-wasi data layout");
assert_wasi_layout(
    offsetof(__wasi_filestat_t, st_ctim) == 56, "non-wasi data layout");
assert_wasi_layout(sizeof(__wasi_filestat_t) == 64, "non-wasi data layout");
assert_wasi_layout(_Alignof(__wasi_filestat_t) == 8, "non-wasi data layout");

typedef struct __wasi_ciovec_t {
    const void *buf;
    size_t buf_len;
} __wasi_ciovec_t;
assert_wasi_layout(offsetof(__wasi_ciovec_t, buf) == 0, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    offsetof(__wasi_ciovec_t, buf_len) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    offsetof(__wasi_ciovec_t, buf_len) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    sizeof(__wasi_ciovec_t) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    sizeof(__wasi_ciovec_t) == 16, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    _Alignof(__wasi_ciovec_t) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    _Alignof(__wasi_ciovec_t) == 8, "non-wasi data layout");

typedef struct __wasi_iovec_t {
    void *buf;
    size_t buf_len;
} __wasi_iovec_t;
assert_wasi_layout(offsetof(__wasi_iovec_t, buf) == 0, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    offsetof(__wasi_iovec_t, buf_len) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    offsetof(__wasi_iovec_t, buf_len) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    sizeof(__wasi_iovec_t) == 8, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    sizeof(__wasi_iovec_t) == 16, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 4 ||
    _Alignof(__wasi_iovec_t) == 4, "non-wasi data layout");
assert_wasi_layout(sizeof(void *) != 8 ||
    _Alignof(__wasi_iovec_t) == 8, "non-wasi data layout");

/**
 * The contents of a `subscription` when type is `eventtype::clock`.
 */
typedef struct ALIGNED_(8) __wasi_subscription_clock_t {
    /**
     * The clock against which to compare the timestamp.
     */
    __wasi_clockid_t clock_id;

    uint8_t __paddings1[4];

    /**
     * The absolute or relative timestamp.
     */
    __wasi_timestamp_t timeout;

    /**
     * The amount of time that the implementation may wait additionally
     * to coalesce with other events.
     */
    __wasi_timestamp_t precision;

    /**
     * Flags specifying whether the timeout is absolute or relative
     */
    __wasi_subclockflags_t flags;

    uint8_t __paddings2[4];

} __wasi_subscription_clock_t;

assert_wasi_layout(sizeof(__wasi_subscription_clock_t) == 32, "witx calculated size");
assert_wasi_layout(_Alignof(__wasi_subscription_clock_t) == 8, "witx calculated align");
assert_wasi_layout(offsetof(__wasi_subscription_clock_t, clock_id) == 0, "witx calculated offset");
assert_wasi_layout(offsetof(__wasi_subscription_clock_t, timeout) == 8, "witx calculated offset");
assert_wasi_layout(offsetof(__wasi_subscription_clock_t, precision) == 16, "witx calculated offset");
assert_wasi_layout(offsetof(__wasi_subscription_clock_t, flags) == 24, "witx calculated offset");

/**
 * The contents of a `subscription` when type is type is
 * `eventtype::fd_read` or `eventtype::fd_write`.
 */
typedef struct __wasi_subscription_fd_readwrite_t {
    /**
     * The file descriptor on which to wait for it to become ready for reading or writing.
     */
    __wasi_fd_t fd;

} __wasi_subscription_fd_readwrite_t;

assert_wasi_layout(sizeof(__wasi_subscription_fd_readwrite_t) == 4, "witx calculated size");
assert_wasi_layout(_Alignof(__wasi_subscription_fd_readwrite_t) == 4, "witx calculated align");
assert_wasi_layout(offsetof(__wasi_subscription_fd_readwrite_t, fd) == 0, "witx calculated offset");

/**
 * The contents of a `subscription`.
 */
typedef union __wasi_subscription_u_u_t {
    __wasi_subscription_clock_t clock;
    __wasi_subscription_fd_readwrite_t fd_readwrite;
} __wasi_subscription_u_u_t ;

typedef struct ALIGNED_(8) __wasi_subscription_u_t {
    __wasi_eventtype_t type;
    __wasi_subscription_u_u_t u;
} __wasi_subscription_u_t;

assert_wasi_layout(sizeof(__wasi_subscription_u_t) == 40, "witx calculated size");
assert_wasi_layout(_Alignof(__wasi_subscription_u_t) == 8, "witx calculated align");
assert_wasi_layout(offsetof(__wasi_subscription_u_t, u) == 8, "witx calculated union offset");
assert_wasi_layout(sizeof(__wasi_subscription_u_u_t) == 32, "witx calculated union size");
assert_wasi_layout(_Alignof(__wasi_subscription_u_u_t) == 8, "witx calculated union align");

/**
 * Subscription to an event.
 */
typedef struct __wasi_subscription_t {
    /**
     * User-provided value that is attached to the subscription in the
     * implementation and returned through `event::userdata`.
     */
    __wasi_userdata_t userdata;

    /**
     * The type of the event to which to subscribe, and its contents
     */
    __wasi_subscription_u_t u;

} __wasi_subscription_t;

assert_wasi_layout(sizeof(__wasi_subscription_t) == 48, "witx calculated size");
assert_wasi_layout(_Alignof(__wasi_subscription_t) == 8, "witx calculated align");
assert_wasi_layout(offsetof(__wasi_subscription_t, userdata) == 0, "witx calculated offset");
assert_wasi_layout(offsetof(__wasi_subscription_t, u) == 8, "witx calculated offset");

/* keep syncing with wasi_socket_ext.h */
typedef enum {
    /* Used only for sock_addr_resolve hints */
    SOCKET_ANY = -1,
    SOCKET_DGRAM = 0,
    SOCKET_STREAM,
} __wasi_sock_type_t;

typedef uint16_t __wasi_ip_port_t;

typedef enum { IPv4 = 0, IPv6 } __wasi_addr_type_t;

/* n0.n1.n2.n3 */
typedef struct __wasi_addr_ip4_t {
    uint8_t n0;
    uint8_t n1;
    uint8_t n2;
    uint8_t n3;
} __wasi_addr_ip4_t;

typedef struct __wasi_addr_ip4_port_t {
    __wasi_addr_ip4_t addr;
    __wasi_ip_port_t port;
} __wasi_addr_ip4_port_t;

typedef struct __wasi_addr_ip6_t {
    uint16_t n0;
    uint16_t n1;
    uint16_t n2;
    uint16_t n3;
    uint16_t h0;
    uint16_t h1;
    uint16_t h2;
    uint16_t h3;
} __wasi_addr_ip6_t;

typedef struct __wasi_addr_ip6_port_t {
    __wasi_addr_ip6_t addr;
    __wasi_ip_port_t port;
} __wasi_addr_ip6_port_t;

typedef struct __wasi_addr_ip_t {
    __wasi_addr_type_t kind;
    union {
        __wasi_addr_ip4_t ip4;
        __wasi_addr_ip6_t ip6;
    } addr;
} __wasi_addr_ip_t;

typedef struct __wasi_addr_t {
    __wasi_addr_type_t kind;
    union {
        __wasi_addr_ip4_port_t ip4;
        __wasi_addr_ip6_port_t ip6;
    } addr;
} __wasi_addr_t;

typedef enum { INET4 = 0, INET6, INET_UNSPEC } __wasi_address_family_t;

typedef struct __wasi_addr_info_t {
    __wasi_addr_t addr;
    __wasi_sock_type_t type;
} __wasi_addr_info_t;

typedef struct __wasi_addr_info_hints_t {
   __wasi_sock_type_t type;
   __wasi_address_family_t family;
   // this is to workaround lack of optional parameters
   uint8_t hints_enabled;
} __wasi_addr_info_hints_t;

#undef assert_wasi_layout

/* clang-format on */

/****************************************************
 *                                                  *
 *                Filesystem interface              *
 *                                                  *
 ****************************************************/

/**
 * NOTES:
 * Fileystem APIs are required for WASI libc support. If you don't need to
 * support WASI libc, there is no need to implement these APIs. With a
 * few exceptions, each filesystem function has been named after the equivalent
 * POSIX filesystem function with an os_ prefix.
 *
 * Filesystem types
 *
 * os_raw_file_handle: the underlying OS file handle type e.g. int on POSIX
 * systems and HANDLE on Windows. This type exists to allow embedders to provide
 * custom file handles for stdout/stdin/stderr.
 *
 * os_file_handle: the file handle type used in the WASI libc fd
 * table. Filesystem implementations can use it as a means to store any
 * necessary platform-specific information which may not be directly available
 * through the raw OS file handle. Similiar to POSIX file descriptors, file
 * handles may also refer to sockets, directories, symbolic links or character
 * devices and any of the filesystem operations which make sense for these
 * resource types should be supported as far as possible.
 *
 * os_dir_stream: a directory stream type in which fileystem implementations
 * can store any necessary state to iterate over the entries in a directory.
 */

/**
 * Obtain information about an open file associated with the given handle.
 *
 * @param handle the handle for which to obtain file information
 * @param buf a buffer in which to store the information
 */
__wasi_errno_t
os_fstat(os_file_handle handle, struct __wasi_filestat_t *buf);

/**
 * Obtain information about an open file or directory.
 * @param handle the directory handle from which to resolve the file/directory
 * path
 * @param path the relative path of the file or directory for which to obtain
 * information
 * @param buf a buffer in which to store the information
 * @param follow_symlink whether to follow symlinks when resolving the path
 */
__wasi_errno_t
os_fstatat(os_file_handle handle, const char *path,
           struct __wasi_filestat_t *buf, __wasi_lookupflags_t lookup_flags);

/**
 * Obtain the file status flags for the provided handle. This is similiar to the
 * POSIX function fcntl called with the F_GETFL command.
 *
 * @param handle the handle for which to obtain the file status flags
 * @param flags a pointer in which to store the output
 */
__wasi_errno_t
os_file_get_fdflags(os_file_handle handle, __wasi_fdflags_t *flags);

/**
 * Set the file status flags for the provided handle. This is similiar to the
 * POSIX function fcntl called with the F_SETFL command.
 *
 * @param handle the handle for which to set the file status flags
 * @param flags the flags to set
 */
__wasi_errno_t
os_file_set_fdflags(os_file_handle handle, __wasi_fdflags_t flags);

/**
 * Synchronize the data of a file to disk.
 *
 * @param handle
 */
__wasi_errno_t
os_fdatasync(os_file_handle handle);

/**
 * Synchronize the data and metadata of a file to disk.
 *
 * @param handle
 */
__wasi_errno_t
os_fsync(os_file_handle handle);

/**
 * Open a preopen directory. The path provided must refer to a directory and the
 * returned handle will allow only readonly operations.
 *
 * @param path the path of the preopen directory to open
 * @param out a pointer in which to store the newly opened handle
 */
__wasi_errno_t
os_open_preopendir(const char *path, os_file_handle *out);

typedef uint8 wasi_libc_file_access_mode;
#define WASI_LIBC_ACCESS_MODE_READ_ONLY 0
#define WASI_LIBC_ACCESS_MODE_WRITE_ONLY 1
#define WASI_LIBC_ACCESS_MODE_READ_WRITE 2

/**
 * Open a file or directory at the given path.
 *
 * @param handle a handle to the directory in which to open the new file or
 * directory
 * @param path the relative path of the file or directory to open
 * @param oflags the flags to determine how the file or directory is opened
 * @param fd_flags the flags to set on the returned handle
 * @param lookup_flags whether to follow symlinks when resolving the path
 * @param access_mode whether the file is opened as read only, write only or
 * both
 * @param out a pointer in which to store the newly opened handle
 */
__wasi_errno_t
os_openat(os_file_handle handle, const char *path, __wasi_oflags_t oflags,
          __wasi_fdflags_t fd_flags, __wasi_lookupflags_t lookup_flags,
          wasi_libc_file_access_mode access_mode, os_file_handle *out);

/**
 * Obtain the file access mode for the provided handle. This is similiar to the
 * POSIX function fcntl called with the F_GETFL command combined with the
 * O_ACCMODE mask.
 *
 * @param handle the handle for which to obtain the access mode
 * @param access_mode a pointer in which to store the access mode
 */
__wasi_errno_t
os_file_get_access_mode(os_file_handle handle,
                        wasi_libc_file_access_mode *access_mode);

/**
 * Close the provided handle. If is_stdio is true, the raw file handle
 * associated with the given file handle will not be closed.
 *
 * @param handle the handle to close
 * @param is_stdio whether the provided handle refers to a stdio device
 */
__wasi_errno_t
os_close(os_file_handle handle, bool is_stdio);

/**
 * Read data from the provided handle at the given offset into multiple buffers.
 *
 * @param handle the handle to read from
 * @param iov the buffers to read into
 * @param iovcnt the number of buffers to read into
 * @param offset the offset to read from
 * @param nread a pointer in which to store the number of bytes read
 */
__wasi_errno_t
os_preadv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
          __wasi_filesize_t offset, size_t *nread);

/**
 * Write data from multiple buffers at the given offset to the provided handle.
 *
 * @param handle the handle to write to
 * @param iov the buffers to write from
 * @param iovcnt the number of buffers to write from
 * @param offset the offset to write from
 * @param nwritten a pointer in which to store the number of bytes written
 */
__wasi_errno_t
os_pwritev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
           __wasi_filesize_t offset, size_t *nwritten);

/**
 * Read data from the provided handle into multiple buffers.
 *
 * @param handle the handle to read from
 * @param iov the buffers to read into
 * @param iovcnt the number of buffers to read into
 * @param nread a pointer in which to store the number of bytes read
 */
__wasi_errno_t
os_readv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
         size_t *nread);

/**
 * Write data from multiple buffers to the provided handle.
 *
 * @param handle the handle to write to
 * @param iov the buffers to write from
 * @param iovcnt the number of buffers to write from
 * @param nwritten a pointer in which to store the number of bytes written
 */
__wasi_errno_t
os_writev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
          size_t *nwritten);

/**
 * Allocate storage space for the file associated with the provided handle. This
 * is similar to the POSIX function posix_fallocate.
 *
 * @param handle the handle to allocate space for
 * @param offset the offset to allocate space at
 * @param length the amount of space to allocate
 */
__wasi_errno_t
os_fallocate(os_file_handle handle, __wasi_filesize_t offset,
             __wasi_filesize_t length);

/**
 * Adjust the size of an open file.
 *
 * @param handle the associated file handle for which to adjust the size
 * @param size the new size of the file
 */
__wasi_errno_t
os_ftruncate(os_file_handle handle, __wasi_filesize_t size);

/**
 * Set file access and modification times on an open file or directory.
 *
 * @param handle the associated file handle for which to adjust the
 * access/modification times
 * @param access_time the timestamp for the new access time
 * @param modification_time the timestamp for the new modification time
 * @param fstflags a bitmask to indicate which timestamps to adjust
 */
__wasi_errno_t
os_futimens(os_file_handle handle, __wasi_timestamp_t access_time,
            __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags);

/**
 * Set file access and modification times on an open file or directory.
 *
 * @param handle the directory handle from which to resolve the path
 * @param path the relative path of the file or directory for which to adjust
 * the access/modification times
 * @param access_time the timestamp for the new access time
 * @param modification_time the timestamp for the new modification time
 * @param fstflags a bitmask to indicate which timestamps to adjust
 * @param lookup_flags whether to follow symlinks when resolving the path
 */
__wasi_errno_t
os_utimensat(os_file_handle handle, const char *path,
             __wasi_timestamp_t access_time,
             __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags,
             __wasi_lookupflags_t lookup_flags);

/**
 * Read the contents of a symbolic link relative to the provided directory
 * handle.
 *
 * @param handle the directory handle
 * @param path the relative path of the symbolic link from which to read
 * @param buf the buffer to read the link contents into
 * @param bufsize the size of the provided buffer
 * @param nread a pointer in which to store the number of bytes read into the
 * buffer
 */
__wasi_errno_t
os_readlinkat(os_file_handle handle, const char *path, char *buf,
              size_t bufsize, size_t *nread);

/**
 * Create a link from one path to another path.
 *
 * @param from_handle the directory handle from which to resolve the origin path
 * @param from_path the origin path to link from
 * @param to_handle the directory handle from which to resolve the destination
 * path
 * @param to_path the destination path at which to create the link
 * @param lookup_flags whether to follow symlinks when resolving the origin path
 */
__wasi_errno_t
os_linkat(os_file_handle from_handle, const char *from_path,
          os_file_handle to_handle, const char *to_path,
          __wasi_lookupflags_t lookup_flags);

/**
 * Create a symbolic link from one path to another path.
 *
 * @param old_path the symbolic link contents
 * @param handle the directory handle from which to resolve the destination path
 * @param new_path the destination path at which to create the symbolic link
 */
__wasi_errno_t
os_symlinkat(const char *old_path, os_file_handle handle, const char *new_path);

/**
 * Create a directory relative to the provided directory handle.
 *
 * @param handle the directory handle
 * @param path the relative path of the directory to create
 */
__wasi_errno_t
os_mkdirat(os_file_handle handle, const char *path);

/**
 * Rename a file or directory.
 *
 * @param old_handle the directory handle from which to resolve the old path
 * @param old_path the source path to rename
 * @param new_handle the directory handle from which to resolve the destination
 * path
 * @param new_path the destination path to which to rename the file or directory
 */
__wasi_errno_t
os_renameat(os_file_handle old_handle, const char *old_path,
            os_file_handle new_handle, const char *new_path);

/**
 * Unlink a file or directory.
 *
 * @param handle the directory handle from which to resolve the path
 * @param path the relative path of the file or directory to unlink
 * @param is_dir whether the provided handle refers to a directory or file
 */
__wasi_errno_t
os_unlinkat(os_file_handle handle, const char *path, bool is_dir);

/**
 * Move the read/write offset of an open file.
 *
 * @param handle the associated file handle for which to adjust the offset
 * @param offset the number of bytes to adjust the offset by
 * @param whence the position whence to adjust the offset
 * @param new_offset a pointer in which to store the new offset
 */
__wasi_errno_t
os_lseek(os_file_handle handle, __wasi_filedelta_t offset,
         __wasi_whence_t whence, __wasi_filesize_t *new_offset);

/**
 * Provide file advisory information for the given handle. This is similar to
 * the POSIX function posix_fadvise.
 *
 * @param handle the associated file handle for which to provide advisory
 * information
 * @param offset the offset within the file to which the advisory
 * information applies
 * @param length the length of the region for which the advisory information
 * applies
 * @param advice the advice to provide
 */
__wasi_errno_t
os_fadvise(os_file_handle handle, __wasi_filesize_t offset,
           __wasi_filesize_t length, __wasi_advice_t advice);

/**
 * Determine if the given handle refers to a terminal device. __WASI_ESUCCESS
 * will be returned if the handle is associated with a terminal device,
 * otherwise an appropriate error code will be returned.
 *
 * @param handle
 */
__wasi_errno_t
os_isatty(os_file_handle handle);

/**
 * Converts a raw file handle to STDIN to a corresponding file handle to STDIN.
 * If the provided raw file handle is invalid, the platform-default raw handle
 * for STDIN will be used.
 *
 * @param raw_stdin a raw file handle to STDIN
 *
 * @return a handle to STDIN
 */
os_file_handle
os_convert_stdin_handle(os_raw_file_handle raw_stdin);

/**
 * Converts a raw file handle to STDOUT to a correponding file handle to STDOUT.
 * If the provided raw file handle is invalid, the platform-default raw handle
 * for STDOUT will be used.
 *
 * @param raw_stdout a raw file handle to STDOUT
 *
 * @return a handle to STDOUT
 */
os_file_handle
os_convert_stdout_handle(os_raw_file_handle raw_stdout);

/**
 * Converts a raw file handle to STDERR to a correponding file handle to STDERR.
 * If the provided raw file handle is invalid, the platform-default raw handle
 * for STDERR will be used.
 *
 * @param raw_stderr a raw file handle to STDERR
 *
 * @return a handle to STDERR
 */
os_file_handle
os_convert_stderr_handle(os_raw_file_handle raw_stderr);

/**
 * Open a directory stream for the provided directory handle. The returned
 * directory stream will be positioned at the first entry in the directory.
 *
 * @param handle the directory handle
 * @param dir_stream a pointer in which to store the new directory stream
 */
__wasi_errno_t
os_fdopendir(os_file_handle handle, os_dir_stream *dir_stream);

/**
 * Reset the position of a directory stream to the beginning of the directory.
 *
 * @param dir_stream the directory stream for which to reset the position
 */
__wasi_errno_t
os_rewinddir(os_dir_stream dir_stream);

/**
 * Set the position of the given directory stream.
 *
 * @param dir_stream the directory stream for which to set the position
 * @param position the position to set
 */
__wasi_errno_t
os_seekdir(os_dir_stream dir_stream, __wasi_dircookie_t position);

/**
 * Read a directory entry from the given directory stream. The directory name
 * will be NULL if the end of the directory is reached or an error is
 * encountered.
 *
 * @param dir_stream the directory stream from which to read the entry
 * @param entry a pointer in which to store the directory entry
 * @param d_name a pointer in which to store the directory entry name
 */
__wasi_errno_t
os_readdir(os_dir_stream dir_stream, __wasi_dirent_t *entry,
           const char **d_name);

/**
 * Close the given directory stream. The handle associated with the directory
 * stream will also be closed.
 *
 * @param dir_stream the directory stream to close
 */
__wasi_errno_t
os_closedir(os_dir_stream dir_stream);

/**
 * Returns an invalid directory stream that is guaranteed to cause failure when
 * called with any directory filesystem operation.
 *
 * @return the invalid directory stream
 */
os_dir_stream
os_get_invalid_dir_stream();

/**
 * Checks whether the given directory stream is valid. An invalid directory
 * stream is guaranteed to cause failure when called with any directory
 * filesystem operation.
 *
 * @param dir_stream a pointer to a directory stream
 */
bool
os_is_dir_stream_valid(os_dir_stream *dir_stream);

/**
 * Returns an invalid handle that is guaranteed to cause failure when
 * called with any filesystem operation.
 *
 * @return the invalid handle
 */
os_file_handle
os_get_invalid_handle();

/**
 * Checks whether the given file handle is valid. An invalid handle is
 * guaranteed to cause failure when called with any filesystem operation.
 *
 * @param handle a pointer to a file handle
 */
bool
os_is_handle_valid(os_file_handle *handle);

/**
 * Resolve a pathname. The generated pathname will be stored as a
 * null-terminated string, with a maximum length of PATH_MAX bytes.
 *
 * @param path the path to resolve
 * @param resolved_path the buffer to store the resolved path in
 *
 * @return the resolved path if success, NULL otherwise
 */
char *
os_realpath(const char *path, char *resolved_path);

/****************************************************
 *                                                  *
 *                Clock functions                   *
 *                                                  *
 ****************************************************/

/**
 * Get the resolution of the specified clock.
 *
 * @param clock_id clock identifier
 * @param resolution output variable to store the clock resolution
 */
__wasi_errno_t
os_clock_res_get(__wasi_clockid_t clock_id, __wasi_timestamp_t *resolution);

/**
 * Get the current time of the specified clock.
 *
 * @param clock_id clock identifier
 * @param precision the maximum lag that the returned time value may have,
 * compared to its actual value.
 * @param time output variable to store the clock time
 */
__wasi_errno_t
os_clock_time_get(__wasi_clockid_t clock_id, __wasi_timestamp_t precision,
                  __wasi_timestamp_t *time);

#ifdef __cplusplus
}
#endif

#endif /* end of _PLATFORM_WASI_H */