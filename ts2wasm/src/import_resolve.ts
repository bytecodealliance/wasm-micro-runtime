/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import { ParserContext } from './frontend.js';
import {
    getExportIdentifierName,
    getGlobalScopeByModuleName,
    getImportIdentifierName,
    getImportModulePath,
} from './utils.js';
import { GlobalScope, Scope } from './scope.js';
import { BuiltinNames } from '../lib/builtin/builtin_name.js';

export class ImportResolver {
    globalScopes: Array<GlobalScope>;
    currentScope: Scope | null = null;
    nodeScopeMap: Map<ts.Node, Scope>;

    constructor(private parserCtx: ParserContext) {
        this.globalScopes = this.parserCtx.globalScopes;
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
    }

    visit() {
        /* Handle import and export nodes */
        this.nodeScopeMap.forEach((scope, node) => {
            ts.forEachChild(node, this.visitNode.bind(this));
        });
        /* Auto import the standard library module for every user file */
        const builtinScope = this.globalScopes[1];
        for (
            let i = this.parserCtx.builtinFileNames.length;
            i < this.globalScopes.length;
            i++
        ) {
            for (const builtinIdentifier of BuiltinNames.builtinIdentifierArray) {
                this.globalScopes[i].addImportIdentifier(
                    builtinIdentifier,
                    builtinScope,
                );
            }
        }
    }

    visitNode(node: ts.Node): void {
        this.currentScope = this.parserCtx.getScopeByNode(node)!;
        switch (node.kind) {
            case ts.SyntaxKind.ImportDeclaration: {
                const importDeclaration = <ts.ImportDeclaration>node;
                const globalScope = this.currentScope!.getRootGloablScope()!;
                // Get the import module name according to the relative position of current scope
                const importModuleName = getImportModulePath(
                    importDeclaration,
                    this.currentScope!.getRootGloablScope()!,
                );
                const importModuleScope = getGlobalScopeByModuleName(
                    importModuleName,
                    this.globalScopes,
                );
                // get import identifier
                const {
                    importIdentifierArray,
                    nameScopeImportName,
                    nameAliasImportMap,
                    defaultImportName,
                } = getImportIdentifierName(importDeclaration);
                for (const importIdentifier of importIdentifierArray) {
                    globalScope.addImportIdentifier(
                        importIdentifier,
                        importModuleScope,
                    );
                }
                globalScope.setImportNameAlias(nameAliasImportMap);
                if (nameScopeImportName) {
                    globalScope.addImportNameScope(
                        nameScopeImportName,
                        importModuleScope,
                    );
                }
                if (defaultImportName) {
                    globalScope.addImportDefaultName(
                        defaultImportName,
                        importModuleScope,
                    );
                }
                break;
            }
            case ts.SyntaxKind.ExportDeclaration: {
                const exportDeclaration = <ts.ExportDeclaration>node;
                const globalScope = this.currentScope!.getRootGloablScope()!;
                const nameAliasExportMap =
                    getExportIdentifierName(exportDeclaration);
                globalScope.setExportNameAlias(nameAliasExportMap);
                break;
            }
            case ts.SyntaxKind.ExportAssignment: {
                const exportAssign = <ts.ExportAssignment>node;
                const globalScope = this.currentScope!.getRootGloablScope()!;
                const defaultIdentifier = <ts.Identifier>(
                    exportAssign.expression
                );
                const defaultName = defaultIdentifier.getText()!;
                globalScope.defaultNoun = defaultName;
                break;
            }
            case ts.SyntaxKind.FunctionDeclaration:
            case ts.SyntaxKind.ClassDeclaration: {
                const curNode = <ts.ClassDeclaration | ts.FunctionDeclaration>(
                    node
                );
                if (ts.getModifiers(curNode)) {
                    for (const modifier of ts.getModifiers(curNode)!) {
                        if (modifier.kind === ts.SyntaxKind.DefaultKeyword) {
                            const globalScope =
                                this.currentScope!.getRootGloablScope()!;
                            const defaultName = curNode.name!.getText()!;
                            globalScope.defaultNoun = defaultName;
                            break;
                        }
                    }
                }
                break;
            }
            default: {
                ts.forEachChild(node, this.visitNode.bind(this));
            }
        }
    }
}
