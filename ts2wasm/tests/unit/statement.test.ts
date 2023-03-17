/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import 'mocha';
import { expect } from 'chai';
import { EmptyStatement, ForStatement } from '../../src/statement.js';
import {
    BinaryExpression,
    IdentifierExpression,
    NumberLiteralExpression,
} from '../../src/expression.js';

describe('testStatement', function () {
    it('validateStatementRelation', function () {
        const numberLiteralExpression = new NumberLiteralExpression(3);
        const identifierExpression = new IdentifierExpression('a');
        const binaryExpression = new BinaryExpression(
            ts.SyntaxKind.MinusToken,
            identifierExpression,
            numberLiteralExpression,
        );
        const emptyStatement = new EmptyStatement();
        const forStatement = new ForStatement(
            'loop0',
            'break0',
            binaryExpression,
            emptyStatement,
            null,
            null,
        );

        expect(forStatement.forLoopLabel).eq('loop0');
        expect(forStatement.forLoopBlockLabel).eq('break0');
        expect(forStatement.forLoopCondtion).eq(binaryExpression);
        expect(forStatement.forLoopBody).eq(emptyStatement);
        expect(forStatement.forLoopInitializer).eq(null);
        expect(forStatement.forLoopIncrementor).eq(null);
    });
});
