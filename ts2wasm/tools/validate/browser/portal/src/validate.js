/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

let wasmMemory;

export function setWasmMemory(value) {
    wasmMemory = value;
}

const TAG_PROPERTY = '__tag__';

const DynType = {
    DynUnknown: 0,
    DynUndefined: 1,
    DynNull: 2,
    DynObject: 3,
    DynBoolean: 4,
    DynNumber: 5,
    DynString: 6,
    DynFunction: 7,
    DynSymbol: 8,
    DynBigInt: 9,
    DynExtRefObj: 10,
    DynExtRefFunc: 11,
    DynExtRefInfc: 12,
    DynExtRefArray: 13,
};

const ExtRefTag = {
    ExtObj: 0,
    ExtFunc: 1,
    ExtInfc: 2,
    ExtArray: 3,
};

const getDynTypeTag = (value) => {
    let res;
    const tag = value[TAG_PROPERTY];
    if (tag === ExtRefTag.ExtObj) {
        res = DynType.DynExtRefObj;
    } else if (tag === ExtRefTag.ExtFunc) {
        res = DynType.DynExtRefFunc;
    } else if (tag === ExtRefTag.ExtInfc) {
        res = DynType.DynExtRefInfc;
    } else if (tag === ExtRefTag.ExtArray) {
        res = DynType.DynExtRefArray;
    } else {
        const type = typeof value;
        switch (type) {
            case 'number':
                res = DynType.DynNumber;
                break;
            case 'boolean':
                res = DynType.DynBoolean;
                break;
            case 'string':
                res = DynType.DynString;
                break;
            case 'function':
                res = DynType.DynFunction;
                break;
            case 'symbol':
                res = DynType.DynSymbol;
                break;
            case 'bigint':
                res = DynType.DynBigInt;
                break;
            case 'object':
                res = DynType.DynObject;
                break;
            case 'undefined':
                res = DynType.DynUndefined;
                break;
            default:
                res = DynType.DynUnknown;
                break;
        }
    }
    return res;
};

const cstringToJsString = (offset) => {
    let length = 0;
    let memory = new Uint8Array(wasmMemory.buffer);
    while (memory[offset + length] !== 0) {
        length++;
    }

    const decoder = new TextDecoder();
    const string = decoder.decode(memory.slice(offset, offset + length));

    return string;
};

export const importObject = {
    libstructdyn: {
        struct_get_dyn_i32: (obj, index) => {},
        struct_get_dyn_i64: (obj, index) => {},
        struct_get_dyn_f32: (obj, index) => {},
        struct_get_dyn_f64: (obj, index) => {},
        struct_get_dyn_anyref: (obj, index) => {},
        struct_get_dyn_funcref: (obj, index) => {},
        struct_set_dyn_i32: (obj, index, value) => {},
        struct_set_dyn_i64: (obj, index, value) => {},
        struct_set_dyn_f32: (obj, index, value) => {},
        struct_set_dyn_f64: (obj, index, value) => {},
        struct_set_dyn_anyref: (obj, index, value) => {},
        struct_set_dyn_funcref: (obj, index, value) => {},
    },
    libdyntype: {
        dyntype_context_init: () => BigInt(0),
        dyntype_context_destroy: (ctx) => { },

        dyntype_new_number: (ctx, value) => {
            return new Number(value);
        },
        dyntype_to_number: (ctx, value) => {
            const res = value.valueOf();
            return res;
        },
        dyntype_is_number: (ctx, value) => {
            return typeof value === 'number' || value instanceof Number;
        },

        dyntype_new_boolean: (ctx, value) => {
            return new Boolean(value);
        },
        dyntype_to_bool: (ctx, value) => {
            const res = value.valueOf();
            return res;
        },
        dyntype_is_bool: (ctx, value) => {
            return typeof value === 'boolean' || value instanceof Boolean;
        },

        dyntype_new_string: (ctx, value) => {
            // TODO
            return new String(value);
        },
        dyntype_to_cstring: (ctx, value) => {
            const memView = new DataView(wasmMemory.buffer);
            let res;
            memView.setInt32(res, value);
        },
        dyntype_free_cstring: (ctx, value) => {
            // no need in js
        },
        dyntype_is_string: (ctx, value) => {
            return typeof value === 'string' || value instanceof String;
        },

        dyntype_new_array: (ctx) => new Array(),
        dyntype_new_array_with_length: (ctx, len) => new Array(len),
        dyntype_is_array: (ctx, value) => {
            return Array.isArray(value);
        },
        dyntype_add_elem: (ctx, arr, elem) => {
            arr.push(elem);
        },
        dyntype_set_elem: (ctx, arr, idx, elem) => {
            arr[idx] = elem;
        },
        dyntype_get_elem: (ctx, arr, idx) => {
            return arr[idx];
        },
        dyntype_typeof: (ctx, value) => {
            const res = getDynTypeTag(value);
            return res;
        },

        dyntype_type_eq: (ctx, l, r) => {
            return l === r;
        },

        dyntype_new_object: (ctx) => new Object(),
        dyntype_set_property: (ctx, obj, prop, value) => {
            obj[prop] = value;
            return true;
        },
        dyntype_get_property: (ctx, obj, prop) => {
            return obj[prop];
        },
        dyntype_has_property: (ctx, obj, prop) => {
            return prop in obj;
        },
        dyntype_delete_property: (ctx, obj, prop) => {
            delete obj[prop];
            return true;
        },
        dyntype_is_object: (ctx, obj) => {
            return typeof obj === 'object';
        },

        dyntype_is_undefined: (ctx, value) => {
            typeof value === 'undefined';
        },
        dyntype_new_undefined: (ctx) => undefined,

        dyntype_new_null: (ctx) => null,

        dyntype_new_extref: (ctx, value, flag) => {
            const ref = new Object();
            /** TODO: ensure it's truely a external reference */
            ref['ptr'] = value;
            ref[TAG_PROPERTY] = flag;
            return ref;
        },
        dyntype_is_extref: (ctx, obj) => {
            /** TODO: ensure it's truely a external reference */
            const tag = obj[TAG_PROPERTY];
            if (
                tag === ExtRefTag.ExtObj ||
                tag === ExtRefTag.ExtFunc ||
                tag === ExtRefTag.ExtInfc ||
                tag === ExtRefTag.ExtArray
            ) {
                return true;
            }
            return false;
        },
        dyntype_to_extref: (ctx, obj) => {
            let res = obj['ptr'];
            return res;
        },

        dyntype_get_prototype: (ctx, obj) => {
            return Object.getPrototypeOf(obj);
        },
        dyntype_set_prototype: (ctx, obj, proto) => {
            Object.setPrototypeOf(obj, proto);
        },
    },
    env: {
        console_log: (obj) => {
            /** TODO: cant log reference type variable */
            console.log(obj);
        },
        console_constructor: (obj) => {},
        strcmp(a, b) {
            let lhs = cstringToJsString(a);
            let rhs = cstringToJsString(b);
            return lhs.localeCompare(rhs);
        },
    },
};

export function typeConvert(type, arg) {
    switch (type) {
        case '0': {
            // boolean
            if (arg == '0') {
                return false;
            } else if (arg == '1') {
                return true;
            } else {
                console.error(`the input argument is not a boolean: ${arg}`);
            }
            break;
        }
        case '1': // number
            return parseFloat(arg);
        case '2': // string, currently not support
            return arg;
        case '3': // undefined
            return undefined;
        case '4': // null
            return null;
        default:
            console.error(
                `the input argument is not a boolean, number or string: [${type}: ${arg}]`,
            );
    }
}
