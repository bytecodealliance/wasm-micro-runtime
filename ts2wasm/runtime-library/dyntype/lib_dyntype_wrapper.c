/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include "wasm_export.h"
#include <stdlib.h>

/******************* Initialization and destroy *******************/
dyn_ctx_t dyntype_context_init_wrapper(wasm_exec_env_t exec_env) {
  return dyntype_context_init();
}

dyn_ctx_t dyntype_context_init_with_opt_wrapper(wasm_exec_env_t exec_env,
                                                dyn_options_t *options) {
  return dyntype_context_init_with_opt(options);
}

void dyntype_context_destroy_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx) {
  return dyntype_context_destroy(ctx);
}

/******************* Field access *******************/
dyn_value_t dyntype_new_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                       double value) {
  return dyntype_new_number(ctx, value);
}

dyn_value_t dyntype_new_boolean_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                        bool value) {
  return dyntype_new_boolean(ctx, value);
}

dyn_value_t dyntype_new_string_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                       const char *str) {
  return dyntype_new_string(ctx, str);
}

dyn_value_t dyntype_new_undefined_wrapper(wasm_exec_env_t exec_env,
                                          dyn_ctx_t ctx) {
  return dyntype_new_undefined(ctx);
}

dyn_value_t dyntype_new_null_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx) {
  return dyntype_new_null(ctx);
}

dyn_value_t dyntype_new_object_wrapper(wasm_exec_env_t exec_env,
                                       dyn_ctx_t ctx) {
  return dyntype_new_object(ctx);
}

dyn_value_t dyntype_new_array_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx) {
  return dyntype_new_array(ctx);
}

dyn_value_t dyntype_new_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                       void *ptr, external_ref_tag tag) {
  return dyntype_new_extref(ctx, ptr, tag);
}

int dyntype_set_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                 dyn_value_t obj, const char *prop,
                                 dyn_value_t value) {
  return dyntype_set_property(ctx, obj, prop, value);
}

int dyntype_define_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                    dyn_value_t obj, const char *prop,
                                    dyn_value_t desc) {
  return dyntype_define_property(ctx, obj, prop, desc);
}

dyn_value_t dyntype_get_property_wrapper(wasm_exec_env_t exec_env,
                                         dyn_ctx_t ctx, dyn_value_t obj,
                                         const char *prop) {
  return dyntype_get_property(ctx, obj, prop);
}

int dyntype_has_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                 dyn_value_t obj, const char *prop) {
  return dyntype_has_property(ctx, obj, prop);
}

int dyntype_delete_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                    dyn_value_t obj, const char *prop) {
  return dyntype_delete_property(ctx, obj, prop);
}

/******************* Runtime type checking *******************/
bool dyntype_is_undefined_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                  dyn_value_t obj) {
  return dyntype_is_undefined(ctx, obj);
}
bool dyntype_is_null_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj) {
  return dyntype_is_null(ctx, obj);
}

bool dyntype_is_bool_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj) {
  return dyntype_is_bool(ctx, obj);
}
int dyntype_to_bool_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                            dyn_value_t bool_obj, bool *pres) {
  return dyntype_to_bool(ctx, bool_obj, pres);
}

bool dyntype_is_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                               dyn_value_t obj) {
  return dyntype_is_number(ctx, obj);
}

int dyntype_to_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                              dyn_value_t obj, double *pres) {
  return dyntype_to_number(ctx, obj, pres);
}

bool dyntype_is_string_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                               dyn_value_t obj) {
  return dyntype_is_string(ctx, obj);
}

int dyntype_to_cstring_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                               dyn_value_t str_obj, char **pres) {
  return dyntype_to_cstring(ctx, str_obj, pres);
}

void dyntype_free_cstring_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                  char *str) {
  return dyntype_free_cstring(ctx, str);
}

bool dyntype_is_object_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                               dyn_value_t obj) {
  return dyntype_is_object(ctx, obj);
}

bool dyntype_is_array_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                              dyn_value_t obj) {
  return dyntype_is_array(ctx, obj);
}

bool dyntype_is_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                               dyn_value_t obj) {
  return dyntype_is_extref(ctx, obj);
}

int dyntype_to_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                              dyn_value_t obj, void **pres) {
  return dyntype_to_extref(ctx, obj, pres);
}

/******************* Type equivalence *******************/
dyn_type_t dyntype_typeof_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                  dyn_value_t obj) {
  return dyntype_typeof(ctx, obj);
}

bool dyntype_type_eq_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t lhs, dyn_value_t rhs) {
  return dyntype_type_eq(ctx, lhs, rhs);
}

/******************* Subtyping *******************/
dyn_value_t dyntype_new_object_with_proto_wrapper(wasm_exec_env_t exec_env,
                                                  dyn_ctx_t ctx,
                                                  const dyn_value_t proto_obj) {
  return dyntype_new_object_with_proto(ctx, proto_obj);
}

int dyntype_set_prototype_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                  dyn_value_t obj,
                                  const dyn_value_t proto_obj) {
  return dyntype_set_prototype(ctx, obj, proto_obj);
}

const dyn_value_t dyntype_get_prototype_wrapper(wasm_exec_env_t exec_env,
                                                dyn_ctx_t ctx,
                                                dyn_value_t obj) {
  return dyntype_get_prototype(ctx, obj);
}

dyn_value_t dyntype_get_own_property_wrapper(wasm_exec_env_t exec_env,
                                             dyn_ctx_t ctx, dyn_value_t obj,
                                             const char *prop) {
  return dyntype_get_own_property(ctx, obj, prop);
}

bool dyntype_instanceof_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                const dyn_value_t src_obj,
                                const dyn_value_t dst_obj) {
  return dyntype_instanceof(ctx, src_obj, dst_obj);
}

/******************* Dumping *******************/
void dyntype_dump_value_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                dyn_value_t obj) {
  return dyntype_dump_value(ctx, obj);
}

int dyntype_dump_value_buffer_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                      dyn_value_t obj, void *buffer, int len) {
  return dyntype_dump_value_buffer(ctx, obj, buffer, len);
}

/******************* Garbage collection *******************/

void dyntype_hold_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj) {
  return dyntype_hold(ctx, obj);
}

void dyntype_release_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj) {
  return dyntype_release(ctx, obj);
}

void dyntype_collect_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx) {
  return dyntype_collect(ctx);
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }

static NativeSymbol native_symbols[] = {
    REG_NATIVE_FUNC(dyntype_context_init, "()I"),
    REG_NATIVE_FUNC(dyntype_context_destroy, "(I)"),

    REG_NATIVE_FUNC(dyntype_new_number, "(IF)r"),
    REG_NATIVE_FUNC(dyntype_new_boolean, "(Ii)r"),
    REG_NATIVE_FUNC(dyntype_new_string, "(Ir)r"),
    REG_NATIVE_FUNC(dyntype_new_undefined, "(I)r"),
    REG_NATIVE_FUNC(dyntype_new_null, "(I)r"),
    REG_NATIVE_FUNC(dyntype_new_object, "(I)r"),
    REG_NATIVE_FUNC(dyntype_new_array, "(I)r"),
    REG_NATIVE_FUNC(dyntype_new_extref, "(IiF)r"),
    REG_NATIVE_FUNC(dyntype_new_object_with_proto, "(Ir)r"),

    REG_NATIVE_FUNC(dyntype_set_prototype, "(Irr)i"),
    REG_NATIVE_FUNC(dyntype_get_prototype, "(Ir)r"),
    REG_NATIVE_FUNC(dyntype_get_own_property, "(Irr)r"),

    REG_NATIVE_FUNC(dyntype_set_property, "(Irrr)i"),
    REG_NATIVE_FUNC(dyntype_define_property, "(Irrr)i"),
    REG_NATIVE_FUNC(dyntype_get_property, "(Irr)r"),
    REG_NATIVE_FUNC(dyntype_has_property, "(Irr)i"),
    REG_NATIVE_FUNC(dyntype_delete_property, "(Irr)i"),

    REG_NATIVE_FUNC(dyntype_is_undefined, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_null, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_bool, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_number, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_string, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_object, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_array, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_extref, "(Ir)i"),

    REG_NATIVE_FUNC(dyntype_to_bool, "(Iri)i"),
    REG_NATIVE_FUNC(dyntype_to_number, "(Iri)i"),
    REG_NATIVE_FUNC(dyntype_to_cstring, "(Iri)i"),
    REG_NATIVE_FUNC(dyntype_to_extref, "(Iri)i"),

    REG_NATIVE_FUNC(dyntype_free_cstring, "(Ir)"),

    REG_NATIVE_FUNC(dyntype_typeof, "(Ir)r"),
    REG_NATIVE_FUNC(dyntype_type_eq, "(Irr)i"),
    REG_NATIVE_FUNC(dyntype_instanceof, "(Irr)i"),

    /* TODO */
};
/* clang-format on */

uint32_t
get_native_lib(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "libdyntype";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
