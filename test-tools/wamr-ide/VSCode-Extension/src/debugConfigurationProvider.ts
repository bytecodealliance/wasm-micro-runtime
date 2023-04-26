/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import * as vscode from 'vscode';
import * as os from 'os';

export class WasmDebugConfigurationProvider
    implements vscode.DebugConfigurationProvider
{
    private wasmDebugConfig!: vscode.DebugConfiguration;

    resolveDebugConfiguration(
        _: vscode.WorkspaceFolder | undefined,
        debugConfiguration: vscode.DebugConfiguration,
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        const defaultConfig: vscode.DebugConfiguration = {
            type: 'wamr-debug',
            name: 'Attach',
            request: 'attach',
            stopOnEntry: true,
            attachCommands: [
                /* default port 1234 */
                'process connect -p wasm connect://127.0.0.1:1234',
            ]
        };

        /* linux and windows has different debug configuration */
        if (os.platform() === 'win32' || os.platform() === 'darwin') {
            defaultConfig.initCommands = ['platform select remote-linux'];
        }

        this.wasmDebugConfig = {
            ...defaultConfig,
            ...debugConfiguration
        };

        return this.wasmDebugConfig;
    }

    public getDebugConfig(): vscode.DebugConfiguration {
        return this.wasmDebugConfig;
    }
}
