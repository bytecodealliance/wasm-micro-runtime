/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm_app.h"

/* User global variable */
static int num = 0;
static user_timer_t g_timer;
static connection_t *g_conn = NULL;

void on_data1(connection_t *conn,
              conn_event_type_t type,
              const char *data,
              uint32 len,
              void *user_data)
{
    if (type == CONN_EVENT_TYPE_DATA) {
        char message[64] = {0};
        memcpy(message, data, len);
        printf("Client got a message from server -> %s\n", message);
    } else if (type == CONN_EVENT_TYPE_DISCONNECT) {
        printf("connection is close by server!\n");
    } else {
        printf("error: got unknown event type!!!\n");
    }
}

/* Timer callback */
void timer1_update(user_timer_t timer)
{
    char message[64] = {0};
    /* Reply to server */
    snprintf(message, sizeof(message), "Hello %d", num++);
    api_send_on_connection(g_conn, message, strlen(message));
}

void my_close_handler(request_t * request)
{
    response_t response[1];

    if (g_conn != NULL) {
        api_timer_cancel(g_timer);
        api_close_connection(g_conn);
    }
     
    make_response_for_request(request, response);
    set_response(response, DELETED_2_02, 0, NULL, 0);
    api_response_send(response);
}

void on_init()
{
    user_timer_t timer;
    attr_container_t *args;
    char *str = "this is client!";

    api_register_resource_handler("/close", my_close_handler);

    args = attr_container_create("");
    attr_container_set_string(&args, "address", "127.0.0.1");
    attr_container_set_uint16(&args, "port", 7777);

    g_conn = api_open_connection("TCP", args, on_data1, NULL);
    if (g_conn == NULL) {
        printf("connect to server fail!\n");
        return;
    }

    printf("connect to server success! handle: %p\n", g_conn);

    /* set up a timer */
    timer = api_timer_create(1000, true, false, timer1_update);
    api_timer_restart(timer, 1000);
}

void on_destroy()
{
    /* real destroy work including killing timer and closing sensor is
       accomplished in wasm app library version of on_destroy() */
}
