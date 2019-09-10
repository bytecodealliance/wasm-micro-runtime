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

#include <stdlib.h>
#include <string.h>

#include "timer_wasm_app.h"
#include "timer_api.h"

#if 1
#include <stdio.h>
#else
#define printf (...)
#endif

struct user_timer {
    struct user_timer * next;
    int timer_id;
    void (*user_timer_callback)(user_timer_t);
};

struct user_timer * g_timers = NULL;

user_timer_t api_timer_create(int interval, bool is_period, bool auto_start,
        on_user_timer_update_f on_timer_update)
{

    int timer_id = wasm_create_timer(interval, is_period, auto_start);

    //TODO
    struct user_timer * timer = (struct user_timer *) malloc(
            sizeof(struct user_timer));
    if (timer == NULL) {
        // TODO: remove the timer_id
        printf("### api_timer_create malloc faild!!! \n");
        return NULL;
    }

    memset(timer, 0, sizeof(*timer));
    timer->timer_id = timer_id;
    timer->user_timer_callback = on_timer_update;

    if (g_timers == NULL)
        g_timers = timer;
    else {
        timer->next = g_timers;
        g_timers = timer;
    }

    return timer;
}

void api_timer_cancel(user_timer_t timer)
{
    user_timer_t t = g_timers, prev = NULL;

    wasm_timer_cancel(timer->timer_id);

    while (t) {
        if (t == timer) {
            if (prev == NULL) {
                g_timers = t->next;
                free(t);
            } else {
                prev->next = t->next;
                free(t);
            }
            return;
        } else {
            prev = t;
            t = t->next;
        }
    }
}

void api_timer_restart(user_timer_t timer, int interval)
{
    wasm_timer_restart(timer->timer_id, interval);
}

void on_timer_callback(int timer_id)
{
    struct user_timer * t = g_timers;

    while (t) {
        if (t->timer_id == timer_id) {
            t->user_timer_callback(t);
            break;
        }
        t = t->next;
    }
}

