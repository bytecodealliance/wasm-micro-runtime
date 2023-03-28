/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import { ParserContext } from '../../../src/frontend.js';
import { fileURLToPath } from 'url';
import { WASMGen } from '../../../src/backend/binaryen/index.js';

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const SAMPLES_DIR = path.join(SCRIPT_DIR, '../../../tests/samples');
const COMPILE_DIR = path.join(SCRIPT_DIR, 'wasm_modules');

if (fs.existsSync(COMPILE_DIR)) {
    fs.rmSync(COMPILE_DIR, { recursive: true });
}

fs.mkdirSync(COMPILE_DIR);

let tsFiles = fs.readdirSync(SAMPLES_DIR);
tsFiles = tsFiles.filter((f) => f !== 'sample_cases.test.ts');
console.log('compiling source files ...');
tsFiles.forEach((file) => {
    const currentTsFile = `${SAMPLES_DIR}/${file}`;

    if (fs.lstatSync(currentTsFile).isDirectory()) return;

    const outputFile = `${COMPILE_DIR}/${path.parse(file).name}.wasm`;

    try {
        const parserCtx = new ParserContext();
        console.log(`compiling [${file}] ...`);

        parserCtx.parse([currentTsFile]);

        const backend = new WASMGen(parserCtx);
        backend.codegen();
        const wasmBuffer = backend.emitBinary();
        fs.writeFileSync(outputFile, wasmBuffer);
        backend.dispose();
    } catch {
        console.error(`Compiling [${file}] failed`);
    }
});

process.exit(0);
