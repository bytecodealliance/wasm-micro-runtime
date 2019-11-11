/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_types.h"
#include "app_manager_host.h"
#include "app_manager.h"
#include "app_manager_export.h"
#include "coap_ext.h"
#include "bh_memory.h"
#include "bh_thread.h"

/* host communication interface */
static host_interface host_commu;

/* IMRTLink Two leading bytes */
static unsigned char leadings[] = { (unsigned char) 0x12, (unsigned char) 0x34 };

/* IMRTLink Receiving Phase */
typedef enum recv_phase_t {
    Phase_Non_Start, Phase_Leading, Phase_Type, Phase_Size, Phase_Payload
} recv_phase_t;

/* IMRTLink Receive Context */
typedef struct recv_context_t {
    recv_phase_t phase;
    bh_link_msg_t message;
    int size_in_phase;
} recv_context_t;

/* Current IMRTLink receive context */
static recv_context_t recv_ctx;

/* Lock for device write */
static korp_mutex host_lock;

static bool enable_log = false;

static bool is_little_endian()
{
    long i = 0x01020304;
    unsigned char* c = (unsigned char*) &i;
    return (*c == 0x04) ? true : false;
}

static void exchange32(uint8* pData)
{
    uint8 value = *pData;
    *pData = *(pData + 3);
    *(pData + 3) = value;

    value = *(pData + 1);
    *(pData + 1) = *(pData + 2);
    *(pData + 2) = value;
}

/* return:
 *  1: complete message received
 *  0: incomplete message received
 */
static int on_imrt_link_byte_arrive(unsigned char ch, recv_context_t *ctx)
{
    if (ctx->phase == Phase_Non_Start) {
        ctx->message.payload_size = 0;

        if (ctx->message.payload) {
            bh_free(ctx->message.payload);
            ctx->message.payload = NULL;
        }

        if (ch == leadings[0]) {
            if (enable_log)
                app_manager_printf("##On byte arrive: got leading 0\n");
            ctx->phase = Phase_Leading;
        }

        return 0;
    } else if (ctx->phase == Phase_Leading) {
        if (ch == leadings[1]) {
            if (enable_log)
                app_manager_printf("##On byte arrive: got leading 1\n");
            ctx->phase = Phase_Type;
        } else
            ctx->phase = Phase_Non_Start;

        return 0;
    } else if (ctx->phase == Phase_Type) {
        if (ctx->size_in_phase++ == 0) {
            if (enable_log)
                app_manager_printf("##On byte arrive: got type 0\n");
            ctx->message.message_type = ch;
        } else {
            if (enable_log)
                app_manager_printf("##On byte arrive: got type 1\n");
            ctx->message.message_type |= (ch << 8);
            ctx->message.message_type = ntohs(ctx->message.message_type);
            ctx->phase = Phase_Size;
            ctx->size_in_phase = 0;
        }

        return 0;
    } else if (ctx->phase == Phase_Size) {
        unsigned char *p = (unsigned char *) &ctx->message.payload_size;

        if (enable_log)
            app_manager_printf("##On byte arrive: got payload_size, byte %d\n",
                    ctx->size_in_phase);
        p[ctx->size_in_phase++] = ch;

        if (ctx->size_in_phase == sizeof(ctx->message.payload_size)) {
#ifndef __ZEPHYR__
            ctx->message.payload_size = ntohl(ctx->message.payload_size);
#else
            if (is_little_endian())
            exchange32((uint8*)&ctx->message.payload_size);
#endif
            ctx->phase = Phase_Payload;

            if (enable_log)
                app_manager_printf("##On byte arrive: payload_size: %d\n",
                        ctx->message.payload_size);
            if (ctx->message.payload) {
                bh_free(ctx->message.payload);
                ctx->message.payload = NULL;
            }

            /* message completion */
            if (ctx->message.payload_size == 0) {
                ctx->phase = Phase_Non_Start;
                if (enable_log)
                    app_manager_printf(
                            "##On byte arrive: receive end, payload_size is 0.\n");
                return 1;
            }

            if (ctx->message.payload_size > 1024 * 1024) {
                ctx->phase = Phase_Non_Start;
                return 0;
            }

            if (ctx->message.message_type != INSTALL_WASM_BYTECODE_APP) {
                ctx->message.payload = (char *) bh_malloc(
                        ctx->message.payload_size);
                if (!ctx->message.payload) {
                    ctx->phase = Phase_Non_Start;
                    return 0;
                }
            }

            ctx->phase = Phase_Payload;
            ctx->size_in_phase = 0;
        }

        return 0;
    } else if (ctx->phase == Phase_Payload) {
        if (ctx->message.message_type == INSTALL_WASM_BYTECODE_APP) {
            int received_size;
            module_on_install_request_byte_arrive_func module_on_install =
                    g_module_interfaces[Module_WASM_App]->module_on_install;

            ctx->size_in_phase++;

            if (module_on_install != NULL) {
                if (module_on_install(ch, ctx->message.payload_size,
                        &received_size)) {
                    if (received_size == ctx->message.payload_size) {
                        /* whole wasm app received */
                        ctx->phase = Phase_Non_Start;
                        return 1;
                    }
                } else {
                    /* receive or handle fail */
                    ctx->phase = Phase_Non_Start;
                    return 0;
                }
                return 0;
            } else {
                ctx->phase = Phase_Non_Start;
                return 0;
            }
        } else {
            ctx->message.payload[ctx->size_in_phase++] = ch;

            if (ctx->size_in_phase == ctx->message.payload_size) {
                ctx->phase = Phase_Non_Start;
                if (enable_log)
                    app_manager_printf(
                            "##On byte arrive: receive end, payload_size is %d.\n",
                            ctx->message.payload_size);
                return 1;
            }
            return 0;
        }
    }

    return 0;
}

int aee_host_msg_callback(void *msg, uint16_t msg_len)
{
    unsigned char *p = msg, *p_end = p + msg_len;

    /*app_manager_printf("App Manager receive %d bytes from Host\n", msg_len);*/

    for (; p < p_end; p++) {
        int ret = on_imrt_link_byte_arrive(*p, &recv_ctx);

        if (ret == 1) {
            if (recv_ctx.message.payload) {
                int msg_type = recv_ctx.message.message_type;

                if (msg_type == REQUEST_PACKET) {
                    request_t request;
                    memset(&request, 0, sizeof(request));

                    if (!unpack_request(recv_ctx.message.payload,
                            recv_ctx.message.payload_size, &request))
                        continue;

                    request.sender = ID_HOST;

                    am_dispatch_request(&request);
                } else {
                    printf("unexpected host msg type: %d\n", msg_type);
                }

                bh_free(recv_ctx.message.payload);
                recv_ctx.message.payload = NULL;
                recv_ctx.message.payload_size = 0;
            }

            memset(&recv_ctx, 0, sizeof(recv_ctx));
        }
    }

    return 0;
}

bool app_manager_host_init(host_interface *interface)
{
    vm_mutex_init(&host_lock);
    memset(&recv_ctx, 0, sizeof(recv_ctx));

    host_commu.init = interface->init;
    host_commu.send = interface->send;
    host_commu.destroy = interface->destroy;

    if (host_commu.init != NULL)
      return host_commu.init();

    return true;
}

int app_manager_host_send_msg(int msg_type, const unsigned char *buf, int size)
{
    /* send an IMRT LINK message contains the buf as payload */
    if (host_commu.send != NULL) {
        int size_s = size, n;
        char header[16];

        vm_mutex_lock(&host_lock);
        /* leading bytes */
        bh_memcpy_s(header, 2, leadings, 2);

        /* message type */
        // todo: check if use network byte order!!!
        *((uint16*) (header + 2)) = htons(msg_type);

        /* payload length */
        if (is_little_endian())
            exchange32((uint8*) &size_s);

        bh_memcpy_s(header + 4, 4, &size_s, 4);
        n = host_commu.send(NULL, header, 8);
        if (n != 8) {
            vm_mutex_unlock(&host_lock);
            return 0;
        }

        /* payload */
        n = host_commu.send(NULL, buf, size);
        vm_mutex_unlock(&host_lock);

        printf("sent %d bytes to host\n", n);
        return n;
    } else {
        printf("no send api provided\n");
    }
    return 0;
}
