/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

void *
run_as_server(void *arg)
{
    int sock = -1;
    struct sockaddr_in addr = { 0 };
    int addrlen = 0;
    int new_sock = -1;
    char *buf = "it is an example of socket";
    struct iovec iov = { .iov_base = buf, .iov_len = strlen(buf) + 1 };
    struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };
    ssize_t send_len = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Create a socket failed");
        goto RETURN;
    }

    /* 0.0.0.0:1234 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    addrlen = sizeof(addr);
    if (bind(sock, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("Bind failed");
        goto SHUTDOWN;
    }

    if (listen(sock, 0) < 0) {
        perror("Listen failed");
        goto SHUTDOWN;
    }

    new_sock = accept(sock, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
    if (new_sock < 0) {
        perror("Accept failed");
        goto SHUTDOWN;
    }

    printf("Start sending: \n");
    printf("data: %s\n", (char *)msg.msg_iov->iov_base);
    send_len = sendmsg(new_sock, &msg, 0);
    if (send_len < 0) {
        perror("Sendmsg failed");
        goto SHUTDOWN;
    }
    printf("Send successfully! SZ=%ld\n", send_len);

SHUTDOWN:
    shutdown(sock, SHUT_RD);

RETURN:
    return NULL;
}

void *
run_as_client(void *arg)
{
    int sock = -1;
    struct sockaddr_in addr = { 0 };
    char buf[1024] = { 0 };
    struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
    struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };
    ssize_t recv_len = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Create a socket failed");
        goto RETURN;
    }

    /* 127.0.0.1:1234 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect failed");
        goto SHUTDOWN;
    }

    printf("Start receiving: \n");
    recv_len = recvmsg(sock, &msg, 0);
    if (recv_len < 0) {
        perror("Recvmsg failed");
        goto SHUTDOWN;
    }
    printf("Receive successlly! SZ=%ld\n", recv_len);
    printf("data: %s\n", (char *)msg.msg_iov->iov_base);

SHUTDOWN:
    shutdown(sock, SHUT_RD);

RETURN:
    return NULL;
}

int
main(int argc, char *argv[])
{
    pthread_t cs[2] = { 0 };
    uint8_t i = 0;

    if (pthread_create(&cs[0], NULL, run_as_server, NULL)) {
        perror("Create a server thread failed");
        return EXIT_FAILURE;
    }

    if (pthread_create(&cs[1], NULL, run_as_client, NULL)) {
        perror("Create a client thread failed");
        return EXIT_FAILURE;
    }

    for (i = 0; i < 2; i++) {
        pthread_join(cs[i], NULL);
    }

    return EXIT_SUCCESS;
}
