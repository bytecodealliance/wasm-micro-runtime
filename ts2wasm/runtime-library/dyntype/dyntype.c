/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include "cutils.h"
#include "quickjs.h"
#include <stdbool.h>

static dyn_ctx_t g_dynamic_context = NULL;

typedef struct DynTypeContext {
  JSRuntime *js_rt;
  JSContext *js_ctx;
} DynTypeContext;

static inline JSValue* dyntype_dup_value(JSContext *ctx, JSValue value) {
    JSValue* ptr = js_malloc(ctx, sizeof(value));
    if (!ptr) {
        return NULL;
    }
    memcpy(ptr, &value, sizeof(value));
    return ptr;
}

static dyn_type_t quickjs_type_to_dyn_type(int quickjs_tag) {
    switch (quickjs_tag) {
#define XX(qtag, dyntype) case qtag: return dyntype;
    XX(69, DynUndefined);
    XX(73, DynObject);
    XX(71, DynBoolean);
    XX(70, DynNumber);
    XX(72, DynString);
    // XX(27, DynFunction); // TODO
    XX(74, DynSymbol);
    // XX(139, DynBigInt); // TODO
#undef XX
    default:
        return DynUnknown;
    }
    return DynUnknown;
}

dyn_ctx_t dyntype_context_init() {
    if (g_dynamic_context) {
        return g_dynamic_context;
    }

    dyn_ctx_t ctx = malloc(sizeof(DynTypeContext));
    if (!ctx) {
        return NULL;
    }
    memset(ctx, 0, sizeof(DynTypeContext));
    ctx->js_rt = JS_NewRuntime();
    if (!ctx->js_rt) {
        goto fail;
    }
    ctx->js_ctx = JS_NewContext(ctx->js_rt);
    if (!ctx->js_ctx) {
        goto fail;
    }

    g_dynamic_context = ctx;
    return ctx;
fail:
    dyntype_context_destroy(ctx);
    return NULL;
}

dyn_ctx_t dyntype_context_init_with_opt(dyn_options_t *options) {
    // TODO
    return NULL;
}

// TODO: there is exist wild pointer
void dyntype_context_destroy(dyn_ctx_t ctx) {
    if (ctx) {
        if (ctx->js_ctx) {
            JS_FreeContext(ctx->js_ctx);
        }
        if (ctx->js_rt) {
            JS_FreeRuntime(ctx->js_rt);
        }
        free(ctx);
    }

    g_dynamic_context = NULL;
}

dyn_ctx_t dyntype_get_context() {
    return g_dynamic_context;
}

dyn_value_t dyntype_new_number(dyn_ctx_t ctx, double value) {
    JSValue v = JS_NewFloat64(ctx->js_ctx, value);
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_boolean(dyn_ctx_t ctx, bool value) {
    JSValue v = JS_NewBool(ctx->js_ctx, value);
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_string(dyn_ctx_t ctx, const char *str) {
    JSValue v = JS_NewString(ctx->js_ctx, str);
    if (JS_IsException(v)) {
        return NULL;
    }
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_undefined(dyn_ctx_t ctx) {
    JSValue v = JS_UNDEFINED;
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_null(dyn_ctx_t ctx) {
    JSValue v = JS_NULL;
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_object(dyn_ctx_t ctx) {
    JSValue v = JS_NewObject(ctx->js_ctx);
    if (JS_IsException(v)) {
        return NULL;
    }
    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_array_with_length(dyn_ctx_t ctx, int len) {
    JSValue v = JS_NewArray(ctx->js_ctx);
    if (JS_IsException(v)) {
        return NULL;
    }

    if (len) {
        JSValue vlen = JS_NewInt32(ctx->js_ctx, len);
        set_array_length1(ctx->js_ctx, JS_VALUE_GET_OBJ(v), vlen, 0);
    }

    return dyntype_dup_value(ctx->js_ctx, v);
}

dyn_value_t dyntype_new_array(dyn_ctx_t ctx) {
    return dyntype_new_array_with_length(ctx, 0);
}

dyn_value_t dyntype_new_extref(dyn_ctx_t ctx, void *ptr, external_ref_tag tag) {
    if (tag != ExtObj && tag != ExtFunc && tag != ExtInfc) {
        return NULL;
    }
    JSValue *v = dyntype_dup_value(ctx->js_ctx, JS_NewInt32(ctx->js_ctx, 0));
    v->u.ptr = ptr;
    v->tag = (tag == ExtObj ? JS_TAG_EXT_OBJ : (tag == ExtFunc ? JS_TAG_EXT_FUNC : JS_TAG_EXT_INFC));
    return v;
}

int dyntype_set_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop,
                         dyn_value_t value) {
    JSValue* obj_ptr = (JSValue *)obj;
    if (!JS_IsObject(*obj_ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    JSValue *val = (JSValue *)value;
    return JS_SetPropertyStr(ctx->js_ctx, *obj_ptr, prop, *val) ? DYNTYPE_SUCCESS : -DYNTYPE_EXCEPTION;
}

int dyntype_define_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop,
                            dyn_value_t desc) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (!JS_IsObject(*obj_ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    JSValue *desc_ptr = (JSValue *)desc;
    if (!JS_IsObject(*desc_ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    JSAtom atom;
    int res;
    atom = JS_NewAtom(ctx->js_ctx, prop);
    if (atom == JS_ATOM_NULL) {
        return -DYNTYPE_EXCEPTION;
    }
    // It will only return TRUE or EXCEPTION, because of JS_PROP_THROW flag
    res = JS_DefinePropertyDesc1(ctx->js_ctx, *obj_ptr, atom, *desc_ptr, JS_PROP_THROW);
    JS_FreeAtom(ctx->js_ctx, atom);
    return res == -1 ? -DYNTYPE_EXCEPTION : DYNTYPE_SUCCESS;
}

dyn_value_t dyntype_get_property(dyn_ctx_t ctx, dyn_value_t obj,
                                 const char *prop) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (!JS_IsObject(*obj_ptr)) {
        return NULL;
    }
    JSValue val = JS_GetPropertyStr(ctx->js_ctx, *obj_ptr, prop);
    if (JS_IsException(val)) {
        return NULL;
    }
    JSValue *ptr = dyntype_dup_value(ctx->js_ctx, val);
    return ptr;
}

int dyntype_has_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (!JS_IsObject(*obj_ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    int res;
    JSAtom atom;
    atom = JS_NewAtom(ctx->js_ctx, prop);
    if (atom == JS_ATOM_NULL) {
        return -DYNTYPE_EXCEPTION;
    }
    res = JS_HasProperty(ctx->js_ctx, *obj_ptr, atom);
    JS_FreeAtom(ctx->js_ctx, atom);
    if (res == -1) {
        return -DYNTYPE_EXCEPTION;
    }
    return res == 0 ? DYNTYPE_FALSE : DYNTYPE_TRUE;
}

int dyntype_delete_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop) {
    if (dyntype_has_property(ctx, obj, prop) != DYNTYPE_TRUE) {
        return -DYNTYPE_FALSE;
    }
    JSValue *obj_ptr = (JSValue *)obj;
    JSAtom atom;
    atom = JS_NewAtom(ctx->js_ctx, prop);
    if (atom == JS_ATOM_NULL) {
        return -DYNTYPE_EXCEPTION;
    }
    // it returns EXCEPTION, FALSE or TRUE.
    int res = JS_DeleteProperty(ctx->js_ctx, *obj_ptr, atom, 0);
    JS_FreeAtom(ctx->js_ctx, atom);
    if (res == -1) {
        return -DYNTYPE_EXCEPTION;
    }
    return res == 0 ? DYNTYPE_FALSE : DYNTYPE_TRUE;
}

bool dyntype_is_undefined(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsUndefined(*ptr);
}

bool dyntype_is_null(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsNull(*ptr);
}

bool dyntype_is_bool(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsBool(*ptr);
}

int dyntype_to_bool(dyn_ctx_t ctx, dyn_value_t bool_obj, bool *pres) {
    JSValue *ptr = (JSValue *)bool_obj;
    if (!JS_IsBool(*ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    *pres = (bool)JS_ToBool(ctx->js_ctx, *ptr);
    return DYNTYPE_SUCCESS;
}

bool dyntype_is_number(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsNumber(*ptr);
}

int dyntype_to_number(dyn_ctx_t ctx, dyn_value_t obj, double *pres) {
    JSValue* ptr = (JSValue *)obj;
    if (!JS_IsNumber(*ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    *pres = (JS_VALUE_GET_TAG(*ptr) == JS_TAG_INT ? JS_VALUE_GET_INT(*ptr) :
            JS_VALUE_GET_FLOAT64(*ptr));
    return DYNTYPE_SUCCESS;
}

bool dyntype_is_string(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return JS_IsString(*ptr);
}

int dyntype_to_cstring(dyn_ctx_t ctx, dyn_value_t str_obj, char **pres) {
    JSValue *ptr = (JSValue *)str_obj;
    if (!JS_IsString(*ptr)) {
        return -DYNTYPE_TYPEERR;
    }
    *pres = (char*)JS_ToCString(ctx->js_ctx, *ptr);
    return DYNTYPE_SUCCESS;
}

void dyntype_free_cstring(dyn_ctx_t ctx, char *str) {
    JS_FreeCString(ctx->js_ctx, (const char *)str);
}

bool dyntype_is_object(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsObject(*ptr);
}

bool dyntype_is_array(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    return (bool)JS_IsArray(ctx->js_ctx, *ptr);
}

bool dyntype_is_extref(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    if (JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_OBJ ||
        JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_FUNC ||
        JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_INFC) {
        return DYNTYPE_TRUE;
    }
    return DYNTYPE_FALSE;
}

int dyntype_to_extref(dyn_ctx_t ctx, dyn_value_t obj, void **pres) {
    if (dyntype_is_extref(ctx, obj) == DYNTYPE_FALSE) {
        return -DYNTYPE_TYPEERR;
    }
    JSValue *ptr = (JSValue *)obj;
    *pres = JS_VALUE_GET_PTR(*ptr);
    return DYNTYPE_SUCCESS;
}

dyn_type_t dyntype_typeof(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValueConst *ptr = (JSValueConst *)obj;
    if (JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_OBJ) {
        return DynExtRefObj;
    }
    if (JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_FUNC) {
        return DynExtRefFunc;
    }
    if (JS_VALUE_GET_TAG(*ptr) == JS_TAG_EXT_INFC) {
        return DynExtRefInfc;
    }
    int q_atom_tag = js_operator_typeof1(ctx->js_ctx, *ptr);
    dyn_type_t tag = quickjs_type_to_dyn_type(q_atom_tag);
    return tag;
}

bool dyntype_type_eq(dyn_ctx_t ctx, dyn_value_t lhs, dyn_value_t rhs) {
    return dyntype_typeof(ctx, lhs) == dyntype_typeof(ctx, rhs);
}

dyn_value_t dyntype_new_object_with_proto(dyn_ctx_t ctx,
                                          const dyn_value_t proto_obj) {
    JSValueConst *proto = (JSValueConst *)proto_obj;
    if (!JS_IsObject(*proto) && !JS_IsNull(*proto)) {
        return NULL;
    }
    JSValue new_obj = JS_NewObjectProto(ctx->js_ctx, *proto);
    if (JS_IsException(new_obj)) {
        return NULL;
    }
    return dyntype_dup_value(ctx->js_ctx, new_obj);
}

int dyntype_set_prototype(dyn_ctx_t ctx, dyn_value_t obj,
                          const dyn_value_t proto_obj) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (JS_VALUE_GET_TAG(*obj_ptr) == JS_TAG_NULL ||
        JS_VALUE_GET_TAG(*obj_ptr) == JS_TAG_UNDEFINED) {
        return -DYNTYPE_TYPEERR;
    }
    JSValue *proto_obj_ptr = (JSValue *)proto_obj;
    if (JS_VALUE_GET_TAG(*proto_obj_ptr) != JS_TAG_NULL &&
        JS_VALUE_GET_TAG(*proto_obj_ptr) != JS_TAG_OBJECT) {
        return -DYNTYPE_TYPEERR;
    }
    int res = JS_SetPrototype(ctx->js_ctx, *obj_ptr, *proto_obj_ptr);
    return res == 1 ? DYNTYPE_SUCCESS : -DYNTYPE_EXCEPTION;
}

const dyn_value_t dyntype_get_prototype(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (JS_VALUE_GET_TAG(*obj_ptr) == JS_TAG_NULL ||
        JS_VALUE_GET_TAG(*obj_ptr) == JS_TAG_UNDEFINED) {
        return NULL;
    }
    JSValue proto = JS_GetPrototype(ctx->js_ctx, *obj_ptr);
    if (JS_IsException(proto)) {
        return NULL;
    }
    JSValue* proto1 = dyntype_dup_value(ctx->js_ctx, proto);
    return proto1;
}

dyn_value_t dyntype_get_own_property(dyn_ctx_t ctx, dyn_value_t obj,
                                     const char *prop) {
    JSValue *obj_ptr = (JSValue *)obj;
    if (JS_VALUE_GET_TAG(*obj_ptr) != JS_TAG_OBJECT) {
        return NULL;
    }
    JSAtom atom = JS_NewAtom(ctx->js_ctx, prop);
    if (atom == JS_ATOM_NULL) {
        return NULL;
    }
    JSPropertyDescriptor desc;
    int res = JS_GetOwnProperty(ctx->js_ctx, &desc, *obj_ptr, atom);
    JS_FreeAtom(ctx->js_ctx, atom);
    if (res != 1) {
        return NULL;
    }
    JSValue *v = dyntype_dup_value(ctx->js_ctx, desc.value);
    return v;
}

bool dyntype_instanceof(dyn_ctx_t ctx, const dyn_value_t src_obj,
                        const dyn_value_t dst_obj) {
    JSValue *src = (JSValue *)src_obj;
    JSValue *dst = (JSValue *)dst_obj;
    int ret = JS_OrdinaryIsInstanceOf1(ctx->js_ctx, *src, *dst);
    if (ret == -1) {
        return -DYNTYPE_EXCEPTION;
    }
    return ret == 1 ? DYNTYPE_TRUE : DYNTYPE_FALSE;
}

void dyntype_dump_value(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *v = (JSValue *)obj;
    JS_Dump1(ctx->js_rt, v);
}

int dyntype_dump_value_buffer(dyn_ctx_t ctx, dyn_value_t obj, void *buffer,
                              int len) {
    JSValue *v = (JSValue *)obj;
    int res = JS_DumpWithBuffer(ctx->js_rt, v, buffer, len);
    return res == -1 ? -DYNTYPE_EXCEPTION : res;
}

void dyntype_hold(dyn_ctx_t ctx, dyn_value_t obj) {
    JSValue *ptr = (JSValue *)obj;
    if (JS_VALUE_HAS_REF_COUNT(*ptr)) {
        JS_DupValue(ctx->js_ctx, *ptr);
    }
}

// TODO: there is exist wild pointer
void dyntype_release(dyn_ctx_t ctx, dyn_value_t obj) {
    if (obj == NULL) {
        return;
    }
    JSValue *ptr = (JSValue *)(obj);
    if (JS_VALUE_HAS_REF_COUNT(*ptr)) {
        JSRefCountHeader *p = (JSRefCountHeader *)JS_VALUE_GET_PTR(*ptr);
        int ref_cnt = p->ref_count;
        JS_FreeValue(ctx->js_ctx, *ptr);
        if (ref_cnt <= 1) {
            js_free(ctx->js_ctx, obj);
        }
    } else {
        js_free(ctx->js_ctx, obj);
    }
}

void dyntype_collect(dyn_ctx_t ctx) {
    // TODO
}
