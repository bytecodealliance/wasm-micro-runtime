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

#ifndef _MODULE_WASM_APP_H_
#define _MODULE_WASM_APP_H_

#include "bh_queue.h"
#include "app_manager_export.h"
#include "wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECTION_TYPE_USER 0
#define SECTION_TYPE_TYPE 1
#define SECTION_TYPE_IMPORT 2
#define SECTION_TYPE_FUNC 3
#define SECTION_TYPE_TABLE 4
#define SECTION_TYPE_MEMORY 5
#define SECTION_TYPE_GLOBAL 6
#define SECTION_TYPE_EXPORT 7
#define SECTION_TYPE_START 8
#define SECTION_TYPE_ELEM 9
#define SECTION_TYPE_CODE 10
#define SECTION_TYPE_DATA 11

enum {
    WASM_Msg_Start = BASE_EVENT_MAX, TIMER_EVENT_WASM, SENSOR_EVENT_WASM,

    WASM_Msg_End = WASM_Msg_Start + 100
};

typedef struct wasm_data {
    /* for easily access the containing wasm module */
    wasm_module_t wasm_module;
    wasm_module_inst_t wasm_module_inst;
    /* Permissions of the WASM app */
    char *perms;
    /*thread list mapped with this WASM module */
    korp_tid thread_id;
    /* for easily access the containing module data */
    module_data* m_data;
    /* section list of wasm bytecode */
    wasm_section_list_t sections;
} wasm_data;

/* sensor event */
typedef struct _sensor_event_data {
    uint32 sensor_id;

    int data_fmt;
    /* event of attribute container from context core */
    void *data;
} sensor_event_data_t;

/* WASM App File */
typedef struct wasm_app_file {
    /* magics */
    int magic;
    /* current version */
    int version;
    /* WASM section list */
    wasm_section_list_t sections;
    /* Last WASM section in the list */
    wasm_section_t *section_end;
} wasm_app_file_t;

extern module_interface wasm_app_module_interface;

typedef void (*message_type_handler_t)(module_data *m_data, bh_message_t msg);
extern bool wasm_register_msg_callback(int msg_type,
        message_type_handler_t message_handler);

typedef void (*resource_cleanup_handler_t)(uint32 module_id);
extern bool wasm_register_cleanup_callback(resource_cleanup_handler_t handler);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _MODULE_WASM_APP_H_ */
