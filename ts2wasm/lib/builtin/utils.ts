/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import binaryen from 'binaryen';

export function getCurPath() {
    return path.dirname(fileURLToPath(import.meta.url));
}

export function getProjectPath() {
    const currentPath = getCurPath();
    return path.join(currentPath, '..', '..');
}

export function getTSFilesDir() {
    const projectPath = getProjectPath();
    return path.join(projectPath, 'lib', 'builtin', 'tsFile');
}

export function getWatFilesDir() {
    const projectPath = getProjectPath();
    return path.join(projectPath, 'lib', 'builtin', 'watFile');
}

export function generateWatFile(module: binaryen.Module, watFileName: string) {
    const output = module.emitText();
    const watFileDirPath = getWatFilesDir();
    const watFilePath = path.join(watFileDirPath, path.basename(watFileName));
    fs.mkdirSync(watFileDirPath, { recursive: true });
    fs.writeFileSync(watFilePath, output);
}

export function getFuncName(
    moduleName: string,
    funcName: string,
    delimiter = '|',
) {
    return moduleName.concat(delimiter).concat(funcName);
}
