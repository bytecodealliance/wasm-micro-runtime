/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gdbserver.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bh_log.h"
#include "handler.h"
#include "packets.h"
#include "utils.h"

typedef void (*PacketHandler)(WASMGDBServer *server, char *payload);

struct packet_handler_elem {
    char request;
    PacketHandler handler;
};

#define DEL_HANDLER(r, h) [r] = { .request = r, .handler = h }

static struct packet_handler_elem packet_handler_table[255] = {
    DEL_HANDLER('Q', handle_generay_set),
    DEL_HANDLER('q', handle_generay_query),
    DEL_HANDLER('v', handle_v_packet),
    DEL_HANDLER('?', handle_threadstop_request),
    DEL_HANDLER('H', handle_set_current_thread),
    DEL_HANDLER('p', handle_get_register),
    DEL_HANDLER('j', handle_get_json_request),
    DEL_HANDLER('m', handle_get_read_memory),
    DEL_HANDLER('M', handle_get_write_memory),
    DEL_HANDLER('x', handle_get_read_binary_memory),
    DEL_HANDLER('Z', handle_add_break),
    DEL_HANDLER('z', handle_remove_break),
    DEL_HANDLER('c', handle_continue_request),
    DEL_HANDLER('k', handle_kill_request),
    DEL_HANDLER('_', handle____request),
};

WASMGDBServer *
wasm_launch_gdbserver(char *host, int port)
{
    int listen_fd = -1;
    const int one = 1;
    struct sockaddr_in addr;
    int ret;
    int sockt_fd = 0;

    WASMGDBServer *server;

    if (!(server = wasm_runtime_malloc(sizeof(WASMGDBServer)))) {
        LOG_ERROR("wasm gdb server error: failed to allocate memory");
        return NULL;
    }

    memset(server, 0, sizeof(WASMGDBServer));

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        LOG_ERROR("wasm gdb server error: socket() failed");
        goto fail;
    }

    ret = fcntl(listen_fd, F_SETFD, FD_CLOEXEC);
    if(ret < 0) {
        LOG_ERROR("wasm gdb server error: fcntl() failed on setting FD_CLOEXEC");
        goto fail;
    }

    ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (ret < 0) {
        LOG_ERROR("wasm gdb server error: setsockopt() failed");
        goto fail;
    }

    LOG_VERBOSE("Listening on %s:%d\n", host, port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);

    ret = bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("wasm gdb server error: bind() failed");
        goto fail;
    }

    ret = listen(listen_fd, 1);
    if (ret < 0) {
        LOG_ERROR("wasm gdb server error: listen() failed");
        goto fail;
    }

    server->listen_fd = listen_fd;

    sockt_fd = accept(listen_fd, NULL, NULL);
    if (sockt_fd < 0) {
        LOG_ERROR("wasm gdb server error: accept() failed");
        goto fail;
    }
    LOG_VERBOSE("accept gdb client");
    server->socket_fd = sockt_fd;
    server->noack = false;
    return server;

fail:
    if (listen_fd >= 0) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
    if (server)
        wasm_runtime_free(server);
    return NULL;
}

void
wasm_close_gdbserver(WASMGDBServer *server)
{
    if (server->socket_fd > 0) {
        shutdown(server->socket_fd, SHUT_RDWR);
        close(server->socket_fd);
    }
    if (server->listen_fd > 0) {
        shutdown(server->listen_fd, SHUT_RDWR);
        close(server->listen_fd);
    }
}

static inline void
handler_packet(WASMGDBServer *server, char request, char *payload)
{
    if (packet_handler_table[(int)request].handler != NULL)
        packet_handler_table[(int)request].handler(server, payload);
}

/**
 * The packet layout is:
 *   '$' + payload + '#' + checksum(2bytes)
 *                    ^
 *                    packetend_ptr
 */
static void
process_packet(WASMGDBServer *server)
{
    uint8_t *inbuf = server->pkt.buf;
    int inbuf_size = server->pkt.size;
    uint8_t *packetend_ptr = (uint8_t *)memchr(inbuf, '#', inbuf_size);
    int packetend = packetend_ptr - inbuf;
    char request = inbuf[1];
    char *payload = NULL;
    uint8_t checksum = 0;

    if (packetend == 1) {
        LOG_VERBOSE("receive empty request, ignore it\n");
        return;
    }

    bh_assert('$' == inbuf[0]);
    inbuf[packetend] = '\0';

    for (int i = 1; i < packetend; i++)
        checksum += inbuf[i];
    bh_assert(checksum
              == (hex(inbuf[packetend + 1]) << 4 | hex(inbuf[packetend + 2])));

    payload = (char *)&inbuf[2];

    LOG_VERBOSE("receive request:%c %s\n", request, payload);
    handler_packet(server, request, payload);

    inbuf_erase_head(server, packetend + 3);
}

bool
wasm_gdbserver_handle_packet(WASMGDBServer *server)
{
    bool ret;
    ret = read_packet(server);
    if (ret)
        process_packet(server);
    return ret;
}
