/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "wasm.h"
#include "wasm_application.h"
#include "wasm_interp.h"
#include "wasm_runtime.h"
#include "wasm_thread.h"
#include "wasm_assert.h"
#include "wasm_log.h"
#include "wasm_memory.h"
#include "wasm_platform_log.h"
#include "bh_common.h"


static WASMFunctionInstance*
resolve_main_function(const WASMModuleInstance *module_inst)
{
    uint32 i;
    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(module_inst->export_functions[i].name, "_main")
            || !strcmp(module_inst->export_functions[i].name, "main"))
            return module_inst->export_functions[i].function;

    LOG_ERROR("WASM execute application failed: main function not found.\n");
    return NULL;
}

static bool
check_main_func_type(const WASMType *type)
{
    if (!(type->param_count == 0 || type->param_count == 2)
        ||type->result_count > 1) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    if (type->param_count == 2
        && !(type->types[0] == VALUE_TYPE_I32
        && type->types[1] == VALUE_TYPE_I32)) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    if (type->result_count
        && type->types[type->param_count] != VALUE_TYPE_I32) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    return true;
}

#if WASM_ENABLE_WASI != 0
static WASMFunctionInstance *
lookup_wasi_start_function(WASMModuleInstance *module_inst)
{
    WASMFunctionInstance *func = NULL;
    uint32 i;
    for (i = 0; i < module_inst->export_func_count; i++) {
        if (!strcmp(module_inst->export_functions[i].name, "_start")) {
            func = module_inst->export_functions[i].function;
            if (func->u.func->func_type->param_count != 0
                || func->u.func->func_type->result_count != 0) {
                LOG_ERROR("Lookup wasi _start function failed: "
                          "invalid function type.\n");
                return NULL;
            }
            return func;
        }
    }
    return NULL;
}
#endif

bool
wasm_application_execute_main(WASMModuleInstance *module_inst,
                              int argc, char *argv[])
{
    WASMFunctionInstance *func;
    uint32 argc1 = 0, argv1[2] = { 0 };
    uint32 total_argv_size = 0;
    uint64 total_size;
    int32 argv_buf_offset, i;
    char *argv_buf, *p, *p_end;
    int32 *argv_offsets;

#if WASM_ENABLE_WASI != 0
    if (module_inst->module->is_wasi_module) {
        /* In wasi mode, we should call function named "_start"
           which initializes the wasi envrionment and then calls
           the actual main function. Directly call main function
           may cause exception thrown. */
        if ((func = lookup_wasi_start_function(module_inst)))
            return wasm_runtime_call_wasm(module_inst, NULL,
                                          func, 0, NULL);
        /* if no start function is found, we execute
           the main function as normal */
    }
#endif

    func = resolve_main_function(module_inst);
    if (!func || func->is_import_func) {
        wasm_runtime_set_exception(module_inst,
                                   "lookup main function failed.");
        return false;
    }

    if (!check_main_func_type(func->u.func->func_type)) {
        wasm_runtime_set_exception(module_inst,
                                   "invalid function type of main function.");
        return false;
    }

    if (func->u.func->func_type->param_count) {
        for (i = 0; i < argc; i++)
            total_argv_size += (uint32)(strlen(argv[i]) + 1);
        total_argv_size = align_uint(total_argv_size, 4);

        total_size = (uint64)total_argv_size + sizeof(int32) * (uint64)argc;

        if (total_size >= UINT32_MAX
            || !(argv_buf_offset =
                    wasm_runtime_module_malloc(module_inst, (uint32)total_size))) {
            wasm_runtime_set_exception(module_inst,
                                       "allocate memory failed.");
            return false;
        }

        argv_buf = p = wasm_runtime_addr_app_to_native(module_inst, argv_buf_offset);
        argv_offsets = (int32*)(p + total_argv_size);
        p_end = p + total_size;

        for (i = 0; i < argc; i++) {
            bh_memcpy_s(p, (uint32)(p_end - p), argv[i], (uint32)(strlen(argv[i]) + 1));
            argv_offsets[i] = argv_buf_offset + (int32)(p - argv_buf);
            p += strlen(argv[i]) + 1;
        }

        argc1 = 2;
        argv1[0] = (uint32)argc;
        argv1[1] = (uint32)wasm_runtime_addr_native_to_app(module_inst, argv_offsets);
    }

    return wasm_runtime_call_wasm(module_inst, NULL, func, argc1, argv1);
}

static WASMFunctionInstance*
resolve_function(const WASMModuleInstance *module_inst, char *name)
{
    uint32 i;
    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(module_inst->export_functions[i].name, name))
            return module_inst->export_functions[i].function;
    return NULL;
}

union ieee754_float {
    float f;

    /* This is the IEEE 754 single-precision format.  */
    union {
        struct {
            unsigned int negative:1;
            unsigned int exponent:8;
            unsigned int mantissa:23;
        } ieee_big_endian;
        struct {
            unsigned int mantissa:23;
            unsigned int exponent:8;
            unsigned int negative:1;
        } ieee_little_endian;
    } ieee;
};

union ieee754_double {
    double d;

    /* This is the IEEE 754 double-precision format.  */
    union {
        struct {
            unsigned int negative:1;
            unsigned int exponent:11;
            /* Together these comprise the mantissa.  */
            unsigned int mantissa0:20;
            unsigned int mantissa1:32;
        } ieee_big_endian;

        struct {
            /* Together these comprise the mantissa.  */
            unsigned int mantissa1:32;
            unsigned int mantissa0:20;
            unsigned int exponent:11;
            unsigned int negative:1;
        } ieee_little_endian;
    } ieee;
};

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

bool
wasm_application_execute_func(WASMModuleInstance *module_inst,
                              char *name, int argc, char *argv[])
{
    WASMFunctionInstance *func;
    WASMType *type;
    uint32 argc1, *argv1 = NULL;
    int32 i, p;
    uint64 total_size;
    const char *exception;
    char buf[128];

    wasm_assert(argc >= 0);
    func = resolve_function(module_inst, name);
    if (!func || func->is_import_func) {
        snprintf(buf, sizeof(buf), "lookup function %s failed.", name);
        wasm_runtime_set_exception(module_inst, buf);
        goto fail;
    }

    type = func->u.func->func_type;
    if (type->param_count != (uint32)argc) {
        wasm_runtime_set_exception(module_inst,
                                   "invalid input argument count.");
        goto fail;
    }

    argc1 = func->param_cell_num;
    total_size = sizeof(uint32) * (uint64)(argc1 > 2 ? argc1 : 2);
    if (total_size >= UINT32_MAX
        || (!(argv1 = wasm_malloc((uint32)total_size)))) {
        wasm_runtime_set_exception(module_inst, "allocate memory failed.");
        goto fail;
    }

    /* Clear errno before parsing arguments */
    errno = 0;

    /* Parse arguments */
    for (i = 0, p = 0; i < argc; i++) {
        char *endptr = NULL;
        wasm_assert(argv[i] != NULL);
        if (argv[i][0] == '\0') {
            snprintf(buf, sizeof(buf), "invalid input argument %d.", i);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
        switch (type->types[i]) {
            case VALUE_TYPE_I32:
                argv1[p++] = (uint32)strtoul(argv[i], &endptr, 0);
                break;
            case VALUE_TYPE_I64:
            {
                union { uint64 val; uint32 parts[2]; } u;
                u.val = strtoull(argv[i], &endptr, 0);
                argv1[p++] = u.parts[0];
                argv1[p++] = u.parts[1];
                break;
            }
            case VALUE_TYPE_F32:
            {
                float32 f32 = strtof(argv[i], &endptr);
                if (isnan(f32)) {
                    if (argv[i][0] == '-') {
                        f32 = -f32;
                    }
                    if (endptr[0] == ':') {
                        uint32 sig;
                        union ieee754_float u;
                        sig = (uint32)strtoul(endptr + 1, &endptr, 0);
                        u.f = f32;
                        if (is_little_endian())
                            u.ieee.ieee_little_endian.mantissa = sig;
                        else
                            u.ieee.ieee_big_endian.mantissa = sig;
                        memcpy(&f32, &u.f, sizeof(float));
                    }
                }
                memcpy(&argv1[p++], &f32, sizeof(float));
                break;
            }
            case VALUE_TYPE_F64:
            {
                union { float64 val; uint32 parts[2]; } u;
                u.val = strtod(argv[i], &endptr);
                if (isnan(u.val)) {
                    if (argv[i][0] == '-') {
                        u.val = -u.val;
                    }
                    if (endptr[0] == ':') {
                        uint64 sig;
                        union ieee754_double ud;
                        sig = strtoull(endptr + 1, &endptr, 0);
                        ud.d = u.val;
                        if (is_little_endian()) {
                            ud.ieee.ieee_little_endian.mantissa0 = sig >> 32;
                            ud.ieee.ieee_little_endian.mantissa1 = (uint32)sig;
                        }
                        else {
                            ud.ieee.ieee_big_endian.mantissa0 = sig >> 32;
                            ud.ieee.ieee_big_endian.mantissa1 = (uint32)sig;
                        }
                        memcpy(&u.val, &ud.d, sizeof(double));
                    }
                }
                argv1[p++] = u.parts[0];
                argv1[p++] = u.parts[1];
                break;
            }
        }
        if (endptr && *endptr != '\0' && *endptr != '_') {
            snprintf(buf, sizeof(buf), "invalid input argument %d: %s.",
                     i, argv[i]);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
        if (errno != 0) {
            snprintf(buf, sizeof(buf),
                     "prepare function argument error, errno: %d.", errno);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
    }
    wasm_assert(p == (int32)argc1);

    wasm_runtime_set_exception(module_inst, NULL);
    if (!wasm_runtime_call_wasm(module_inst, NULL, func, argc1, argv1)) {
        goto fail;
    }

    /* print return value */
    switch (type->types[type->param_count]) {
        case VALUE_TYPE_I32:
            wasm_printf("0x%x:i32", argv1[0]);
            break;
        case VALUE_TYPE_I64:
        {
            char buf[16];
            union { uint64 val; uint32 parts[2]; } u;
            u.parts[0] = argv1[0];
            u.parts[1] = argv1[1];
            if (sizeof(long) == 4)
                snprintf(buf, sizeof(buf), "%s", "0x%llx:i64");
            else
                snprintf(buf, sizeof(buf), "%s", "0x%lx:i64");
            wasm_printf(buf, u.val);
            break;
        }
        case VALUE_TYPE_F32:
            wasm_printf("%.7g:f32", *(float32*)argv1);
        break;
        case VALUE_TYPE_F64:
        {
            union { float64 val; uint32 parts[2]; } u;
            u.parts[0] = argv1[0];
            u.parts[1] = argv1[1];
            wasm_printf("%.7g:f64", u.val);
            break;
        }
    }
    wasm_printf("\n");

    wasm_free(argv1);
    return true;

fail:
    if (argv1)
        wasm_free(argv1);

    exception = wasm_runtime_get_exception(module_inst);
    wasm_assert(exception);
    wasm_printf("%s\n", exception);
    return false;
}

static bool
check_type(uint8 type, const char *p)
{
    const char *str = "i32";

    if (strlen(p) < 3)
        return false;

    switch (type) {
        case VALUE_TYPE_I32:
            str = "i32";
            break;
        case VALUE_TYPE_I64:
            str = "i64";
            break;
        case VALUE_TYPE_F32:
            str = "f32";
            break;
        case VALUE_TYPE_F64:
            str = "f64";
            break;
    }
    if (strncmp(p, str, 3))
        return false;

    return true;
}

static bool
check_function_type(const WASMType *type,
                    const char *signature)
{
    uint32 i;
    const char *p = signature;

    if (!p || *p++ != '(')
        return false;

    for (i = 0; i < type->param_count; i++) {
        if (!check_type(type->types[i], p))
            return false;
        p += 3;
    }

    if (*p++ != ')')
        return false;

    if (type->result_count) {
        if (!check_type(type->types[type->param_count], p))
            return false;
        p += 3;
    }

    if (*p != '\0')
        return false;

    return true;
}

WASMFunctionInstance*
wasm_runtime_lookup_function(const WASMModuleInstance *module_inst,
                             const char *name,
                             const char *signature)
{
    uint32 i;
    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(module_inst->export_functions[i].name, name)
            && check_function_type(
                module_inst->export_functions[i].function->u.func->func_type,
                signature))
            return module_inst->export_functions[i].function;
    return NULL;
}

