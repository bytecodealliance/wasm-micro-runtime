/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

int
main(int argc, char *argv[])
{
    int socket_fd = -1, new_socket, addrlen = 0;
    struct sockaddr_in addr = { 0 };
    const char *message = "Say Hi from the Server";
    uint32_t connections = 0;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Create socket failed");
        goto fail;
    }

    /* 0.0.0.0:1234 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    addrlen = sizeof(addr);
    if (bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("Bind failed");
        goto fail;
    }

    if (listen(socket_fd, 3) < 0) {
        perror("Listen failed");
        goto fail;
    }

    while (true) {
        new_socket =
            accept(socket_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            break;
        }

        connections++;

#ifndef __wasi__
        {
            struct sockaddr client_addr = { 0 };
            if (getpeername(new_socket, &client_addr, &addrlen) != 0) {
                perror("Getpeername failed");
            }
            else {
                printf("The connected address is %s\n",
                       inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
            }
        }
#endif

        if (send(new_socket, message, strlen(message), 0) < 0) {
            perror("Send failed");
        }
        printf("[Server] Shuting down the new connection ... \n");
        shutdown(new_socket, SHUT_RDWR);

        if (connections == 1) {
            break;
        }
    }

    printf("[Server] Shuting down ... \n");
    shutdown(socket_fd, SHUT_RDWR);
    sleep(3);
    printf("[Server] BYE \n");
    return EXIT_SUCCESS;

fail:
    printf("[Server] Shuting down ... \n");
    if (socket_fd >= 0)
        close(socket_fd);
    sleep(3);
    return EXIT_FAILURE;
}