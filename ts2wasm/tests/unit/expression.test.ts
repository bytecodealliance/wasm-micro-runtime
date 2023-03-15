/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import 'mocha';
import { expect } from 'chai';
import {
    Expression,
    BinaryExpression,
    IdentifierExpression,
    NumberLiteralExpression,
    ObjectLiteralExpression,
    PropertyAccessExpression,
    StringLiteralExpression,
} from '../../src/expression.js';

describe('testExpression', function () {
    it('validateExpressionRelation', function () {
        const numberLiteralExpression = new NumberLiteralExpression(3);
        const stringLiteralExpression = new StringLiteralExpression('test');
        const identifierExpression = new IdentifierExpression('a');
        const objectLiteralExpression = new ObjectLiteralExpression(
            [identifierExpression],
            [numberLiteralExpression],
        );
        const binaryExpression = new BinaryExpression(
            ts.SyntaxKind.PlusToken,
            numberLiteralExpression,
            stringLiteralExpression,
        );
        const propertyAccessExpression = new PropertyAccessExpression(
            objectLiteralExpression,
            identifierExpression,
        );

        expect(identifierExpression.identifierName).eq('a');
        expect(binaryExpression.leftOperand).eq(numberLiteralExpression);
        expect(binaryExpression.rightOperand).eq(stringLiteralExpression);
        expect(propertyAccessExpression.propertyAccessExpr).eq(
            objectLiteralExpression,
        );
        expect(propertyAccessExpression.propertyExpr).eq(identifierExpression);
    });
});
