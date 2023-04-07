/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import path from 'path';
import {
    BlockScope,
    ClassScope,
    FunctionScope,
    GlobalScope,
    NamespaceScope,
    Scope,
} from './scope.js';
import ExpressionProcessor, { Expression } from './expression.js';
import { BuiltinNames } from '../lib/builtin/builtin_name.js';
import { Type } from './type.js';
import { UnimplementError } from './error.js';

export interface importGlobalInfo {
    internalName: string;
    externalModuleName: string;
    externalBaseName: string;
    globalType: Type;
}

export interface importFunctionInfo {
    internalName: string;
    externalModuleName: string;
    externalBaseName: string;
    funcType: Type;
}

export enum MatchKind {
    ExactMatch,
    ToAnyMatch,
    FromAnyMatch,
    ClassMatch,
    ClassInheritMatch,
    ClassInfcMatch,
    ToArrayAnyMatch,
    FromArrayAnyMatch,
    MisMatch,
}

export class Stack<T> {
    private items: T[] = [];
    push(item: T) {
        this.items.push(item);
    }
    pop() {
        if (this.isEmpty()) {
            throw new Error('Current stack is empty, can not pop');
        }
        return this.items.pop()!;
    }
    peek() {
        if (this.isEmpty()) {
            throw new Error('Current stack is empty, can not get peek item');
        }
        return this.items[this.items.length - 1];
    }
    isEmpty() {
        return this.items.length === 0;
    }
    clear() {
        this.items = [];
    }
    size() {
        return this.items.length;
    }
    getItemAtIdx(index: number) {
        if (index >= this.items.length) {
            throw new Error('index is greater than the size of the stack');
        }
        return this.items[index];
    }
}

export function getCurScope(
    node: ts.Node,
    nodeScopeMap: Map<ts.Node, Scope>,
): Scope | null {
    if (!node) return null;
    const scope = nodeScopeMap.get(node);
    if (scope) return scope;
    return getCurScope(node.parent, nodeScopeMap);
}

export function getNearestFunctionScopeFromCurrent(currentScope: Scope | null) {
    if (!currentScope) {
        throw new Error('current scope is null');
    }
    const functionScope = currentScope.getNearestFunctionScope();
    if (!functionScope) {
        return null;
    }
    return functionScope;
}

export function generateNodeExpression(
    exprCompiler: ExpressionProcessor,
    node: ts.Node,
): Expression {
    return exprCompiler.visitNode(node);
}

export function parentIsFunctionLike(node: ts.Node) {
    if (
        node.parent.kind === ts.SyntaxKind.FunctionDeclaration ||
        node.parent.kind === ts.SyntaxKind.MethodDeclaration ||
        node.parent.kind === ts.SyntaxKind.SetAccessor ||
        node.parent.kind === ts.SyntaxKind.GetAccessor ||
        node.parent.kind === ts.SyntaxKind.FunctionExpression ||
        node.parent.kind === ts.SyntaxKind.ArrowFunction
    ) {
        return true;
    }

    return false;
}

export function parentIsLoopLike(node: ts.Node) {
    if (
        node.parent.kind === ts.SyntaxKind.ForStatement ||
        node.parent.kind === ts.SyntaxKind.DoStatement ||
        node.parent.kind === ts.SyntaxKind.WhileStatement
    ) {
        return true;
    }

    return false;
}

export function parentIsCaseClause(node: ts.Node) {
    if (
        node.parent.kind === ts.SyntaxKind.CaseClause ||
        node.parent.kind === ts.SyntaxKind.DefaultClause
    ) {
        return true;
    }

    return false;
}

export function isScopeNode(node: ts.Node) {
    if (
        node.kind === ts.SyntaxKind.SourceFile ||
        node.kind === ts.SyntaxKind.ModuleDeclaration ||
        node.kind === ts.SyntaxKind.FunctionDeclaration ||
        node.kind === ts.SyntaxKind.FunctionExpression ||
        node.kind === ts.SyntaxKind.ArrowFunction ||
        node.kind === ts.SyntaxKind.ClassDeclaration ||
        node.kind === ts.SyntaxKind.SetAccessor ||
        node.kind === ts.SyntaxKind.GetAccessor ||
        node.kind === ts.SyntaxKind.Constructor ||
        node.kind === ts.SyntaxKind.MethodDeclaration ||
        node.kind === ts.SyntaxKind.ForStatement ||
        node.kind === ts.SyntaxKind.WhileStatement ||
        node.kind === ts.SyntaxKind.DoStatement ||
        node.kind === ts.SyntaxKind.CaseClause ||
        node.kind === ts.SyntaxKind.DefaultClause
    ) {
        if (ts.isAccessor(node) && !node.body) {
            return false;
        }
        return true;
    }
    if (node.kind === ts.SyntaxKind.Block && !parentIsFunctionLike(node)) {
        return true;
    }
    return false;
}

export function mangling(
    scopeArray: Array<Scope>,
    delimiter = BuiltinNames.moduleDelimiter,
    prefixStack: Array<string> = [],
) {
    scopeArray.forEach((scope) => {
        const currName = scope.getName();
        if (scope instanceof GlobalScope) {
            scope.startFuncName = `${currName}|start`;
            prefixStack.push(currName);

            scope.varArray.forEach((v) => {
                v.mangledName = `${prefixStack.join(delimiter)}|${v.varName}`;
            });
        } else if (scope instanceof NamespaceScope) {
            prefixStack.push(currName);

            scope.varArray.forEach((v) => {
                v.mangledName = `${prefixStack.join(delimiter)}|${v.varName}`;
            });
        } else if (scope instanceof FunctionScope) {
            prefixStack.push(currName);
        } else if (scope instanceof ClassScope) {
            prefixStack.push(currName);
            scope.classType.mangledName = `${prefixStack.join(delimiter)}`;
        } else if (scope instanceof BlockScope) {
            prefixStack.push(currName);
        }

        scope.mangledName = `${prefixStack.join(delimiter)}`;

        mangling(scope.children, delimiter, prefixStack);
        prefixStack.pop();
    });
}

export function getImportModulePath(
    importDeclaration: ts.ImportDeclaration,
    currentGlobalScope: GlobalScope,
) {
    /* moduleSpecifier contains quotation marks, so we must slice them to get real module name */
    const moduleSpecifier = importDeclaration.moduleSpecifier
        .getText()
        .slice("'".length, -"'".length);
    const currentModuleName = currentGlobalScope.moduleName;
    const importModuleName = path.relative(
        process.cwd(),
        path.resolve(path.dirname(currentModuleName), moduleSpecifier),
    );
    return importModuleName;
}

export function getGlobalScopeByModuleName(
    moduleName: string,
    globalScopes: Array<GlobalScope>,
) {
    const res = globalScopes.find((s) => s.moduleName === moduleName);
    if (!res) {
        throw Error(`no such module: ${moduleName}`);
    }

    return res;
}

export function getImportIdentifierName(
    importDeclaration: ts.ImportDeclaration,
) {
    const importIdentifierArray: string[] = [];
    const nameAliasImportMap = new Map<string, string>();
    let nameScopeImportName: string | null = null;
    let defaultImportName: string | null = null;
    const importClause = importDeclaration.importClause;
    if (!importClause) {
        /** import "otherModule" */
        throw new UnimplementError('TODO: importing modules with side effects');
    }
    const namedBindings = importClause.namedBindings;
    const importElement = importClause.name;
    if (importElement) {
        /**
         * import default export from other module
         * import module_case4_var1 from './module-case4';
         */
        const importElementName = importElement.getText();
        defaultImportName = importElementName;
    }
    if (namedBindings) {
        if (ts.isNamedImports(namedBindings)) {
            /** import regular exports from other module */
            for (const importSpecifier of namedBindings.elements) {
                const specificIdentifier = <ts.Identifier>importSpecifier.name;
                const specificName = specificIdentifier.getText()!;
                const propertyIdentifier = importSpecifier.propertyName;
                if (propertyIdentifier) {
                    /** import {module_case2_var1 as a, module_case2_func1 as b} from './module-case2'; */
                    const propertyName = (<ts.Identifier>(
                        propertyIdentifier
                    )).getText()!;
                    nameAliasImportMap.set(specificName, propertyName);
                    importIdentifierArray.push(propertyName);
                } else {
                    /** import {module_case2_var1, module_case2_func1} from './module-case2'; */
                    importIdentifierArray.push(specificName);
                }
            }
        } else if (ts.isNamespaceImport(namedBindings)) {
            /**
             * import entire module into a variable
             * import * as xx from './yy'
             */
            const identifier = <ts.Identifier>namedBindings.name;
            nameScopeImportName = identifier.getText()!;
        } else {
            throw Error('unexpected case');
        }
    }

    return {
        importIdentifierArray,
        nameScopeImportName,
        nameAliasImportMap,
        defaultImportName,
    };
}

export function getExportIdentifierName(
    exportDeclaration: ts.ExportDeclaration,
) {
    const nameAliasExportMap = new Map<string, string>();
    // only need to record export alias
    const exportClause = exportDeclaration.exportClause;
    if (!exportClause) {
        throw Error('exportClause is undefined');
    }
    if (ts.isNamedExports(exportClause)) {
        const exportSpecifiers = exportClause.elements;
        for (const exportSpecifier of exportSpecifiers) {
            const specificIdentifier = <ts.Identifier>exportSpecifier.name;
            const specificName = specificIdentifier.getText()!;
            const propertyIdentifier = exportSpecifier.propertyName;
            if (propertyIdentifier) {
                const propertyName = (<ts.Identifier>(
                    propertyIdentifier
                )).getText()!;
                nameAliasExportMap.set(specificName, propertyName);
            }
        }
    }

    return nameAliasExportMap;
}

export function getBuiltInFuncName(oriFuncName: string) {
    return BuiltinNames.builtinModuleName
        .concat(BuiltinNames.moduleDelimiter)
        .concat(oriFuncName);
}
