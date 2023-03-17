/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { testCompile } from '../utils/test_helper.js';

import 'mocha';
import { expect } from 'chai';
import { fstat, readdirSync } from 'fs';
import path from 'path';

import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

describe('basic_cases', function () {
    this.timeout(5000);
    readdirSync(__dirname)
        .filter((d) => {
            return d.endsWith('.ts') && !d.endsWith('.test.ts');
        })
        .forEach((f) => {
            it(`${f}`, function () {
                expect(testCompile(path.join(__dirname, f))).eq(true);
            });
        });
});
