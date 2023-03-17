/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import 'mocha';
import { expect } from 'chai';
import {
    Primitive,
    TSArray,
    TSClass,
    TSFunction,
    Type,
    TypeKind,
    TsClassField,
    FunctionKind,
} from '../../src/type.js';

describe('testType', function () {
    it('validateTypeKind', function () {
        const baseType = new Type();
        const numberType = new Primitive('number');
        const stringType = new Primitive('string');
        const boolType = new Primitive('boolean');
        const anyType = new Primitive('any');
        const nullType = new Primitive('null');
        const voidType = new Primitive('void');
        const classType = new TSClass();
        const funcType = new TSFunction();
        const numberArrayType = new TSArray(numberType);

        expect(baseType.kind).eq(TypeKind.UNKNOWN);
        expect(numberType.kind).eq(TypeKind.NUMBER);
        expect(stringType.kind).eq(TypeKind.STRING);
        expect(boolType.kind).eq(TypeKind.BOOLEAN);
        expect(anyType.kind).eq(TypeKind.ANY);
        expect(nullType.kind).eq(TypeKind.NULL);
        expect(voidType.kind).eq(TypeKind.VOID);
        expect(classType.kind).eq(TypeKind.CLASS);
        expect(funcType.kind).eq(TypeKind.FUNCTION);
        expect(numberArrayType.kind).eq(TypeKind.ARRAY);
    });

    it('judgeFunctionType', function () {
        const funcType = new TSFunction();
        const numberType = new Primitive('number');
        funcType.addParamType(numberType);
        funcType.addParamType(numberType);
        funcType.returnType = numberType;

        expect(funcType.getParamTypes().length).eq(2);
        expect(funcType.getParamTypes()[0]).eq(numberType);
        expect(funcType.getParamTypes()[1]).eq(numberType);
        expect(funcType.returnType).eq(numberType);
        expect(funcType.hasRest()).eq(false);

        funcType.setRest();
        expect(funcType.hasRest()).eq(true);
    });

    it('judgeObjectLiteralType', function () {
        const objLiteralType = new TSClass();
        const objField: TsClassField = {
            name: 'a',
            type: new Primitive('number'),
        };
        objLiteralType.addMemberField(objField);
        const funcType = new TSFunction();
        const numberType = new Primitive('number');
        funcType.addParamType(numberType);
        funcType.addParamType(numberType);
        funcType.returnType = numberType;
        objLiteralType.addMethod({
            name: 'add',
            type: funcType,
        });

        expect(objLiteralType.getMemberField('a')).eq(objField);
        expect(
            objLiteralType.getMethod('add', FunctionKind.DEFAULT).method?.type,
        ).eq(funcType);
        expect(objLiteralType.getMethod('add', FunctionKind.DEFAULT).index).eq(
            0,
        );
    });

    it('judgeClassType', function () {
        const numberType = new Primitive('number');
        const stringType = new Primitive('string');

        const baseClassType = new TSClass();
        const baseClassField: TsClassField = {
            name: 'b',
            type: numberType,
        };
        baseClassType.addMemberField(baseClassField);
        const baseFuncType = new TSFunction();
        baseFuncType.addParamType(numberType);
        baseFuncType.addParamType(numberType);
        baseFuncType.returnType = numberType;

        baseClassType.addMethod({
            name: 'add',
            type: baseFuncType,
        });

        const classType = new TSClass();
        const classField1: TsClassField = {
            name: 'a1',
            type: numberType,
        };
        const classField2: TsClassField = {
            name: 'a2',
            type: stringType,
        };
        classType.addMemberField(classField1);
        classType.addStaticMemberField(classField2);
        const funcType = new TSFunction();
        funcType.addParamType(numberType);
        funcType.addParamType(numberType);
        funcType.returnType = stringType;
        classType.addMethod({
            name: 'add',
            type: funcType,
        });
        classType.setBase(baseClassType);

        expect(baseClassType.getMemberField('b')).eq(baseClassField);
        expect(
            baseClassType.getMethod('add', FunctionKind.DEFAULT).method?.type,
        ).eq(baseFuncType);
        expect(baseClassType.getMethod('add', FunctionKind.DEFAULT).index).eq(
            0,
        );
        expect(
            classType.getMethod('add', FunctionKind.DEFAULT).method?.type,
        ).eq(funcType);
        expect(classType.getMethod('add', FunctionKind.DEFAULT).index).eq(0);
    });

    it('judgeArrayType', function () {
        const numberType = new Primitive('number');
        const arrayType1 = new TSArray(numberType);

        const stringType = new Primitive('string');
        const arrayType2 = new TSArray(stringType);

        const funcType = new TSFunction();
        funcType.addParamType(numberType);
        funcType.addParamType(numberType);
        funcType.returnType = numberType;
        const arrayType3 = new TSArray(funcType);

        const objLiteralType = new TSClass();
        const objField: TsClassField = {
            name: 'a',
            type: new Primitive('number'),
        };
        objLiteralType.addMemberField(objField);
        objLiteralType.addMethod({
            name: 'add',
            type: funcType,
        });
        const arrayType4 = new TSArray(objLiteralType);

        expect(arrayType1.elementType).eq(numberType);
        expect(arrayType2.elementType).eq(stringType);
        expect(arrayType3.elementType).eq(funcType);
        expect(arrayType4.elementType).eq(objLiteralType);
    });
});
