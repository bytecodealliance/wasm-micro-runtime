/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import { WASMGen } from '../../src/backend/binaryen/index.js';
import { ParserContext } from '../../src/frontend.js';
import { getTSFilesDir, generateWatFile } from './utils.js';

const doCompile = (tsFileName: string) => {
    const parserCtx = new ParserContext();
    /* Compile to a wat file */

    parserCtx.parse([tsFileName], {
        isBuiltIn: true,
    });
    const backend = new WASMGen(parserCtx);
    backend.codegen({
        isBuiltIn: true,
    });

    const watFileName = tsFileName.replace('.ts', '.wat');
    generateWatFile(backend.module, watFileName);
    backend.dispose();
};

/**
 * Compile the typescript builtIn files
 */
export function initCompile() {
    try {
        const builtInTSFilesDir = getTSFilesDir();
        const files = fs.readdirSync(builtInTSFilesDir);
        for (const file of files) {
            const filePath = path.join(builtInTSFilesDir, file);
            if (filePath.includes('.ts')) {
                doCompile(filePath);
            }
        }
        return true;
    } catch (e) {
        console.error(e);
        return false;
    }
}

const initStatus = initCompile();
if (!initStatus) {
    console.error('something wrong happens in init builtin ts files');
}
