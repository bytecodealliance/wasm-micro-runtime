/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "gdbserver.h"
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
wasm_create_gdbserver(const char *host, int32 *port)
{
    bh_socket_t listen_fd = (bh_socket_t)-1;
    WASMGDBServer *server;

    bh_assert(port);

    if (!(server = wasm_runtime_malloc(sizeof(WASMGDBServer)))) {
        LOG_ERROR("wasm gdb server error: failed to allocate memory");
        return NULL;
    }

    memset(server, 0, sizeof(WASMGDBServer));

    if (0 != os_socket_create(&listen_fd, 1)) {
        LOG_ERROR("wasm gdb server error: create socket failed");
        goto fail;
    }

    if (0 != os_socket_bind(listen_fd, host, port)) {
        LOG_ERROR("wasm gdb server error: socket bind failed");
        goto fail;
    }

    LOG_WARNING("Debug server listening on %s:%" PRIu32 "\n", host, *port);
    server->listen_fd = listen_fd;

    return server;

fail:
    if (listen_fd >= 0) {
        os_socket_shutdown(listen_fd);
        os_socket_close(listen_fd);
    }
    if (server)
        wasm_runtime_free(server);
    return NULL;
}

bool
wasm_gdbserver_listen(WASMGDBServer *server)
{
    bh_socket_t sockt_fd = (bh_socket_t)-1;
    int32 ret;

    ret = os_socket_listen(server->listen_fd, 1);
    if (ret != 0) {
        LOG_ERROR("wasm gdb server error: socket listen failed");
        goto fail;
    }

    os_socket_accept(server->listen_fd, &sockt_fd, NULL, NULL);
    if (sockt_fd < 0) {
        LOG_ERROR("wasm gdb server error: socket accept failed");
        goto fail;
    }

    LOG_VERBOSE("accept gdb client");
    server->socket_fd = sockt_fd;
    server->noack = false;
    return true;

fail:
    os_socket_shutdown(server->listen_fd);
    os_socket_close(server->listen_fd);
    return false;
}

void
wasm_close_gdbserver(WASMGDBServer *server)
{
    if (server->socket_fd > 0) {
        os_socket_shutdown(server->socket_fd);
        os_socket_close(server->socket_fd);
    }
    if (server->listen_fd > 0) {
        os_socket_shutdown(server->listen_fd);
        os_socket_close(server->listen_fd);
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
    uint8 *inbuf = server->pkt.buf;
    int32 inbuf_size = server->pkt.size;
    uint8 *packetend_ptr = (uint8 *)memchr(inbuf, '#', inbuf_size);
    int32 packet_size = (int32)(uintptr_t)(packetend_ptr - inbuf);
    char request = inbuf[1];
    char *payload = NULL;
    uint8 checksum = 0;

    if (packet_size == 1) {
        LOG_VERBOSE("receive empty request, ignore it\n");
        return;
    }

    bh_assert('$' == inbuf[0]);
    inbuf[packet_size] = '\0';

    for (int i = 1; i < packet_size; i++)
        checksum += inbuf[i];
    bh_assert(
        checksum
        == (hex(inbuf[packet_size + 1]) << 4 | hex(inbuf[packet_size + 2])));

    payload = (char *)&inbuf[2];

    LOG_VERBOSE("receive request:%c %s\n", request, payload);
    handler_packet(server, request, payload);

    inbuf_erase_head(server, packet_size + 3);
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
