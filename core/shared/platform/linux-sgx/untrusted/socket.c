/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stddef.h>

int ocall_socket(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

int ocall_getsockopt(int sockfd, int level, int optname, void *val_buf,
                     unsigned int val_buf_size, void *len_buf)
{
    return getsockopt(sockfd, level, optname, val_buf,
                      (socklen_t *)len_buf);
}

ssize_t ocall_sendmsg(int sockfd, void *msg_buf,
                      unsigned int msg_buf_size, int flags)
{
    struct msghdr *msg = (struct msghdr *)msg_buf;
    int i;
    ssize_t ret;

    if (msg->msg_name != NULL)
        msg->msg_name = msg_buf + (unsigned)(uintptr_t)msg->msg_name;

    if (msg->msg_control != NULL)
        msg->msg_control = msg_buf + (unsigned)(uintptr_t)msg->msg_control;

    if (msg->msg_iov != NULL) {
        msg->msg_iov = msg_buf + (unsigned)(uintptr_t)msg->msg_iov;
        for (i = 0; i < msg->msg_iovlen; i++) {
            msg->msg_iov[i].iov_base = msg_buf + (unsigned)(uintptr_t)
                                       msg->msg_iov[i].iov_base;
        }
    }

    return sendmsg(sockfd, msg, flags);
}

ssize_t ocall_recvmsg(int sockfd, void *msg_buf, unsigned int msg_buf_size,
                      int flags)
{
    struct msghdr *msg = (struct msghdr *)msg_buf;
    int i;
    ssize_t ret;

    if (msg->msg_name != NULL)
        msg->msg_name = msg_buf + (unsigned)(uintptr_t)msg->msg_name;

    if (msg->msg_control != NULL)
        msg->msg_control = msg_buf + (unsigned)(uintptr_t)msg->msg_control;

    if (msg->msg_iov != NULL) {
        msg->msg_iov = msg_buf + (unsigned)(uintptr_t)msg->msg_iov;
        for (i = 0; i <msg->msg_iovlen; i++) {
            msg->msg_iov[i].iov_base = msg_buf + (unsigned)(uintptr_t)
                                       msg->msg_iov[i].iov_base;
        }
    }

    return recvmsg(sockfd, msg, flags);
}

int ocall_shutdown(int sockfd, int how)
{
    return shutdown(sockfd, how);
}