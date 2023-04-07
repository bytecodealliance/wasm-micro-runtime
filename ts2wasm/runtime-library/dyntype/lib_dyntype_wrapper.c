/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include "wasm_export.h"
#include "wasm_runtime.h"
#include "gc_object.h"

/* Convert host pointer to anyref */
#define BOX_ANYREF(ptr) do {                                                        \
    wasm_module_inst_t module_inst = get_module_inst(exec_env);                     \
    void *gc_heap_handle = ((WASMModuleInstance *)module_inst)->e->gc_heap_handle;  \
    return wasm_anyref_obj_new(exec_env, gc_heap_handle, ptr);                      \
} while (0)

/* Convert anyref to host pointer */
#define UNBOX_ANYREF(anyref) wasm_anyref_obj_get_value(anyref)

/******************* Initialization and destroy *******************/
dyn_ctx_t
dyntype_context_init_wrapper(wasm_exec_env_t exec_env)
{
    return dyntype_context_init();
}

dyn_ctx_t
dyntype_context_init_with_opt_wrapper(wasm_exec_env_t exec_env,
                                      dyn_options_t *options)
{
    return dyntype_context_init_with_opt(options);
}

void
dyntype_context_destroy_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    return dyntype_context_destroy(ctx);
}

/******************* Field access *******************/
dyn_value_t
dyntype_new_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                           double value)
{
    BOX_ANYREF(dyntype_new_number(ctx, value));
}

dyn_value_t
dyntype_new_boolean_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, bool value)
{
    BOX_ANYREF(dyntype_new_boolean(ctx, value));
}

dyn_value_t
dyntype_new_string_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                           WASMStructObjectRef str_obj)
{
    WASMValue arr_obj = { 0 };
    wasm_struct_obj_get_field(str_obj, 1, false, &arr_obj);
    const char *str = (char *)wasm_array_obj_first_elem_addr(arr_obj.gc_obj);
    BOX_ANYREF(dyntype_new_string(ctx, str));
}

dyn_value_t
dyntype_new_undefined_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    BOX_ANYREF(dyntype_new_undefined(ctx));
}

dyn_value_t
dyntype_new_null_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    BOX_ANYREF(dyntype_new_null(ctx));
}

dyn_value_t
dyntype_new_object_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    BOX_ANYREF(dyntype_new_object(ctx));
}

dyn_value_t
dyntype_new_array_with_length_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, int len)
{
    BOX_ANYREF(dyntype_new_array_with_length(ctx, len));
}

dyn_value_t
dyntype_new_array_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
    BOX_ANYREF(dyntype_new_array(ctx));
}

void
dyntype_add_elem_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                         dyn_value_t obj, dyn_value_t elem)
{

}

void
dyntype_set_elem_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                         dyn_value_t obj, int index, dyn_value_t elem)
{

}

dyn_value_t
dyntype_get_elem_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                         dyn_value_t obj, int index)
{
    return 0;
}

dyn_value_t
dyntype_new_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, void *ptr,
                           external_ref_tag tag)
{
    BOX_ANYREF(dyntype_new_extref(ctx, ptr, tag));
}

int
dyntype_set_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj, const char *prop,
                             dyn_value_t value)
{
    return dyntype_set_property(ctx, UNBOX_ANYREF(obj), prop, UNBOX_ANYREF(value));
}

int
dyntype_define_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                dyn_value_t obj, const char *prop,
                                dyn_value_t desc)
{
    return dyntype_define_property(ctx, UNBOX_ANYREF(obj), prop, UNBOX_ANYREF(desc));
}

dyn_value_t
dyntype_get_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj, const char *prop)
{
    BOX_ANYREF(dyntype_get_property(ctx, UNBOX_ANYREF(obj), prop));
}

int
dyntype_has_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj, const char *prop)
{
    return dyntype_has_property(ctx, UNBOX_ANYREF(obj), prop);
}

int
dyntype_delete_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                dyn_value_t obj, const char *prop)
{
    return dyntype_delete_property(ctx, UNBOX_ANYREF(obj), prop);
}

/******************* Runtime type checking *******************/
bool
dyntype_is_undefined_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                             dyn_value_t obj)
{
    return dyntype_is_undefined(ctx, UNBOX_ANYREF(obj));
}

bool
dyntype_is_null_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                        dyn_value_t obj)
{
    return dyntype_is_null(ctx, UNBOX_ANYREF(obj));
}

bool
dyntype_is_bool_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                        dyn_value_t obj)
{
    return dyntype_is_bool(ctx, UNBOX_ANYREF(obj));
}

int
dyntype_to_bool_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                        dyn_value_t bool_obj)
{
    bool value = 0, ret;

    ret = dyntype_to_bool(ctx, UNBOX_ANYREF(bool_obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to bool");
    }

    return value;
}

bool
dyntype_is_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    return dyntype_is_number(ctx, UNBOX_ANYREF(obj));
}

int
dyntype_to_number_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    double value = 0;
    bool ret;

    ret = dyntype_to_number(ctx, UNBOX_ANYREF(obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to number");
    }

    return value;
}

bool
dyntype_is_string_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    return dyntype_is_string(ctx, UNBOX_ANYREF(obj));
}

char *
dyntype_to_cstring_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                           dyn_value_t str_obj)
{
    wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                               "libdyntype: string not supported");

    return NULL;
}

void
dyntype_free_cstring_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, char *str)
{
    return dyntype_free_cstring(ctx, str);
}

bool
dyntype_is_object_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    return dyntype_is_object(ctx, UNBOX_ANYREF(obj));
}

bool
dyntype_is_array_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                         dyn_value_t obj)
{
    return dyntype_is_array(ctx, UNBOX_ANYREF(obj));
}

bool
dyntype_is_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    return dyntype_is_extref(ctx, UNBOX_ANYREF(obj));
}

void *
dyntype_to_extref_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                          dyn_value_t obj)
{
    void *value = NULL;
    bool ret;

    ret = dyntype_to_extref(ctx, UNBOX_ANYREF(obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to extref");
    }

    return value;
}

/******************* Type equivalence *******************/
dyn_type_t
dyntype_typeof_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, dyn_value_t obj)
{
    return dyntype_typeof(ctx, UNBOX_ANYREF(obj));
}

bool
dyntype_type_eq_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                        dyn_value_t lhs, dyn_value_t rhs)
{
    return dyntype_type_eq(ctx, UNBOX_ANYREF(lhs), UNBOX_ANYREF(rhs));
}

/******************* Subtyping *******************/
dyn_value_t
dyntype_new_object_with_proto_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                      const dyn_value_t proto_obj)
{
    BOX_ANYREF(dyntype_new_object_with_proto(ctx, UNBOX_ANYREF(proto_obj)));
}

int
dyntype_set_prototype_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                              dyn_value_t obj, const dyn_value_t proto_obj)
{
    return dyntype_set_prototype(ctx, obj, UNBOX_ANYREF(proto_obj));
}

const dyn_value_t
dyntype_get_prototype_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                              dyn_value_t obj)
{
    BOX_ANYREF(dyntype_get_prototype(ctx, UNBOX_ANYREF(obj)));
}

dyn_value_t
dyntype_get_own_property_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                 dyn_value_t obj, const char *prop)
{
    BOX_ANYREF(dyntype_get_own_property(ctx, UNBOX_ANYREF(obj), prop));
}

bool
dyntype_instanceof_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                           const dyn_value_t src_obj, const dyn_value_t dst_obj)
{
    return dyntype_instanceof(ctx, UNBOX_ANYREF(src_obj), UNBOX_ANYREF(dst_obj));
}

/******************* Dumping *******************/
void
dyntype_dump_value_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                           dyn_value_t obj)
{
    return dyntype_dump_value(ctx, UNBOX_ANYREF(obj));
}

int
dyntype_dump_value_buffer_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                                  dyn_value_t obj, void *buffer, int len)
{
    return dyntype_dump_value_buffer(ctx, UNBOX_ANYREF(obj), buffer, len);
}

/******************* Garbage collection *******************/

void
dyntype_hold_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx, dyn_value_t obj)
{
    return dyntype_hold(ctx, UNBOX_ANYREF(obj));
}

void
dyntype_release_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx,
                        dyn_value_t obj)
{
    return dyntype_release(ctx, UNBOX_ANYREF(obj));
}

void
dyntype_collect_wrapper(wasm_exec_env_t exec_env, dyn_ctx_t ctx)
{
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
    REG_NATIVE_FUNC(dyntype_new_array_with_length, "(Ii)r"),
    REG_NATIVE_FUNC(dyntype_new_array, "(I)r"),
    REG_NATIVE_FUNC(dyntype_add_elem, "(Irr)"),
    REG_NATIVE_FUNC(dyntype_set_elem, "(Irir)"),
    REG_NATIVE_FUNC(dyntype_get_elem, "(Iri)r"),
    REG_NATIVE_FUNC(dyntype_new_extref, "(Iii)r"),
    REG_NATIVE_FUNC(dyntype_new_object_with_proto, "(Ir)r"),

    REG_NATIVE_FUNC(dyntype_set_prototype, "(Irr)i"),
    REG_NATIVE_FUNC(dyntype_get_prototype, "(Ir)r"),
    REG_NATIVE_FUNC(dyntype_get_own_property, "(Irir)r"),

    REG_NATIVE_FUNC(dyntype_set_property, "(Ir$r)i"),
    REG_NATIVE_FUNC(dyntype_define_property, "(Irrr)i"),
    REG_NATIVE_FUNC(dyntype_get_property, "(Ir$)r"),
    REG_NATIVE_FUNC(dyntype_has_property, "(Ir$)i"),
    REG_NATIVE_FUNC(dyntype_delete_property, "(Ir$)i"),

    REG_NATIVE_FUNC(dyntype_is_undefined, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_null, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_bool, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_number, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_string, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_object, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_array, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_is_extref, "(Ir)i"),

    REG_NATIVE_FUNC(dyntype_to_bool, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_to_number, "(Ir)F"),
    REG_NATIVE_FUNC(dyntype_to_cstring, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_to_extref, "(Ir)i"),

    REG_NATIVE_FUNC(dyntype_free_cstring, "(Ii)"),

    REG_NATIVE_FUNC(dyntype_typeof, "(Ir)i"),
    REG_NATIVE_FUNC(dyntype_type_eq, "(Irr)i"),
    REG_NATIVE_FUNC(dyntype_instanceof, "(Irr)i"),

    /* TODO */
};
/* clang-format on */

uint32_t
get_libdyntype_symbols(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "libdyntype";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
