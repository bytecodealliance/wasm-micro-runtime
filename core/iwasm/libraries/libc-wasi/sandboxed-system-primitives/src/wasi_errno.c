/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasi_errno.h"

__wasi_errno_t
convert_errno(int error)
{
    __wasi_errno_t code = __WASI_ENOSYS;
#define X(v)               \
    case v:                \
        code = __WASI_##v; \
        break;
    switch (error) {
        X(E2BIG)
        X(EACCES)
        X(EADDRINUSE)
        X(EADDRNOTAVAIL)
        X(EAFNOSUPPORT)
        X(EAGAIN)
        X(EALREADY)
        X(EBADF)
        X(EBADMSG)
        X(EBUSY)
        X(ECANCELED)
        X(ECHILD)
        X(ECONNABORTED)
        X(ECONNREFUSED)
        X(ECONNRESET)
        X(EDEADLK)
        X(EDESTADDRREQ)
        X(EDOM)
#ifdef EDQUOT
        X(EDQUOT)
#endif
        X(EEXIST)
        X(EFAULT)
        X(EFBIG)
        X(EHOSTUNREACH)
        X(EIDRM)
        X(EILSEQ)
        X(EINPROGRESS)
        X(EINTR)
        X(EINVAL)
        X(EIO)
        X(EISCONN)
        X(EISDIR)
        X(ELOOP)
        X(EMFILE)
        X(EMLINK)
        X(EMSGSIZE)
#ifdef EMULTIHOP
        X(EMULTIHOP)
#endif
        X(ENAMETOOLONG)
        X(ENETDOWN)
        X(ENETRESET)
        X(ENETUNREACH)
        X(ENFILE)
        X(ENOBUFS)
        X(ENODEV)
        X(ENOENT)
        X(ENOEXEC)
        X(ENOLCK)
        X(ENOLINK)
        X(ENOMEM)
        X(ENOMSG)
        X(ENOPROTOOPT)
        X(ENOSPC)
        X(ENOSYS)
#ifdef ENOTCAPABLE
        X(ENOTCAPABLE)
#endif
        X(ENOTCONN)
        X(ENOTDIR)
        X(ENOTEMPTY)
        X(ENOTRECOVERABLE)
        X(ENOTSOCK)
        X(ENOTSUP)
        X(ENOTTY)
        X(ENXIO)
        X(EOVERFLOW)
        X(EOWNERDEAD)
        X(EPERM)
        X(EPIPE)
        X(EPROTO)
        X(EPROTONOSUPPORT)
        X(EPROTOTYPE)
        X(ERANGE)
        X(EROFS)
        X(ESPIPE)
        X(ESRCH)
#ifdef ESTALE
        X(ESTALE)
#endif
        X(ETIMEDOUT)
        X(ETXTBSY)
        X(EXDEV)
        default:
            if (error == EOPNOTSUPP)
                code = __WASI_ENOTSUP;
            else if (code == EWOULDBLOCK)
                code = __WASI_EAGAIN;
            break;
    }
#undef X
    return code;
}