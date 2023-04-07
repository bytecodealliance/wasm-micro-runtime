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
        struct_set_dyn_i32: (obj, index, value) => {},
        struct_set_dyn_i64: (obj, index, value) => {},
        struct_set_dyn_f32: (obj, index, value) => {},
        struct_set_dyn_f64: (obj, index, value) => {},
        struct_set_dyn_anyref: (obj, index, value) => {},
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
        dyntype_is_object: (ctx, obj) => {
            return typeof obj === 'object';
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

export const validateData = [
    'any_case1.wasm 1 1 1 anyTest',
    'any_case2.wasm 0 0 0 anyTest',
    'any_case3.wasm 1 0 1 anyTest',
    'any_case4.wasm 1 1 1 anyTest',
    'any_case5.wasm 0 0 0 anyTest',
    'any_case6.wasm 1 0 1 anyTest',
    'any_case7.wasm 1 1 3 anyTest',
    'any_case8.wasm 1 4 0 anyTest',
    'any_case9.wasm 1 3 0 anyTest',
    'any_case10.wasm 0 0 0 anyTest',
    'any_case11.wasm 1 0 1 anyTest',
    'any_case12.wasm 1 0 1 anyTest',
    'any_case13.wasm 1 1 1 anyTest',
    'any_case14.wasm 1 1 1 anyTest',
    'any_case15.wasm 1 1 4 anyTest',
    'any_case16.wasm 1 1 2 anyTest',
    'any_case17.wasm 1 1 2 anyTest',
    'any_case18.wasm 1 1 3 anyTest',
    'any_case19.wasm 1 1 1 anyTest',
    'any_case20.wasm 1 1 9 anyTest',
    'any_case21.wasm 1 1 2 anyTest',
    'any_case22.wasm 0 0 0 anyTest',
    'any_case23.wasm 0 0 0 anyTest',
    'any_case24.wasm 1 1 1 anyTest',
    'array_case1.wasm 1 1 3 arrayTest1',
    'array_case2.wasm 0 0 0 arrayTest2',
    'array_case3.wasm 1 1 1 arrayTest3',
    'array_case4.wasm 0 1 1 arrayTest4',
    'array_case5.wasm 1 1 5 arrayTest5',
    'array_case6.wasm 1 1 1 arrayTest6',
    'array_case7.wasm 1 1 3 arrayTest7',
    'array_case8.wasm 0 1 1 arrayTest8',
    'array_case9.wasm 0 1 1 arrayTest9',
    'array_case10.wasm 1 1 3 arrayTest10',
    'array_case11.wasm 1 1 5 arrayTest11',
    'array_case12.wasm 1 1 3 arrayTest12',
    'array_case13.wasm 1 1 11 arrayTest13',
    'array_case14.wasm 1 0 1 arrayTest14',
    'array_case15.wasm 1 1 20 arrayTest15',
    'array_case16.wasm 0 0 0 arrayTest16',
    'array_case17.wasm 0 0 0 arrayTest17',
    'array_case18.wasm 0 0 0 arrayTest18',
    'array_case19.wasm 1 1 10 arrayTest19 1 10',
    'binary_expression_case1.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case2.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case3.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case4.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case5.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case6.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case7.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case8.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case9.wasm 1 1 3 binaryExpressionTest',
    'binary_expression_case10.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case11.wasm 1 1 4 binaryExpressionTest',
    'binary_expression_case12.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case13.wasm 1 1 3 binaryExpressionTest',
    'binary_expression_case14.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case15.wasm 1 1 6 binaryExpressionTest',
    'binary_expression_case16.wasm 1 1 2 binaryExpressionTest',
    'binary_expression_case17.wasm 1 1 -1 binaryExpressionTest',
    'binary_expression_case18.wasm 1 1 -1 binaryExpressionTest',
    'binary_expression_case19.wasm 1 1 -1 binaryExpressionTest',
    'binary_expression_case20.wasm 1 1 -1 binaryExpressionTest',
    'binary_expression_case21.wasm 1 1 3 binaryExpressionTest',
    'binary_expression_case22.wasm 1 1 -1 binaryExpressionTest',
    'binary_expression_case23.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case24.wasm 1 1 20 binaryExpressionTest',
    'binary_expression_case25.wasm 1 1 0 binaryExpressionTest',
    'binary_expression_case26.wasm 1 1 0 binaryExpressionTest',
    'binary_expression_case27.wasm 1 1 0 binaryExpressionTest',
    'binary_expression_case28.wasm 1 1 10 binaryExpressionTest',
    'binary_expression_case29.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case30.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case31.wasm 1 1 0 binaryExpressionTest',
    'binary_expression_case32.wasm 1 1 1 binaryExpressionTest',
    'binary_expression_case33.wasm 1 1 1 binaryExpressionTest',
    'block_inner_block.wasm 1 1 11 innerBlock',
    'boolean_basic.wasm 1 1 1 booleanBasicTrue',
    'boolean_basic.wasm 1 1 0 booleanBasicFalse',
    'boolean_not.wasm 1 1 0 notNumber',
    'boolean_not.wasm 1 1 1 notBoolean',
    'boolean_not.wasm 1 0 0 notWithLogicOperator',
    'boolean_logic_operator.wasm 1 1 1 logicOr',
    'boolean_logic_operator.wasm 1 1 0 logicAnd',
    'boolean_logic_operator.wasm 1 1 2 conditionExpr',
    'builtin_array_case1.wasm 1 1 3 arrayTest',
    'builtin_array_case2.wasm 1 1 1 builtInArray',
    'builtin_boolean_case1.wasm 0 0 0 builtInBoolean',
    'builtin_console_case1.wasm 0 0 0 builtInConsole',
    'builtin_func_case1.wasm 1 1 1 builtInFunc',
    'builtin_Math_case1.wasm 1 1 3 mathTest',
    'builtin_Math_case2.wasm 1 1 9 mathTest',
    'builtin_Math_case3.wasm 1 1 16 mathTest',
    'builtin_number_case1.wasm 1 1 3.1415926 builtInNumber',
    'builtin_string_case1.wasm 0 0 0 strTest',
    'builtin_string_case2.wasm 0 0 0 strTest',
    'builtin_string_case3.wasm 1 1 5 strTest',
    'call_expression_param.wasm 1 1 6 allDefaultParam',
    'call_expression_param.wasm 1 1 5 someDefaultParam',
    'call_expression_get_value.wasm 1 1 116 getValueWithDefaultParam',
    'call_expression_param.wasm 1 1 6 noDefaultParam',
    'call_expression_get_value.wasm 1 1 30 callInnerFunc 1 10 1 11',
    'call_expression_param.wasm 1 1 3 paramIsAny',
    'call_expression_function_hoisting_case1.wasm 1 1 110 funcHosting',
    'call_expression_get_value.wasm 1 1 55 recursive 1 10',
    'class_basic.wasm 1 1 123 withoutCtor',
    'class_basic.wasm 1 1 10 basic',
    'class_extend.wasm 1 1 90 extendWithNewProp',
    'class_extend.wasm 1 1 40 methodOverwrite',
    'class_basic.wasm 1 1 25 getterSetter',
    'class_field_assign.wasm 1 1 18 withCtor',
    'class_field_assign.wasm 1 1 0 withoutCtor',
    'class_static_prop.wasm 1 1 2 staticMethodWithOverwrite',
    'class_static_prop.wasm 1 1 1 staticMethod',
    'class_type.wasm 1 1 1 uniqueType',
    'class_static_prop.wasm 1 1 74 staticFields',
    'class_declare_case1.wasm 0 0 0 classDecl',
    'closure_basic.wasm 1 1 2 accessOuterVars',
    'closure_basic.wasm 1 1 10 returnOuterFuncCall',
    'closure_first_class_func.wasm 1 1 3 returnAFunction',
    'closure_basic.wasm 0 1 3 closureTest',
    'closure_first_class_func.wasm 1 1 10 functionAsParam',
    'closure_set_ctx_value.wasm 1 1 31 setCtxValue 1 10',
    'complexType_case1.wasm 1 1 2 complexTypeTest',
    'complexType_case2.wasm 0 0 0 cpxCase2Func3',
    'complexType_case3.wasm 1 1 10 cpxCase3Func1',
    'complexType_case4.wasm 1 1 3 cpxCase3Func1',
    'complexType_case5.wasm 1 1 6 cpxCase3Func1',
    'do_statement_case1.wasm 1 1 16 doTest',
    'do_statement_case2.wasm 1 1 10 doTest',
    'do_statement_case3.wasm 1 1 21 doTest',
    'do_statement_case4.wasm 1 1 16 doTest',
    'do_statement_case5.wasm 1 1 16 doTest',
    'export_case1.wasm 0 0 0 exportTest',
    'export_case2.wasm 0 0 0 theDefault',
    'extref_case1.wasm 1 0 1 extrefTest',
    'extref_case2.wasm 1 1 1 extrefTest',
    'extref_case3.wasm 1 2 hi extrefTest',
    'extref_case4.wasm 1 1 1 extrefTest',
    'extref_case5.wasm 1 4 0 extrefTest',
    'extref_case6.wasm 1 0 1 extrefTest',
    'extref_case7.wasm 1 3 0 extrefTest',
    'extref_case8.wasm 1 1 1 extrefTest',
    'for_statement_case1.wasm 1 1 100 forTest',
    'for_statement_case2.wasm 1 1 90 forTest',
    'for_statement_case3.wasm 1 1 100 forTest',
    'for_statement_case4.wasm 1 1 106 forTest',
    'for_statement_case5.wasm 1 1 105 forTest',
    'for_statement_case6.wasm 1 1 115 forTest',
    'for_statement_case7.wasm 1 1 4905 forTest',
    'function_declaration_case1.wasm 1 1 10 functionTest 1 2 1 8',
    'function_declaration_case2.wasm 1 1 3 functionTest 1 1 1 2',
    'function_declaration_case3.wasm 1 1 1 functionTest',
    'function_declaration_case4.wasm 1 1 106.5 functionTest 1 100 1 6.5',
    'function_declaration_case5.wasm 1 1 5 functionTest',
    'function_declaration_case6.wasm 1 1 7 functionTest',
    'function_declaration_case7.wasm 1 0 0 functionTest',
    'function_declaration_case8.wasm 1 1 2 functionTest',
    'function_expression_case1.wasm 1 1 2004.1 functionTest',
    'function_expression_case2.wasm 1 1 2 functionTest',
    'global_statement_case1.wasm 1 1 99 globalTest',
    'global_statement_case2.wasm 1 1 42 globalTest',
    'global_statement_case3.wasm 1 1 101 globalTest',
    'global_statement_case4.wasm 1 1 98 globalTest',
    'global_statement_case5.wasm 1 1 95 globalTest',
    'global_statement_case6.wasm 1 1 20 globalTest',
    'global_variable_case1.wasm 1 1 198 globalVarTest',
    'global_variable_case2.wasm 1 0 1 globalVarTest',
    'if_statement_case1.wasm 1 1 26 ifTest',
    'if_statement_case2.wasm 1 1 36 ifTest',
    'if_statement_case3.wasm 1 1 17 ifTest',
    'if_statement_case4.wasm 1 1 10 ifTest 1 10',
    'import_case1.wasm 1 1 5 impExpTest',
    'import_case2.wasm 0 0 0 impExpTest',
    'import_case3.wasm 0 0 0 impExpTest',
    'import_case4.wasm 0 0 0 impExpTest',
    'import_case5.wasm 1 1 2 print2',
    'infc_assign_obj.wasm 1 1 1 objLiteralAndInfc',
    'infc_assign_class.wasm 1 1 1 classAndInfc',
    'infc_assign_infc.wasm 1 1 10 infcAndInfc',
    'infc_return_value.wasm 1 0 1 returnInfc',
    'infc_return_value.wasm 1 1 10 returnClass',
    'infc_parameter.wasm 1 1 1 infcToClass',
    'infc_parameter.wasm 1 1 2 classToInfc',
    'infc_parameter.wasm 1 0 0 infcAsParameter',
    'infc_with_array.wasm 1 1 12 infcWithArray',
    'infc_field_assign.wasm 1 1 1 fieldAssignToOther',
    'infc_field_assign.wasm 1 1 20 otherAssignToField',
    'infc_method.wasm 1 1 10 infcSetter',
    'infc_method.wasm 1 0 0 infcMethod',
    'infc_method.wasm 1 1 1 infcGetter',
    'module_case1.wasm 1 1 8 modTest',
    'module_case2.wasm 1 1 2 modTest',
    'module_case3.wasm 1 1 6 modTest',
    'module_case4.wasm 1 1 6 modTest',
    'module_case5.wasm 0 0 0 modTest',
    'module_case6.wasm 1 1 19 modTest',
    'module_case7.wasm 1 1 8 modTest',
    'namespace_case1.wasm 1 1 1 namespaceTest',
    'namespace_case2.wasm 1 1 2 namespaceTest',
    'namespace_case3.wasm 1 1 2 namespaceTest',
    'null_type_case1.wasm 1 1 20 nullTypeTest',
    'obj_case1.wasm 1 1 1 objTest',
    'obj_case2.wasm 1 1 4 objTest',
    'obj_case3.wasm 1 0 0 objTest',
    'obj_case4.wasm 1 1 6 objTest',
    'obj_case5.wasm 1 1 117 objTest',
    'parenthesized_expression_case1.wasm 1 1 5.833333333333333 parenthesizedTest',
    'primitiveType_case1.wasm 1 1 3 primitiveTest',
    'primitiveType_case2.wasm 1 1 6 primitiveTest',
    'prototype_case1.wasm 0 0 0 protoTest',
    'prototype_case2.wasm 0 0 1 protoTest',
    'rest_parameter_case1.wasm 1 1 47 restParameterTest',
    'rest_parameter_case2.wasm 0 0 0 restParameterTest',
    'return_case1.wasm 1 1 110 returnTest1',
    'return_case2.wasm 1 1 1 returnTest2 1 10 1 9',
    'return_case3.wasm 1 1 119 returnTest3 1 119',
    'return_case4.wasm 1 1 120 returnTest4 1 120',
    'scopeScanner_case1.wasm 0 0 0 scopeScannerCase1Func1',
    'scopeScanner_case2.wasm 1 1 3 scopeScannerCase2Func2 1 100',
    'scopeScanner_case3.wasm 1 1 3 scopeScannerCase3Func3 1 1',
    'scopeScanner_case4.wasm 1 1 102 scopeScannerCase4Func4 1 100',
    'scopeScanner_case5.wasm 1 1 2 scopeScannerCase5Func5 1 -100',
    'scopeScanner_case6.wasm 1 1 10 scopeScannerCase6Func6 1 1',
    'scopeScanner_case7.wasm 1 1 7 scopeScannerCase7Func7 1 3',
    'scopeScanner_case8.wasm 1 1 3 scopeScannerCase8Func8 1 1',
    'string_case1.wasm 0 0 0 strTest',
    'string_case2.wasm 0 0 0 strTest',
    'string_case3.wasm 0 0 0 strTest',
    'string_case4.wasm 0 0 0 strTest',
    'string_case5.wasm 0 0 0 strTest',
    'switch_case_case1.wasm 1 1 1 switchCaseCase1',
    'switch_case_case2.wasm 1 1 0 switchCaseCase2',
    'switch_case_case3.wasm 1 1 0 switchCaseCase3',
    'switch_case_case4.wasm 1 1 10 switchCaseCase4',
    'switch_case_case5.wasm 1 1 11 switchCaseCase5',
    'switch_case_case6.wasm 1 1 10 switchCaseCase6',
    'switch_case_case7.wasm 1 1 11 switchCaseCase7',
    'switch_case_case8.wasm 1 1 11 switchCaseCase8',
    'switch_case_case9.wasm 1 1 1 switchCaseCase9',
    'switch_case_case10.wasm 1 1 20 switchCaseCase10',
    'variable_var_case1.wasm 1 1 1 varType',
    'variable_var_case2.wasm 1 1 16 varType',
    'variable_var_case3.wasm 1 1 10 funcvv3',
    'wasmGen_globalVar.wasm 1 1 14 globalVar',
    'while_statement_case1.wasm 1 1 10 whileTest',
    'while_statement_case2.wasm 1 1 100 whileTest',
    'while_statement_case3.wasm 1 1 49 whileTest',
];

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
