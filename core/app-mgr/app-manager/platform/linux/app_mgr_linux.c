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

#include "app_manager.h"

void*
app_manager_timer_create(void (*timer_callback)(void*),
        watchdog_timer *wd_timer)
{
    /* TODO */
    return NULL;
}

void app_manager_timer_destroy(void *timer)
{
    /* TODO */
}

void app_manager_timer_start(void *timer, int timeout)
{
    /* TODO */
}

void app_manager_timer_stop(void *timer)
{
    /* TODO */
}

watchdog_timer *
app_manager_get_wd_timer_from_timer_handle(void *timer)
{
    /* TODO */
    return NULL;
}

int app_manager_signature_verify(const uint8_t *file, unsigned int file_len,
        const uint8_t *signature, unsigned int sig_size)
{
    return 1;
}

