/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import * as fileSystem from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

import { WasmTaskProvider } from './taskProvider';
import { TargetConfigPanel } from './view/TargetConfigPanel';
import { NewProjectPanel } from './view/NewProjectPanel';
import {
    WriteIntoFile,
    ReadFromFile,
    WriteIntoFileAsync,
} from './utilities/directoryUtilities';
import { decorationProvider } from './explorer/decorationProvider';

import { WasmDebugConfigurationProvider } from './debug/debugConfigurationProvider';

let wasmTaskProvider: WasmTaskProvider;
let wasmDebugConfigProvider: WasmDebugConfigurationProvider;
var currentPrjDir = '';
var extensionPath = '';

export async function activate(context: vscode.ExtensionContext) {
    var OS_PLATFORM = '',
        buildScript = '',
        runScript = '',
        debugScript = '',
        buildScriptFullPath = '',
        runScriptFullPath = '',
        debugScriptFullPath = '',
        typeMap = new Map(),
        /* include paths array used for written into config file */
        includePathArr = new Array(),
        /* exclude files array used for written into config file */
        excludeFileArr = new Array(),
        scriptMap = new Map();

    /**
     * Get OS platform information for differ windows and linux execution script
     */
    OS_PLATFORM = os.platform();

    /**
     * Provide Build & Run Task with Task Provider instead of "tasks.json"
     */

    /* set relative path of build.bat|sh script */
    let scriptPrefix = 'resource/scripts/';
    if (OS_PLATFORM === 'win32') {
        buildScript = scriptPrefix.concat('build.bat');
        runScript = scriptPrefix.concat('run.bat');
        debugScript = scriptPrefix.concat('boot_debugger_server.bat');
    } else if (OS_PLATFORM === 'linux') {
        buildScript = scriptPrefix.concat('build.sh');
        runScript = scriptPrefix.concat('run.sh');
        debugScript = scriptPrefix.concat('boot_debugger_server.sh');
    }

    /* get extension's path, and set scripts' absolute path */
    extensionPath = context.extensionPath;

    buildScriptFullPath = path.join(extensionPath, buildScript);
    runScriptFullPath = path.join(extensionPath, runScript);
    debugScriptFullPath = path.join(extensionPath, debugScript);

    scriptMap.set('buildScript', buildScriptFullPath);
    scriptMap.set('runScript', runScriptFullPath);
    scriptMap.set('debugScript', debugScriptFullPath);

    typeMap.set('Build', 'Build');
    typeMap.set('Run', 'Run');
    typeMap.set('Debug', 'Debug');

    wasmTaskProvider = new WasmTaskProvider(typeMap, scriptMap);

    vscode.tasks.registerTaskProvider('wasm', wasmTaskProvider);

    /* set current project directory */
    if (vscode.workspace.workspaceFolders?.[0]) {
        if (OS_PLATFORM === 'win32') {
            currentPrjDir = vscode.workspace.workspaceFolders?.[0].uri
                .fsPath as string;
        } else if (OS_PLATFORM === 'linux') {
            currentPrjDir = vscode.workspace.workspaceFolders?.[0].uri
                .path as string;
        }
    }

    //TODO: add flag to judge whether the project is wasm project,
    //TODO: then enable or disable the `build`, `run` & `debug` button

    /**
     * register debug configuration
     */
    wasmDebugConfigProvider = new WasmDebugConfigurationProvider();
    wasmDebugConfigProvider.setDebugConfig(currentPrjDir, 1234);

    vscode.debug.registerDebugConfigurationProvider(
        'wamr-debug',
        wasmDebugConfigProvider
    );

    /**
     * update ext.includePaths to show or hide 'Remove' button in menus
     */
    vscode.commands.executeCommand('setContext', 'ext.supportedFileType', [
        '.c',
        '.cpp',
        '.cxx',
    ]);

    if (readFromConfigFile() !== '') {
        let configData = JSON.parse(readFromConfigFile());
        includePathArr = configData['include_paths'];
        excludeFileArr = configData['exclude_files'];

        if (Object.keys(configData['build_args']).length !== 0) {
            TargetConfigPanel.BUILD_ARGS = configData['build_args'];
        }
    }

    let disposableNewProj = vscode.commands.registerCommand(
        'wasmide.newProject',
        () => {
            let _ok = 'Set up now';
            let _cancle = 'Maybe later';
            let curWorkspace =
                readWorkSpaceFromExt() !== ''
                    ? JSON.parse(readWorkSpaceFromExt())['workspace']
                    : '';

            /* if user has not set up workspace yet, prompt to set up */
            if (curWorkspace === '' || curWorkspace === undefined) {
                vscode.window
                    .showWarningMessage(
                        'Please setup your workspace firstly.',
                        _ok,
                        _cancle
                    )
                    .then(item => {
                        if (item === _ok) {
                            vscode.commands.executeCommand(
                                'wasmide.changeWorkspace'
                            );
                        } else {
                            return;
                        }
                    });
            } else {
                NewProjectPanel.render(context);
            }
        }
    );
=
    let disposableTargetConfig = vscode.commands.registerCommand(
        'wasmide.targetConfig',
        () => {
            if (currentPrjDir !== '') {
                TargetConfigPanel.render(context);
            } else {
                vscode.window.showWarningMessage(
                    'Please create and open project firstly.',
                    'OK'
                );
            }
        }
    );

    let disposableChangeWorkspace = vscode.commands.registerCommand(
        'wasmide.changeWorkspace',
        async () => {
            let options: vscode.OpenDialogOptions = {
                canSelectFiles: false,
                canSelectFolders: true,
                openLabel: 'Select Workspace',
            };

            let Workspace = await vscode.window
                .showOpenDialog(options)
                .then(res => {
                    if (res) {
                        return res[0].fsPath as string;
                    } else {
                        return '';
                    }
                });

            writeIntoExtCustomFileAsync(Workspace);
        }
    );

    let disposableBuild = vscode.commands.registerCommand(
        'wasmide.build',
        () => {
            generateCMakeFile(includePathArr, excludeFileArr);

            return vscode.commands.executeCommand(
                'workbench.action.tasks.runTask',
                'Build: Wasm'
            );
        }
    );

    let disposableDebug = vscode.commands.registerCommand(
        'wasmide.debug',
        () => {
            vscode.commands
                .executeCommand('workbench.action.tasks.runTask', 'Debug: Wasm')
                .then(() => {
                    vscode.debug.startDebugging(
                        undefined,
                        wasmDebugConfigProvider.getDebugConfig()
                    );
                });
        }
    );

    let disposableRun = vscode.commands.registerCommand('wasmide.run', () => {
        return vscode.commands.executeCommand(
            'workbench.action.tasks.runTask',
            'Run: Wasm'
        );
    });

    let disposableToggleIncludePath = vscode.commands.registerCommand(
        'wasmide.build.toggleStateIncludePath',
        fileUri => {
            let pathRelative: string;
            let path =
                fileUri._fsPath !== null && fileUri._fsPath !== undefined
                    ? fileUri._fsPath
                    : vscode.Uri.parse(fileUri.path as string).fsPath;
            pathRelative = path.replace(currentPrjDir, '..');

            if (includePathArr.indexOf(pathRelative) > -1) {
                /* this folder has been added to include path, remove it */
                includePathArr = includePathArr.filter(value => {
                    return value !== pathRelative;
                });
            } else {
                includePathArr.push(pathRelative);
            }

            writeIntoConfigFile(
                includePathArr,
                excludeFileArr,
                TargetConfigPanel.BUILD_ARGS
            );

            decorationProvider.updateDecorationsForSource(fileUri);
        }
    );

    let disposableToggleExcludeFile = vscode.commands.registerCommand(
        'wasmide.build.toggleStateExclude',
        fileUri => {
            let pathRelative: string;

            let path =
                fileUri._fsPath !== null && fileUri._fsPath !== undefined
                    ? fileUri._fsPath
                    : vscode.Uri.parse(fileUri.path as string).fsPath;

            /* replace the current project absolute path with .. to change to relative path */
            pathRelative = path.replace(currentPrjDir, '..');

            if (excludeFileArr.indexOf(pathRelative) > -1) {
                excludeFileArr = excludeFileArr.filter(val => {
                    return val !== pathRelative;
                });
            } else {
                excludeFileArr.push(pathRelative);
            }

            writeIntoConfigFile(
                includePathArr,
                excludeFileArr,
                TargetConfigPanel.BUILD_ARGS
            );

            /* update decoration for this source file */
            decorationProvider.updateDecorationsForSource(fileUri);
        }
    );

    let disposableOpenFolder = vscode.commands.registerCommand(
        'wasmide.openFolder',
        () => {
            let _ok = 'Set up now';
            let _cancle = 'Maybe later';
            let curWorkspace =
                readWorkSpaceFromExt() !== ''
                    ? JSON.parse(readWorkSpaceFromExt())['workspace']
                    : '';

            /* if user has not set up workspace yet, prompt to set up */
            if (curWorkspace === '' || curWorkspace === undefined) {
                vscode.window
                    .showWarningMessage(
                        'Please setup your workspace firstly.',
                        _ok,
                        _cancle
                    )
                    .then(item => {
                        if (item === _ok) {
                            vscode.commands.executeCommand(
                                'wasmide.changeWorkspace'
                            );
                        } else {
                            return;
                        }
                    });
            } else {
                /* get all directories within directory, ignore files */
                let directoryArrDirent, directoryArr;
                try {
                    directoryArrDirent = fileSystem.readdirSync(curWorkspace, {
                        withFileTypes: true,
                    });
                } catch (err) {
                    vscode.window.showErrorMessage(
                        'Read from current workspace failed, please check.'
                    );
                }

                if (directoryArrDirent !== undefined) {
                    directoryArr = directoryArrDirent
                        .filter(dirent => dirent.isDirectory())
                        .map(dirent => dirent.name);

                    vscode.window
                        .showQuickPick(directoryArr, {
                            title: 'Select project',
                            placeHolder: 'Please select project',
                        })
                        .then(option => {
                            if (!option) {
                                return;
                            }

                            let _path = curWorkspace.concat(
                                OS_PLATFORM === 'win32'
                                    ? '\\'
                                    : OS_PLATFORM === 'linux'
                                    ? '/'
                                    : '',
                                option
                            );

                            openWindoWithSituation(vscode.Uri.file(_path));
                        });
                }
            }
        }
    );

    context.subscriptions.push(
        disposableNewProj,
        disposableTargetConfig,
        disposableChangeWorkspace,
        disposableBuild,
        disposableRun,
        disposableToggleIncludePath,
        disposableOpenFolder,
        disposableToggleExcludeFile,
        disposableDebug
    );
}

function openWindoWithSituation(uri: vscode.Uri) {
    /**
     * check if the workspace folder is empty,
     * if yes, open new window, else open in current window
     */
    let isWorkspaceEmpty: boolean;
    isWorkspaceEmpty = !vscode.workspace.workspaceFolders?.[0] ? true : false;

    isWorkspaceEmpty === false
        ? vscode.commands.executeCommand('vscode.openFolder', uri, {
              forceNewWindow: true,
          })
        : vscode.commands.executeCommand('vscode.openFolder', uri);

    return;
}

interface BuildArgs {
    output_file_name: string;
    init_memory_size: string;
    max_memory_size: string;
    stack_size: string;
    exported_symbols: string;
}

/**
 * @param: includePathArr
 * @param: excludeFileArr
 *   Get current includePathArr and excludeFileArr from the json string that
 *   will be written into compilation_config.json
 */
export function writeIntoConfigFile(
    includePathArr: string[],
    excludeFileArr: string[],
    buildArgs?: BuildArgs
) {
    let jsonStr = JSON.stringify({
        include_paths: includePathArr,
        exclude_files: excludeFileArr,
        build_args: buildArgs ? buildArgs : '{}',
    });

    let prjConfigDir = path.join(currentPrjDir, '.wamr');
    let configFilePath = path.join(prjConfigDir, 'compilation_config.json');
    WriteIntoFile(configFilePath, jsonStr);
}

export function readFromConfigFile(): string {
    let prjConfigDir = path.join(currentPrjDir, '.wamr');
    let configFilePath = path.join(prjConfigDir, 'compilation_config.json');
    return ReadFromFile(configFilePath);
}

export function readWorkSpaceFromExt(): string {
    let extCustomFilePath = path.join(
        extensionPath,
        'resource/custom/workspace.json'
    );
    return ReadFromFile(extCustomFilePath);
}

function writeIntoExtCustomFileAsync(workspace: string) {
    let workspaceJsonStr = JSON.stringify({ workspace: workspace });
    let extCustomFilePath = path.join(
        extensionPath,
        'resource/custom/workspace.json'
    );

    WriteIntoFileAsync(extCustomFilePath, workspaceJsonStr, () => {
        vscode.window.showInformationMessage('Workspace set up successfully!');
    });
}

/**
 * will be triggered when the user clicking `build` button
 */
function generateCMakeFile(
    includePathArr: string[],
    excludeFileArr: string[]
): void {
    // -Wl,--export=${EXPORT_SYMBOLS}
    let srcFilePath = path.join(currentPrjDir, 'src');
    let prjConfigDir = path.join(currentPrjDir, '.wamr');
    let cmakeFilePath = path.join(prjConfigDir, 'project.cmake');

    let strIncludeList = 'set (PROJECT_INCLUDES';
    let strSrcList = 'set (PROJECT_SRC_LIST';

    let strOutputFileName = 'set (OUTPUT_FILE_NAME';
    let strInitMemSize = 'set (INIT_MEM_SIZE';
    let strMaxMemSize = 'set (MAX_MEM_SIZE';
    let strStackSize = 'set (STACK_SIZE';
    let strExportedSymbols = 'set (EXPORTED_SYMBOLS';

    let fullStr = '';
    let i, s, e: number;

    /* change the absolute path into relative path */
    let _re = currentPrjDir;
    let _substr = '${CMAKE_CURRENT_SOURCE_DIR}/..';

    let srcPathArr: Array<{ path: string }> | undefined;
    /**
     * set PROJECT_SRC_LIST
     *     default ADD every c OR c++ OR cpp under the src/ path
     *     except the files saved in the exclude_files arrary
     */

    srcPathArr = getAllSrcFiles(srcFilePath);

    if (srcPathArr === undefined) {
        return;
    }

    for (s = 0; s < srcPathArr.length; s++) {
        if (
            excludeFileArr.indexOf(
                srcPathArr[s].path.replace(currentPrjDir, '..')
            ) === -1
        ) {
            /* replace currentPrjDir with ${CMAKE_CURRENT_SOURCE_DIR} */
            let _newStr = srcPathArr[s].path
                .replace(_re, _substr)
                .replace(/\\/g, '/');

            strSrcList = strSrcList.concat(' ', _newStr);
        }
    }
    strSrcList = strSrcList.concat(' )');

    for (i = 0; i < includePathArr.length; i++) {
        let _newStr = includePathArr[i]
            .replace(/../, _substr)
            .replace(/\\/g, '/');
        strIncludeList = strIncludeList.concat(' ', _newStr);
    }
    strIncludeList = strIncludeList.concat(' )');

    /* set up user customized input in configBuildArgs webview */
    strOutputFileName = strOutputFileName.concat(
        ' ',
        TargetConfigPanel.BUILD_ARGS.output_file_name + ')'
    );

    strInitMemSize = strInitMemSize.concat(
        ' ',
        TargetConfigPanel.BUILD_ARGS.init_memory_size + ')'
    );

    strMaxMemSize = strMaxMemSize.concat(
        ' ',
        TargetConfigPanel.BUILD_ARGS.max_memory_size + ')'
    );

    strStackSize = strStackSize.concat(
        ' ',
        TargetConfigPanel.BUILD_ARGS.stack_size + ')'
    );

    let exportedSymbolArr =
        TargetConfigPanel.BUILD_ARGS.exported_symbols.split(',');

    strExportedSymbols = strExportedSymbols.concat(' "');

    for (e = 0; e < exportedSymbolArr.length; e++) {
        strExportedSymbols = strExportedSymbols.concat(
            ' -Wl,',
            '--export=',
            exportedSymbolArr[e]
        );
    }

    strExportedSymbols = strExportedSymbols.concat('")');

    fullStr = strOutputFileName
        .concat('\n', strInitMemSize)
        .concat('\n', strMaxMemSize)
        .concat('\n', strStackSize)
        .concat('\n', strExportedSymbols)
        .concat('\n', strSrcList)
        .concat('\n', strIncludeList);

    WriteIntoFile(cmakeFilePath, fullStr);
}

function getAllSrcFiles(_path: string) {
    try {
        const entries = fileSystem.readdirSync(_path, {
            withFileTypes: true,
        });

        const files = entries
            .filter(
                /* filter files mismatch .c |.cpp |.cxx */
                file =>
                    !file.isDirectory() && file.name.match('(.c|.cpp|.cxx)$')
            )
            .map(file => ({
                path: path.join(_path, file.name),
            }));

        const folders = entries.filter(folder => folder.isDirectory());

        for (const folder of folders) {
            let fileArr = getAllSrcFiles(path.join(_path, folder.name));
            fileArr ? files.push(...fileArr) : '';
        }

        return files;
    } catch (error) {
        vscode.window.showErrorMessage(error as string);
    }
}
