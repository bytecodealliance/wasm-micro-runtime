/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_assert.h"
#include "gc_type.h"
#include "wasm.h"
#include "wasm_export.h"
#include "gc_object.h"

static WASMStructObjectRef
check_struct_obj_type(wasm_exec_env_t exec_env, WASMObjectRef obj, int index,
                      uint8_t type)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    WASMStructType *struct_type;
    WASMRttTypeRef rtt_type;
    uint8_t field_type;

    if (!wasm_obj_is_struct_obj(obj)) {
        wasm_runtime_set_exception(
            module_inst, "can't access field of non-struct reference");
        return NULL;
    }

    rtt_type = (WASMRttTypeRef)wasm_object_header((WASMObjectRef)obj);
    struct_type = (WASMStructType *)rtt_type->defined_type;

    if (index < 0 || index >= struct_type->field_count) {
        wasm_runtime_set_exception(module_inst,
                                   "struct field index out of bounds");
        return NULL;
    }

    field_type = struct_type->fields[index].field_type;
    if (!((field_type == type)
          || (type == REF_TYPE_ANYREF && wasm_is_type_reftype(field_type)))) {
        wasm_runtime_set_exception(module_inst, "struct field type mismatch");
        return NULL;
    }

    return (WASMStructObjectRef)obj;
}

int struct_get_dyn_i32(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_I32);
    if (!struct_obj) {
        return 0;
    }

    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.i32;
}

long long struct_get_dyn_i64(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_I64);
    if (!struct_obj) {
        return 0;
    }

    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.i64;
}

float struct_get_dyn_f32(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_F32);
    if (!struct_obj) {
        return 0;
    }

    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.f32;
}

double struct_get_dyn_f64(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_F64);
    if (!struct_obj) {
        return 0;
    }

    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.f64;
}

void* struct_get_dyn_anyref(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, REF_TYPE_ANYREF);
    if (!struct_obj) {
        return NULL;
    }

    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.gc_obj;
}

void* struct_get_dyn_funcref(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index) {
    WASMValue result = { 0 };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, REF_TYPE_ANYREF);
    if (!struct_obj) {
        return NULL;
    }
    wasm_struct_obj_get_field(struct_obj, index, false, &result);

    return result.gc_obj;
}

void struct_set_dyn_i32(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, int value) {
    WASMValue val = { .i32 = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_I32);
    if (!struct_obj) {
        return;
    }

    wasm_struct_obj_set_field(struct_obj, index, &val);
}

void struct_set_dyn_i64(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, long long value) {
    WASMValue val = { .i64 = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_I64);
    if (!struct_obj) {
        return;
    }

    wasm_struct_obj_set_field(struct_obj, index, &val);
}

void struct_set_dyn_f32(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, float value) {
    WASMValue val = { .f32 = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_F32);
    if (!struct_obj) {
        return;
    }

    wasm_struct_obj_set_field(struct_obj, index, &val);
}

void struct_set_dyn_f64(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, double value) {
    WASMValue val = { .f64 = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, VALUE_TYPE_F64);
    if (!struct_obj) {
        return;
    }

    wasm_struct_obj_set_field(struct_obj, index, &val);
}

void struct_set_dyn_anyref(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, void* value) {
    WASMValue val = { .gc_obj = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, REF_TYPE_ANYREF);
    if (!struct_obj) {
        return;
    }

    wasm_struct_obj_set_field(struct_obj, index, &val);
}

void struct_set_dyn_funcref(wasm_exec_env_t exec_env, WASMAnyrefObjectRef obj, int index, void* value) {
    WASMValue val = { .gc_obj = value };
    WASMStructObjectRef struct_obj = check_struct_obj_type(
        exec_env, (WASMObjectRef)obj, index, REF_TYPE_ANYREF);
    if (!struct_obj) {
        return;
    }
    wasm_struct_obj_set_field(struct_obj, index, &val);
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name, signature, NULL }

static NativeSymbol native_symbols[] = {
    REG_NATIVE_FUNC(struct_get_dyn_i32, "(ri)i"),
    REG_NATIVE_FUNC(struct_get_dyn_i64, "(ri)I"),
    REG_NATIVE_FUNC(struct_get_dyn_f32, "(ri)f"),
    REG_NATIVE_FUNC(struct_get_dyn_f64, "(ri)F"),
    REG_NATIVE_FUNC(struct_get_dyn_anyref, "(ri)r"),
    REG_NATIVE_FUNC(struct_get_dyn_funcref, "(ri)r"),
    REG_NATIVE_FUNC(struct_set_dyn_i32, "(rii)"),
    REG_NATIVE_FUNC(struct_set_dyn_i64, "(riI)"),
    REG_NATIVE_FUNC(struct_set_dyn_f32, "(rif)"),
    REG_NATIVE_FUNC(struct_set_dyn_f64, "(riF)"),
    REG_NATIVE_FUNC(struct_set_dyn_anyref, "(rir)"),
    REG_NATIVE_FUNC(struct_set_dyn_funcref, "(rir)"),
};
/* clang-format on */

uint32_t
get_struct_dyn_symbols(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "libstructdyn";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
