/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "module_wasm_app.h"

#include "native_interface.h" /* for request_t type */
#include "app_manager_host.h"
#include "bh_queue.h"
#include "attr_container.h"
#include "bh_thread.h"
#include "bh_memory.h"
#include "coap_ext.h"
#include "event.h"
#include "watchdog.h"
#include "runtime_lib.h"

/* Wasm app 4 magic bytes */
static unsigned char wasm_app_magics[] = {
    (unsigned char) 0x00,
    (unsigned char) 0x61,
    (unsigned char) 0x73,
    (unsigned char) 0x6d
};

/* Wasm app 4 version bytes */
static unsigned char wasm_app_version[] = {
    (unsigned char) 0x01,
    (unsigned char) 0x00,
    (unsigned char) 0x00,
    (unsigned char) 0x00
};

/* Wasm App Install Request Receiving Phase */
typedef enum wasm_app_install_req_recv_phase_t {
    Phase_Req_Ver,
    Phase_Req_Action,
    Phase_Req_Fmt,
    Phase_Req_Mid,
    Phase_Req_Sender,
    Phase_Req_Url_Len,
    Phase_Req_Payload_Len, /* payload is wasm app binary */
    Phase_Req_Url,
    Phase_Wasm_Magic,
    Phase_Wasm_Version,
    Phase_Wasm_Section_Type,
    Phase_Wasm_Section_Size,
    Phase_Wasm_Section_Content
} wasm_app_install_req_recv_phase_t;

/* Message for insall wasm app */
typedef struct install_wasm_app_msg_t {
    uint8_t request_version;
    uint8_t request_action;
    uint16_t request_fmt;
    uint32_t request_mid;
    uint32_t request_sender;
    uint16_t request_url_len;
    uint32_t wasm_app_size; /* payload size is just wasm app binary size */
    char *request_url;
    wasm_app_file_t wasm_app_binary;
} install_wasm_app_msg_t;

/* Wasm App Install Request Receive Context */
typedef struct wasm_app_install_req_recv_ctx_t {
    wasm_app_install_req_recv_phase_t phase;
    int size_in_phase;
    install_wasm_app_msg_t message;
    int total_received_size;
} wasm_app_install_req_recv_ctx_t;

/* Current wasm bytecode app install request receive context */
static wasm_app_install_req_recv_ctx_t recv_ctx;

static bool wasm_app_module_init(void);
static bool wasm_app_module_install(request_t *msg);
static bool wasm_app_module_uninstall(request_t *msg);
static void wasm_app_module_watchdog_kill(module_data *module_data);
static bool wasm_app_module_handle_host_url(void *queue_msg);
static module_data *wasm_app_module_get_module_data(void *inst);
static bool
wasm_app_module_on_install_request_byte_arrive(uint8 ch, int request_total_size,
                                               int *received_size);

static bool module_wasm_app_handle_install_msg(install_wasm_app_msg_t *message);
static void destroy_wasm_sections_list(wasm_section_t *sections);
static void destroy_wasm_section_from_list(wasm_section_t **sections, int type);

#define Max_Msg_Callback 10
int g_msg_type[Max_Msg_Callback] = { 0 };
message_type_handler_t g_msg_callbacks[Max_Msg_Callback] = { 0 };

#define Max_Cleanup_Callback 10
static resource_cleanup_handler_t
g_cleanup_callbacks[Max_Cleanup_Callback] = { 0 };

module_interface wasm_app_module_interface = {
    wasm_app_module_init,
    wasm_app_module_install,
    wasm_app_module_uninstall,
    wasm_app_module_watchdog_kill,
    wasm_app_module_handle_host_url,
    wasm_app_module_get_module_data,
    wasm_app_module_on_install_request_byte_arrive
};

static unsigned align_uint(unsigned v, unsigned b)
{
    unsigned m = b - 1;
    return (v + m) & ~m;
}

static wasm_function_inst_t
app_manager_lookup_function(const wasm_module_inst_t module_inst,
                            const char *name, const char *signature)
{
    wasm_function_inst_t func;

    func = wasm_runtime_lookup_function(module_inst, name, signature);
    if (!func && name[0] == '_')
        func = wasm_runtime_lookup_function(module_inst, name + 1, signature);
    return func;
}


static void app_instance_queue_callback(void *queue_msg, void *arg)
{
    uint32 argv[2];
    wasm_function_inst_t func_onRequest, func_onTimer;

    wasm_module_inst_t inst = (wasm_module_inst_t)arg;
    module_data *m_data = app_manager_get_module_data(Module_WASM_App, inst);
    int message_type = bh_message_type(queue_msg);

    bh_assert(m_data);

    if (message_type < BASE_EVENT_MAX) {
        switch (message_type) {
            case RESTFUL_REQUEST: {
                 request_t *request = (request_t *)bh_message_payload(queue_msg);
                 int size;
                 char *buffer;
                 int32 buffer_offset;

                 app_manager_printf("App %s got request, url %s, action %d\n",
                                    m_data->module_name,
                                    request->url,
                                    request->action);

                 func_onRequest = app_manager_lookup_function(inst,
                                                              "_on_request",
                                                              "(i32i32)");
                 if (!func_onRequest) {
                     app_manager_printf("Cannot find function onRequest\n");
                     break;
                 }

                 buffer = pack_request(request, &size);
                 if (buffer == NULL)
                     break;

                 buffer_offset = wasm_runtime_module_dup_data(inst, buffer, size);
                 if (buffer_offset == 0) {
                     const char *exception = wasm_runtime_get_exception(inst);
                     if (exception) {
                         app_manager_printf("Got exception running wasm code: %s\n",
                                            exception);
                         wasm_runtime_clear_exception(inst);
                     }
                     free_req_resp_packet(buffer);
                     break;
                 }

                 free_req_resp_packet(buffer);

                 argv[0] = (uint32) buffer_offset;
                 argv[1] = (uint32) size;

                 if (!wasm_runtime_call_wasm(inst, NULL, func_onRequest, 2, argv)) {
                     const char *exception = wasm_runtime_get_exception(inst);
                     bh_assert(exception);
                     app_manager_printf("Got exception running wasm code: %s\n",
                                        exception);
                     wasm_runtime_clear_exception(inst);
                     wasm_runtime_module_free(inst, buffer_offset);
                     break;
                 }

                 wasm_runtime_module_free(inst, buffer_offset);
                 app_manager_printf("Wasm app process request success.\n");
                 break;
            }
            case RESTFUL_RESPONSE: {
                 wasm_function_inst_t func_onResponse;
                 response_t *response = (response_t *) bh_message_payload(queue_msg);
                 int size;
                 char *buffer;
                 int32 buffer_offset;

                 app_manager_printf("App %s got response_t,status %d\n",
                                    m_data->module_name, response->status);

                 func_onResponse =
                     app_manager_lookup_function(inst, "_on_response", "(i32i32)");
                 if (!func_onResponse) {
                     app_manager_printf("Cannot find function on_response\n");
                     break;
                 }

                 buffer = pack_response(response, &size);
                 if (buffer == NULL)
                     break;

                 buffer_offset = wasm_runtime_module_dup_data(inst, buffer, size);
                 if (buffer_offset == 0) {
                     const char *exception = wasm_runtime_get_exception(inst);
                     if (exception) {
                         app_manager_printf("Got exception running wasm code: %s\n",
                                            exception);
                         wasm_runtime_clear_exception(inst);
                     }
                     free_req_resp_packet(buffer);
                     break;
                 }

                 free_req_resp_packet(buffer);

                 argv[0] = (uint32) buffer_offset;
                 argv[1] = (uint32) size;

                 if (!wasm_runtime_call_wasm(inst, NULL, func_onResponse, 2, argv)) {
                     const char *exception = wasm_runtime_get_exception(inst);
                     bh_assert(exception);
                     app_manager_printf("Got exception running wasm code: %s\n",
                                        exception);
                     wasm_runtime_clear_exception(inst);
                     wasm_runtime_module_free(inst, buffer_offset);
                     break;
                 }

                 wasm_runtime_module_free(inst, buffer_offset);
                 app_manager_printf("Wasm app process response success.\n");
                 break;
            }
            default: {
                 for (int i = 0; i < Max_Msg_Callback; i++) {
                     if (g_msg_type[i] == message_type) {
                         g_msg_callbacks[i](m_data, queue_msg);
                         return;
                     }
                 }
                 app_manager_printf("Invalid message type of WASM app queue message.\n");
                 break;

            }
        }
    }
    else {
        switch (message_type) {
            case TIMER_EVENT_WASM: {
                 unsigned int timer_id;
                 if (bh_message_payload(queue_msg)) {
                     /* Call Timer.callOnTimer() method */
                     func_onTimer =
                         app_manager_lookup_function(inst,
                                                      "_on_timer_callback",
                                                      "(i32)");

                     if (!func_onTimer) {
                         app_manager_printf("Cannot find function _on_timer_callback\n");
                         break;
                     }
                     timer_id =
                         (unsigned int)(uintptr_t)bh_message_payload(queue_msg);
                     argv[0] = timer_id;
                     if (!wasm_runtime_call_wasm(inst, NULL, func_onTimer, 1, argv)) {
                         const char *exception = wasm_runtime_get_exception(inst);
                         bh_assert(exception);
                         app_manager_printf("Got exception running wasm code: %s\n",
                                            exception);
                         wasm_runtime_clear_exception(inst);
                     }
                 }
                 break;
            }
            default: {
                 for (int i = 0; i < Max_Msg_Callback; i++) {
                     if (g_msg_type[i] == message_type) {
                         g_msg_callbacks[i](m_data, queue_msg);
                         return;
                     }
                 }
                 app_manager_printf("Invalid message type of WASM app queue message.\n");
                 break;
            }

        }
    }
}

/* WASM app thread main routine */
static void*
wasm_app_routine(void *arg)
{
    wasm_function_inst_t func_onInit;
    wasm_function_inst_t func_onDestroy;

    module_data *m_data = (module_data *) arg;
    wasm_data *wasm_app_data = (wasm_data*) m_data->internal_data;
    wasm_module_inst_t inst = wasm_app_data->wasm_module_inst;
    korp_tid thread = wasm_app_data->thread_id;

    /* Set m_data to the VM managed instance's custom data */
    wasm_runtime_set_custom_data(inst, m_data);

    app_manager_printf("WASM app '%s' started\n", m_data->module_name);

    /* Call app's onInit() method */
    func_onInit = app_manager_lookup_function(inst, "_on_init", "()");
    if (!func_onInit) {
        app_manager_printf("Cannot find function on_init().\n");
        goto fail1;
    }

    if (!wasm_runtime_call_wasm(inst, NULL, func_onInit, 0, NULL)) {
        const char *exception = wasm_runtime_get_exception(inst);
        bh_assert(exception);
        printf("Got exception running WASM code: %s\n",
               exception);
        wasm_runtime_clear_exception(inst);
        /* call on_destroy() in case some resources are opened in on_init()
         * and then exception thrown */
        goto fail2;
    }

    /* Enter queue loop run to receive and process applet queue message */
    bh_queue_enter_loop_run(m_data->queue, app_instance_queue_callback, inst);

    app_manager_printf("App instance main thread exit.\n");

fail2:
    /* Call WASM app onDestroy() method if there is */
    func_onDestroy = app_manager_lookup_function(inst, "_on_destroy", "()");
    if (func_onDestroy)
        wasm_runtime_call_wasm(inst, NULL, func_onDestroy, 0, NULL);

fail1:
    vm_thread_detach(thread);
    vm_thread_exit(NULL);

    return NULL;
}

static void cleanup_app_resource(module_data *m_data)
{
    int i;
    wasm_data *wasm_app_data = (wasm_data*) m_data->internal_data;

    am_cleanup_registeration(m_data->id);

    am_unregister_event(NULL, m_data->id);

    for (i = 0; i < Max_Cleanup_Callback; i++) {
        if (g_cleanup_callbacks[i] != NULL)
            g_cleanup_callbacks[i](m_data->id);
        else
            break;
    }

    wasm_runtime_deinstantiate(wasm_app_data->wasm_module_inst);

    /* Destroy remain sections (i.e. data segment section) from list. */
    destroy_wasm_sections_list(wasm_app_data->sections);

    if (wasm_app_data->wasm_module)
        wasm_runtime_unload(wasm_app_data->wasm_module);

    /* Destroy watchdog timer */
    watchdog_timer_destroy(&m_data->wd_timer);

    /* Remove module data from module data list and free it */
    app_manager_del_module_data(m_data);

}

/************************************************************/
/*        Module specific functions implementation          */
/************************************************************/

static bool wasm_app_module_init(void)
{
    /* Initialize WASM VM*/
    if (!wasm_runtime_init()) {
        app_manager_printf("WASM runtime environment initialization failed.\n");
        return false;
    }

    return true;
}

#define APP_NAME_MAX_LEN 128
#define MAX_INT_STR_LEN 11

static bool wasm_app_module_install(request_t * msg)
{
    unsigned int m_data_size, wasm_app_aot_file_len, heap_size;
    unsigned int timeout, timers, err_size;
    char *properties;
    int properties_offset, i;
    uint8 *wasm_app_aot_file;
    wasm_app_file_t *wasm_app_file;
    wasm_data *wasm_app_data;
    package_type_t package_type;
    module_data *m_data;
    wasm_module_t module = NULL;
    wasm_module_inst_t inst = NULL;
    char m_name[APP_NAME_MAX_LEN] = { 0 };
    char timeout_str[MAX_INT_STR_LEN] = { 0 };
    char heap_size_str[MAX_INT_STR_LEN] = { 0 };
    char timers_str[MAX_INT_STR_LEN] = { 0 }, err[256];
    /* Useless sections after load */
    uint8 sections1[] = {
        SECTION_TYPE_USER,
        SECTION_TYPE_TYPE,
        SECTION_TYPE_IMPORT,
        SECTION_TYPE_FUNC,
        SECTION_TYPE_TABLE,
        SECTION_TYPE_MEMORY,
        SECTION_TYPE_GLOBAL,
        SECTION_TYPE_EXPORT,
        SECTION_TYPE_START,
        SECTION_TYPE_ELEM,
        /*SECTION_TYPE_CODE,*/
        /*SECTION_TYPE_DATA*/
    };
    /* Useless sections after instantiate */
    uint8 sections2[] = { SECTION_TYPE_DATA };

    err_size = sizeof(err);

    /* Check payload */
    if (!msg->payload || msg->payload_len == 0) {
        SEND_ERR_RESPONSE(msg->mid, "Install WASM app failed: invalid wasm file.");
        return false;
    }

    /* Check app name */
    properties_offset = check_url_start(msg->url, strlen(msg->url), "/applet");
    bh_assert(properties_offset > 0);
    if (properties_offset <= 0)
        return false;
    properties = msg->url + properties_offset;
    find_key_value(properties, strlen(properties), "name", m_name,
                   sizeof(m_name) - 1, '&');

    if (strlen(m_name) == 0) {
        SEND_ERR_RESPONSE(msg->mid, "Install WASM app failed: invalid app name.");
        return false;
    }

    if (app_manager_lookup_module_data(m_name)) {
        SEND_ERR_RESPONSE(msg->mid, "Install WASM app failed: app already installed.");
        return false;
    }

    /* Parse heap size */
    heap_size = APP_HEAP_SIZE_DEFAULT;
    find_key_value(properties, strlen(properties), "heap", heap_size_str,
                   sizeof(heap_size_str) - 1, '&');
    if (strlen(heap_size_str) > 0) {
        heap_size = atoi(heap_size_str);
        if (heap_size < APP_HEAP_SIZE_MIN)
            heap_size = APP_HEAP_SIZE_MIN;
        else if (heap_size > APP_HEAP_SIZE_MAX)
            heap_size = APP_HEAP_SIZE_MAX;
    }

    /* Judge the app type is AOTed or not */
    package_type = get_package_type((uint8 *) msg->payload, msg->payload_len);

    /* Load WASM file and instantiate*/
    if (package_type == Wasm_Module_AoT) {
        wasm_app_aot_file = (uint8 *) msg->payload;
        wasm_app_aot_file_len = msg->payload_len;
        inst = wasm_runtime_load_aot(wasm_app_aot_file, wasm_app_aot_file_len,
                                     heap_size, err, err_size);
        if (!inst) {
            SEND_ERR_RESPONSE(msg->mid,
                              "Install WASM app failed: load wasm aot binary failed.");
            return false;
        }
    }
    else if (package_type == Wasm_Module_Bytecode) {
        wasm_app_file = (wasm_app_file_t *) msg->payload;
        module = wasm_runtime_load_from_sections(wasm_app_file->sections, err,
                                                 err_size);
        if (!module) {
            SEND_ERR_RESPONSE(msg->mid,
                              "Install WASM app failed: load WASM file failed.");
            printf("error: %s\n", err);
            destroy_wasm_sections_list(wasm_app_file->sections);
            return false;
        }

        /* Destroy useless sections from list after load */
        for (i = 0; i < sizeof(sections1); i++)
            destroy_wasm_section_from_list(&wasm_app_file->sections,
                                           sections1[i]);

        inst = wasm_runtime_instantiate(module, 0, heap_size, err, err_size);
        if (!inst) {
            SEND_ERR_RESPONSE(msg->mid,
                    "Install WASM app failed: instantiate wasm runtime failed.");
            printf("error: %s\n", err);
            wasm_runtime_unload(module);
            destroy_wasm_sections_list(wasm_app_file->sections);
            return false;
        }

        /* Destroy useless sections from list after instantiate */
        for (i = 0; i < sizeof(sections2); i++)
            destroy_wasm_section_from_list(&wasm_app_file->sections,
                                           sections2[i]);

    }
    else {
        SEND_ERR_RESPONSE(msg->mid,
                          "Install WASM app failed: invalid wasm package type.");
        return false;
    }

    /* Create module data including the wasm_app_data as its internal_data*/
    m_data_size = offsetof(module_data, module_name) + strlen(m_name) + 1;
    m_data_size = align_uint(m_data_size, 4);
    m_data = bh_malloc(m_data_size + sizeof(wasm_data));
    if (!m_data) {
        SEND_ERR_RESPONSE(msg->mid, "Install WASM app failed: allocate memory failed.");
        goto fail;
    }
    memset(m_data, 0, m_data_size + sizeof(wasm_data));

    m_data->module_type = Module_WASM_App;
    m_data->internal_data = (uint8*) m_data + m_data_size;
    wasm_app_data = (wasm_data*) m_data->internal_data;
    wasm_app_data->wasm_module_inst = inst;
    wasm_app_data->wasm_module = module;
    wasm_app_data->m_data = m_data;
    wasm_app_data->sections = wasm_app_file->sections;

    /* Set module data - name and module type */
    bh_strcpy_s(m_data->module_name, strlen(m_name) + 1, m_name);

    /* Set module data - execution timeout */
    timeout = DEFAULT_WATCHDOG_INTERVAL;
    find_key_value(properties, strlen(properties), "wd", timeout_str,
                   sizeof(timeout_str) - 1, '&');
    if (strlen(timeout_str) > 0)
        timeout = atoi(timeout_str);
    m_data->timeout = timeout;

    /* Set module data - create queue */
    m_data->queue = bh_queue_create();
    if (!m_data->queue) {
        SEND_ERR_RESPONSE(msg->mid, "Install WASM app failed: create app queue failed.");
        goto fail;
    }

    /* Set heap size */
    m_data->heap_size = heap_size;

    /* Set module data - timers number */
    timers = DEFAULT_TIMERS_PER_APP;
    find_key_value(properties, strlen(properties), "timers", timers_str,
                   sizeof(timers_str) - 1, '&');
    if (strlen(timers_str) > 0) {
        timers = atoi(timers_str);
        if (timers > MAX_TIMERS_PER_APP)
            timers = MAX_TIMERS_PER_APP;
    }

    /* Attention: must add the module before start the thread! */
    app_manager_add_module_data(m_data);

    m_data->timer_ctx = create_wasm_timer_ctx(m_data->id, timers);
    if (!m_data->timer_ctx) {
        SEND_ERR_RESPONSE(msg->mid,
                          "Install WASM app failed: create app timers failed.");
        goto fail;
    }

    /* Initialize watchdog timer */
    if (!watchdog_timer_init(m_data)) {
        SEND_ERR_RESPONSE(msg->mid,
                          "Install WASM app failed: create app watchdog timer failed.");
        goto fail;
    }

    /* Create WASM app thread. */
    if (vm_thread_create(&wasm_app_data->thread_id, wasm_app_routine,
                         (void*) m_data, APP_THREAD_STACK_SIZE_DEFAULT) != 0) {
        module_data_list_remove(m_data);
        SEND_ERR_RESPONSE(msg->mid,
                          "Install WASM app failed: create app threadf failed.");
        goto fail;
    }

    /* only when thread is created it is the flag of installation success */
    app_manager_post_applets_update_event();

    app_manager_printf("Install WASM app success!\n");
    send_error_response_to_host(msg->mid, CREATED_2_01, NULL); /* CREATED */

    return true;

fail:
    if (m_data)
        release_module(m_data);

    wasm_runtime_deinstantiate(inst);

    if (package_type == Wasm_Module_Bytecode)
        wasm_runtime_unload(module);

    if (package_type == Wasm_Module_Bytecode)
        destroy_wasm_sections_list(wasm_app_file->sections);

    return false;
}

/* Uninstall WASM app */
static bool wasm_app_module_uninstall(request_t *msg)
{
    module_data *m_data;
    wasm_data *wasm_app_data;
    char m_name[APP_NAME_MAX_LEN] = { 0 };
    char *properties;
    int properties_offset;

    properties_offset = check_url_start(msg->url, strlen(msg->url), "/applet");
    /* TODO: assert(properties_offset > 0) */
    if (properties_offset <= 0)
        return false;
    properties = msg->url + properties_offset;
    find_key_value(properties, strlen(properties), "name", m_name,
                   sizeof(m_name) - 1, '&');

    if (strlen(m_name) == 0) {
        SEND_ERR_RESPONSE(msg->mid, "Uninstall WASM app failed: invalid app name.");
        return false;
    }

    m_data = app_manager_lookup_module_data(m_name);
    if (!m_data) {
        SEND_ERR_RESPONSE(msg->mid, "Uninstall WASM app failed: no app found.");
        return false;
    }

    if (m_data->module_type != Module_WASM_App) {
        SEND_ERR_RESPONSE(msg->mid, "Uninstall WASM app failed: invalid module type.");
        return false;
    }

    if (m_data->wd_timer.is_interrupting) {
        SEND_ERR_RESPONSE(msg->mid,
                          "Uninstall WASM app failed: app is being interrupted by watchdog.");
        return false;
    }

    /* Exit app queue loop run */
    bh_queue_exit_loop_run(m_data->queue);

    /* Wait for wasm app thread to exit */
    wasm_app_data = (wasm_data*) m_data->internal_data;
    vm_thread_join(wasm_app_data->thread_id, NULL, -1);

    cleanup_app_resource(m_data);

    app_manager_post_applets_update_event();

    app_manager_printf("Uninstall WASM app successful!\n");

    send_error_response_to_host(msg->mid, DELETED_2_02, NULL); /* DELETED */
    return true;
}

static bool wasm_app_module_handle_host_url(void *queue_msg)
{
    //todo: implement in future
    app_manager_printf("App handles host url address %d\n",
                       (int)(uintptr_t)queue_msg);
    return false;
}

static module_data*
wasm_app_module_get_module_data(void *inst)
{
    wasm_module_inst_t module_inst = (wasm_module_inst_t)inst;
    return (module_data *)wasm_runtime_get_custom_data(module_inst);
}

static void wasm_app_module_watchdog_kill(module_data *m_data)
{
    //todo: implement in future
    app_manager_printf("Watchdog kills app: %s\n", m_data->module_name);
    return;
}

bool wasm_register_msg_callback(int message_type,
                                message_type_handler_t message_handler)
{
    int i;
    int freeslot = -1;
    for (i = 0; i < Max_Msg_Callback; i++) {
        // replace handler for the same event registered
        if (g_msg_type[i] == message_type)
            break;

        if (g_msg_callbacks[i] == NULL && freeslot == -1)
            freeslot = i;
    }

    if (i != Max_Msg_Callback)
        g_msg_callbacks[i] = message_handler;
    else if (freeslot != -1) {
        g_msg_callbacks[freeslot] = message_handler;
        g_msg_type[freeslot] = message_type;
    } else
        return false;

    return true;
}

bool wasm_register_cleanup_callback(resource_cleanup_handler_t handler)
{
    int i;

    for (i = 0; i < Max_Cleanup_Callback; i++) {
        if (g_cleanup_callbacks[i] == NULL) {
            g_cleanup_callbacks[i] = handler;
            return true;
        }
    }

    return false;
}

#define RECV_INTEGER(value, next_phase) do {        \
    unsigned char *p = (unsigned char *)&value;     \
    p[recv_ctx.size_in_phase++] = ch;               \
    if (recv_ctx.size_in_phase == sizeof(value)) {  \
      if (sizeof(value) == 4)                       \
        value = ntohl(value);                       \
      else if (sizeof(value) == 2)                  \
        value = ntohs(value);                       \
        recv_ctx.phase = next_phase;                \
        recv_ctx.size_in_phase = 0;                 \
    }                                               \
  } while(0)

/* return:
 * 1: whole wasm app arrived
 * 0: one valid byte arrived
 * -1: fail to process the byte arrived, e.g. allocate memory fail
 */
static bool
wasm_app_module_on_install_request_byte_arrive(uint8 ch, int request_total_size,
                                               int *received_size)
{
    if (recv_ctx.phase == Phase_Req_Ver) {
        recv_ctx.phase = Phase_Req_Ver;
        recv_ctx.size_in_phase = 0;
        recv_ctx.total_received_size = 0;
    }

    recv_ctx.total_received_size++;
    *received_size = recv_ctx.total_received_size;

    if (recv_ctx.phase == Phase_Req_Ver) {
        if (ch != 1 /* REQUES_PACKET_VER from restful_utils.c */)
            return false;
        recv_ctx.phase = Phase_Req_Action;
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Action) {
        recv_ctx.message.request_action = ch;
        recv_ctx.phase = Phase_Req_Fmt;
        recv_ctx.size_in_phase = 0;
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Fmt) {
        RECV_INTEGER(recv_ctx.message.request_fmt, Phase_Req_Mid);
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Mid) {
        RECV_INTEGER(recv_ctx.message.request_mid, Phase_Req_Sender);
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Sender) {
        RECV_INTEGER(recv_ctx.message.request_sender, Phase_Req_Url_Len);
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Url_Len) {
        unsigned char *p = (unsigned char *) &recv_ctx.message.request_url_len;

        p[recv_ctx.size_in_phase++] = ch;
        if (recv_ctx.size_in_phase ==
                sizeof(recv_ctx.message.request_url_len)) {
            recv_ctx.message.request_url_len =
                ntohs(recv_ctx.message.request_url_len);
            recv_ctx.message.request_url =
                bh_malloc(recv_ctx.message.request_url_len + 1);
            if (NULL == recv_ctx.message.request_url)
                goto fail;
            memset(recv_ctx.message.request_url, 0,
                   recv_ctx.message.request_url_len + 1);
            recv_ctx.phase = Phase_Req_Payload_Len;
            recv_ctx.size_in_phase = 0;
        }
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Payload_Len) {
        RECV_INTEGER(recv_ctx.message.wasm_app_size, Phase_Req_Url);
        return true;
    }
    else if (recv_ctx.phase == Phase_Req_Url) {
        recv_ctx.message.request_url[recv_ctx.size_in_phase++] = ch;
        if (recv_ctx.size_in_phase == recv_ctx.message.request_url_len) {
            recv_ctx.phase = Phase_Wasm_Magic;
            recv_ctx.size_in_phase = 0;
        }
        return true;
    }
    else if (recv_ctx.phase == Phase_Wasm_Magic) {
        /* start to receive wasm app binary */
        unsigned char *p =
                (unsigned char *) &recv_ctx.message.wasm_app_binary.magic;

        if (ch == wasm_app_magics[recv_ctx.size_in_phase])
            p[recv_ctx.size_in_phase++] = ch;
        else
            goto fail;

        if (recv_ctx.size_in_phase ==
                sizeof(recv_ctx.message.wasm_app_binary.magic)) {
            recv_ctx.phase = Phase_Wasm_Version;
            recv_ctx.size_in_phase = 0;
        }
        return true;
    }
    else if (recv_ctx.phase == Phase_Wasm_Version) {
        unsigned char *p =
                (unsigned char *) &recv_ctx.message.wasm_app_binary.version;

        if (ch == wasm_app_version[recv_ctx.size_in_phase])
            p[recv_ctx.size_in_phase++] = ch;
        else
            goto fail;

        if (recv_ctx.size_in_phase ==
                sizeof(recv_ctx.message.wasm_app_binary.version)) {
            recv_ctx.phase = Phase_Wasm_Section_Type;
            recv_ctx.size_in_phase = 0;
        }
        return true;
    }
    else if (recv_ctx.phase == Phase_Wasm_Section_Type) {
        wasm_section_t *new_section;

        if (!(new_section = (wasm_section_t *) bh_malloc(sizeof(wasm_section_t))))
            goto fail;

        memset(new_section, 0, sizeof(wasm_section_t));
        new_section->section_type = ch;
        new_section->next = NULL;

        /* add the section to tail of link list */
        if (NULL == recv_ctx.message.wasm_app_binary.sections) {
            recv_ctx.message.wasm_app_binary.sections = new_section;
            recv_ctx.message.wasm_app_binary.section_end = new_section;
        }
        else {
            recv_ctx.message.wasm_app_binary.section_end->next = new_section;
            recv_ctx.message.wasm_app_binary.section_end = new_section;
        }

        recv_ctx.phase = Phase_Wasm_Section_Size;
        recv_ctx.size_in_phase = 0;

        return true;
    }
    else if (recv_ctx.phase == Phase_Wasm_Section_Size) {
        /* the last section is the current receiving one */
        wasm_section_t *section = recv_ctx.message.wasm_app_binary.section_end;
        uint32 byte;

        bh_assert(section);

        byte = ch;

        section->section_body_size |= ((byte & 0x7f)
                << recv_ctx.size_in_phase * 7);
        recv_ctx.size_in_phase++;
        /* check leab128 overflow for uint32 value */
        if (recv_ctx.size_in_phase >
                (sizeof(section->section_body_size) * 8 + 7 - 1) / 7) {
            app_manager_printf(" LEB overflow when parsing section size\n");
            goto fail;
        }

        if ((byte & 0x80) == 0) {
            /* leb128 encoded section size parsed done */
            if (!(section->section_body = bh_malloc(section->section_body_size)))
                goto fail;
            recv_ctx.phase = Phase_Wasm_Section_Content;
            recv_ctx.size_in_phase = 0;
        }

        return true;
    }
    else if (recv_ctx.phase == Phase_Wasm_Section_Content) {
        /* the last section is the current receiving one */
        wasm_section_t *section = recv_ctx.message.wasm_app_binary.section_end;

        bh_assert(section);

        section->section_body[recv_ctx.size_in_phase++] = ch;

        if (recv_ctx.size_in_phase == section->section_body_size) {
            if (recv_ctx.total_received_size == request_total_size) {
                /* whole wasm app received */
                if (module_wasm_app_handle_install_msg(&recv_ctx.message)) {
                    bh_free(recv_ctx.message.request_url);
                    recv_ctx.message.request_url = NULL;
                    memset(&recv_ctx, 0, sizeof(recv_ctx));
                    return true;
                }
                else
                    goto fail;
            }
            else {
                recv_ctx.phase = Phase_Wasm_Section_Type;
                recv_ctx.size_in_phase = 0;
                return true;
            }
        }

        return true;
    }

fail:
    if (recv_ctx.message.wasm_app_binary.sections != NULL) {
        destroy_wasm_sections_list(recv_ctx.message.wasm_app_binary.sections);
        recv_ctx.message.wasm_app_binary.sections = NULL;
    }

    if (recv_ctx.message.request_url != NULL) {
        bh_free(recv_ctx.message.request_url);
        recv_ctx.message.request_url = NULL;
    }

    recv_ctx.phase = Phase_Req_Ver;
    recv_ctx.size_in_phase = 0;
    recv_ctx.total_received_size = 0;

    return false;
}

static bool module_wasm_app_handle_install_msg(install_wasm_app_msg_t *message)
{
    request_t *request = NULL;
    bh_message_t msg;

    request = (request_t *) bh_malloc(sizeof(request_t));
    if (request == NULL)
        return false;

    memset(request, 0, sizeof(*request));
    request->action = message->request_action;
    request->fmt = message->request_fmt;
    request->url = bh_strdup(message->request_url);
    request->sender = ID_HOST;
    request->mid = message->request_mid;
    request->payload_len = sizeof(message->wasm_app_binary);
    request->payload = bh_malloc(request->payload_len);

    if (request->url == NULL || request->payload == NULL) {
        request_cleaner(request);
        return false;
    }

    /* Request payload is set to wasm_app_file_t struct,
     * but not whole app buffer */
    bh_memcpy_s(request->payload, request->payload_len,
                &message->wasm_app_binary, request->payload_len);

    /* Since it's a wasm app install request, so directly post to app-mgr's
     * queue. The benefit is that section list can be freed when the msg
     * failed to post to app-mgr's queue. The defect is missing url check. */
    if (!(msg = bh_new_msg(RESTFUL_REQUEST, request, sizeof(*request),
                           request_cleaner))) {
        request_cleaner(request);
        return false;
    }

    if (!bh_post_msg2(get_app_manager_queue(), msg))
        return false;

    return true;
}

static void destroy_wasm_sections_list(wasm_section_t *sections)
{
    wasm_section_t *cur = sections;

    /* App-manager-host and module_wasm won't access the
     * section list concurrently, so need lock to protect. */

    while (cur) {
        wasm_section_t *next = cur->next;
        if (cur->section_body != NULL)
            bh_free(cur->section_body);
        bh_free(cur);
        cur = next;
    }
}

static void destroy_wasm_section_from_list(wasm_section_t **sections, int type)
{
    wasm_section_t *cur, *prev = NULL;

    /* App-manager-host and module_wasm won't access the
     * section list concurrently, so need lock to protect. */

    cur = *sections;

    while (cur) {
        wasm_section_t *next = cur->next;

        if (type == cur->section_type) {
            if (prev)
                prev->next = next;
            else
                *sections = next;

            if (cur->section_body != NULL)
                bh_free(cur->section_body);
            bh_free(cur);
            break;
        }
        else {
            prev = cur;
        }
        cur = next;
    }
}

