/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "bh_platform.h"
#include "runtime_lib.h"
#include "native_interface.h"
#include "app_manager_export.h"
#include "board_config.h"
#include "bh_common.h"
#include "bh_queue.h"
#include "bh_thread.h"
#include "bh_memory.h"
#include "runtime_sensor.h"
#include "bi-inc/attr_container.h"
#include "module_wasm_app.h"
#include "wasm_export.h"
#include "sensor_native_api.h"
#include "connection_native_api.h"
#include "display_indev.h"

#include <zephyr.h>
#include <drivers/uart.h>
#include <device.h>


extern void init_sensor_framework();
extern void exit_sensor_framework();
extern int aee_host_msg_callback(void *msg, uint16_t msg_len);

int uart_char_cnt = 0;

static void uart_irq_callback(struct device *dev)
{
    unsigned char ch;

    while (uart_poll_in(dev, &ch) == 0) {
        uart_char_cnt++;
        aee_host_msg_callback(&ch, 1);
    }
}

struct device *uart_dev = NULL;

static bool host_init()
{
    uart_dev = device_get_binding(HOST_DEVICE_COMM_UART_NAME);
    if (!uart_dev) {
        printf("UART: Device driver not found.\n");
        return false;
    }
    uart_irq_rx_enable(uart_dev);
    uart_irq_callback_set(uart_dev, uart_irq_callback);
    return true;
}

int host_send(void * ctx, const char *buf, int size)
{
    if (!uart_dev)
        return 0;

    for (int i = 0; i < size; i++)
        uart_poll_out(uart_dev, buf[i]);

    return size;
}

void host_destroy()
{
}

host_interface interface = {
    .init = host_init,
    .send = host_send,
    .destroy = host_destroy
};

timer_ctx_t timer_ctx;

static char global_heap_buf[370 * 1024] = { 0 };

static NativeSymbol native_symbols[] = {
    #include "runtime_sensor.inl"
    #include "connection.inl"
    EXPORT_WASM_API_WITH_SIG(display_input_read, "(*)i"),
    EXPORT_WASM_API_WITH_SIG(display_flush, "(iiii*)"),
    EXPORT_WASM_API_WITH_SIG(display_fill, "(iiii*)"),
    EXPORT_WASM_API_WITH_SIG(display_vdb_write, "(*iii*i)"),
    EXPORT_WASM_API_WITH_SIG(display_map, "(iiii*)"),
    EXPORT_WASM_API_WITH_SIG(time_get_ms, "()i")
};

int iwasm_main()
{
    korp_thread tid, tm_tid;
    uint32 n_native_symbols;

    host_init();

    if (bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf))
            != 0) {
        printf("Init global heap failed.\n");
        return -1;
    }

    if (vm_thread_sys_init() != 0) {
        goto fail1;
    }

    /* Register native functions */
    n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    if (!wasm_runtime_register_natives("env",
                                       native_symbols, n_native_symbols)) {
        goto fail1;
    }

    display_init();

    // timer manager
    init_wasm_timer();

    // TODO:
    app_manager_startup(&interface);

fail1:
    bh_memory_destroy();
    return -1;
}
