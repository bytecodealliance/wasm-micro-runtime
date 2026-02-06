/*
 * Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>

#include "bh_platform.h"
#include "wasm_export.h"

typedef struct {
    const char *dir_list[8];
    uint32 dir_list_size;
    const char *map_dir_list[8];
    uint32 map_dir_list_size;
    const char *env_list[8];
    uint32 env_list_size;
    const char *addr_pool[8];
    uint32 addr_pool_size;
    const char *ns_lookup_pool[8];
    uint32 ns_lookup_pool_size;
} libc_wasi_parse_context_t;

typedef enum {
    LIBC_WASI_PARSE_RESULT_OK = 0,
    LIBC_WASI_PARSE_RESULT_NEED_HELP,
    LIBC_WASI_PARSE_RESULT_BAD_PARAM
} libc_wasi_parse_result_t;

typedef struct {
    const char *model_names[10];
    const uint32_t *encoding[10];
    const uint32_t *target[10];
    const char *graph_paths[10];
    uint32 n_graphs;
} wasi_nn_parse_context_t;

typedef enum {
    wasi_nn_openvino = 0,
    wasi_nn_onnx,
    wasi_nn_tensorflow,
    wasi_nn_pytorch,
    wasi_nn_tensorflowlite,
    wasi_nn_ggml,
    wasi_nn_autodetect,
    wasi_nn_unknown_backend,
} wasi_nn_encoding;

typedef enum wasi_nn_target {
    wasi_nn_cpu = 0,
    wasi_nn_gpu,
    wasi_nn_tpu,
    wasi_nn_unsupported_target,
} wasi_nn_target;

static wasi_nn_encoding
str2encoding(char *str_encoding)
{
    if (!str_encoding) {
        printf("Got empty string encoding");
        return -1;
    }

    if (!strcmp(str_encoding, "openvino"))
        return wasi_nn_openvino;
    else if (!strcmp(str_encoding, "tensorflowlite"))
        return wasi_nn_tensorflowlite;
    else if (!strcmp(str_encoding, "ggml"))
        return wasi_nn_ggml;
    else if (!strcmp(str_encoding, "onnx"))
        return wasi_nn_onnx;
    else
        return wasi_nn_unknown_backend;
    // return autodetect;
}

static wasi_nn_target
str2target(char *str_target)
{
    if (!str_target) {
        printf("Got empty string target");
        return -1;
    }

    if (!strcmp(str_target, "cpu"))
        return wasi_nn_cpu;
    else if (!strcmp(str_target, "gpu"))
        return wasi_nn_gpu;
    else if (!strcmp(str_target, "tpu"))
        return wasi_nn_tpu;
    else
        return wasi_nn_unsupported_target;
    // return autodetect;
}

static void
libc_wasi_print_help(void)
{
    printf("  --env=<env>              Pass wasi environment variables with "
           "\"key=value\"\n");
    printf("                           to the program, for example:\n");
    printf("                             --env=\"key1=value1\" "
           "--env=\"key2=value2\"\n");
    printf("  --dir=<dir>              Grant wasi access to the given host "
           "directories\n");
    printf("                           to the program, for example:\n");
    printf("                             --dir=<dir1> --dir=<dir2>\n");
    printf("  --map-dir=<guest::host>  Grant wasi access to the given host "
           "directories\n");
    printf("                           to the program at a specific guest "
           "path, for example:\n");
    printf("                             --map-dir=<guest-path1::host-path1> "
           "--map-dir=<guest-path2::host-path2>\n");
    printf("  --addr-pool=<addr/mask>  Grant wasi access to the given network "
           "addresses in\n");
    printf("                           CIDR notation to the program, separated "
           "with ',',\n");
    printf("                           for example:\n");
    printf("                             --addr-pool=1.2.3.4/15,2.3.4.5/16\n");
    printf("  --allow-resolve=<domain> Allow the lookup of the specific domain "
           "name or domain\n");
    printf("                           name suffixes using a wildcard, for "
           "example:\n");
    printf("                           --allow-resolve=example.com # allow the "
           "lookup of the specific domain\n");
    printf("                           --allow-resolve=*.example.com # allow "
           "the lookup of all subdomains\n");
    printf("                           --allow-resolve=* # allow any lookup\n");
}

static bool
validate_env_str(char *env)
{
    char *p = env;
    int key_len = 0;

    while (*p != '\0' && *p != '=') {
        key_len++;
        p++;
    }

    if (*p != '=' || key_len == 0)
        return false;

    return true;
}

libc_wasi_parse_result_t
libc_wasi_parse(char *arg, libc_wasi_parse_context_t *ctx)
{
    if (!strncmp(arg, "--dir=", 6)) {
        if (arg[6] == '\0')
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;
        if (ctx->dir_list_size >= sizeof(ctx->dir_list) / sizeof(char *)) {
            printf("Only allow max dir number %d\n",
                   (int)(sizeof(ctx->dir_list) / sizeof(char *)));
            return LIBC_WASI_PARSE_RESULT_BAD_PARAM;
        }
        ctx->dir_list[ctx->dir_list_size++] = arg + 6;
    }
    else if (!strncmp(arg, "--map-dir=", 10)) {
        if (arg[10] == '\0')
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;
        if (ctx->map_dir_list_size
            >= sizeof(ctx->map_dir_list) / sizeof(char *)) {
            printf("Only allow max map dir number %d\n",
                   (int)(sizeof(ctx->map_dir_list) / sizeof(char *)));
            return 1;
        }
        ctx->map_dir_list[ctx->map_dir_list_size++] = arg + 10;
    }
    else if (!strncmp(arg, "--env=", 6)) {
        char *tmp_env;

        if (arg[6] == '\0')
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;
        if (ctx->env_list_size >= sizeof(ctx->env_list) / sizeof(char *)) {
            printf("Only allow max env number %d\n",
                   (int)(sizeof(ctx->env_list) / sizeof(char *)));
            return LIBC_WASI_PARSE_RESULT_BAD_PARAM;
        }
        tmp_env = arg + 6;
        if (validate_env_str(tmp_env))
            ctx->env_list[ctx->env_list_size++] = tmp_env;
        else {
            printf("Wasm parse env string failed: expect \"key=value\", "
                   "got \"%s\"\n",
                   tmp_env);
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;
        }
    }
    /* TODO: parse the configuration file via --addr-pool-file */
    else if (!strncmp(arg, "--addr-pool=", strlen("--addr-pool="))) {
        /* like: --addr-pool=100.200.244.255/30 */
        char *token = NULL;

        if ('\0' == arg[12])
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;

        token = strtok(arg + strlen("--addr-pool="), ",");
        while (token) {
            if (ctx->addr_pool_size
                >= sizeof(ctx->addr_pool) / sizeof(char *)) {
                printf("Only allow max address number %d\n",
                       (int)(sizeof(ctx->addr_pool) / sizeof(char *)));
                return LIBC_WASI_PARSE_RESULT_BAD_PARAM;
            }

            ctx->addr_pool[ctx->addr_pool_size++] = token;
            token = strtok(NULL, ",");
        }
    }
    else if (!strncmp(arg, "--allow-resolve=", 16)) {
        if (arg[16] == '\0')
            return LIBC_WASI_PARSE_RESULT_NEED_HELP;
        if (ctx->ns_lookup_pool_size
            >= sizeof(ctx->ns_lookup_pool) / sizeof(ctx->ns_lookup_pool[0])) {
            printf("Only allow max ns lookup number %d\n",
                   (int)(sizeof(ctx->ns_lookup_pool)
                         / sizeof(ctx->ns_lookup_pool[0])));
            return LIBC_WASI_PARSE_RESULT_BAD_PARAM;
        }
        ctx->ns_lookup_pool[ctx->ns_lookup_pool_size++] = arg + 16;
    }
    else {
        return LIBC_WASI_PARSE_RESULT_NEED_HELP;
    }
    return LIBC_WASI_PARSE_RESULT_OK;
}

static void
libc_wasi_set_init_args(struct InstantiationArgs2 *args, int argc, char **argv,
                        libc_wasi_parse_context_t *ctx)
{
    wasm_runtime_instantiation_args_set_wasi_arg(args, argv, argc);
    wasm_runtime_instantiation_args_set_wasi_env(args, ctx->env_list,
                                                 ctx->env_list_size);
    wasm_runtime_instantiation_args_set_wasi_dir(
        args, ctx->dir_list, ctx->dir_list_size, ctx->map_dir_list,
        ctx->map_dir_list_size);
    wasm_runtime_instantiation_args_set_wasi_addr_pool(args, ctx->addr_pool,
                                                       ctx->addr_pool_size);
    wasm_runtime_instantiation_args_set_wasi_ns_lookup_pool(
        args, ctx->ns_lookup_pool, ctx->ns_lookup_pool_size);
}

#if WASM_ENABLE_WASI_NN != 0 || WASM_ENABLE_WASI_EPHEMERAL_NN != 0
libc_wasi_parse_result_t
wasi_nn_parse(char **argv, wasi_nn_parse_context_t *ctx)
{
    if ('\0' == argv[16])
        return LIBC_WASI_PARSE_RESULT_NEED_HELP;

    if (ctx->n_graphs >= sizeof(ctx->graph_paths) / sizeof(char *)) {
        printf("Only allow max graph number %d\n",
               (int)(sizeof(ctx->graph_paths) / sizeof(char *)));
        return LIBC_WASI_PARSE_RESULT_BAD_PARAM;
    }

    char *token;
    char *saveptr = NULL;
    int token_count = 0, ret = 0;
    char *tokens[12] = { 0 };

    // encoding:tensorflowlite|openvino|llama  target:cpu|gpu|tpu
    // --wasi-nn-graph=encoding1:target1:model_file_path1
    // --wasi-nn-graph=encoding2:target2:model_file_path2 ...
    token = strtok_r(argv[0] + 16, ":", &saveptr);
    while (token) {
        if (strlen(token) > 0) {
            tokens[token_count] = token;
            token_count++;
            token = strtok_r(NULL, ":", &saveptr);
        }
    }

    if (token_count != 4) {
        ret = LIBC_WASI_PARSE_RESULT_NEED_HELP;
        printf("4 arguments are needed for wasi-nn.\n");
        goto fail;
    }

    ctx->model_names[ctx->n_graphs] = tokens[0];
    ctx->encoding[ctx->n_graphs] = (uint32_t)str2encoding(tokens[1]);
    ctx->target[ctx->n_graphs] = (uint32_t)str2target(tokens[2]);
    ctx->graph_paths[ctx->n_graphs] = tokens[3];

    if ((!ctx->model_names[ctx->n_graphs])
        || (ctx->encoding[ctx->n_graphs] == wasi_nn_unknown_backend)
        || (ctx->target[ctx->n_graphs] == wasi_nn_unsupported_target)) {
        ret = LIBC_WASI_PARSE_RESULT_NEED_HELP;
        printf("Invalid arguments for wasi-nn.\n");
        goto fail;
    }

    ctx->n_graphs++;
fail:

    return ret;
}

static void
wasi_nn_set_init_args(struct InstantiationArgs2 *args,
                      struct WASINNRegistry *nn_registry,
                      wasi_nn_parse_context_t *ctx)
{
    wasm_runtime_wasi_nn_registry_set_args(nn_registry, ctx->model_names,
                                           ctx->encoding, ctx->target,
                                           ctx->n_graphs, ctx->graph_paths);
    wasm_runtime_instantiation_args_set_wasi_nn_registry(args, nn_registry);
}
#endif