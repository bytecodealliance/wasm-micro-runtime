/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import cp from 'child_process';
import { ParserContext } from '../../../src/frontend.js';
import { fileURLToPath } from 'url';
import { WASMGen } from '../../../src/backend/binaryen/index.js';
import validationItems from './validation.json' assert { type: 'json' };

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const SAMPLES_DIR = path.join(SCRIPT_DIR, '../../../tests/samples');
const COMPILE_DIR = path.join(SCRIPT_DIR, 'wasm_modules');
const IWASM_GC_DIR = path.join(
    SCRIPT_DIR,
    '../../../runtime-library/build/iwasm_gc',
);
const BUILD_SCRIPT_DIR = path.join(
    SCRIPT_DIR,
    '../../../runtime-library/build.sh',
);
const TEST_LOG_FILE = path.join(SCRIPT_DIR, 'test.log');

if (!fs.existsSync(IWASM_GC_DIR)) {
    console.error('iwasm_gc not found, build it firstly');
    const result = cp.execFileSync(BUILD_SCRIPT_DIR, { stdio: 'inherit' });
}

fs.writeFileSync(
    TEST_LOG_FILE,
    `Start validation on WAMR ... ${new Date()}\n\n`,
);

if (fs.existsSync(COMPILE_DIR)) {
    fs.rmSync(COMPILE_DIR, { recursive: true });
}

fs.mkdirSync(COMPILE_DIR);

let totalCases = 0;
let totalFail = 0;
let totalCompilationFail = 0;
let totalNeedManualValidation = 0;

validationItems.forEach((item) => {
    const sourceFile = `${SAMPLES_DIR}/${item.module}.ts`;
    const outputFile = `${COMPILE_DIR}/${item.module}.wasm`;

    const moduleEntries = item.entries.length;
    totalCases += moduleEntries;

    let compilationSuccess = false;

    try {
        const parserCtx = new ParserContext();
        console.log(`Validating [${item.module}] ...`);

        parserCtx.parse([sourceFile]);

        const backend = new WASMGen(parserCtx);
        backend.codegen();
        const wasmBuffer = backend.emitBinary();
        fs.writeFileSync(outputFile, wasmBuffer);
        backend.dispose();
        compilationSuccess = true;
    } catch {
        console.error(`Compiling [${item.module}] failed`);
    }

    item.entries.forEach((entry) => {
        if (!compilationSuccess) {
            totalCompilationFail++;

            fs.appendFileSync(
                TEST_LOG_FILE,
                `===================================================================================\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `Running [${item.module}:${entry.name}] failed due to compilation error\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `-----------------------------------------------------------------------------------\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `source code: \n\t${sourceFile}\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `===================================================================================\n\n\n`,
            );
            totalFail++;
        }

        const iwasmArgs = [
            '-f',
            entry.name,
            outputFile,
            ...entry.args.map((a: any) => a.toString()),
        ];
        const result = cp.spawnSync(IWASM_GC_DIR, iwasmArgs);
        const cmdStr = `${IWASM_GC_DIR} ${iwasmArgs.join(' ')}`;
        if (result.status !== 0) {
            fs.appendFileSync(
                TEST_LOG_FILE,
                `===================================================================================\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `Running [${item.module}:${entry.name}] get invalid return code: ${result.status}\n`,
            );
            fs.appendFileSync(TEST_LOG_FILE, `stdout:\n`);
            fs.appendFileSync(TEST_LOG_FILE, result.stdout.toString('utf-8'));
            fs.appendFileSync(TEST_LOG_FILE, `stderr:\n`);
            fs.appendFileSync(TEST_LOG_FILE, result.stderr.toString('utf-8'));
            fs.appendFileSync(
                TEST_LOG_FILE,
                `-----------------------------------------------------------------------------------\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `source code: \n\t${sourceFile}\n`,
            );
            fs.appendFileSync(
                TEST_LOG_FILE,
                `wasm module: \n\t${outputFile}\n`,
            );
            fs.appendFileSync(TEST_LOG_FILE, `reproduce cmd: \n\t${cmdStr}\n`);
            fs.appendFileSync(
                TEST_LOG_FILE,
                `===================================================================================\n\n\n`,
            );
            totalFail++;
        } else {
            const expected = entry.result;
            const executOutput = result.stdout.toString('utf-8').trim();
            if (executOutput !== expected) {
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `===================================================================================\n`,
                );
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `Running [${item.module}:${entry.name}] get unexpected output\n`,
                );
                fs.appendFileSync(TEST_LOG_FILE, `\tExpected: ${expected}\n`);
                fs.appendFileSync(TEST_LOG_FILE, `\tGot: ${executOutput}\n`);
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `-----------------------------------------------------------------------------------\n`,
                );
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `source code: \n\t${sourceFile}\n`,
                );
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `wasm module: \n\t${outputFile}\n`,
                );
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `reproduce cmd: \n\t${cmdStr}\n`,
                );
                fs.appendFileSync(
                    TEST_LOG_FILE,
                    `===================================================================================\n\n\n`,
                );
                totalFail++;

                if (executOutput.indexOf('ref')) {
                    totalNeedManualValidation++;
                }
            }
        }
    });
});

console.log(`${totalCases - totalFail} / ${totalCases} passed!`);
console.log(`-------------------------------------------------------------`);
console.log(`In the ${totalFail} failed cases:`);
console.log(
    `    * ${totalCompilationFail} cases failed due to compilation error`,
);
console.log(
    `    * ${totalNeedManualValidation} cases need manual validation due to complex return type`,
);

process.exit(0);
