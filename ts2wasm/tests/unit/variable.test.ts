/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import 'mocha';
import { expect } from 'chai';
import { Variable, Parameter, ModifierKind } from '../../src/variable.js';
import { Type } from '../../src/type.js';

describe('testVariable', function () {
    it('testVariableModifier', function () {
        const param = new Parameter(
            'param',
            new Type(),
            [ts.SyntaxKind.ReadonlyKeyword],
            0,
        );
        const var1 = new Variable(
            'var1',
            new Type(),
            [ModifierKind.const, ts.SyntaxKind.DeclareKeyword],
            1,
        );
        const var2 = new Variable(
            'var2',
            new Type(),
            [
                ModifierKind.let,
                ts.SyntaxKind.DeclareKeyword,
                ts.SyntaxKind.ExportKeyword,
            ],
            2,
        );

        expect(param.isReadOnly()).eq(true);
        expect(var1.isReadOnly()).eq(false);
        expect(var1.isConst()).eq(true);
        expect(var2.isConst()).eq(false);
        expect(var1.isDeclare()).eq(true);
        expect(param.isDeclare()).eq(false);
        expect(var1.isExport()).eq(false);
        expect(var2.isExport()).eq(true);
    });
});
