/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <stdio.h>
#include <stdbool.h>
#include "display_indev.h"
#include "display.h"
#include "wasm_export.h"
#include "app_manager_export.h"

#define MONITOR_HOR_RES 320
#define MONITOR_VER_RES 240
#ifndef MONITOR_ZOOM
#define MONITOR_ZOOM        1
#endif

static int lcd_initialized = 0;

void
display_init(wasm_exec_env_t exec_env)
{
    if (lcd_initialized != 0) {
        return;
    }
    lcd_initialized = 1;
    xpt2046_init();
    ili9340_init();
    display_blanking_off(NULL);
}

void
display_flush(wasm_exec_env_t exec_env,
              int32_t x1, int32_t y1, int32_t x2, int32_t y2,
              int32 color_p_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    if (!wasm_runtime_validate_app_addr(module_inst, color_p_offset, 1))
        return;
    lv_color_t * color_p = wasm_runtime_addr_app_to_native(module_inst,
            color_p_offset);

    u16_t w = x2 - x1 + 1;
    u16_t h = y2 - y1 + 1;
    struct display_buffer_descriptor desc;

    desc.buf_size = 3 * w * h;
    desc.width = w;
    desc.pitch = w;
    desc.height = h;
    display_write(NULL, x1, y1, &desc, (void *) color_p);

    /*lv_flush_ready();*/
}

void
display_fill(wasm_exec_env_t exec_env,
             int32_t x1, int32_t y1, int32_t x2, int32_t y2,
             lv_color_t color_p)
{
}

void
display_map(wasm_exec_env_t exec_env,
            int32_t x1, int32_t y1, int32_t x2, int32_t y2,
            const lv_color_t * color_p)
{
}

bool
display_input_read(wasm_exec_env_t exec_env, int32 data_p_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    if (!wasm_runtime_validate_app_addr(module_inst, data_p_offset, 1))
        return false;
    lv_indev_data_t * data = wasm_runtime_addr_app_to_native(module_inst,
            data_p_offset);

    return touchscreen_read(data);
}

void
display_deinit(wasm_exec_env_t exec_env)
{
}

void
display_vdb_write(wasm_exec_env_t exec_env,
                  int32 buf_offset, lv_coord_t buf_w, lv_coord_t x,
                  lv_coord_t y, int32 color_p_offset, lv_opa_t opa)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    if (!wasm_runtime_validate_app_addr(module_inst, color_p_offset, 1))
        return;
    lv_color_t *color = wasm_runtime_addr_app_to_native(module_inst,
            color_p_offset);

    void *buf = wasm_runtime_addr_app_to_native(module_inst, buf_offset);

    u8_t *buf_xy = buf + 3 * x + 3 * y * buf_w;
    /*
     if (opa != LV_OPA_COVER) {
     lv_color_t mix_color;

     mix_color.red = *buf_xy;
     mix_color.green = *(buf_xy+1);
     mix_color.blue = *(buf_xy+2);
     color = lv_color_mix(color, mix_color, opa);
     }
     */
    *buf_xy = color->red;
    *(buf_xy + 1) = color->green;
    *(buf_xy + 2) = color->blue;
}

int
time_get_ms(wasm_exec_env_t exec_env)
{
    return k_uptime_get_32();
}

