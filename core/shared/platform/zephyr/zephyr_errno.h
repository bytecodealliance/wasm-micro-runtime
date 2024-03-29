#ifndef ZEPHYR_ERRNO_H
#define ZEPHYR_ERRNO_H

#include "platform_wasi_types.h"
#include <errno.h>

            // Add your custom code here
static inline __wasi_errno_t
zephyr_to_wasi_errno(int zephyr_errno) {
    switch (zephyr_errno) {
        case EPERM: 
            return __WASI_EPERM;            // Operation not permitted
        case ENOENT: 
            return __WASI_ENOENT;           // No such file or directory
        case ESRCH: 
            return __WASI_ESRCH;            // No such process
        case EINTR: 
            return __WASI_EINTR;            // Interrupted system call
        case EIO: 
            return __WASI_EIO;              // I/O error
        case ENXIO: 
            return __WASI_ENXIO;            // No such device or address
        case E2BIG: 
            return __WASI_E2BIG;            // Argument list too long
        case ENOEXEC: 
            return __WASI_ENOEXEC;          // Exec format error
        case EBADF: 
            return __WASI_EBADF;            // Bad file descriptor
        case ECHILD: 
            return __WASI_ECHILD;           // No child processes
        case EAGAIN: 
            return __WASI_EAGAIN;          // Try again
        case ENOMEM: 
            return __WASI_ENOMEM;          // Out of memory
        case EACCES: 
            return __WASI_EACCES;          // Permission denied
        case EFAULT: 
            return __WASI_EFAULT;          // Bad address
        case EBUSY: 
            return __WASI_EBUSY;            // Device or resource busy
        case EEXIST: 
            return __WASI_EEXIST;          // File exists
        case EXDEV: 
            return __WASI_EXDEV;            // Cross-device link
        case ENODEV: 
            return __WASI_ENODEV;          // No such device
        case ENOTDIR: 
            return __WASI_ENOTDIR;        // Not a directory
        case EISDIR: 
            return __WASI_EISDIR;          // Is a directory
        case EINVAL: 
            return __WASI_EINVAL;          // Invalid argument
        case ENFILE: 
            return __WASI_ENFILE;          // File table overflow
        case EMFILE: 
            return __WASI_EMFILE;          // Too many open files
        case ENOTTY: 
            return __WASI_ENOTTY;          // Not a typewriter
        case EFBIG: 
            return __WASI_EFBIG;            // File too large
        case ENOSPC: 
            return __WASI_ENOSPC;          // No space left on device
        case EROFS: 
            return __WASI_EROFS;            // Read-only file system
        case EMLINK: 
            return __WASI_EMLINK;          // Too many links
        case EPIPE: 
            return __WASI_EPIPE;            // Broken pipe
        case EDOM: 
            return __WASI_EDOM;              // Math argument out of domain of func
        case ERANGE: 
            return __WASI_ERANGE;          // Math result not representable
        case ENOMSG: 
            return __WASI_ENOMSG;          // No message of desired type
        case EDEADLK: 
            return __WASI_EDEADLK;        // Resource deadlock would occur
        case ENOLCK: 
            return __WASI_ENOLCK;          // No record locks available
        case ENOSYS: 
            return __WASI_ENOSYS;          // Function not implemented
        case ENOTEMPTY: 
            return __WASI_ENOTEMPTY;    // Directory not empty
        case ENAMETOOLONG: 
            return __WASI_ENAMETOOLONG; // File name too long
        case ELOOP: 
            return __WASI_ELOOP;            // Too many symbolic links encountered
        case EOPNOTSUPP: 
            return __WASI_ENOTSUP;     // Operation not supported on transport endpoint
        case EPFNOSUPPORT: 
            return __WASI_EAFNOSUPPORT; // Protocol family not supported
        case ECONNRESET: 
            return __WASI_ECONNRESET;  // Connection reset by peer
        case ENOBUFS: 
            return __WASI_ENOBUFS;        // No buffer space available
        case EAFNOSUPPORT: 
            return __WASI_EAFNOSUPPORT; // Address family not supported by protocol
        case EPROTOTYPE: 
            return __WASI_EPROTOTYPE;  // Protocol wrong type for socket
        case ENOTSOCK: 
            return __WASI_ENOTSOCK;      // Socket operation on non-socket
        case ENOPROTOOPT: 
            return __WASI_ENOPROTOOPT; // Protocol not available
        case ESHUTDOWN: 
            return __WASI_ECANCELED;     // Cannot send after transport endpoint shutdown
        case ECONNREFUSED: 
            return __WASI_ECONNREFUSED; // Connection refused
        case EADDRINUSE: 
            return __WASI_EADDRINUSE;   // Address already in use
        case ECONNABORTED: 
            return __WASI_ECONNABORTED; // Connection aborted
        case ENETUNREACH: 
            return __WASI_ENETUNREACH; // Network is unreachable
        case ENETDOWN: 
            return __WASI_ENETDOWN;       // Network is down
        case ETIMEDOUT: 
            return __WASI_ETIMEDOUT;     // Connection timed out
        case EHOSTDOWN: 
            return __WASI_ENETDOWN;      // Host is down
        case EHOSTUNREACH: 
            return __WASI_EHOSTUNREACH; // No route to host
        case EINPROGRESS: 
            return __WASI_EINPROGRESS; // Operation now in progress
        case EALREADY: 
            return __WASI_EALREADY;       // Operation already in progress
        case EDESTADDRREQ: 
            return __WASI_EDESTADDRREQ; // Destination address required
        case EMSGSIZE: 
            return __WASI_EMSGSIZE;       // Message too long
        case EPROTONOSUPPORT: 
            return __WASI_EPROTONOSUPPORT; // Protocol not supported
        case ESOCKTNOSUPPORT: 
        //    return __WASI_ESOCKTNOSUPPORT; // Socket type not supported
        case EADDRNOTAVAIL: 
            return __WASI_EADDRNOTAVAIL;     // Cannot assign requested address
        case ENETRESET: 
            return __WASI_ENETRESET;     // Network dropped connection because of reset
        case EISCONN: 
            return __WASI_EISCONN;         // Transport endpoint is already connected
        case ENOTCONN: 
            return __WASI_ENOTCONN;       // Transport endpoint is not connected
        case ETOOMANYREFS: 
            return __WASI_ENOTRECOVERABLE; // Too many references: cannot splice
        case ENOTSUP: 
            return __WASI_ENOTSUP;         // Operation not supported
        case EILSEQ: 
            return __WASI_EILSEQ;           // Illegal byte sequence
        case EOVERFLOW: 
            return __WASI_EOVERFLOW;     // Value too large for defined data type
        case ECANCELED: 
            return __WASI_ECANCELED;     // Operation canceled
        default: 
            return __WASI_ENOSYS;               // Function not implemented
    }
}


static inline int
wasi_to_zephyr_errno(__wasi_errno_t wasi_errno) {
    switch (wasi_errno) {
        case __WASI_EPERM: 
            return EPERM;
        case __WASI_ENOENT: 
            return ENOENT;
        case __WASI_ESRCH: 
            return ESRCH;
        case __WASI_EINTR: 
            return EINTR;
        case __WASI_EIO: 
            return EIO;
        case __WASI_ENXIO: 
            return ENXIO;
        case __WASI_E2BIG: 
            return E2BIG;
        case __WASI_ENOEXEC: 
            return ENOEXEC;
        case __WASI_EBADF: 
            return EBADF;
        case __WASI_ECHILD: 
            return ECHILD;
        case __WASI_EAGAIN: 
            return EAGAIN;
        case __WASI_ENOMEM: 
            return ENOMEM;
        case __WASI_EACCES: 
            return EACCES;
        case __WASI_EFAULT: 
            return EFAULT;
        case __WASI_EBUSY: 
            return EBUSY;
        case __WASI_EEXIST: 
            return EEXIST;
        case __WASI_EXDEV: 
            return EXDEV;
        case __WASI_ENODEV: 
            return ENODEV;
        case __WASI_ENOTDIR: 
            return ENOTDIR;
        case __WASI_EISDIR: 
            return EISDIR;
        case __WASI_EINVAL: 
            return EINVAL;
        case __WASI_ENFILE: 
            return ENFILE;
        case __WASI_EMFILE: 
            return EMFILE;
        case __WASI_ENOTTY: 
            return ENOTTY;
        case __WASI_EFBIG: 
            return EFBIG;
        case __WASI_ENOSPC: 
            return ENOSPC;
        case __WASI_EROFS: 
            return EROFS;
        case __WASI_EMLINK: 
            return EMLINK;
        case __WASI_EPIPE: 
            return EPIPE;
        case __WASI_EDOM: 
            return EDOM;
        case __WASI_ERANGE: 
            return ERANGE;
        case __WASI_ENOMSG: 
            return ENOMSG;
        case __WASI_EDEADLK: 
            return EDEADLK;
        case __WASI_ENOLCK: 
            return ENOLCK;
        case __WASI_ENOSYS: 
            return ENOSYS;
        case __WASI_ENOTEMPTY: 
            return ENOTEMPTY;
        case __WASI_ENAMETOOLONG: 
            return ENAMETOOLONG;
        case __WASI_ELOOP: 
            return ELOOP;
        //case __WASI_ENOTSUPP : 
        //    return EOPNOTSUPP;
        case __WASI_EAFNOSUPPORT: 
            return EPFNOSUPPORT;
        case __WASI_ECONNRESET: 
            return ECONNRESET;
        case __WASI_ENOBUFS: 
            return ENOBUFS;
        case __WASI_EPROTOTYPE: 
            return EPROTOTYPE;
        case __WASI_ENOTSOCK: 
            return ENOTSOCK;
        case __WASI_ENOPROTOOPT: 
            return ENOPROTOOPT;
        //case __WASI_ECANCELED: 
        //    return ESHUTDOWN;
        case __WASI_ECONNREFUSED: 
            return ECONNREFUSED;
        case __WASI_EADDRINUSE: 
            return EADDRINUSE;
        case __WASI_ECONNABORTED: 
            return ECONNABORTED;
        case __WASI_ENETUNREACH: 
            return ENETUNREACH;
        case __WASI_ENETDOWN: 
            return ENETDOWN;
        case __WASI_ETIMEDOUT: 
            return ETIMEDOUT;
        // case __WASI_EHOSTDOWN: 
        //     return EHOSTDOWN;
        case __WASI_EHOSTUNREACH: 
            return EHOSTUNREACH;
        case __WASI_EINPROGRESS: 
            return EINPROGRESS;
        case __WASI_EALREADY: 
            return EALREADY;
        case __WASI_EDESTADDRREQ: 
            return EDESTADDRREQ;
        case __WASI_EMSGSIZE: 
            return EMSGSIZE;
        case __WASI_EPROTONOSUPPORT: 
            return EPROTONOSUPPORT;
        // case __WASI_ESOCKTNOSUPPORT: 
        //    return ESOCKTNOSUPPORT;
        case __WASI_EADDRNOTAVAIL: 
            return EADDRNOTAVAIL;
        case __WASI_ENETRESET: 
            return ENETRESET;
        case __WASI_EISCONN: 
            return EISCONN;
        case __WASI_ENOTCONN: 
            return ENOTCONN;
        case __WASI_ENOTRECOVERABLE: 
            return ETOOMANYREFS;
        case __WASI_ENOTSUP: 
            return ENOTSUP;
        case __WASI_EILSEQ: 
            return EILSEQ;
        case __WASI_EOVERFLOW: 
            return EOVERFLOW;
        case __WASI_ECANCELED: 
            return ECANCELED;
        default: 
            return ENOSYS;
    }
}
#endif /* ZEPHYR_ERRNO_H */
