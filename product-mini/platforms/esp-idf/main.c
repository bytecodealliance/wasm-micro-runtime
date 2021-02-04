/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"

extern void
iwasm_main(void);

static void
wamr_task(void *param)
{
    os_printf("WAMR task started\n");

    iwasm_main();

    while (1) {
        task_sleep(1000);
        /*os_printf("Hello WAMR\n");*/
    }
    (void)param;
}

static bool
app_init(uint32_t id)
{
    os_printf("WAMR init, id: %d\n", id);
    task_start("wamr_task", 8192, 4, wamr_task, NULL);
    return true;
}

static void
app_exit(uint32_t id)
{
    os_printf("WAMR exit, id: %d\n", id);
}

INTERNAL_APP_DEFINE("WAMR", APP_VERSION(0, 0, 0, 1), 0, app_init, app_exit);
