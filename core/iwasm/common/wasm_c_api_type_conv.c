/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_log.h"
#include "wasm_c_api_internal.h"
#include "wasm_export.h"

/* wasm_export.h types -> wasm_c_api.h types*/

wasm_valkind_t
val_type_rt_2_valkind(uint8 val_type_rt)
{
    switch (val_type_rt) {
#define WAMR_VAL_TYPE_2_WASM_VAL_KIND(name) \
    case VALUE_TYPE_##name:                 \
        return WASM_##name;

        WAMR_VAL_TYPE_2_WASM_VAL_KIND(I32)
        WAMR_VAL_TYPE_2_WASM_VAL_KIND(I64)
        WAMR_VAL_TYPE_2_WASM_VAL_KIND(F32)
        WAMR_VAL_TYPE_2_WASM_VAL_KIND(F64)
        WAMR_VAL_TYPE_2_WASM_VAL_KIND(V128)
        WAMR_VAL_TYPE_2_WASM_VAL_KIND(FUNCREF)
#undef WAMR_VAL_TYPE_2_WASM_VAL_KIND

        default:
            return WASM_EXTERNREF;
    }
}

/* wasm_c_api.h types -> wasm_export.h types*/
bool
valkind_to_WASMValueType(wasm_valkind_t src, uint8 *dst)
{
    switch (src) {
#define WASM_VAL_KIND_2_WAMR_VAL_TYPE(name) \
    case WASM_##name:                       \
    {                                       \
        *dst = VALUE_TYPE_##name;           \
        return true;                        \
    }

        WASM_VAL_KIND_2_WAMR_VAL_TYPE(I32)
        WASM_VAL_KIND_2_WAMR_VAL_TYPE(I64)
        WASM_VAL_KIND_2_WAMR_VAL_TYPE(F32)
        WASM_VAL_KIND_2_WAMR_VAL_TYPE(F64)
        WASM_VAL_KIND_2_WAMR_VAL_TYPE(V128)
        WASM_VAL_KIND_2_WAMR_VAL_TYPE(FUNCREF)
#undef WASM_VAL_KIND_2_WAMR_VAL_TYPE

        default:
        {
            bh_assert(0);
            return false;
        }
    }
}

wasm_import_export_kind_t
externkind_to_import_export_kind(wasm_externkind_t src)
{
    if (src == WASM_EXTERN_FUNC) {
        return WASM_IMPORT_EXPORT_KIND_FUNC;
    }
    else if (src == WASM_EXTERN_GLOBAL) {
        return WASM_IMPORT_EXPORT_KIND_GLOBAL;
    }
    else if (src == WASM_EXTERN_TABLE) {
        return WASM_IMPORT_EXPORT_KIND_TABLE;
    }
    else if (src == WASM_EXTERN_MEMORY) {
        return WASM_IMPORT_EXPORT_KIND_MEMORY;
    }
    else {
        bh_assert(0);
        LOG_ERROR("Invalid wasm_externkind_t value. Can't convert to "
                  "wasm_import_export_kind_t");
        return 0;
    }
}

bool
globaltype_to_WASMGlobalType(wasm_globaltype_t *src, WASMGlobalType *dst)
{
    if (!valkind_to_WASMValueType(src->val_type->kind, &dst->val_type)) {
        return false;
    }

    dst->is_mutable = src->mutability == WASM_VAR ? WASM_VAR : WASM_CONST;
    return true;
}

bool
val_to_WASMValue(wasm_val_t *src, WASMValue *dst)
{
    switch (src->kind) {
        case WASM_I32:
        {
            dst->i32 = src->of.i32;
            break;
        }
        case WASM_I64:
        {
            dst->i64 = src->of.i64;
            break;
        }
        case WASM_F32:
        {
            dst->f32 = src->of.f32;
            break;
        }
        case WASM_F64:
        {
            dst->f64 = src->of.f64;
            break;
        }
        case WASM_FUNCREF:
        case WASM_EXTERNREF:
        {
            LOG_WARNING("Unsupported value type: %d", src->kind);
            return false;
        }
        default:
        {
            return false;
        }
    }

    return true;
}

bool
extern_t_to_WASMExternInstance(const wasm_module_t *module, wasm_extern_t *src,
                               wasm_importtype_t *type, WASMExternInstance *dst)
{
    dst->module_name = wasm_importtype_module(type)->data;
    dst->field_name = wasm_importtype_name(type)->data;

    dst->kind = externkind_to_import_export_kind(wasm_extern_kind(src));

    switch (wasm_extern_kind(src)) {
        case WASM_EXTERN_FUNC:
        {
            wasm_func_t *function_in = wasm_extern_as_func(src);

            WASMFuncType type = { 0 };
            /*TODO: fill more fields? */
            type.param_count = (uint16)wasm_func_param_arity(function_in);
            type.result_count = (uint16)wasm_func_result_arity(function_in);

            void *function_host_ptr = function_in->with_env
                                          ? (void *)function_in->u.cb_env.cb
                                          : (void *)function_in->u.cb;

            dst->u.function =
                wasm_runtime_create_function(*module, &type, function_host_ptr);
            if (!dst->u.function) {
                return false;
            }
            break;
        }
        case WASM_EXTERN_GLOBAL:
        {
            wasm_global_t *global_in = wasm_extern_as_global(src);

            WASMGlobalType type = { 0 };
            if (!globaltype_to_WASMGlobalType(wasm_global_type(global_in),
                                              &type)) {
                return false;
            }

            dst->u.global =
                wasm_runtime_create_global_internal(*module, NULL, &type);
            if (!dst->u.global) {
                return false;
            }

            wasm_val_t val_in = { 0 };
            wasm_global_get(global_in, &val_in);

            WASMValue val_out = { 0 };
            if (!val_to_WASMValue(&val_in, &val_out)) {
                return false;
            }

            wasm_runtime_set_global_value(*module, dst->u.global, &val_out);
            break;
        }
        case WASM_EXTERN_MEMORY:
        {
            break;
        }
        case WASM_EXTERN_TABLE:
        {
            break;
        }
        default:
        {
            return false;
        }
    }

    return true;
}