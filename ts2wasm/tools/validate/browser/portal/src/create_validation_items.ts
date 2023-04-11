/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import { ParserContext } from '../../../../../src/frontend.js';
import { fileURLToPath } from 'url';
import { FunctionScope } from '../../../../../src/scope.js';

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const SAMPLES_DIR = path.join(SCRIPT_DIR, '../../../../../tests/samples');

const validationData: any = [];
let tsFiles = fs.readdirSync(SAMPLES_DIR);
tsFiles = tsFiles.filter((f) => f !== 'sample_cases.test.ts');
console.log('scanning source files ...');
for (const file of tsFiles) {
    const currentTsFile = `${SAMPLES_DIR}/${file}`;

    if (fs.lstatSync(currentTsFile).isDirectory()) continue;

    try {
        const parserCtx = new ParserContext();

        console.log(`analyzing [${file}] ...`);
        try {
            parserCtx.parse([currentTsFile]);
        } catch (e) {
            console.error(`Parsing [${file}] failed, skip it`);
            continue;
        }

        const entryScope =
            parserCtx.globalScopes[parserCtx.globalScopes.length - 1];
        const exportFuncs = entryScope.children.filter((s) => {
            return s instanceof FunctionScope && s.isExport();
        });

        const newItem: any = {
            module: path.parse(file).name,
            entries: [],
        };

        for (const f of exportFuncs) {
            let result: any = '';
            const args = (f as FunctionScope).paramArray
                .filter((p: any) => p.varName !== '@context')
                .map((p: any) => Math.floor(Math.random() * 40));

            try {
                result = (await import(currentTsFile))[f.getName()](...args);
                if (typeof result === 'undefined') {
                    result = 'undefined';
                } else if (typeof result === 'boolean') {
                    result = result ? 1 : 0;
                }
            } catch (e) {
                console.error(
                    `Failed to execute ${f.getName()}(${args}) by ts-node:`,
                );
                console.error(`${e}`);
                result = 'need_manual_fill';
            }

            newItem.entries.push({
                name: f.getName(),
                args: args,
                result: result,
            });
        }

        validationData.push(newItem);
    } catch (e) {
        console.error(`Analyzing [${file}] failed`);
        throw e;
    }
}

fs.writeFileSync(
    path.join(SCRIPT_DIR, 'validate_data.json'),
    JSON.stringify(validationData, null, 4),
);

process.exit(0);
