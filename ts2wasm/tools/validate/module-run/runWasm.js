/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const moduleName = arguments[0];

let expectedRes;
let exportFunc;
const parameters = [];

const notValidate = isNaN(Number(arguments[1]));

if (notValidate) {
    // just run a wasm module
    exportFunc = arguments[1];
    for (let i = 2; i < arguments.length; i += 2) {
        parameters.push(typeConvert(arguments[i], arguments[i + 1]));
    }
} else {
    expectedRes = typeConvert(arguments[2], arguments[3]);
    exportFunc = arguments[4];
    for (let i = 5; i < arguments.length; i += 2) {
        parameters.push(typeConvert(arguments[i], arguments[i + 1]));
    }
}

const buf = read(moduleName, 'binary');
let wasmMemory;

const importObject = {
    libstructdyn: {
        struct_get_dyn_i32: (obj, index) => {},
        struct_get_dyn_i64: (obj, index) => {},
        struct_get_dyn_f32: (obj, index) => {},
        struct_get_dyn_f64: (obj, index) => {},
        struct_get_dyn_anyref: (obj, index) => {},
        struct_set_dyn_i32: (obj, index, value) => {},
        struct_set_dyn_i64: (obj, index, value) => {},
        struct_set_dyn_f32: (obj, index, value) => {},
        struct_set_dyn_f64: (obj, index, value) => {},
        struct_set_dyn_anyref: (obj, index, value) => {},
    },
    libdyntype: {
        dyntype_context_init: () => BigInt(0),
        dyntype_context_destroy: (ctx) => {},

        dyntype_new_number: (ctx, value) => {
            return new Number(value);
        },
        dyntype_to_number: (ctx, value, pres) => {
            const memView = new DataView(wasmMemory.buffer);
            memView.setFloat64(pres, value, true);
        },
        dyntype_is_number: (ctx, value) => {
            return typeof value === 'number' || value instanceof Number;
        },

        dyntype_new_boolean: (ctx, value) => {
            return new Boolean(value);
        },
        dyntype_to_bool: (ctx, value, pres) => {
            const memView = new DataView(wasmMemory.buffer);
            memView.setInt32(pres, value.valueOf(), true);
        },
        dyntype_is_bool: (ctx, value) => {
            return typeof value === 'boolean' || value instanceof Boolean;
        },

        dyntype_new_string: (ctx, value) => {
            // TODO
            return new String(value);
        },
        dyntype_to_cstring: (ctx, value, pres) => {
            // TODO
        },
        dyntype_free_cstring: (ctx, value) => {
            // TODO
        },
        dyntype_is_string: (ctx, value) => {
            // TODO
            return typeof value === 'string' || value instanceof String;
        },

        dyntype_new_array: (ctx, len) => new Array(len),
        dyntype_is_array: (ctx, value) => {
            return Array.isArray(value);
        },
        dyntype_add_elem: (ctx, arr, elem) => {
            arr.push(elem);
            return true;
        },
        dyntype_set_elem: (ctx, arr, idx, elem) => {
            arr[idx] = elem;
            return true;
        },
        dyntype_get_elem: (ctx, arr, idx) => {
            return arr[idx];
        },
        dyntype_typeof: (ctx, value) => {
            // TODO
            return 0;
        },

        dyntype_type_eq: (ctx, a, b) => {
            // TODO
            return 1;
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

        dyntype_new_undefined: (ctx) => undefined,

        dyntype_new_null: (ctx) => null,

        dyntype_new_extref: (ctx, value, flag) => {
            const ref = new Object();
            ref['ptr'] = value;
            ref['tag'] = flag;
            return ref;
        },
        dyntype_is_extref: (ctx, obj) => {
            const tag = obj['tag'];
            if (tag == 1 || tag == 2 || tag == 3 || tag == 4) {
                return true;
            }
            return false;
        },
        dyntype_to_extref: (ctx, obj, pres) => {
            const memView = new DataView(wasmMemory.buffer);
            memView.setUint32(pres, obj['ptr'], true);
            return true;
        },

        dyntype_get_prototype: (ctx, obj) => {
            // TODO
            return obj;
        },
        dyntype_set_prototype: (ctx, obj1, obj2) => {
            // TODO
            return 1;
        },
    },
    env: {
        console_log: (obj) => {
            //
        },
        strcmp(a, b) {
            return a == b;
        },
    },
};

WebAssembly.instantiate(buf, importObject).then((wasmModule) => {
    wasmMemory = wasmModule.instance.exports.default;
    const func = wasmModule.instance.exports[exportFunc];
    const res = func.call(func, ...parameters);
    if (notValidate) {
        console.log(res);
    } else {
        console.log(expectedRes == res);
    }
});

function typeConvert(type, arg) {
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
