/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>

#include "host_tool_utils.h"
#include "shared_utils.h"
#include "attr_container.h"
#include "coap_ext.h"
#include "cJSON.h"
#include "app_manager_export.h" /* for Module_WASM_App */
#include "host_link.h" /* for REQUEST_PACKET */
#include "transport.h"

#define BUF_SIZE 1024
#define TIMEOUT_EXIT_CODE 2
#define URL_MAX_LEN 256
#define DEFAULT_TIMEOUT_MS 5000
#define DEFAULT_ALIVE_TIME_MS 0

#define CONNECTION_MODE_TCP 1
#define CONNECTION_MODE_UART 2

typedef enum {
    INSTALL, UNINSTALL, QUERY, REQUEST, REGISTER, UNREGISTER
} op_type;

typedef struct {
    const char *file;
    const char *name;
    const char *module_type;
    int heap_size;
    /* max timers number */
    int timers;
    int watchdog_interval;
} inst_info;

typedef struct {
    const char *name;
    const char *module_type;
} uninst_info;

typedef struct {
    const char *name;
} query_info;

typedef struct {
    const char *url;
    int action;
    const char *json_payload_file;
} req_info;

typedef struct {
    const char *urls;
} reg_info;

typedef struct {
    const char *urls;
} unreg_info;

typedef union operation_info {
    inst_info inst;
    uninst_info uinst;
    query_info query;
    req_info req;
    reg_info reg;
    unreg_info unreg;
} operation_info;

typedef struct {
    op_type type;
    operation_info info;
} operation;

typedef enum REPLY_PACKET_TYPE {
    REPLY_TYPE_EVENT = 0, REPLY_TYPE_RESPONSE = 1
} REPLY_PACKET_TYPE;

static uint32_t g_timeout_ms = DEFAULT_TIMEOUT_MS;
static uint32_t g_alive_time_ms = DEFAULT_ALIVE_TIME_MS;
static char *g_redirect_file_name = NULL;
static int g_redirect_udp_port = -1;
static int g_conn_fd; /* may be tcp or uart */
static char *g_server_addr = "127.0.0.1";
static int g_server_port = 8888;
static char *g_uart_dev = "/dev/ttyS2";
static int g_baudrate = B115200;
static int g_connection_mode = CONNECTION_MODE_TCP;

extern int g_mid;
extern unsigned char leading[2];

/* -1 fail, 0 success */
static int send_request(request_t *request, bool is_install_wasm_bytecode_app)
{
    char *req_p;
    int req_size, req_size_n, ret = -1;
    uint16_t msg_type = REQUEST_PACKET;

    if (is_install_wasm_bytecode_app)
        msg_type = INSTALL_WASM_BYTECODE_APP;

    if ((req_p = pack_request(request, &req_size)) == NULL)
        return -1;

    /* leanding bytes */
    if (!host_tool_send_data(g_conn_fd, leading, sizeof(leading)))
        goto ret;

    /* message type */
    msg_type = htons(msg_type);
    if (!host_tool_send_data(g_conn_fd, (char *) &msg_type, sizeof(msg_type)))
        goto ret;

    /* payload length */
    req_size_n = htonl(req_size);
    if (!host_tool_send_data(g_conn_fd, (char *) &req_size_n,
            sizeof(req_size_n)))
        goto ret;

    /* payload */
    if (!host_tool_send_data(g_conn_fd, req_p, req_size))
        goto ret;

    ret = 0;

    ret: free_req_resp_packet(req_p);

    return ret;
}

static package_type_t get_app_package_type(const char *buf, int size)
{
    if (buf && size > 4) {
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 's' && buf[3] == 'm')
            return Wasm_Module_Bytecode;
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 'o' && buf[3] == 't')
            return Wasm_Module_AoT;
    }
    return Package_Type_Unknown;
}

#define url_remain_space (sizeof(url) - strlen(url))

/*return:
 0: success
 others: fail*/
static int install(inst_info *info)
{
    request_t request[1] = { 0 };
    char *app_file_buf;
    char url[URL_MAX_LEN] = { 0 };
    int ret = -1, app_size;
    bool is_wasm_bytecode_app;

    snprintf(url, sizeof(url) - 1, "/applet?name=%s", info->name);

    if (info->module_type != NULL && url_remain_space > 0)
        snprintf(url + strlen(url), url_remain_space, "&type=%s",
                info->module_type);

    if (info->heap_size > 0 && url_remain_space > 0)
        snprintf(url + strlen(url), url_remain_space, "&heap=%d",
                info->heap_size);

    if (info->timers > 0 && url_remain_space > 0)
        snprintf(url + strlen(url), url_remain_space, "&timers=%d",
                info->timers);

    if (info->watchdog_interval > 0 && url_remain_space > 0)
        snprintf(url + strlen(url), url_remain_space, "&wd=%d",
                info->watchdog_interval);

    /*TODO: permissions to access JLF resource: AUDIO LOCATION SENSOR VISION platform.SERVICE */

    if ((app_file_buf = read_file_to_buffer(info->file, &app_size)) == NULL)
        return -1;

    init_request(request, url, COAP_PUT,
    FMT_APP_RAW_BINARY, app_file_buf, app_size);
    request->mid = gen_random_id();

    if ((info->module_type == NULL || strcmp(info->module_type, "wasm") == 0)
            && get_app_package_type(app_file_buf, app_size) == Wasm_Module_Bytecode)
        is_wasm_bytecode_app = true;
    else
        is_wasm_bytecode_app = false;

    ret = send_request(request, is_wasm_bytecode_app);

    free(app_file_buf);

    return ret;
}

static int uninstall(uninst_info *info)
{
    request_t request[1] = { 0 };
    char url[URL_MAX_LEN] = { 0 };

    snprintf(url, sizeof(url) - 1, "/applet?name=%s", info->name);

    if (info->module_type != NULL && url_remain_space > 0)
        snprintf(url + strlen(url), url_remain_space, "&type=%s",
                info->module_type);

    init_request(request, url, COAP_DELETE,
    FMT_ATTR_CONTAINER,
    NULL, 0);
    request->mid = gen_random_id();

    return send_request(request, false);
}

static int query(query_info *info)
{
    request_t request[1] = { 0 };
    int ret = -1;
    char url[URL_MAX_LEN] = { 0 };

    if (info->name != NULL)
        snprintf(url, sizeof(url) - 1, "/applet?name=%s", info->name);
    else
        snprintf(url, sizeof(url) - 1, "/applet");

    init_request(request, url, COAP_GET,
    FMT_ATTR_CONTAINER,
    NULL, 0);
    request->mid = gen_random_id();

    ret = send_request(request, false);

    return ret;
}

static int request(req_info *info)
{
    request_t request[1] = { 0 };
    attr_container_t *payload = NULL;
    int ret = -1, payload_len = 0;

    if (info->json_payload_file != NULL) {
        char *payload_file;
        cJSON *json;
        int payload_file_size;

        if ((payload_file = read_file_to_buffer(info->json_payload_file,
                &payload_file_size)) == NULL)
            return -1;

        if (NULL == (json = cJSON_Parse(payload_file))) {
            free(payload_file);
            goto fail;
        }

        if (NULL == (payload = json2attr(json))) {
            cJSON_Delete(json);
            free(payload_file);
            goto fail;
        }
        payload_len = attr_container_get_serialize_length(payload);

        cJSON_Delete(json);
        free(payload_file);
    }

    init_request(request, (char *)info->url, info->action,
    FMT_ATTR_CONTAINER, payload, payload_len);
    request->mid = gen_random_id();

    ret = send_request(request, false);

    if (info->json_payload_file != NULL && payload != NULL)
        attr_container_destroy(payload);

    fail: return ret;
}

/*
 TODO: currently only support 1 url.
 how to handle multiple responses and set process's exit code?
 */
static int subscribe(reg_info *info)
{
    request_t request[1] = { 0 };
    int ret = -1;
#if 0
    char *p;

    p = strtok(info->urls, ",");
    while(p != NULL) {
        char url[URL_MAX_LEN] = {0};
        sprintf(url, "%s%s", "/event/", p);
        init_request(request,
                url,
                COAP_PUT,
                FMT_ATTR_CONTAINER,
                NULL,
                0);
        request->mid = gen_random_id();
        ret = send_request(request, false);
        p = strtok (NULL, ",");
    }
#else
    char url[URL_MAX_LEN] = { 0 };
    char *prefix = info->urls[0] == '/' ? "/event" : "/event/";
    sprintf(url, "%s%s", prefix, info->urls);
    init_request(request, url, COAP_PUT,
    FMT_ATTR_CONTAINER,
    NULL, 0);
    request->mid = gen_random_id();
    ret = send_request(request, false);
#endif
    return ret;
}

static int unsubscribe(unreg_info *info)
{
    request_t request[1] = { 0 };
    int ret = -1;
#if 0
    char *p;

    p = strtok(info->urls, ",");
    while(p != NULL) {
        char url[URL_MAX_LEN] = {0};
        sprintf(url, "%s%s", "/event/", p);
        init_request(request,
                url,
                COAP_DELETE,
                FMT_ATTR_CONTAINER,
                NULL,
                0);
        request->mid = gen_random_id();
        ret = send_request(request, false);
        p = strtok (NULL, ",");
    }
#else
    char url[URL_MAX_LEN] = { 0 };
    sprintf(url, "%s%s", "/event/", info->urls);
    init_request(request, url, COAP_DELETE,
    FMT_ATTR_CONTAINER,
    NULL, 0);
    request->mid = gen_random_id();
    ret = send_request(request, false);
#endif
    return ret;
}

static int init()
{
    if (g_connection_mode == CONNECTION_MODE_TCP) {
        int fd;
        if (!tcp_init(g_server_addr, g_server_port, &fd))
            return -1;
        g_conn_fd = fd;
        return 0;
    } else if (g_connection_mode == CONNECTION_MODE_UART) {
        int fd;
        if (!uart_init(g_uart_dev, g_baudrate, &fd))
            return -1;
        g_conn_fd = fd;
        return 0;
    }

    return -1;
}

static void deinit()
{
    close(g_conn_fd);
}

static int parse_action(const char *str)
{
    if (strcasecmp(str, "PUT") == 0)
        return COAP_PUT;
    if (strcasecmp(str, "GET") == 0)
        return COAP_GET;
    if (strcasecmp(str, "DELETE") == 0)
        return COAP_DELETE;
    if (strcasecmp(str, "POST") == 0)
        return COAP_POST;
    return -1;
}

static void showUsage()
{
    printf("\n");
    /*printf("Usage: host_tool [-i|--install]|[-u|--uninstall]|[-q|--query]|[-r|--request]|[-s|--register]|[-d|--deregister] ...\n");*/
    printf("Usage:\n\thost_tool -i|-u|-q|-r|-s|-d ...\n\n");

    printf("\thost_tool -i <App Name> -f <App File>\n"
            "\t\t [--type=<App Type>]\n"
            "\t\t [--heap=<Heap Size>]\n"
            "\t\t [--timers=<Timers Number>]\n"
            "\t\t [--watchdog=<Watchdog Interval>]\n"
            "\t\t [<Control Options> ...] \n");
    printf("\thost_tool -u <App Name> [<Control Options> ...]\n");
    printf("\thost_tool -q[<App Name>][<Control Options> ...]\n");
    printf(
            "\thost_tool -r <Resource URL> -A <Action> [-p <Payload File>] [<Control Options> ...]\n");
    printf("\thost_tool -s <Event URLs> [<Control Options> ...]\n");
    printf("\thost_tool -d <Event URLs> [<Control Options> ...]\n\n");

    printf(
            "\t-i, --install                           Install an application\n");
    printf(
            "\t-u, --uninstall                         Uninstall an application\n");
    printf(
            "\t-q, --query                             Query all applications\n");
    printf("\t-r, --request                           Send a request\n");
    printf("\t-s, --register                          Register event(s)\n");
    printf("\t-d, --deregister                        De-register event(s)\n");
    printf(
            "\t-f, --file                              Specify app binary file path\n");
    printf(
            "\t-A, --action                            Specify action of the request\n");
    printf(
            "\t-p, --payload                           Specify payload of the request\n");
    printf("\n");

    printf("\n\tControl Options:\n");
    printf(" \t-S <Address>|--address=<Address>          Set server address, default to 127.0.0.1\n");
    printf(" \t-P <Port>|--port=<Port>                   Set server port, default to 8888\n");
    printf(" \t-D <Device>|--uart=<Device>               Set uart device, default to /dev/ttyS2\n");
    printf(" \t-B <Baudrate>|--baudrate=<Baudrate>       Set uart device baudrate, default to 115200\n");

    printf(
            "\t-t <timeout>|--timeout=<timeout>          Operation timeout in ms, default to 5000\n");
    printf(
            "\t-a <alive_time>|--alive=<alive_time>      Alive time in ms after last operation done, default to 0\n");
    printf(
            "\t-o <output_file>|--output=<output_file>   Redirect the output to output a file\n");
    printf(
            "\t-U <udp_port>|--udp=<udp_port>            Redirect the output to an UDP port in local machine\n");

    printf("\nNotes:\n");
    printf("\t<App Name>=name of the application\n");
    printf("\t<App File>=path of the application binary file in wasm format\n");
    printf(
            "\t<Resource URL>=resource descriptor, such as /app/<App Name>/res1 or /res1\n");
    printf(
            "\t<Event URLs>=event url list separated by ',', such as /event1,/event2,/event3\n");
    printf(
            "\t<Action>=action of the request, can be PUT, GET, DELETE or POST (case insensitive)\n");
    printf("\t<Payload File>=path of the payload file in json format\n");
    printf("\t<App Type>=Type of app. Can be 'wasm'(default) or 'jeff'\n");
    printf("\t<Heap Size>=Heap size of app.\n");
    printf("\t<Timers Number>=Max timers number app can use.\n");
    printf("\t<Watchdog Interval>=Watchdog interval in ms.\n");
}

#define CHECK_DUPLICATE_OPERATION do{ \
  if (operation_parsed)               \
  {                                   \
    showUsage();                      \
    return false;                     \
  }                                   \
}while(0)

#define ERROR_RETURN do{   \
  showUsage();             \
  return false;            \
}while(0)

#define CHECK_ARGS_UNMATCH_OPERATION(op_type) do{ \
  if (!operation_parsed || op->type != op_type)   \
  {                                    \
    showUsage();                       \
    return false;                      \
  }                                    \
}while(0)

static bool parse_args(int argc, char *argv[], operation *op)
{
    int c;
    bool operation_parsed = false;
    bool conn_mode_parsed = false;

    while (1) {
        int optIndex = 0;
        static struct option longOpts[] = {
            { "install",    required_argument, NULL, 'i' },
            { "uninstall",  required_argument, NULL, 'u' },
            { "query",      optional_argument, NULL, 'q' },
            { "request",    required_argument, NULL, 'r' },
            { "register",   required_argument, NULL, 's' },
            { "deregister", required_argument, NULL, 'd' },
            { "timeout",    required_argument, NULL, 't' },
            { "alive",      required_argument, NULL, 'a' },
            { "output",     required_argument, NULL, 'o' },
            { "udp",        required_argument, NULL, 'U' },
            { "action",     required_argument, NULL, 'A' },
            { "file",       required_argument, NULL, 'f' },
            { "payload",    required_argument, NULL, 'p' },
            { "type",       required_argument, NULL, 0 },
            { "heap",       required_argument, NULL, 1 },
            { "timers",     required_argument, NULL, 2 },
            { "watchdog",   required_argument, NULL, 3 },
            { "address",    required_argument, NULL, 'S' },
            { "port",       required_argument, NULL, 'P' },
            { "uart_device",required_argument, NULL, 'D' },
            { "baudrate",   required_argument, NULL, 'B' },
            { "help",       required_argument, NULL, 'h' },
            { 0, 0, 0, 0 }
        };

        c = getopt_long(argc, argv, "i:u:q::r:s:d:t:a:o:U:A:f:p:S:P:D:B:h",
                longOpts, &optIndex);
        if (c == -1)
            break;

        switch (c) {
        case 'i':
            CHECK_DUPLICATE_OPERATION;
            op->type = INSTALL;
            op->info.inst.name = optarg;
            operation_parsed = true;
            break;
        case 'u':
            CHECK_DUPLICATE_OPERATION;
            op->type = UNINSTALL;
            op->info.uinst.name = optarg;
            operation_parsed = true;
            break;
        case 'q':
            CHECK_DUPLICATE_OPERATION;
            op->type = QUERY;
            op->info.query.name = optarg;
            break;
        case 'r':
            CHECK_DUPLICATE_OPERATION;
            op->type = REQUEST;
            op->info.req.url = optarg;
            operation_parsed = true;
            break;
        case 's':
            CHECK_DUPLICATE_OPERATION;
            op->type = REGISTER;
            op->info.reg.urls = optarg;
            operation_parsed = true;
            break;
        case 'd':
            CHECK_DUPLICATE_OPERATION;
            op->type = UNREGISTER;
            op->info.unreg.urls = optarg;
            operation_parsed = true;
            break;
        case 't':
            g_timeout_ms = atoi(optarg);
            break;
        case 'a':
            g_alive_time_ms = atoi(optarg);
            break;
        case 'o':
            g_redirect_file_name = optarg;
            break;
        case 'U':
            g_redirect_udp_port = atoi(optarg);
            break;
        case 'A':
            CHECK_ARGS_UNMATCH_OPERATION(REQUEST);
            op->info.req.action = parse_action(optarg);
            break;
        case 'f':
            CHECK_ARGS_UNMATCH_OPERATION(INSTALL);
            op->info.inst.file = optarg;
            break;
        case 'p':
            CHECK_ARGS_UNMATCH_OPERATION(REQUEST);
            op->info.req.json_payload_file = optarg;
            break;
            /* module type */
        case 0:
            /* TODO: use bit mask */
            /* CHECK_ARGS_UNMATCH_OPERATION(INSTALL | UNINSTALL); */
            if (op->type == INSTALL)
                op->info.inst.module_type = optarg;
            else if (op->type == UNINSTALL)
                op->info.uinst.module_type = optarg;
            break;
            /* heap */
        case 1:
            CHECK_ARGS_UNMATCH_OPERATION(INSTALL);
            op->info.inst.heap_size = atoi(optarg);
            break;
            /* timers */
        case 2:
            CHECK_ARGS_UNMATCH_OPERATION(INSTALL);
            op->info.inst.timers = atoi(optarg);
            break;
            /* watchdog */
        case 3:
            CHECK_ARGS_UNMATCH_OPERATION(INSTALL);
            op->info.inst.watchdog_interval = atoi(optarg);
            break;
        case 'S':
            if (conn_mode_parsed) {
                showUsage();
                return false;
            }
            g_connection_mode = CONNECTION_MODE_TCP;
            g_server_addr = optarg;
            conn_mode_parsed = true;
            break;
        case 'P':
            g_server_port = atoi(optarg);
            break;
        case 'D':
            if (conn_mode_parsed) {
                showUsage();
                return false;
            }
            g_connection_mode = CONNECTION_MODE_UART;
            g_uart_dev = optarg;
            conn_mode_parsed = true;
            break;
        case 'B':
            g_baudrate = parse_baudrate(atoi(optarg));
            break;
        case 'h':
            showUsage();
            return false;
        default:
            showUsage();
            return false;
        }
    }

    /* check mandatory options for the operation */
    switch (op->type) {
    case INSTALL:
        if (NULL == op->info.inst.file || NULL == op->info.inst.name)
            ERROR_RETURN;
        break;
    case UNINSTALL:
        if (NULL == op->info.uinst.name)
            ERROR_RETURN;
        break;
    case QUERY:
        break;
    case REQUEST:
        if (NULL == op->info.req.url || op->info.req.action <= 0)
            ERROR_RETURN;
        break;
    case REGISTER:
        if (NULL == op->info.reg.urls)
            ERROR_RETURN;
        break;
    case UNREGISTER:
        if (NULL == op->info.unreg.urls)
            ERROR_RETURN;
        break;
    default:
        return false;
    }

    return true;
}

/*
 return value:
 < 0: not complete message
 REPLY_TYPE_EVENT: event(request)
 REPLY_TYPE_RESPONSE: response
 */
static int preocess_reply_data(const char *buf, int len,
        imrt_link_recv_context_t *ctx)
{
    int result = -1;
    const char *pos = buf;

#if DEBUG
    int i = 0;
    for (; i < len; i++) {
        printf(" 0x%02x", buf[i]);
    }
    printf("\n");
#endif

    while (len-- > 0) {
        result = on_imrt_link_byte_arrive((unsigned char) *pos++, ctx);
        switch (result) {
        case 0: {
            imrt_link_message_t *message = &ctx->message;
            if (message->message_type == RESPONSE_PACKET)
                return REPLY_TYPE_RESPONSE;
            if (message->message_type == REQUEST_PACKET)
                return REPLY_TYPE_EVENT;
            break;
        }
        default:
            break;
        }
    }
    return -1;
}

static response_t *
parse_response_from_imrtlink(imrt_link_message_t *message, response_t *response)
{
    if (!unpack_response(message->payload, message->payload_size, response))
        return NULL;

    return response;
}

static request_t *
parse_event_from_imrtlink(imrt_link_message_t *message, request_t *request)
{
    if (!unpack_request(message->payload, message->payload_size, request))
        return NULL;

    return request;
}

static void output(const char *header, attr_container_t *payload, int foramt,
        int payload_len)
{
    cJSON *json = NULL;
    char *json_str = NULL;

    /* output the header */
    printf("%s", header);
    if (g_redirect_file_name != NULL)
        wirte_buffer_to_file(g_redirect_file_name, header, strlen(header));
    if (g_redirect_udp_port > 0 && g_redirect_udp_port < 65535)
        udp_send("127.0.0.1", g_redirect_udp_port, header, strlen(header));

    if (foramt != FMT_ATTR_CONTAINER || payload == NULL || payload_len <= 0)
        return;

    if ((json = attr2json(payload)) == NULL)
        return;

    if ((json_str = cJSON_Print(json)) == NULL) {
        cJSON_Delete(json);
        return;
    }

    /* output the payload as json format */
    printf("%s", json_str);
    if (g_redirect_file_name != NULL)
        wirte_buffer_to_file(g_redirect_file_name, json_str, strlen(json_str));
    if (g_redirect_udp_port > 0 && g_redirect_udp_port < 65535)
        udp_send("127.0.0.1", g_redirect_udp_port, json_str, strlen(json_str));

    free(json_str);
    cJSON_Delete(json);
}

static void output_response(response_t *obj)
{
    char header[32] = { 0 };
    snprintf(header, sizeof(header), "\nresponse status %d\n", obj->status);
    output(header, obj->payload, obj->fmt, obj->payload_len);
}

static void output_event(request_t *obj)
{
    char header[256] = { 0 };
    snprintf(header, sizeof(header), "\nreceived an event %s\n", obj->url);
    output(header, obj->payload, obj->fmt, obj->payload_len);
}

int main(int argc, char *argv[])
{
    int ret;
    imrt_link_recv_context_t recv_ctx = { 0 };
    char buffer[BUF_SIZE] = { 0 };
    uint32_t last_check, total_elpased_ms = 0;
    bool is_responsed = false;
    operation op;

    memset(&op, 0, sizeof(op));

    if (!parse_args(argc, argv, &op))
        return -1;

    //TODO: reconnect 3 times
    if (init() != 0)
        return -1;

    switch (op.type) {
    case INSTALL:
        ret = install((inst_info *) &op.info.inst);
        break;
    case UNINSTALL:
        ret = uninstall((uninst_info *) &op.info.uinst);
        break;
    case QUERY:
        ret = query((query_info *) &op.info.query);
        break;
    case REQUEST:
        ret = request((req_info *) &op.info.req);
        break;
    case REGISTER:
        ret = subscribe((reg_info *) &op.info.reg);
        break;
    case UNREGISTER:
        ret = unsubscribe((unreg_info *) &op.info.unreg);
        break;
    default:
        goto ret;
    }

    if (ret != 0)
        goto ret;

    bh_get_elpased_ms(&last_check);

    while (1) {
        int result = 0;
        fd_set readfds;
        struct timeval tv;

        total_elpased_ms += bh_get_elpased_ms(&last_check);

        if (!is_responsed) {
            if (total_elpased_ms >= g_timeout_ms) {
                output("operation timeout\n", NULL, 0, 0);
                ret = TIMEOUT_EXIT_CODE;
                goto ret;
            }
        } else {
            if (total_elpased_ms >= g_alive_time_ms) {
                /*ret = 0;*/
                goto ret;
            }
        }

        if (g_conn_fd == -1) {
            if (init() != 0) {
                sleep(1);
                continue;
            }
        }

        FD_ZERO(&readfds);
        FD_SET(g_conn_fd, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (result < 0) {
            if (errno != EINTR) {
                printf("Error in select, errno: 0x%x\n", errno);
                ret = -1;
                goto ret;
            }
        } else if (result == 0) { /* select timeout */
        } else if (result > 0) {
            int n;
            if (FD_ISSET(g_conn_fd, &readfds)) {
                int reply_type = -1;

                n = read(g_conn_fd, buffer, BUF_SIZE);
                if (n <= 0) {
                    g_conn_fd = -1;
                    continue;
                }

                reply_type = preocess_reply_data((char *) buffer, n, &recv_ctx);

                if (reply_type == REPLY_TYPE_RESPONSE) {
                    response_t response[1] = { 0 };

                    parse_response_from_imrtlink(&recv_ctx.message, response);

                    if (response->mid != g_mid) {
                        /* ignore invalid response */
                        continue;
                    }

                    is_responsed = true;
                    ret = response->status;
                    output_response(response);

                    if (op.type == REGISTER || op.type == UNREGISTER) {
                        /* alive time start */
                        total_elpased_ms = 0;
                        bh_get_elpased_ms(&last_check);
                    }
                } else if (reply_type == REPLY_TYPE_EVENT) {
                    request_t event[1] = { 0 };

                    parse_event_from_imrtlink(&recv_ctx.message, event);

                    if (op.type == REGISTER || op.type == UNREGISTER) {
                        output_event(event);
                    }
                }
            }
        }
    } /* end of while(1) */

    ret: if (recv_ctx.message.payload != NULL)
        free(recv_ctx.message.payload);

    deinit();

    return ret;
}
