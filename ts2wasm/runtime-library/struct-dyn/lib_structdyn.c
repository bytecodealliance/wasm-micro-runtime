/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include <stdlib.h>

int struct_get_dyn_i32(wasm_exec_env_t exec_env, int obj, int index) {
    // TODO
    return 0;
}

long long struct_get_dyn_i64(wasm_exec_env_t exec_env, int obj, int index) {
    // TODO
    return 0;
}

float struct_get_dyn_f32(wasm_exec_env_t exec_env, int obj, int index) {
    // TODO
    return 0;
}

double struct_get_dyn_f64(wasm_exec_env_t exec_env, int obj, int index) {
    // TODO
    return 0;
}

void* struct_get_dyn_anyref(wasm_exec_env_t exec_env, int obj, int index) {
    // TODO
    return malloc(0);
}

void struct_set_dyn_i32(wasm_exec_env_t exec_env, int obj, int index, int value) {
    // TODO
}

void struct_set_dyn_i64(wasm_exec_env_t exec_env, int obj, int index, long long value) {
    // TODO
}

void struct_set_dyn_f32(wasm_exec_env_t exec_env, int obj, int index, float value) {
    // TODO
}

void struct_set_dyn_f64(wasm_exec_env_t exec_env, int obj, int index, double value) {
    // TODO
}

void struct_set_dyn_anyref(wasm_exec_env_t exec_env, int obj, int index, void* value) {
    // TODO
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name, signature, NULL }

static NativeSymbol native_symbols[] = {
    REG_NATIVE_FUNC(struct_get_dyn_i32, "(ii)i"),
    REG_NATIVE_FUNC(struct_get_dyn_i64, "(ii)I"),
    REG_NATIVE_FUNC(struct_get_dyn_f32, "(iif"),
    REG_NATIVE_FUNC(struct_get_dyn_f64, "(ii)F"),
    REG_NATIVE_FUNC(struct_get_dyn_anyref, "(ii)i"),
    REG_NATIVE_FUNC(struct_set_dyn_i32, "(iii)"),
    REG_NATIVE_FUNC(struct_set_dyn_i64, "(iiI)"),
    REG_NATIVE_FUNC(struct_set_dyn_f32, "(iif)"),
    REG_NATIVE_FUNC(struct_set_dyn_f64, "(iiF)"),
    REG_NATIVE_FUNC(struct_set_dyn_anyref, "(iii)"),
    /* TODO */
};
/* clang-format on */

uint32_t
get_native_lib(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "libdstructdyn";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
