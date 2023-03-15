/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import {
    Scope,
    FunctionScope,
    GlobalScope,
    BlockScope,
    ClassScope,
    NamespaceScope,
} from './scope.js';
import {
    TSClass,
    TSInterface,
    Type,
    TypeKind,
    Primitive,
    TSArray,
} from './type.js';
import { Logger } from './log.js';

// Array() will be checked by type checker
export const enum ErrorKind {
    NominalClass = 'nominal class',
    InnerFuncDefaultParam = 'closure with default parameters',
    ExplicitlyAnyAssign = 'explicitly any',
    OperateDiffTypes = 'operate between different types',
    InvokeAnyObj = 'invoke any object',
    ArrayWithoutType = 'array without explicit type',
}

interface SematicError {
    // error kind
    errorKind: ErrorKind;
    errorFlag: number;
    // error message
    message: string;
    // function scope or global scope the error occurs in
    scope: Scope;
}

export default class SematicCheck {
    private errors: SematicError[] = [];
    private _curScope: Scope | undefined = undefined;

    constructor() {
        //
    }

    set curScope(scope: Scope | undefined) {
        this._curScope = scope;
    }

    checkRes() {
        if (this.errors.length > 0) {
            Logger.error(
                `sematic check failed, details: ${this.printErrors()}`,
            );
            throw new Error(`sematic check failed, details in log.`);
        }
    }

    checkBinaryOperate(typel: Type, typer: Type, kind?: number) {
        this.nominalClassCheck(
            typel,
            typer,
            0,
            `binaryen operate between different nominal classes`,
        );
        if (kind === ts.SyntaxKind.EqualsToken) {
            // XXX: a(number) = b(any)
            this.explicitAnyCheck(
                typel,
                typer,
                1,
                `binary operate between ${typel.kind}(left) and explicitly any(right) without type cast`,
            );
        }
        if (kind) {
            this.diffTypesOprtCheck(
                typel,
                typer,
                2,
                kind,
                `binary operate between ${typel.kind} and ${typer.kind}`,
            );
        }
        if (kind && kind !== ts.SyntaxKind.EqualsToken) {
            this.invokeAnyObj(
                typel,
                typer,
                3,
                `binary operate between any type and non-primitive type, ${typel.kind}(left), ${typer.kind}(right)`,
            );
        }
    }

    checkReturnType(type: Type) {
        const funcScope =
            this._curScope!.getNearestFunctionScope() as FunctionScope;
        const returnType = funcScope.funcType.returnType;
        this.nominalClassCheck(
            returnType,
            type,
            4,
            `return type is not the same as function return type`,
        );
        this.explicitAnyCheck(
            returnType,
            type,
            5,
            `explicitly return any type without type cast`,
        );
    }

    checkDefaultParam() {
        const parent = this._curScope!.getNearestFunctionScope()!.parent;
        const parentLevelFuncScope = parent!.getNearestFunctionScope();
        if (parentLevelFuncScope) {
            this.errors.push({
                errorKind: ErrorKind.InnerFuncDefaultParam,
                errorFlag: 6,
                message: `inner function has default parameters`,
                scope: this._curScope!,
            });
        }
    }
    // check if argsType is assignable to paramType
    checkArgTypes(paramType: Type[], argsType: Type[], hasRest: boolean) {
        let rest = new Type();
        if (hasRest) {
            rest = (<TSArray>paramType[paramType.length - 1]).elementType;
        }
        for (let i = 0; i < argsType.length; i++) {
            const param = i >= paramType.length ? rest : paramType[i];
            const arg = argsType[i];
            this.nominalClassCheck(
                param,
                arg,
                7,
                `argument type and parameter type are nominal class types`,
            );
            this.explicitAnyCheck(
                param,
                arg,
                8,
                `explicitly pass any type without type cast as argument`,
            );
        }
    }

    checkArrayType(hasTypeArg: boolean) {
        if (!hasTypeArg) {
            this.errors.push({
                errorKind: ErrorKind.ArrayWithoutType,
                errorFlag: 9,
                message: `new Array without a explicit type`,
                scope: this._curScope!,
            });
        }
    }

    private nominalClassCheck(
        typel: Type,
        typer: Type,
        flag: number,
        msg: string,
    ) {
        if (typel instanceof TSInterface || typer instanceof TSInterface) {
            return;
        }
        if (!(typel instanceof TSClass) || !(typer instanceof TSClass)) {
            return;
        }
        if (typel.className !== typer.className) {
            // is downcast
            let base = typer.getBase();
            while (base) {
                if (base.className === typel.className) {
                    return;
                }
                base = base.getBase();
            }
            this.errors.push({
                errorKind: ErrorKind.NominalClass,
                errorFlag: flag,
                message: msg,
                scope: this._curScope!,
            });
        }
    }

    private explicitAnyCheck(
        typel: Type,
        typer: Type,
        flag: number,
        msg: string,
    ) {
        if (typel.kind !== TypeKind.ANY && typer.kind === TypeKind.ANY) {
            this.errors.push({
                errorKind: ErrorKind.ExplicitlyAnyAssign,
                errorFlag: flag,
                message: msg,
                scope: this._curScope!,
            });
        }
    }

    // number + string
    private diffTypesOprtCheck(
        typel: Type,
        typer: Type,
        flag: number,
        operator: ts.SyntaxKind,
        msg: string,
    ) {
        if (
            operator === ts.SyntaxKind.PlusToken ||
            operator === ts.SyntaxKind.PlusEqualsToken
        ) {
            if (
                (typel.kind == TypeKind.NUMBER &&
                    typer.kind == TypeKind.STRING) ||
                (typel.kind == TypeKind.STRING && typer.kind == TypeKind.NUMBER)
            )
                this.errors.push({
                    errorKind: ErrorKind.OperateDiffTypes,
                    errorFlag: flag,
                    message: msg + ` with Plus or PlusEquals operator`,
                    scope: this._curScope!,
                });
        }
    }

    private invokeAnyObj(typel: Type, typer: Type, flag: number, msg: string) {
        if (
            (typel instanceof Primitive && typer.kind === TypeKind.ANY) ||
            (typer instanceof Primitive && typel.kind === TypeKind.ANY)
        ) {
            return;
        }
        if (typel.kind === TypeKind.ANY || typer.kind === TypeKind.ANY) {
            this.errors.push({
                errorKind: ErrorKind.InvokeAnyObj,
                errorFlag: flag,
                message: msg,
                scope: this._curScope!,
            });
        }
    }

    private printErrors() {
        let res = '';
        for (const error of this.errors) {
            res += `[${error.errorKind}]: in ${this.getScopeName(
                error.scope,
            )}, error flag: '${error.errorFlag}', message: '${
                error.message
            }' \n `;
        }
        return res;
    }

    private getScopeName(scope: Scope): string {
        if (scope instanceof FunctionScope) {
            return `[${scope.mangledName}]`;
        }
        if (scope instanceof GlobalScope) {
            return `[${scope.mangledName}]`;
        }
        if (scope instanceof BlockScope) {
            const nearstScope = scope.getNearestFunctionScope();
            if (nearstScope === null) {
                return `[${scope.mangledName}]`;
            } else {
                return this.getScopeName(nearstScope);
            }
        }
        if (scope instanceof ClassScope) {
            return `[${scope.mangledName}]`;
        }
        if (scope instanceof NamespaceScope) {
            return `[${scope.mangledName}]`;
        }
        return '';
    }
}
