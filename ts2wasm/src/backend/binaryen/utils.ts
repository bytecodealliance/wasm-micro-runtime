import binaryen from 'binaryen';
import { BuiltinNames } from '../../../lib/builtin/builtinUtil.js';
import { anyArrayTypeInfo } from './glue/packType.js';

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

export function getOriginFuncName(builtInFuncName: string) {
    const strs = builtInFuncName.split(BuiltinNames.module_delimiter);
    strs.shift();
    return strs.join(BuiltinNames.module_delimiter);
}

export function addWatFuncImports(
    funcName: string,
    curModule: binaryen.Module,
) {
    curModule.addFunctionImport(
        funcName,
        BuiltinNames.external_module_name,
        getOriginFuncName(funcName),
        getParamTypeByBuiltInFuncName(funcName),
        getReturnTypeByBuiltInFuncName(funcName),
    );
}

export function getParamTypeByBuiltInFuncName(funcName: string) {
    const originFuncName = getOriginFuncName(funcName);
    switch (originFuncName) {
        case BuiltinNames.console_log_funcName: {
            return anyArrayTypeInfo.typeRef;
        }
        default: {
            return binaryen.none;
        }
    }
}

export function getReturnTypeByBuiltInFuncName(funcName: string) {
    const originFuncName = getOriginFuncName(funcName);
    switch (originFuncName) {
        case BuiltinNames.console_log_funcName: {
            return binaryen.none;
        }
        default: {
            return binaryen.none;
        }
    }
}
