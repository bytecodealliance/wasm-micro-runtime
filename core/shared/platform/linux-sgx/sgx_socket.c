/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

#ifndef SGX_DISABLE_WASI

#define TRACE_OCALL_FAIL() os_printf("ocall %s failed!\n", __FUNCTION__)

int ocall_socket(int *p_ret, int domain, int type, int protocol);
int ocall_getsockopt(int *p_ret, int sockfd, int level, int optname,
                     void *val_buf, unsigned int val_buf_size,
                     void *len_buf);

int ocall_sendmsg(ssize_t *p_ret, int sockfd, void *msg_buf,
                  unsigned int msg_buf_size, int flags);
int ocall_recvmsg(ssize_t *p_ret, int sockfd, void *msg_buf,
                  unsigned int msg_buf_size, int flags);
int ocall_shutdown(int *p_ret, int sockfd, int how);

int socket(int domain, int type, int protocol)
{
    int ret;

    if (ocall_socket(&ret, domain, type, protocol) != SGX_SUCCESS) {
        TRACE_OCALL_FAIL();
        return -1;
    }

    if (ret == -1)
        errno = get_errno();

    return ret;
}

int getsockopt(int sockfd, int level, int optname,
               void *optval, socklen_t *optlen)
{
    int ret;
    unsigned int val_buf_size = *optlen;

    if (ocall_getsockopt(&ret, sockfd, level, optname, optval,
                         val_buf_size, (void *)optlen) != SGX_SUCCESS) {
        TRACE_OCALL_FAIL();
        return -1;
    }

    if (ret == -1)
        errno = get_errno();

    return ret;
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    ssize_t ret;
    int i;
    char *p;
    struct msghdr *msg1;

    uint64 total_size = sizeof(struct msghdr) + (uint64)msg->msg_namelen
                        + (uint64)msg->msg_controllen;

    total_size += sizeof(struct iovec) * (msg->msg_iovlen);

    for (i = 0; i < msg->msg_iovlen; i++) {
        total_size += msg->msg_iov[i].iov_len;
    }

    if (total_size >= UINT32_MAX)
        return -1;

    msg1 = BH_MALLOC((uint32)total_size);

    if (msg1 == NULL)
        return -1;

    p = (char*)(uintptr_t)sizeof(struct msghdr);

    if (msg->msg_name != NULL) {
        msg1->msg_name = p;
        memcpy((uintptr_t)p + (char *)msg1, msg->msg_name,
               (size_t)msg->msg_namelen);
        p += msg->msg_namelen;
    }

    if (msg->msg_control != NULL) {
        msg1->msg_control = p;
        memcpy((uintptr_t)p + (char *)msg1, msg->msg_control,
               (size_t)msg->msg_control);
        p += msg->msg_controllen;
    }

    if (msg->msg_iov != NULL) {
        msg1->msg_iov = (struct iovec *)p;
        p += (uintptr_t)(sizeof(struct iovec) * (msg->msg_iovlen));

        for (i = 0; i < msg->msg_iovlen; i++) {
            msg1->msg_iov[i].iov_base = p;
            msg1->msg_iov[i].iov_len = msg->msg_iov[i].iov_len;
            memcpy((uintptr_t)p + (char *)msg1, msg->msg_iov[i].iov_base,
                   (size_t)(msg->msg_iov[i].iov_len));
            p += msg->msg_iov[i].iov_len;
        }
    }

    if (ocall_sendmsg(&ret, sockfd, (void *)msg1, (uint32)total_size,
                      flags) != SGX_SUCCESS) {
        TRACE_OCALL_FAIL();
        return -1;
    }

    if (ret == -1)
        errno = get_errno();

    return ret;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    ssize_t ret;
    int i;
    char *p;
    struct msghdr *msg1;

    uint64 total_size = sizeof(struct msghdr) + (uint64)msg->msg_namelen
                        + (uint64)msg->msg_controllen;

    total_size += sizeof(struct iovec) * (msg->msg_iovlen);

    for (i = 0; i < msg->msg_iovlen; i++) {
        total_size += msg->msg_iov[i].iov_len;
    }

    if (total_size >= UINT32_MAX)
        return -1;

    msg1 = BH_MALLOC((uint32)total_size);

    if (msg1 == NULL)
        return -1;

    memset(msg1, 0, total_size);

    p = (char*)(uintptr_t)sizeof(struct msghdr);

    if (msg->msg_name != NULL) {
        msg1->msg_name = p;
        p += msg->msg_namelen;
    }

    if (msg->msg_control != NULL) {
        msg1->msg_control = p;
        p += msg->msg_controllen;
    }

    if (msg->msg_iov != NULL) {
        msg1->msg_iov = (struct iovec *)p;
        p += (uintptr_t)(sizeof(struct iovec) * (msg->msg_iovlen));

        for (i = 0; i < msg->msg_iovlen; i++) {
            msg1->msg_iov[i].iov_base = p;
            msg1->msg_iov[i].iov_len = msg->msg_iov[i].iov_len;
            p += msg->msg_iov[i].iov_len;
        }
    }

    if (ocall_recvmsg(&ret, sockfd, (void *)msg1, (uint32)total_size,
                      flags) != SGX_SUCCESS) {
        TRACE_OCALL_FAIL();
        return -1;
    }

    p = (char *)(uintptr_t)(sizeof(struct msghdr));

    if (msg1->msg_name != NULL) {
        memcpy(msg->msg_name, (uintptr_t)p + (char *)msg1,
               (size_t)msg1->msg_namelen);
        p += msg1->msg_namelen;
    }

    if (msg1->msg_control != NULL) {
        memcpy(msg->msg_control, (uintptr_t)p + (char *)msg1,
               (size_t)msg1->msg_control);
        p += msg->msg_controllen;
    }

    if (msg1->msg_iov != NULL) {
        p += (uintptr_t)(sizeof(struct iovec) * (msg1->msg_iovlen));

        for (i = 0; i < msg1->msg_iovlen; i++) {
            memcpy(msg->msg_iov[i].iov_base, (uintptr_t)p + (char *)msg1,
                   (size_t)(msg1->msg_iov[i].iov_len));
            p += msg1->msg_iov[i].iov_len;
        }
    }

    if (ret == -1)
        errno = get_errno();

    return ret;
}

int shutdown(int sockfd, int how)
{
    int ret;

    if (ocall_shutdown(&ret, sockfd, how) != SGX_SUCCESS) {
        TRACE_OCALL_FAIL();
        return -1;
    }

    if (ret == -1)
        errno = get_errno();

    return ret;
}

#endif

