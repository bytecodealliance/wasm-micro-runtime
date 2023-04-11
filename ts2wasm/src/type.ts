/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import { ParserContext } from './frontend.js';
import {
    ClassScope,
    FunctionScope,
    GlobalScope,
    Scope,
    ScopeKind,
} from './scope.js';
import { Parameter, Variable } from './variable.js';
import { Expression } from './expression.js';
import { Logger } from './log.js';

export const enum TypeKind {
    VOID = 'void',
    BOOLEAN = 'boolean',
    NUMBER = 'number',
    ANY = 'any',
    STRING = 'string',
    ARRAY = 'array',
    FUNCTION = 'function',
    CLASS = 'class',
    NULL = 'null',
    INTERFACE = 'interface',
    UNKNOWN = 'unknown',
}

export class Type {
    typeKind = TypeKind.UNKNOWN;
    isPrimitive = false;

    get kind(): TypeKind {
        return this.typeKind;
    }
}

export class Primitive extends Type {
    typeKind;
    constructor(private type: string) {
        super();
        this.isPrimitive = true;
        switch (type) {
            case 'number': {
                this.typeKind = TypeKind.NUMBER;
                break;
            }
            case 'string': {
                this.typeKind = TypeKind.STRING;
                break;
            }
            case 'boolean': {
                this.typeKind = TypeKind.BOOLEAN;
                break;
            }
            case 'any': {
                this.typeKind = TypeKind.ANY;
                break;
            }
            case 'void': {
                this.typeKind = TypeKind.VOID;
                break;
            }
            case 'null': {
                this.typeKind = TypeKind.NULL;
                break;
            }
            default: {
                this.typeKind = TypeKind.UNKNOWN;
            }
        }
    }
}

export const builtinTypes = new Map<string, Type>([
    ['number', new Primitive('number')],
    ['string', new Primitive('string')],
    ['boolean', new Primitive('boolean')],
    ['any', new Primitive('any')],
    ['void', new Primitive('void')],
    ['null', new Primitive('null')],
]);

export interface TsClassField {
    name: string;
    type: Type;
    modifier?: 'readonly';
    visibility?: 'public' | 'protected' | 'private';
    static?: 'static';
}

export const enum FunctionKind {
    DEFAULT = 'default',
    CONSTRUCTOR = 'constructor',
    METHOD = 'method',
    GETTER = 'getter',
    SETTER = 'setter',
    STATIC = 'static',
}

export function getMethodPrefix(kind: FunctionKind): string {
    switch (kind) {
        case FunctionKind.CONSTRUCTOR:
            return 'constructor';
        case FunctionKind.GETTER:
            return 'get_';
        case FunctionKind.SETTER:
            return 'set_';
        default:
            return '';
    }
}

export interface TsClassFunc {
    name: string;
    type: TSFunction;
}

export interface ClassMethod {
    index: number;
    method: TsClassFunc | null;
}

export class TSClass extends Type {
    typeKind = TypeKind.CLASS;
    private _typeId = 0;
    private _name = '';
    private _mangledName = '';
    private _memberFields: Array<TsClassField> = [];
    private _staticFields: Array<TsClassField> = [];
    private _methods: Array<TsClassFunc> = [];
    private _baseClass: TSClass | null = null;

    public staticFieldsInitValueMap: Map<number, Expression> = new Map();
    /* override or own methods */
    public overrideOrOwnMethods: Set<string> = new Set();

    constructor() {
        super();
    }

    get fields(): Array<TsClassField> {
        return this._memberFields;
    }

    get staticFields(): TsClassField[] {
        return this._staticFields;
    }

    get memberFuncs(): Array<TsClassFunc> {
        return this._methods;
    }

    setBase(base: TSClass): void {
        this._baseClass = base;
    }

    getBase(): TSClass | null {
        return this._baseClass;
    }

    addMemberField(memberField: TsClassField): void {
        this._memberFields.push(memberField);
    }

    getMemberField(name: string): TsClassField | null {
        return (
            this._memberFields.find((f) => {
                return f.name === name;
            }) || null
        );
    }

    getMemberFieldIndex(name: string): number {
        return this._memberFields.findIndex((f) => {
            return f.name === name;
        });
    }

    addStaticMemberField(memberField: TsClassField): void {
        this._staticFields.push(memberField);
    }

    getStaticMemberField(name: string): TsClassField | null {
        return (
            this._staticFields.find((f) => {
                return f.name === name;
            }) || null
        );
    }

    getStaticFieldIndex(name: string): number {
        return this._staticFields.findIndex((f) => {
            return f.name === name;
        });
    }

    addMethod(classMethod: TsClassFunc): void {
        classMethod.type.isMethod = true;
        this._methods.push(classMethod);
    }

    getMethod(
        name: string,
        kind: FunctionKind = FunctionKind.METHOD,
    ): ClassMethod {
        const res = this.memberFuncs.findIndex((f) => {
            return name === f.name && kind === f.type.funcKind;
        });
        if (res !== -1) {
            return { index: res, method: this.memberFuncs[res] };
        }
        return { index: -1, method: null };
    }

    setClassName(name: string) {
        this._name = name;
    }

    get className(): string {
        return this._name;
    }

    get mangledName(): string {
        return this._mangledName;
    }

    set mangledName(name: string) {
        this._mangledName = name;
    }

    setTypeId(id: number) {
        this._typeId = id;
    }

    get typeId() {
        return this._typeId;
    }
}

export class TSInterface extends TSClass {
    typeKind = TypeKind.INTERFACE;

    constructor() {
        super();
    }
}

export class TSArray extends Type {
    typeKind = TypeKind.ARRAY;
    constructor(private _elemType: Type) {
        super();
    }

    get elementType(): Type {
        return this._elemType;
    }
}

export class TSFunction extends Type {
    typeKind = TypeKind.FUNCTION;
    private _parameterTypes: Type[] = [];
    private _returnType: Type = new Primitive('void');
    // iff last parameter is rest paremeter
    private _hasRestParameter = false;
    private _isMethod = false;
    private _isDeclare = false;
    private _isStatic = false;

    constructor(public funcKind: FunctionKind = FunctionKind.DEFAULT) {
        super();
    }

    set returnType(type: Type) {
        this._returnType = type;
    }

    get returnType(): Type {
        return this._returnType;
    }

    addParamType(paramType: Type) {
        this._parameterTypes.push(paramType);
    }

    getParamTypes(): Type[] {
        return this._parameterTypes;
    }

    setRest() {
        this._hasRestParameter = true;
    }

    hasRest() {
        return this._hasRestParameter === true;
    }

    get isMethod() {
        return this._isMethod;
    }

    set isMethod(value: boolean) {
        this._isMethod = value;
    }

    get isDeclare() {
        return this._isDeclare;
    }

    set isDeclare(value: boolean) {
        this._isDeclare = value;
    }

    get isStatic() {
        return this._isStatic;
    }

    set isStatic(value: boolean) {
        this._isStatic = value;
    }

    // shadow copy, content of parameterTypes and returnType is not copied
    public clone(): TSFunction {
        const func = new TSFunction(this.funcKind);
        func.returnType = this.returnType;
        func._parameterTypes = this._parameterTypes;
        func._hasRestParameter = this._hasRestParameter;
        func.isMethod = this.isMethod;
        func.isDeclare = this.isDeclare;
        func.isStatic = this.isStatic;
        return func;
    }
}

export default class TypeResolver {
    typechecker: ts.TypeChecker | undefined = undefined;
    globalScopes: Array<GlobalScope>;
    currentScope: Scope | null = null;
    nodeScopeMap: Map<ts.Node, Scope>;
    // cache class shape layout string, <class name, type string>
    methodShapeStr = new Map<string, string>();
    fieldShapeStr = new Map<string, string>();
    // cache class shape layout, <ts.Node, tsType>
    nodeTypeCache = new Map<ts.Node, TSClass>();

    constructor(private parserCtx: ParserContext) {
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
        this.globalScopes = this.parserCtx.globalScopes;
    }

    visit() {
        this.typechecker = this.parserCtx.typeChecker;
        this.nodeScopeMap.forEach((scope, node) => {
            ts.forEachChild(node, this.visitNode.bind(this));
        });
    }

    private visitNode(node: ts.Node): void {
        this.currentScope = this.parserCtx.getScopeByNode(node)!;

        switch (node.kind) {
            case ts.SyntaxKind.VariableDeclaration:
            case ts.SyntaxKind.Parameter:
            case ts.SyntaxKind.FunctionDeclaration:
            case ts.SyntaxKind.FunctionExpression:
            case ts.SyntaxKind.ArrowFunction: {
                const type = this.generateNodeType(node);
                this.addTypeToTypeMap(type, node);
                break;
            }
            case ts.SyntaxKind.ClassDeclaration: {
                const type = this.parseClassDecl(node as ts.ClassDeclaration);
                this.addTypeToTypeMap(type, node);
                break;
            }
            case ts.SyntaxKind.InterfaceDeclaration: {
                const type = this.parseInfcDecl(
                    node as ts.InterfaceDeclaration,
                );
                this.addTypeToTypeMap(type, node);
                break;
            }
        }
        ts.forEachChild(node, this.visitNode.bind(this));
    }

    private addTypeToTypeMap(type: Type, node: ts.Node) {
        const tsTypeString = this.typechecker!.typeToString(
            this.typechecker!.getTypeAtLocation(node),
        );

        if (
            this.currentScope!.kind === ScopeKind.FunctionScope &&
            type.kind === TypeKind.FUNCTION &&
            !ts.isParameter(node) &&
            !ts.isVariableDeclaration(node)
        ) {
            (<FunctionScope>this.currentScope!).setFuncType(type as TSFunction);
        }
        if (ts.isClassDeclaration(node)) {
            this.currentScope!.parent!.addType(tsTypeString, type);
            if (this.currentScope! instanceof ClassScope) {
                this.currentScope!.setClassType(type as TSClass);
            }
        } else {
            this.currentScope!.addType(tsTypeString, type);
        }
    }

    generateNodeType(node: ts.Node): Type {
        if (ts.isConstructorDeclaration(node)) {
            return this.parseSignature(
                this.typechecker!.getSignatureFromDeclaration(node)!,
            );
        }
        let tsType = this.typechecker!.getTypeAtLocation(node);
        if ('isThisType' in tsType && (tsType as any).isThisType) {
            /* For "this" keyword, tsc will inference the actual type */
            tsType = this.typechecker!.getDeclaredTypeOfSymbol(tsType.symbol);
        }
        const type = this.tsTypeToType(tsType);
        /* for example, a: string[] = new Array(), the type of new Array() should be string[]
         instead of any[]*/
        if (type instanceof TSArray) {
            const parentNode = node.parent;
            if (
                ts.isVariableDeclaration(parentNode) ||
                ts.isBinaryExpression(parentNode)
            ) {
                return this.generateNodeType(parentNode);
            }
            if (
                ts.isNewExpression(parentNode) ||
                ts.isArrayLiteralExpression(parentNode)
            ) {
                return (<TSArray>this.generateNodeType(parentNode)).elementType;
            }
        }
        return type;
    }

    private tsTypeToType(type: ts.Type): Type {
        const typeFlag = type.flags;
        // basic types
        if (
            typeFlag & ts.TypeFlags.Number ||
            typeFlag & ts.TypeFlags.NumberLiteral
        ) {
            return builtinTypes.get('number')!;
        }
        if (
            typeFlag & ts.TypeFlags.String ||
            typeFlag & ts.TypeFlags.StringLiteral
        ) {
            return builtinTypes.get('string')!;
        }
        if (
            typeFlag & ts.TypeFlags.Boolean ||
            typeFlag & ts.TypeFlags.BooleanLiteral
        ) {
            return builtinTypes.get('boolean')!;
        }
        if (typeFlag & ts.TypeFlags.Void) {
            return builtinTypes.get('void')!;
        }
        if (typeFlag & ts.TypeFlags.Any || typeFlag & ts.TypeFlags.Undefined) {
            return builtinTypes.get('any')!;
        }
        if (typeFlag & ts.TypeFlags.Null) {
            return builtinTypes.get('null')!;
        }
        // union type ==> type of first elem, iff all types are same, otherwise, any
        if (type.isUnion()) {
            const nodeTypeArray = type.types.map((elem) => {
                return this.tsTypeToType(elem);
            });
            let res = builtinTypes.get('any')!;
            // iff there is at least one null type
            if (nodeTypeArray.find((type) => type.kind === TypeKind.NULL)) {
                const nonNullTypes = nodeTypeArray.filter(
                    (type) => type.kind !== TypeKind.NULL,
                );
                // iff A | null => ref.null A, otherwise => any
                if (
                    nonNullTypes.length > 0 &&
                    nonNullTypes.every((type) => type === nonNullTypes[0]) &&
                    !nonNullTypes[0].isPrimitive
                ) {
                    res = nonNullTypes[0];
                }
            } else {
                if (nodeTypeArray.every((type) => type === nodeTypeArray[0])) {
                    res = nodeTypeArray[0];
                }
            }
            return res;
        }
        // sophisticated types
        //               object
        //           /    \         \
        // typereference objliteral function
        //    / \
        // array class/infc

        // iff array type
        if (this.isArray(type)) {
            if (!type.typeArguments) {
                throw new Error('array type has no type arguments');
            }
            const elemType = this.tsTypeToType(type.typeArguments![0]);
            return new TSArray(elemType);
        }
        // iff class/infc
        if (this.isTypeReference(type) || this.isInterface(type)) {
            const decl = type.symbol.declarations![0];
            const tsType = this.nodeTypeCache.get(decl);
            if (!tsType) {
                throw new Error(
                    `class/interface not found, type name <${type.symbol.name}>. `,
                );
            }
            return tsType;
        }
        // iff object literal type
        if (this.isObjectLiteral(type)) {
            const tsClass = new TSClass();
            tsClass.setClassName('@object_literal');
            const methodTypeStrs: string[] = [];
            const fieldTypeStrs: string[] = [];
            type.getProperties().map((prop) => {
                const propertyAssignment =
                    prop.valueDeclaration as ts.PropertyAssignment;
                const propType =
                    this.typechecker!.getTypeAtLocation(propertyAssignment);
                let typeString = this.typeToString(propertyAssignment);
                // ts.Type's intrinsicName is `true` or `false`, instead of `boolean`
                if (typeString === 'true' || typeString === 'false') {
                    typeString = 'boolean';
                }
                const fieldName = prop.name;
                const tsType = this.tsTypeToType(propType);
                if (tsType instanceof TSFunction) {
                    tsType.funcKind = FunctionKind.DEFAULT;
                    tsClass.addMethod({
                        name: fieldName,
                        type: tsType,
                    });
                    methodTypeStrs.push(`${fieldName}: ${typeString}`);
                } else {
                    tsClass.addMemberField({
                        name: fieldName,
                        type: tsType,
                    });
                    fieldTypeStrs.push(`${fieldName}: ${typeString}`);
                }
            });
            const typeString =
                methodTypeStrs.join(', ') + ', ' + fieldTypeStrs.join(', ');
            tsClass.setTypeId(this.generateTypeId(typeString));
            Logger.info(
                `Assign type id [${tsClass.typeId}] for object literal type: ${typeString}`,
            );
            return tsClass;
        }

        // iff function type
        if (this.isFunction(type)) {
            const signature = type.getCallSignatures()[0];
            return this.parseSignature(signature);
        }

        Logger.debug(`Encounter un-processed type: ${type.flags}`);
        /* cases have not been considered or covered yet... */
        return new Type();
    }

    private isObject(type: ts.Type): type is ts.ObjectType {
        return !!(type.flags & ts.TypeFlags.Object);
    }

    private isTypeReference(type: ts.Type): type is ts.TypeReference {
        return (
            this.isObject(type) &&
            !!(type.objectFlags & ts.ObjectFlags.Reference)
        );
    }

    private isInterface(type: ts.Type): type is ts.InterfaceType {
        return (
            this.isObject(type) &&
            !!(type.objectFlags & ts.ObjectFlags.Interface)
        );
    }

    private isObjectLiteral(type: ts.Type) {
        return this.isObject(type) && type.symbol.name === '__object';
    }

    private isArray(type: ts.Type): type is ts.TypeReference {
        return this.isTypeReference(type) && type.symbol.name === 'Array';
    }

    private isFunction(type: ts.Type): type is ts.ObjectType {
        if (this.isObject(type)) {
            return type.getCallSignatures().length > 0;
        }
        return false;
    }

    private parseSignature(signature: ts.Signature | undefined) {
        if (!signature) {
            throw new Error('signature is undefined');
        }

        const tsFunction = new TSFunction();
        signature.getParameters().map((param) => {
            const valueDecl = param.valueDeclaration!;
            if (ts.isParameter(valueDecl) && valueDecl.dotDotDotToken) {
                tsFunction.setRest();
            }
            const tsType = this.tsTypeToType(
                this.typechecker!.getTypeAtLocation(valueDecl),
            );

            tsFunction.addParamType(tsType);
        });

        const returnType =
            this.typechecker!.getReturnTypeOfSignature(signature);
        if (
            !signature.declaration ||
            !ts.isConstructorDeclaration(signature.declaration)
        ) {
            tsFunction.returnType = this.tsTypeToType(returnType);
        }

        tsFunction.isDeclare = signature.declaration
            ? this.parseNestDeclare(
                  <
                      | ts.FunctionLikeDeclaration
                      | ts.ModuleDeclaration
                      | ts.ClassDeclaration
                  >signature.declaration,
              )
            : false;

        tsFunction.isStatic = signature.declaration
            ? this.parseStatic(
                  <ts.FunctionLikeDeclaration>signature.declaration,
              )
            : false;
        return tsFunction;
    }

    private parseNestDeclare(
        node:
            | ts.FunctionLikeDeclaration
            | ts.ModuleDeclaration
            | ts.ClassDeclaration,
    ): boolean {
        let res = false;
        if (node.modifiers) {
            const hasDeclareKeyword = node.modifiers.find((modifier) => {
                return modifier.kind === ts.SyntaxKind.DeclareKeyword;
            });
            if (hasDeclareKeyword) {
                return true;
            }
        }
        if (node.parent.kind === ts.SyntaxKind.ModuleBlock) {
            res = this.parseNestDeclare(
                <ts.ModuleDeclaration>node.parent.parent,
            );
        } else if (node.parent.kind === ts.SyntaxKind.ClassDeclaration) {
            res = this.parseNestDeclare(<ts.ModuleDeclaration>node.parent);
        }
        return res;
    }

    private parseStatic(node: ts.FunctionLikeDeclaration): boolean {
        let res = false;
        if (node.modifiers) {
            const hasStaticKeyword = node.modifiers.find((modifier) => {
                return modifier.kind === ts.SyntaxKind.StaticKeyword;
            });
            if (hasStaticKeyword) {
                res = true;
            }
        }
        return res;
    }

    private parseClassDecl(node: ts.ClassDeclaration): TSClass {
        const classType = new TSClass();
        this.nodeTypeCache.set(node, classType);
        classType.setClassName(node.name!.getText());
        let methodTypeStrs: string[] = [];
        let fieldTypeStrs: string[] = [];

        const heritage = node.heritageClauses;
        let baseType: TSClass | null = null;
        if (
            heritage !== undefined &&
            heritage[0].token !== ts.SyntaxKind.ImplementsKeyword
        ) {
            /* base class node, iff it really has the one */
            const heritageName = heritage[0].types[0].getText();
            methodTypeStrs = this.methodShapeStr.get(heritageName)!.split(', ');
            fieldTypeStrs = this.fieldShapeStr.get(heritageName)!.split(', ');
            const scope = this.currentScope!;
            const heritageType = <TSClass>scope.findType(heritageName);
            classType.setBase(heritageType);
            baseType = heritageType;
            for (const field of heritageType.fields) {
                classType.addMemberField(field);
            }
            for (const pair of heritageType.staticFieldsInitValueMap) {
                classType.staticFieldsInitValueMap.set(pair[0], pair[1]);
            }
            for (const staticField of heritageType.staticFields) {
                classType.addStaticMemberField(staticField);
            }
            for (const method of heritageType.memberFuncs) {
                classType.addMethod(method);
            }
        }
        // 1. parse constructor
        const constructor = node.members.find((member) => {
            return ts.isConstructorDeclaration(member);
        });
        let ctorScope: FunctionScope;
        let ctorType: TSFunction;
        // iff not, add a default constructor
        const defaultCtor = this.currentScope!.children.find((child) => {
            if (child instanceof FunctionScope) {
                return child.funcName === 'constructor';
            }
            return false;
        });
        if (!constructor) {
            if (defaultCtor) {
                ctorScope = <FunctionScope>defaultCtor;
                ctorType = ctorScope.funcType;
            } else {
                ctorScope = new FunctionScope(this.currentScope!);
                ctorScope.setFuncName('constructor');
                ctorScope.setClassName(node.name!.getText());
                ctorScope.addParameter(
                    new Parameter('@context', new Type(), [], 0, false, false),
                );
                ctorScope.addParameter(
                    new Parameter('@this', new Type(), [], 1, false, false),
                );
                ctorScope.addVariable(new Variable('this', classType, [], -1));
                ctorType = new TSFunction(FunctionKind.CONSTRUCTOR);
            }
        } else {
            const func = <ts.ConstructorDeclaration>constructor;
            ctorType = this.generateNodeType(func) as TSFunction;
            ctorScope =
                <FunctionScope>this.parserCtx.getScopeByNode(func) || undefined;
        }
        ctorType.returnType = classType;
        ctorType.funcKind = FunctionKind.CONSTRUCTOR;
        ctorScope.setFuncType(ctorType);

        // 2. parse other fields
        for (const member of node.members) {
            if (ts.isSemicolonClassElement(member)) {
                /* ES6 allows Semicolon as class elements, we just skip them */
                continue;
            }
            const name = ts.isConstructorDeclaration(member)
                ? 'constructor'
                : member.name!.getText();

            const typeString = this.typeToString(member);
            if (member.kind === ts.SyntaxKind.PropertyDeclaration) {
                const field = <ts.PropertyDeclaration>member;
                const type = this.generateNodeType(field);
                const modifier = field.modifiers?.find((m) => {
                    return m.kind === ts.SyntaxKind.ReadonlyKeyword;
                })
                    ? 'readonly'
                    : undefined;
                const staticModifier = field.modifiers?.find((m) => {
                    return m.kind === ts.SyntaxKind.StaticKeyword;
                })
                    ? 'static'
                    : undefined;
                const classField: TsClassField = {
                    name: name,
                    type: type,
                    modifier: modifier,
                    visibility: 'public',
                    static: staticModifier,
                };
                if (field.initializer) {
                    let index = classType.getStaticFieldIndex(name);
                    if (index === -1) {
                        index = classType.staticFields.length;
                    }
                    if (classField.static) {
                        classType.staticFieldsInitValueMap.set(
                            index,
                            this.parserCtx.expressionProcessor.visitNode(
                                field.initializer,
                            ),
                        );
                    } else {
                        ctorScope.addStatement(
                            this.parserCtx.statementProcessor.createFieldAssignStmt(
                                field.initializer,
                                classType,
                                type,
                                name,
                            ),
                        );
                    }
                }
                if (!classField.static) {
                    fieldTypeStrs.push(`${name}: ${typeString}`);
                }
                if (
                    classType.getMemberField(name) ||
                    classType.getStaticMemberField(name)
                ) {
                    continue;
                }
                if (!classField.static) {
                    classType.addMemberField(classField);
                } else {
                    classType.addStaticMemberField(classField);
                }
            }
            if (member.kind === ts.SyntaxKind.SetAccessor) {
                const func = <ts.SetAccessorDeclaration>member;
                methodTypeStrs.push(`${name}: ${typeString}`);
                this.setMethod(func, baseType, classType, FunctionKind.SETTER);
            }
            if (member.kind === ts.SyntaxKind.GetAccessor) {
                const func = <ts.GetAccessorDeclaration>member;
                methodTypeStrs.push(`${name}: ${typeString}`);
                this.setMethod(func, baseType, classType, FunctionKind.GETTER);
            }
            if (member.kind === ts.SyntaxKind.MethodDeclaration) {
                const func = <ts.MethodDeclaration>member;
                const kind = func.modifiers?.find((m) => {
                    return m.kind === ts.SyntaxKind.StaticKeyword;
                })
                    ? FunctionKind.STATIC
                    : FunctionKind.METHOD;
                methodTypeStrs.push(`${name}: ${typeString}`);
                this.setMethod(func, baseType, classType, kind);
            }
        }

        const methodType = methodTypeStrs.join(', ');
        const fieldType = fieldTypeStrs.join(', ');
        this.methodShapeStr.set(classType.className, methodType);
        this.fieldShapeStr.set(classType.className, fieldType);
        const typeString = methodType + ', ' + fieldType;
        classType.setTypeId(this.generateTypeId(typeString));
        Logger.info(
            `Assign type id [${classType.typeId}] for class [${classType.className}], type string: ${typeString}`,
        );
        return classType;
    }

    private parseInfcDecl(node: ts.InterfaceDeclaration): TSInterface {
        const infc = new TSInterface();
        this.nodeTypeCache.set(node, infc);
        const methodTypeStrs: string[] = [];
        const fieldTypeStrs: string[] = [];

        node.members.map((member) => {
            /** Currently, we only handle PropertySignature and MethodSignature */
            if (
                member.kind !== ts.SyntaxKind.PropertySignature &&
                member.kind !== ts.SyntaxKind.MethodSignature &&
                member.kind !== ts.SyntaxKind.GetAccessor &&
                member.kind !== ts.SyntaxKind.SetAccessor
            ) {
                return;
            }
            let fieldType = this.generateNodeType(member);
            const typeString = this.typeToString(member);
            let funcKind = FunctionKind.METHOD;
            if (ts.isSetAccessor(member)) {
                const type = new TSFunction();
                type.addParamType(fieldType);
                fieldType = type;
                funcKind = FunctionKind.SETTER;
            }
            if (ts.isGetAccessor(member)) {
                const type = new TSFunction(FunctionKind.GETTER);
                type.returnType = fieldType;
                fieldType = type;
                funcKind = FunctionKind.GETTER;
            }
            const fieldName = member.name!.getText();
            if (fieldType instanceof TSFunction) {
                fieldType.funcKind = funcKind;
                infc.addMethod({
                    name: fieldName,
                    type: fieldType,
                });
                methodTypeStrs.push(`${fieldName}: ${typeString}`);
            } else {
                infc.addMemberField({
                    name: fieldName,
                    type: fieldType,
                });
                fieldTypeStrs.push(`${fieldName}: ${typeString}`);
            }
        });
        const typeString =
            methodTypeStrs.join(', ') + ', ' + fieldTypeStrs.join(', ');
        infc.setTypeId(this.generateTypeId(typeString));
        Logger.info(
            `Assign type id [${infc.typeId}] for interface: ${typeString}`,
        );

        return infc;
    }

    private generateTypeId(typeString: string): number {
        if (this.parserCtx.typeIdMap.has(typeString)) {
            return this.parserCtx.typeIdMap.get(typeString)!;
        }
        const id = this.parserCtx.typeIdMap.size;
        this.parserCtx.typeIdMap.set(typeString, id);
        return id;
    }

    private typeToString(node: ts.Node) {
        const type = this.typechecker!.getTypeAtLocation(node);
        let typeString = this.typechecker!.typeToString(type);
        // setter : T ==> (x: T) => void
        if (ts.isSetAccessor(node)) {
            const paramName = node.parameters[0].getText();
            typeString = `(${paramName}) => void`;
        }
        // getter : T ==> () => T
        if (ts.isGetAccessor(node)) {
            typeString = `() => ${typeString}`;
        }
        // typeReference: T ==> typeId
        // TODO
        return typeString;
    }

    private setMethod(
        func: ts.AccessorDeclaration | ts.MethodDeclaration,
        baseType: TSClass | null,
        classType: TSClass,
        funcKind: FunctionKind,
    ) {
        const methodName = func.name.getText();

        const type = this.generateNodeType(func);
        let tsFuncType = new TSFunction(funcKind);

        const nameWithPrefix = getMethodPrefix(funcKind) + func.name.getText();

        if (type instanceof TSFunction) {
            type.funcKind = tsFuncType.funcKind;
            tsFuncType = type;
        }
        if (funcKind === FunctionKind.GETTER) {
            tsFuncType.returnType = type;
        }
        if (funcKind === FunctionKind.SETTER) {
            tsFuncType.addParamType(type);
        }
        const isOverride =
            baseType && baseType.getMethod(methodName, funcKind).method
                ? true
                : false;
        if (!isOverride) {
            /* override methods has been copied from base class,
                only add non-override methods here */
            classType.addMethod({
                name: methodName,
                type: tsFuncType,
            });
        }

        const funcDef = this.parserCtx.getScopeByNode(func);
        if (funcDef && funcDef instanceof FunctionScope) {
            funcDef.setFuncType(tsFuncType);
        }
        classType.overrideOrOwnMethods.add(nameWithPrefix);
    }
}
