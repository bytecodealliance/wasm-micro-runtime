/*
 * Copyright (C) 2020 XiaoMi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_extension.h"
#include "platform_api_vmcore.h"

int
bh_platform_init()
{
    return 0;
}

void
bh_platform_destroy()
{}

void *
os_malloc(unsigned size)
{
    return malloc(size);
}

void *
os_realloc(void *ptr, unsigned size)
{
    return realloc(ptr, size);
}

void
os_free(void *ptr)
{
    free(ptr);
}

void *
os_mmap(void *hint, size_t size, int prot, int flags)
{
    if ((uint64)size >= UINT32_MAX)
        return NULL;
    return malloc((uint32)size);
}

void
os_munmap(void *addr, size_t size)
{
    return free(addr);
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{}

int
openat(int fd, const char *path, int oflags, ...)
{
    errno = ENOSYS;
    return -1;
}

int
fstatat(int fd, const char *path, struct stat *buf, int flag)
{
    errno = ENOSYS;
    return -1;
}

int
mkdirat(int fd, const char *path, mode_t mode)
{
    errno = ENOSYS;
    return -1;
}

ssize_t
readlinkat(int fd, const char *path, char *buf, size_t bufsize)
{
    errno = ENOSYS;
    return -1;
}

int
linkat(int fd1, const char *path1, int fd2, const char *path2, int flag)
{
    errno = ENOSYS;
    return -1;
}

int
renameat(int fromfd, const char *from, int tofd, const char *to)
{
    errno = ENOSYS;
    return -1;
}
int
symlinkat(const char *target, int fd, const char *path)
{
    errno = ENOSYS;
    return -1;
}
int
unlinkat(int fd, const char *path, int flag)
{
    errno = ENOSYS;
    return -1;
}
int
utimensat(int fd, const char *path, const struct timespec ts[2], int flag)
{
    errno = ENOSYS;
    return -1;
}

DIR *
fdopendir(int fd)
{
    errno = ENOSYS;
    return NULL;
}

ssize_t
preadv(int fd, const struct iovec *iov, int iovcnt, off_t off)
{
    errno = ENOSYS;
    return -1;
}
ssize_t
pwritev(int fd, const struct iovec *iov, int iovcnt, off_t off)
{
    errno = ENOSYS;
    return -1;
}
