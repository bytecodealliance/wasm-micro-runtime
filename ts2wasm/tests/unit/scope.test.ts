/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import 'mocha';
import { expect } from 'chai';
import {
    GlobalScope,
    FunctionScope,
    BlockScope,
    ScopeKind,
} from '../../src/scope.js';
import { Variable, Parameter, ModifierKind } from '../../src/variable.js';
import { Primitive, TSFunction, Type } from '../../src/type.js';
import { Statement } from '../../src/statement.js';
import { Expression } from '../../src/expression.js';
import { mangling } from '../../src/utils.js';

describe('testScope', function () {
    it('nestedScope', function () {
        const globalScope = new GlobalScope();
        const funcScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(funcScope);

        expect(globalScope.parent).eq(null);
        expect(funcScope.parent).eq(globalScope);
        expect(blockScope.parent).eq(funcScope);
    });

    it('scopeKind', function () {
        const globalScope = new GlobalScope();
        const funcScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(funcScope);

        expect(globalScope.kind).eq(ScopeKind.GlobalScope);
        expect(funcScope.kind).eq(ScopeKind.FunctionScope);
        expect(blockScope.kind).eq(ScopeKind.BlockScope);
    });

    it('findVariableInScope', function () {
        const globalScope = new GlobalScope();
        const var1 = new Variable('var1', new Type(), [], 0);
        globalScope.addVariable(var1);

        const funcScope = new FunctionScope(globalScope);
        const var2 = new Variable('var2', new Type(), [], 0);
        funcScope.addVariable(var2);

        const blockScope = new BlockScope(funcScope);
        const var3 = new Variable('var3', new Type(), [], 0);
        blockScope.addVariable(var3);

        expect(globalScope.findVariable('var1')).eq(var1);
        expect(globalScope.findVariable('var2')).eq(undefined);
        expect(globalScope.findVariable('var3')).eq(undefined);

        expect(funcScope.findVariable('var1')).eq(var1);
        expect(funcScope.findVariable('var2')).eq(var2);
        expect(funcScope.findVariable('var3')).eq(undefined);

        expect(blockScope.findVariable('var1')).eq(var1);
        expect(blockScope.findVariable('var2')).eq(var2);
        expect(blockScope.findVariable('var3')).eq(var3);
    });

    it('findVariableAcrossModule', function () {
        const globalScope2 = new GlobalScope();
        globalScope2.moduleName = 'module2';
        const global2var1 = new Variable('global2var1', new Type(), [], 0);
        globalScope2.addVariable(global2var1);

        const globalScope = new GlobalScope();
        globalScope.moduleName = 'module1';
        globalScope.addImportIdentifier('global2var1', globalScope2);
        const var1 = new Variable('var1', new Type(), [], 0);
        globalScope.addVariable(var1);

        const funcScope = new FunctionScope(globalScope);
        const var2 = new Variable('global2var1', new Type(), [], 0);
        const param1 = new Parameter('parma1', new Type(), [], 0, true, false);
        funcScope.addVariable(var1);
        funcScope.addVariable(var2);
        funcScope.addParameter(param1);

        expect(globalScope.findVariable('var1')).eq(var1);
        expect(globalScope.findVariable('var2')).eq(undefined);
        expect(globalScope.findIdentifier('global2var1')).eq(global2var1);

        expect(funcScope.findVariable('var1')).eq(var1);
        expect(funcScope.findVariable('global2var1')).eq(var2);
        expect(funcScope.findVariable('parma1')).eq(param1);
    });

    it('findParameterInScope', function () {
        const globalScope = new GlobalScope();
        const funcScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(funcScope);

        const param1 = new Parameter('param1', new Type(), [], 0, false, false);
        funcScope.addVariable(param1);

        expect(globalScope.findVariable('param1')).eq(undefined);
        expect(funcScope.findVariable('param1')).eq(param1);
        expect(blockScope.findVariable('param1')).eq(param1);
    });

    it('getParentAndChildren', function () {
        const globalScope = new GlobalScope();
        const functionScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(functionScope);

        expect(globalScope.parent).eq(null);
        expect(globalScope.children[0]).eq(functionScope);
        expect(functionScope.parent).eq(globalScope);
        expect(functionScope.children[0]).eq(blockScope);
        expect(blockScope.parent).eq(functionScope);
        expect(blockScope.children.length).eq(0);
    });

    it('judgeScopeKind', function () {
        const globalScope = new GlobalScope();
        const functionScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(functionScope);

        expect(globalScope.kind).eq(ScopeKind.GlobalScope);
        expect(functionScope.kind).eq(ScopeKind.FunctionScope);
        expect(blockScope.kind).eq(ScopeKind.BlockScope);
    });

    it('findFunctionScope', function () {
        const globalScope = new GlobalScope();
        const functionScope = new FunctionScope(globalScope);
        const blockScope = new BlockScope(functionScope);
        functionScope.setFuncName('function1');

        const internalFunctionScope = new FunctionScope(blockScope);
        const internalBlockScope = new BlockScope(internalFunctionScope);
        internalFunctionScope.setFuncName('function2');

        expect(blockScope.findFunctionScope('function1')).eq(functionScope);
        expect(functionScope.findFunctionScope('function1', false)).eq(
            undefined,
        );
        expect(functionScope.findFunctionScope('function1')).eq(functionScope);
        expect(blockScope.findFunctionScope('function2')).eq(
            internalFunctionScope,
        );
        expect(internalBlockScope.findFunctionScope('function2', false)).eq(
            undefined,
        );
        expect(internalBlockScope.findFunctionScope('function1')).eq(
            functionScope,
        );
    });

    it('getFunctionFromGlobalScope', function () {
        const globalScope = new GlobalScope();
        globalScope.moduleName = 'moduleA';
        mangling([globalScope]);
        expect(globalScope.startFuncName).eq('moduleA|start');
    });

    it('getStartFunctionStatementArray', function () {
        const globalScope = new GlobalScope();
        globalScope.addStatement(new Statement(ts.SyntaxKind.IfStatement));
        globalScope.addStatement(new Statement(ts.SyntaxKind.ForInStatement));

        expect(globalScope.statements.length).eq(2);
        expect(globalScope.statements[0].statementKind).eq(
            ts.SyntaxKind.IfStatement,
        );
        expect(globalScope.statements[1].statementKind).eq(
            ts.SyntaxKind.ForInStatement,
        );
    });

    it('getNearestFunctionScope', function () {
        const globalScope = new GlobalScope();
        const funcScope1 = new FunctionScope(globalScope);
        const blockScope1 = new BlockScope(funcScope1);
        const funcScope2 = new FunctionScope(funcScope1);
        const funcScope3 = new FunctionScope(blockScope1);
        const blockScope2 = new BlockScope(funcScope2);

        expect(blockScope1.getNearestFunctionScope()).eq(funcScope1);
        expect(funcScope3.getNearestFunctionScope()).eq(funcScope3);
        expect(blockScope2.getNearestFunctionScope()).eq(funcScope2);
    });

    it('getRootGloablScope', function () {
        const globalScope1 = new GlobalScope();
        const funcScope1 = new FunctionScope(globalScope1);
        const blockScope1 = new BlockScope(funcScope1);
        const globalScope2 = new GlobalScope();
        const funcScope2 = new FunctionScope(globalScope2);
        const blockScope2 = new BlockScope(funcScope2);

        expect(blockScope1.getRootGloablScope()).eq(globalScope1);
        expect(blockScope2.getRootGloablScope()).eq(globalScope2);
    });

    it('getTSType', function () {
        const globalScope = new GlobalScope();
        const funcScope = new FunctionScope(globalScope);
        const funcType = new TSFunction();
        funcScope.addType('test', funcType);

        expect(funcScope.findType('test')).eq(funcType);
    });
});
