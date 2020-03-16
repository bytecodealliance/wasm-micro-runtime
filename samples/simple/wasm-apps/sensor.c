/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_app.h"
#include "wa-inc/sensor.h"

static sensor_t sensor = NULL;

/* Sensor event callback*/
void sensor_event_handler(sensor_t sensor, attr_container_t *event,
                          void *user_data)
{
    printf("### app get sensor event\n");
    attr_container_dump(event);
}

void on_init()
{
    char *user_data;
    attr_container_t *config;

    printf("### app on_init 1\n");
    /* open a sensor */
    user_data = malloc(100);
    printf("### app on_init 2\n");
    sensor = sensor_open("sensor_test", 0, sensor_event_handler, user_data);
    printf("### app on_init 3\n");

    /* config the sensor */
    sensor_config(sensor, 1000, 0, 0);
    printf("### app on_init 4\n");

    /*
     config = attr_container_create("sensor config");
     sensor_config(sensor, config);
     attr_container_destroy(config);
     */
}

void on_destroy()
{
    if (NULL != sensor) {
        sensor_config(sensor, 0, 0, 0);
    }
    /* real destroy work including killing timer and closing sensor is
       accomplished in wasm app library version of on_destroy() */
}
