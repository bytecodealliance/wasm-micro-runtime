/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const hooks = ['pre-commit'];

hooks.forEach(function (hook) {
    const __filename = fileURLToPath(import.meta.url);
    const __dirname = path.dirname(__filename);
    const hookInSourceControl = path.resolve(__dirname, 'hooks', hook);

    if (fs.existsSync(hookInSourceControl)) {
        const hookInHiddenDirectory = path.resolve(
            __dirname,
            '..',
            '.git',
            'hooks',
            hook,
        );

        if (fs.existsSync(hookInHiddenDirectory)) {
            fs.unlinkSync(hookInHiddenDirectory);
        }

        fs.linkSync(hookInSourceControl, hookInHiddenDirectory);
    }
});
