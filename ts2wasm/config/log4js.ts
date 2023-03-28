/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import os from 'os';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const curPath = path.dirname(fileURLToPath(import.meta.url));
const logDir = fs.mkdtempSync(path.join(os.tmpdir(), 'ts2wasm-log-'));

const logConfig = {
    appenders: {
        console: {
            type: 'console',
        },
        file: {
            type: 'file',
            filename: `${path.join(logDir, 'output.log')}`,
            backups: 3,
        },
        errorFile: {
            type: 'file',
            filename: `${path.join(logDir, 'error.log')}`,
            backups: 3,
        },
        errors: {
            type: 'logLevelFilter',
            level: 'ERROR',
            appender: 'errorFile',
        },
    },
    categories: {
        default: {
            appenders: ['file', 'errors'],
            level: 'debug',
        },
        console: {
            appenders: ['console'],
            level: 'error',
        },
    },
};

export default logConfig;
