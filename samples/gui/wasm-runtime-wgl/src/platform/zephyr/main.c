/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_platform_log.h"
#include "wasm_export.h"
#include "bh_memory.h"

extern int iwasm_main();

void main(void)
{
    iwasm_main();
    for(;;){
        k_sleep(1000);
    }
}

int time_get_ms()
{
    return k_uptime_get_32();
}
