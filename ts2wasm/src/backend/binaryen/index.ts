/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import binaryen from 'binaryen';
import * as binaryenCAPI from './glue/binaryen.js';
import { FunctionKind, TSClass } from '../../type.js';
import { builtinTypes, Type, TypeKind } from '../../type.js';
import { Variable } from '../../variable.js';
import {
    arrayToPtr,
    emptyStructType,
    initStructType,
    Pakced,
} from './glue/transform.js';
import {
    FunctionScope,
    GlobalScope,
    ClassScope,
    ScopeKind,
    Scope,
    ClosureEnvironment,
    BlockScope,
    NamespaceScope,
} from '../../scope.js';
import { Stack } from '../../utils.js';
import { GLOBAL_INIT_FUNC, typeInfo } from './glue/utils.js';
import {
    importAnyLibAPI,
    importInfcLibAPI,
    generateGlobalContext,
    generateInitDynContext,
    generateFreeDynContext,
    addItableFunc,
    addDecoratorFunc,
} from '../../../lib/envInit.js';
import { WASMTypeGen } from './wasmTypeGen.js';
import {
    WASMExpressionGen,
    WASMDynExpressionGen,
    WASMExpressionBase,
} from './wasmExprGen.js';
import { WASMStatementGen } from './wasmStmtGen.js';
import {
    initGlobalOffset,
    initDefaultMemory,
    initDefaultTable,
} from './memory.js';
import { ArgNames, BuiltinNames } from '../../../lib/builtin/builtinUtil.js';
import {
    addBuiltInNoAnyFunc,
    addBuiltInAnyFunc,
} from '../../../lib/builtin/addBuiltIn.js';
import { Ts2wasmBackend, ParserContext } from '../index.js';
import { Logger } from '../../log.js';

export class WASMFunctionContext {
    private binaryenCtx: WASMGen;
    private currentScope: Scope;
    private funcScope: FunctionScope | GlobalScope | NamespaceScope;
    private funcOpcodeArray: Array<binaryen.ExpressionRef>;
    private opcodeArrayStack = new Stack<Array<binaryen.ExpressionRef>>();
    private returnOpcode: binaryen.ExpressionRef;
    private returnIndex = 0;

    constructor(
        binaryenCtx: WASMGen,
        scope: FunctionScope | GlobalScope | NamespaceScope,
    ) {
        this.binaryenCtx = binaryenCtx;
        this.currentScope = scope;
        this.funcScope = scope;
        this.funcOpcodeArray = new Array<binaryen.ExpressionRef>();
        this.opcodeArrayStack.push(this.funcOpcodeArray);
        this.returnOpcode = this.binaryenCtx.module.return();
    }

    insert(insn: binaryen.ExpressionRef) {
        this.opcodeArrayStack.peek().push(insn);
    }

    setReturnOpcode(returnOpcode: binaryen.ExpressionRef) {
        this.returnOpcode = returnOpcode;
    }

    get returnOp() {
        return this.returnOpcode;
    }

    insertAtFuncEntry(insn: binaryen.ExpressionRef) {
        this.funcOpcodeArray.push(insn);
    }

    enterScope(scope: Scope) {
        this.currentScope = scope;
        this.opcodeArrayStack.push(new Array<binaryen.ExpressionRef>());
        /* Init context variable */
        if (scope.getNearestFunctionScope()) {
            /* Only create context for scopes inside function scope */
            this.insert(
                this.binaryenCtx.createClosureContext(
                    scope as ClosureEnvironment,
                ),
            );
        }
    }

    exitScope() {
        const topMostArray = this.opcodeArrayStack.pop();
        this.currentScope = this.currentScope.parent!;

        return topMostArray;
    }

    getCurrentScope() {
        return this.currentScope;
    }

    getFuncScope() {
        return this.funcScope;
    }

    getBody() {
        return this.funcOpcodeArray;
    }

    set returnIdx(idx: number) {
        this.returnIndex = idx;
    }

    get returnIdx() {
        return this.returnIndex;
    }
}

interface segmentInfo {
    data: Uint8Array;
    offset: number;
}

class DataSegmentContext {
    static readonly reservedSpace: number = 1024;
    private binaryenCtx: WASMGen;
    currentOffset;
    stringOffsetMap;
    /* cache <typeid, itable*>*/
    itableMap;
    dataArray: Array<segmentInfo> = [];

    constructor(binaryenCtx: WASMGen) {
        /* Reserve 1024 bytes at beggining */
        this.binaryenCtx = binaryenCtx;
        this.currentOffset = DataSegmentContext.reservedSpace;
        this.stringOffsetMap = new Map<string, number>();
        this.itableMap = new Map<number, number>();
    }

    addData(data: Uint8Array) {
        /* there is no efficient approach to cache the data buffer,
            currently we don't cache it */
        const offset = this.currentOffset;
        this.currentOffset += data.length;

        this.dataArray.push({
            data: data,
            offset: offset,
        });

        return offset;
    }

    addString(str: string) {
        if (this.stringOffsetMap.has(str)) {
            /* Re-use the string to save space */
            return this.stringOffsetMap.get(str)!;
        }

        const offset = this.currentOffset;
        this.stringOffsetMap.set(str, offset);
        this.currentOffset += str.length + 1;

        const buffer = new Uint8Array(str.length + 1);
        for (let i = 0; i < str.length; i++) {
            const byte = str.charCodeAt(i);
            if (byte >= 256) {
                throw Error('UTF-16 string not supported in data segment');
            }
            buffer[i] = byte;
        }
        buffer[str.length] = 0;

        this.dataArray.push({
            data: buffer,
            offset: offset,
        });

        return offset;
    }

    generateSegment(): binaryen.MemorySegment | null {
        const offset = DataSegmentContext.reservedSpace;
        const size = this.currentOffset - offset;

        if (this.dataArray.length === 0) {
            return null;
        }

        const data = new Uint8Array(size);
        this.dataArray.forEach((info) => {
            for (let i = 0; i < info.data.length; i++) {
                const targetOffset =
                    i + info.offset - DataSegmentContext.reservedSpace;
                data[targetOffset] = info.data[i];
            }
        });

        return {
            offset: this.binaryenCtx.module.i32.const(offset),
            data: data,
            passive: false,
        };
    }

    getDataEnd(): number {
        return this.currentOffset;
    }
}

export class WASMGen extends Ts2wasmBackend {
    private currentFuncCtx: WASMFunctionContext | null = null;
    private dataSegmentContext: DataSegmentContext | null = null;
    private binaryenModule: binaryen.Module;
    private globalScopeStack: Stack<GlobalScope>;
    static contextOfScope: Map<Scope, typeInfo> = new Map<Scope, typeInfo>();
    private wasmTypeCompiler = new WASMTypeGen(this);
    wasmExprCompiler = new WASMExpressionGen(this);
    wasmDynExprCompiler = new WASMDynExpressionGen(this);
    wasmExprBase = new WASMExpressionBase(this);
    private wasmStmtCompiler = new WASMStatementGen(this);
    enterModuleScope: GlobalScope | null = null;
    private startBodyArray: Array<binaryen.ExpressionRef> = [];
    private globalInitArray: Array<binaryen.ExpressionRef> = [];
    private globalInitFuncName = '';

    constructor(parserContext: ParserContext) {
        super(parserContext);
        this.binaryenModule = new binaryen.Module();
        this.globalScopeStack = parserContext.globalScopeStack;
        this.dataSegmentContext = new DataSegmentContext(this);
    }

    public codegen(options?: any): void {
        this.binaryenModule.setFeatures(binaryen.Features.All);
        this.binaryenModule.autoDrop();
        this.WASMGenerate();

        /* Sometimes binaryen can't generate binary module,
            we dump the module to text and load it back.
           This is just a simple workaround, we need to find out the root cause
        */
        const textModule = this.binaryenModule.emitText();
        this.binaryenModule.dispose();

        try {
            this.binaryenModule = binaryen.parseText(textModule);
        } catch (e) {
            Logger.debug(textModule);
            Logger.debug(e);
            Logger.error(`Generated module is invalid`);
            throw e;
        }
        this.binaryenModule.setFeatures(binaryen.Features.All);
        this.binaryenModule.autoDrop();

        if (options && options[ArgNames.opt]) {
            binaryen.setOptimizeLevel(options[ArgNames.opt]);
            this.binaryenModule.optimize();
        }

        if (process.env['TS2WASM_VALIDATE']) {
            this.binaryenModule.validate();
        }
    }

    public emitBinary(options?: any): Uint8Array {
        return this.binaryenModule.emitBinary();
    }

    public emitText(options?: any): string {
        if (options?.format === 'Stack-IR') {
            return this.binaryenModule.emitStackIR();
        }
        return this.binaryenModule.emitText();
    }

    public dispose(): void {
        this.binaryenModule.dispose();
    }

    private WASMGenerate() {
        WASMGen.contextOfScope.clear();
        this.enterModuleScope = this.globalScopeStack.peek();

        // init wasm environment
        initGlobalOffset(this.module);
        initDefaultTable(this.module);
        if (!this.parserContext.compileArgs[ArgNames.disableBuiltIn]) {
            addBuiltInNoAnyFunc(this.module);
            if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
                addBuiltInAnyFunc(this.module);
            }
        }
        if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
            importAnyLibAPI(this.module);
        }
        if (!this.parserContext.compileArgs[ArgNames.disableInterface]) {
            importInfcLibAPI(this.module);
            addItableFunc(this.module);
        }

        for (let i = 0; i < this.globalScopeStack.size(); i++) {
            const globalScope = this.globalScopeStack.getItemAtIdx(i);
            this.globalInitFuncName = `${globalScope.moduleName}|${GLOBAL_INIT_FUNC}`;
            this.WASMGenHelper(globalScope);
            this.WASMStartFunctionGen(globalScope);
            this.WASMGlobalFuncGen();
        }

        if (this.parserContext.compileArgs[ArgNames.disableAny]) {
            if (
                this.wasmTypeCompiler.tsType2WASMTypeMap.has(
                    builtinTypes.get(TypeKind.ANY)!,
                )
            ) {
                throw Error('any type is in source');
            }
        }

        if (this.parserContext.compileArgs[ArgNames.disableInterface]) {
            if (
                this.wasmTypeCompiler.tsType2WASMTypeMap.has(
                    builtinTypes.get(TypeKind.INTERFACE)!,
                )
            ) {
                throw Error('interface type is in source');
            }
        }

        const startFuncOpcodes = [];
        if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
            generateGlobalContext(this.module);
            startFuncOpcodes.push(generateInitDynContext(this.module));
        }
        startFuncOpcodes.push(
            this.module.call(
                this.enterModuleScope.startFuncName,
                [],
                binaryen.none,
            ),
        );
        if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
            startFuncOpcodes.push(generateFreeDynContext(this.module));
        }
        // set enter module start function as wasm start function
        const wasmStartFuncRef = this.module.addFunction(
            BuiltinNames.start,
            binaryen.none,
            binaryen.none,
            [],
            this.module.block(null, startFuncOpcodes),
        );
        this.module.setStart(wasmStartFuncRef);

        const segments = [];
        const segmentInfo = this.dataSegmentContext!.generateSegment();
        if (segmentInfo) {
            segments.push(segmentInfo);
        }
        initDefaultMemory(this.module, segments);
    }

    WASMGenHelper(scope: Scope) {
        switch (scope.kind) {
            case ScopeKind.GlobalScope:
                this.WASMGlobalGen(<GlobalScope>scope);
                break;
            case ScopeKind.NamespaceScope:
                this.WASMGlobalGen(<NamespaceScope>scope);
                break;
            case ScopeKind.FunctionScope:
                this.WASMFunctionGen(<FunctionScope>scope);
                break;
            case ScopeKind.ClassScope:
                this.WASMClassGen(<ClassScope>scope);
                break;
            default:
                break;
        }
        for (let i = 0; i !== scope.children.length; ++i) {
            this.WASMGenHelper(scope.children[i]);
        }
    }

    get module(): binaryen.Module {
        return this.binaryenModule;
    }

    get wasmType(): WASMTypeGen {
        return this.wasmTypeCompiler;
    }

    get wasmExpr(): WASMExpressionGen {
        return this.wasmExprCompiler;
    }

    get curFunctionCtx(): WASMFunctionContext | null {
        return this.currentFuncCtx;
    }

    generateStartFuncVarTypes(
        scope: Scope,
        globalScope: GlobalScope,
        varWasmTypes: Array<binaryen.ExpressionRef>,
    ) {
        /* Don't process global vars */
        if (scope !== globalScope) {
            for (const variable of scope.varArray) {
                if (variable.varType.kind !== TypeKind.FUNCTION) {
                    varWasmTypes.push(
                        this.wasmType.getWASMType(variable.varType),
                    );
                } else {
                    varWasmTypes.push(
                        this.wasmType.getWASMFuncStructType(variable.varType),
                    );
                }
            }
        }

        scope.children.forEach((s) => {
            if (s instanceof BlockScope) {
                /* Only process block scope, inner functions will be processed separately */
                this.generateStartFuncVarTypes(s, globalScope, varWasmTypes);
            }
        });

        if (scope === globalScope) {
            /* Append temp vars */
            (scope as FunctionScope).getTempVars().forEach((v) => {
                if (v.varType.kind !== TypeKind.FUNCTION) {
                    varWasmTypes.push(this.wasmType.getWASMType(v.varType));
                } else {
                    varWasmTypes.push(
                        this.wasmType.getWASMFuncStructType(v.varType),
                    );
                }
            });
        }
    }

    /* add global variables, and generate start function */
    WASMStartFunctionGen(globalScope: GlobalScope) {
        this.startBodyArray.unshift(
            this.module.call(this.globalInitFuncName, [], binaryen.none),
        );
        const body = this.module.block(null, this.startBodyArray);

        const wasmTypes = new Array<binaryen.ExpressionRef>();
        this.generateStartFuncVarTypes(globalScope, globalScope, wasmTypes);
        // generate module start function
        this.module.addFunction(
            globalScope.startFuncName,
            binaryen.none,
            binaryen.none,
            wasmTypes,
            body,
        );
    }

    generateFuncVarTypes(
        scope: Scope,
        funcScope: FunctionScope,
        varWasmTypes: Array<binaryen.ExpressionRef>,
    ) {
        varWasmTypes.push(
            (<typeInfo>WASMGen.contextOfScope.get(scope)).typeRef,
        );
        const remainVars = scope.varArray.slice(1);

        /* the first one is context struct, no need to parse */
        for (const variable of remainVars) {
            if (variable.varType.kind !== TypeKind.FUNCTION) {
                varWasmTypes.push(this.wasmType.getWASMType(variable.varType));
            } else {
                varWasmTypes.push(
                    this.wasmType.getWASMFuncStructType(variable.varType),
                );
            }
        }

        scope.children.forEach((s) => {
            if (s instanceof BlockScope) {
                /* Only process block scope, inner functions will be processed separately */
                this.generateFuncVarTypes(s, funcScope, varWasmTypes);
            }
        });

        if (scope === funcScope) {
            /* Append temp vars */
            (scope as FunctionScope).getTempVars().forEach((v) => {
                if (v.varType.kind !== TypeKind.FUNCTION) {
                    varWasmTypes.push(this.wasmType.getWASMType(v.varType));
                } else {
                    varWasmTypes.push(
                        this.wasmType.getWASMFuncStructType(v.varType),
                    );
                }
            });
        }
    }

    createClosureContext(scope: ClosureEnvironment) {
        const closureVarTypes = new Array<binaryenCAPI.TypeRef>();
        const closureVarValues = new Array<binaryen.ExpressionRef>();
        const muts = new Array<boolean>();

        closureVarTypes.push(emptyStructType.typeRef);
        muts.push(false);
        closureVarValues.push(
            this.module.local.get(0, emptyStructType.typeRef),
        );

        let parentScope = scope.parent;
        // skip class scope
        while (
            parentScope !== null &&
            (parentScope.kind === ScopeKind.ClassScope ||
                parentScope?.kind === ScopeKind.NamespaceScope)
        ) {
            parentScope = parentScope.parent;
        }
        // free variable in parent level scope
        let parentCtxType: typeInfo | null = null;
        if (
            parentScope !== null &&
            parentScope.kind !== ScopeKind.GlobalScope
        ) {
            const parentLevelCtx = parentScope as ClosureEnvironment;
            const parentCtxIndex = parentLevelCtx.contextVariable!.varIndex;
            parentCtxType = WASMGen.contextOfScope.get(parentLevelCtx)!;
            closureVarTypes[0] = parentCtxType.typeRef;

            if (scope.kind === ScopeKind.FunctionScope) {
                if (parentCtxType.typeRef !== emptyStructType.typeRef) {
                    closureVarValues[0] = binaryenCAPI._BinaryenRefCast(
                        this.module.ptr,
                        closureVarValues[0],
                        parentCtxType.typeRef,
                    );
                }
            } else {
                closureVarValues[0] = this.module.local.get(
                    parentCtxIndex,
                    closureVarTypes[0],
                );
            }
        }

        if (!scope.hasFreeVar) {
            WASMGen.contextOfScope.set(
                scope!,
                parentCtxType === null ? emptyStructType : parentCtxType,
            );
            return this.module.local.set(
                scope.contextVariable!.varIndex,
                closureVarValues[0],
            );
        } else {
            let closureIndex = 1;
            if (scope instanceof FunctionScope) {
                for (const param of scope.paramArray) {
                    if (param.varIsClosure) {
                        closureVarTypes.push(
                            this.wasmType.getWASMType(param.varType),
                        );
                        closureVarValues.push(
                            this.module.local.get(
                                param.varIndex,
                                closureVarTypes[closureIndex],
                            ),
                        );
                        param.setClosureIndex(closureIndex);
                        muts.push(true);
                        closureIndex++;
                    }
                }
            }
            for (const variable of scope.varArray) {
                if (variable.varIsClosure) {
                    closureVarTypes.push(
                        this.wasmType.getWASMType(variable.varType),
                    );
                    closureVarValues.push(
                        this.module.local.get(
                            variable.varIndex,
                            closureVarTypes[closureIndex],
                        ),
                    );
                    variable.setClosureIndex(closureIndex);
                    muts.push(true);
                    closureIndex++;
                }
            }
            const packed = new Array<binaryenCAPI.PackedType>(
                closureVarTypes.length,
            ).fill(Pakced.Not);
            const contextType = initStructType(
                closureVarTypes,
                packed,
                muts,
                closureVarTypes.length,
                true,
            );
            WASMGen.contextOfScope.set(scope, contextType);
            const context = binaryenCAPI._BinaryenStructNew(
                this.module.ptr,
                arrayToPtr(closureVarValues).ptr,
                closureVarValues.length,
                contextType.heapTypeRef,
            );
            return this.binaryenModule.local.set(
                scope.contextVariable!.varIndex,
                context,
            );
        }
    }

    WASMGlobalGen(scope: NamespaceScope | GlobalScope) {
        this.currentFuncCtx = new WASMFunctionContext(this, scope);

        // parse global scope statements, generate start function body
        for (const stmt of scope.statements) {
            const stmtRef = this.wasmStmtCompiler.WASMStmtGen(stmt);
            if (
                stmt.statementKind === ts.SyntaxKind.Unknown ||
                stmt.statementKind === ts.SyntaxKind.VariableStatement
            ) {
                continue;
            }
            this.curFunctionCtx!.insert(stmtRef);
        }
        this.startBodyArray = this.startBodyArray.concat(
            this.curFunctionCtx!.getBody(),
        );
    }

    /* parse function scope */
    WASMFunctionGen(functionScope: FunctionScope) {
        const tsFuncType = functionScope.funcType;
        const paramWASMType =
            this.wasmTypeCompiler.getWASMFuncParamType(tsFuncType);
        const originParamWasmType =
            this.wasmTypeCompiler.getWASMFuncOrignalParamType(tsFuncType);
        const returnWASMType =
            this.wasmTypeCompiler.getWASMFuncReturnType(tsFuncType);

        if (functionScope.isDeclare()) {
            this.module.addFunctionImport(
                functionScope.mangledName,
                BuiltinNames.external_module_name,
                functionScope.mangledName,
                originParamWasmType,
                returnWASMType,
            );
            return;
        }

        if (functionScope.hasDecorator(BuiltinNames.decorator)) {
            const builtInFuncName = functionScope.mangledName.replace(
                functionScope.getRootGloablScope()!.moduleName,
                BuiltinNames.bulitIn_module_name,
            );
            addDecoratorFunc(this.module, builtInFuncName);
            return;
        }

        this.currentFuncCtx = new WASMFunctionContext(this, functionScope);
        this.curFunctionCtx!.insert(this.createClosureContext(functionScope));

        /* Class's "this" parameter */
        if (
            functionScope.funcType.funcKind !== FunctionKind.DEFAULT &&
            functionScope.funcType.funcKind !== FunctionKind.STATIC
        ) {
            const classType = (<ClassScope>functionScope.parent).classType;
            const wasmClassype = this.wasmType.getWASMType(classType);
            const thisVarIndex = functionScope.getThisIndex();
            this.currentFuncCtx!.insert(
                this.module.local.set(
                    thisVarIndex,
                    binaryenCAPI._BinaryenRefCast(
                        this.module.ptr,
                        this.module.local.get(1, emptyStructType.typeRef),
                        wasmClassype,
                    ),
                ),
            );
        }
        // add return value iff return type is not void
        if (
            functionScope.funcType.funcKind !== FunctionKind.CONSTRUCTOR &&
            functionScope.funcType.returnType.kind !== TypeKind.VOID
        ) {
            const returnVarIdx = functionScope.allocateLocalIndex();
            const returnVar = new Variable(
                '~returnVar',
                functionScope.funcType.returnType,
                [],
                returnVarIdx,
                true,
            );
            this.currentFuncCtx!.returnIdx = returnVarIdx;
            functionScope.addTempVar(returnVar);
        }

        // generate wasm statements
        for (const stmt of functionScope.statements) {
            const stmtRef = this.wasmStmtCompiler.WASMStmtGen(stmt);
            if (stmt.statementKind === ts.SyntaxKind.VariableStatement) {
                continue;
            }
            this.currentFuncCtx!.insert(stmtRef);
        }

        // add return in last iff return type is not void
        if (functionScope.funcType.returnType.kind !== TypeKind.VOID) {
            let returnValue = this.module.local.get(
                this.curFunctionCtx!.returnIdx,
                returnWASMType,
            );
            if (functionScope.funcType.funcKind === FunctionKind.CONSTRUCTOR) {
                returnValue = this.module.local.get(
                    functionScope.paramArray.length + 1,
                    returnWASMType,
                );
            }
            this.currentFuncCtx!.setReturnOpcode(
                this.module.return(returnValue),
            );
        }

        const varWASMTypes = new Array<binaryen.ExpressionRef>();
        this.generateFuncVarTypes(functionScope, functionScope, varWASMTypes);

        // add wrapper function if exported
        const isExport =
            functionScope.parent === this.enterModuleScope &&
            functionScope.isExport();
        if (isExport) {
            const functionStmts: binaryen.ExpressionRef[] = [];
            if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
                functionStmts.push(generateInitDynContext(this.module));
            }
            functionStmts.push(
                this.module.call(this.globalInitFuncName, [], binaryen.none),
            );
            // call origin function
            let idx = 0;
            const tempLocGetParams = tsFuncType
                .getParamTypes()
                .map((p) =>
                    this.module.local.get(
                        idx++,
                        this.wasmTypeCompiler.getWASMType(p),
                    ),
                );
            const targetCall = this.module.call(
                functionScope.mangledName,
                [
                    binaryenCAPI._BinaryenRefNull(
                        this.module.ptr,
                        emptyStructType.typeRef,
                    ),
                ].concat(tempLocGetParams),
                returnWASMType,
            );
            const isReturn = returnWASMType === binaryen.none ? false : true;
            functionStmts.push(
                isReturn ? this.module.local.set(idx, targetCall) : targetCall,
            );
            if (!this.parserContext.compileArgs[ArgNames.disableAny]) {
                functionStmts.push(generateFreeDynContext(this.module));
            }

            // return value
            const functionVars: binaryen.ExpressionRef[] = [];
            if (isReturn) {
                functionStmts.push(
                    this.module.return(
                        this.module.local.get(idx, returnWASMType),
                    ),
                );
                functionVars.push(returnWASMType);
            }
            // add export function
            this.module.addFunction(
                functionScope.funcName + '-wrapper',
                originParamWasmType,
                returnWASMType,
                functionVars,
                this.module.block(null, functionStmts),
            );

            this.module.addFunctionExport(
                functionScope.funcName + '-wrapper',
                functionScope.funcName,
            );
        }

        this.module.addFunction(
            functionScope.mangledName,
            paramWASMType,
            returnWASMType,
            varWASMTypes,
            this.module.block(
                null,
                [
                    this.module.block(
                        'statements',
                        this.currentFuncCtx.getBody(),
                    ),
                    this.currentFuncCtx.returnOp,
                ],
                returnWASMType,
            ),
        );
    }

    WASMClassGen(classScope: ClassScope) {
        const tsType = classScope.classType;
        if (!tsType.staticFields.length) {
            return;
        }
        const wasmStaticFieldsType =
            this.wasmType.getWASMClassStaticFieldsType(tsType);
        const wasmStaticFieldsHeapType =
            this.wasmType.getWASMClassStaticFieldsHeapType(tsType);
        // new_default_struct
        const init = binaryenCAPI._BinaryenStructNew(
            this.module.ptr,
            arrayToPtr([]).ptr,
            0,
            wasmStaticFieldsHeapType,
        );
        this.module.addGlobal(
            `${classScope.mangledName}_static_fields`,
            wasmStaticFieldsType,
            false,
            init,
        );
        for (let i = 0; i < tsType.staticFields.length; i++) {
            if (tsType.staticFieldsInitValueMap.has(i)) {
                const initValue = this.wasmExprCompiler.WASMExprGen(
                    tsType.staticFieldsInitValueMap.get(i)!,
                );
                const staticFieldValue = binaryenCAPI._BinaryenStructSet(
                    this.module.ptr,
                    i,
                    this.module.global.get(
                        `${classScope.mangledName}_static_fields`,
                        wasmStaticFieldsType,
                    ),
                    initValue.binaryenRef,
                );
                this.globalInitArray.push(staticFieldValue);
            }
        }
    }

    getVariableInitValue(varType: Type): binaryen.ExpressionRef {
        const module = this.module;
        if (varType.kind === TypeKind.NUMBER) {
            return module.f64.const(0);
        } else if (varType.kind === TypeKind.BOOLEAN) {
            return module.i32.const(0);
        } else if (varType.kind === TypeKind.DYNCONTEXTTYPE) {
            return module.i64.const(0, 0);
        }
        return binaryenCAPI._BinaryenRefNull(module.ptr, binaryen.anyref);
    }

    generateRawString(str: string): number {
        const offset = this.dataSegmentContext!.addString(str);
        return offset;
    }

    generateItable(shape: TSClass): number {
        if (this.dataSegmentContext!.itableMap.has(shape.typeId)) {
            return this.dataSegmentContext!.itableMap.get(shape.typeId)!;
        }
        const methodLen = shape.memberFuncs.length;
        const fieldLen = shape.fields.length;
        const dataLength = methodLen + fieldLen;
        const buffer = new Uint32Array(2 + 3 * dataLength);
        buffer[0] = this.module.i32.const(shape.typeId);
        buffer[1] = dataLength;
        for (let i = 0, j = 2; i < methodLen; i++, j += 3) {
            const method = shape.memberFuncs[i];
            if (method.type.funcKind === FunctionKind.STATIC) {
                continue;
            }
            const flag =
                method.type.funcKind === FunctionKind.METHOD
                    ? 1
                    : method.type.funcKind === FunctionKind.GETTER
                    ? 2
                    : 3;
            buffer[j] = this.generateRawString(method.name);
            buffer[j + 1] = this.module.i32.const(flag);
            buffer[j + 2] = this.module.i32.const(i);
        }
        const previousPartLength = 2 + shape.memberFuncs.length * 3;
        for (let i = 0, j = previousPartLength; i < fieldLen; i++, j += 3) {
            buffer[j] = this.generateRawString(shape.fields[i].name);
            buffer[j + 1] = this.module.i32.const(0);
            buffer[j + 2] = this.module.i32.const(i + 1);
        }
        const offset = this.dataSegmentContext!.addData(
            new Uint8Array(buffer.buffer),
        );
        this.dataSegmentContext!.itableMap.set(shape.typeId, offset);
        return offset;
    }

    private WASMGlobalFuncGen() {
        this.module.addFunction(
            this.globalInitFuncName,
            binaryen.none,
            binaryen.none,
            [],
            this.module.block(null, this.globalInitArray),
        );
        this.globalInitArray = [];
    }
}
