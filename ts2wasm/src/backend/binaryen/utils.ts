import binaryen from 'binaryen';
import * as binaryenCAPI from './glue/binaryen.js';
import ts from 'typescript';
import { BuiltinNames } from '../../../lib/builtin/builtin_name.js';
import { UnimplementError } from '../../error.js';
import { TypeKind } from '../../type.js';
import { dyntype } from './lib/dyntype/utils.js';

export interface FlattenLoop {
    label: string;
    condition?: binaryen.ExpressionRef;
    statements: binaryen.ExpressionRef;
    incrementor?: binaryen.ExpressionRef;
}

export interface IfStatementInfo {
    condition: binaryen.ExpressionRef;
    ifTrue: binaryen.ExpressionRef;
    ifFalse: binaryen.ExpressionRef;
}

export function flattenLoopStatement(
    loopStatementInfo: FlattenLoop,
    kind: ts.SyntaxKind,
    module: binaryen.Module,
): binaryen.ExpressionRef {
    const condition = loopStatementInfo.condition || module.i32.const(1);
    const ifStatementInfo: IfStatementInfo = {
        condition: condition,
        ifTrue: binaryen.none,
        ifFalse: binaryen.none,
    };
    if (kind !== ts.SyntaxKind.DoStatement) {
        const ifTrueBlockArray: binaryen.ExpressionRef[] = [];
        if (loopStatementInfo.statements !== binaryen.none) {
            ifTrueBlockArray.push(loopStatementInfo.statements);
        }
        if (
            kind === ts.SyntaxKind.ForStatement &&
            loopStatementInfo.incrementor
        ) {
            ifTrueBlockArray.push(
                <binaryen.ExpressionRef>loopStatementInfo.incrementor,
            );
        }
        ifTrueBlockArray.push(module.br(loopStatementInfo.label));
        const ifTrueBlock = module.block(null, ifTrueBlockArray);
        ifStatementInfo.ifTrue = ifTrueBlock;
        return module.if(ifStatementInfo.condition, ifStatementInfo.ifTrue);
    } else {
        ifStatementInfo.ifTrue = module.br(loopStatementInfo.label);
        const blockArray: binaryen.ExpressionRef[] = [];
        if (loopStatementInfo.statements !== binaryen.none) {
            blockArray.push(loopStatementInfo.statements);
        }
        const ifExpression = module.if(
            ifStatementInfo.condition,
            ifStatementInfo.ifTrue,
        );
        blockArray.push(ifExpression);
        return module.block(null, blockArray);
    }
}

export function addWatFuncs(
    watModule: binaryen.Module,
    funcName: string,
    curModule: binaryen.Module,
) {
    const funcRef = watModule.getFunction(funcName);
    const funcInfo = binaryen.getFunctionInfo(funcRef);
    curModule.addFunction(
        funcInfo.name,
        funcInfo.params,
        funcInfo.results,
        funcInfo.vars,
        curModule.copyExpression(funcInfo.body),
    );
}

export function getClassNameByTypeKind(typeKind: TypeKind): string {
    switch (typeKind) {
        case TypeKind.BOOLEAN:
            return BuiltinNames.BOOLEAN;
        case TypeKind.NUMBER:
            return BuiltinNames.NUMBER;
        case TypeKind.FUNCTION:
            return BuiltinNames.FUNCTION;
        case TypeKind.STRING:
            return BuiltinNames.STRING;
        case TypeKind.ARRAY:
            return BuiltinNames.ARRAY;
        default:
            throw new UnimplementError('unimplement type class: ${typeKind}');
    }
}

export function unboxAnyTypeToBaseType(
    module: binaryen.Module,
    anyExprRef: binaryen.ExpressionRef,
    typeKind: TypeKind,
) {
    let condFuncName = '';
    let cvtFuncName = '';
    let binaryenType: binaryen.Type;
    if (typeKind === TypeKind.ANY) {
        return anyExprRef;
    }
    if (typeKind === TypeKind.NULL) {
        return binaryenCAPI._BinaryenRefNull(
            module.ptr,
            binaryenCAPI._BinaryenTypeStructref(),
        );
    }
    switch (typeKind) {
        case TypeKind.NUMBER: {
            condFuncName = dyntype.dyntype_is_number;
            cvtFuncName = dyntype.dyntype_to_number;
            binaryenType = binaryen.f64;
            break;
        }
        case TypeKind.BOOLEAN: {
            condFuncName = dyntype.dyntype_is_bool;
            cvtFuncName = dyntype.dyntype_to_bool;
            binaryenType = binaryen.i32;
            break;
        }
        default: {
            throw Error(
                `unboxing any type to static type, unsupported static type : ${typeKind}`,
            );
        }
    }
    const isBaseTypeRef = module.call(
        condFuncName,
        [
            module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
            anyExprRef,
        ],
        dyntype.bool,
    );
    const condition = module.i32.ne(isBaseTypeRef, module.i32.const(0));
    // iff True
    const value = module.call(
        cvtFuncName,
        [
            module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
            anyExprRef,
        ],
        binaryenType,
    );
    // iff False
    const unreachableRef = module.unreachable();

    const blockStmt = module.if(condition, value, unreachableRef);
    return module.block(null, [blockStmt], binaryenType);
}

export function isBaseType(
    module: binaryen.Module,
    anyExprRef: binaryen.ExpressionRef,
    condFuncName: string,
) {
    return module.call(
        condFuncName,
        [
            module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
            anyExprRef,
        ],
        dyntype.bool,
    );
}
