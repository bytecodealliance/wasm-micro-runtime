import binaryen from 'binaryen';
import ts from 'typescript';
import { BuiltinNames } from '../../../lib/builtin/builtin_name.js';
import { UnimplementError } from '../../error.js';
import { TypeKind } from '../../type.js';

export interface FlattenLoop {
    label: string;
    condition: binaryen.ExpressionRef;
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
    const ifStatementInfo: IfStatementInfo = {
        condition: loopStatementInfo.condition,
        ifTrue: binaryen.none,
        ifFalse: binaryen.none,
    };
    if (kind !== ts.SyntaxKind.DoStatement) {
        const ifTrueBlockArray: binaryen.ExpressionRef[] = [];
        if (loopStatementInfo.statements !== binaryen.none) {
            ifTrueBlockArray.push(loopStatementInfo.statements);
        }
        if (kind === ts.SyntaxKind.ForStatement) {
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
