import * as vscode from 'vscode';
import * as os from 'os';
import * as path from 'path';
import * as fs from 'fs';
import { CheckIfDirectoryExist, DownloadFile, UnzipFile } from './directoryUtilities';

const LLDB_RESOURCE_DIR = "resource/debug";
const LLDB_OS_DIR_MAP: Partial<Record<NodeJS.Platform, string>> = {
    "linux": "linux",
    "darwin": "osx"
};
const LLDB_OS_DOWNLOAD_URL_SUFFIX_MAP: Partial<Record<NodeJS.Platform, string>> = {
    "linux": "x86_64-ubuntu-22.04",
    "darwin": "universal-macos-latest"
};

const WamrLLDBNotSupportedError = new Error("WAMR LLDB is not supported on this platform");

function getLLDBUnzipFilePath(destinationFolder: string, filename: string) {
    const dirs = filename.split("/");
    if (dirs[0] == "inst") {
        dirs.shift();
    }

    return path.join(destinationFolder, ...dirs);
}

function getLLDBDownloadUrl(context: vscode.ExtensionContext): string {
    const wamrVersion = require(path.join(context.extensionPath, "package.json")).version;
    const lldbOsUrlSuffix = LLDB_OS_DOWNLOAD_URL_SUFFIX_MAP[os.platform()];

    if (!lldbOsUrlSuffix) {
        throw WamrLLDBNotSupportedError;
    }

    return `https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-${wamrVersion}/wamr-lldb-${wamrVersion}-${lldbOsUrlSuffix}.zip`
}

export function isLLDBInstalled(context: vscode.ExtensionContext): boolean {
    const extensionPath = context.extensionPath;
    const lldbOSDir = LLDB_OS_DIR_MAP[os.platform()]
    if (!lldbOSDir) {
        throw WamrLLDBNotSupportedError;
    }

    const lldbBinaryPath = path.join(extensionPath, LLDB_RESOURCE_DIR, lldbOSDir, "bin", "lldb");

    return CheckIfDirectoryExist(lldbBinaryPath);
}

export async function promptInstallLLDB(context: vscode.ExtensionContext) {
    const extensionPath = context.extensionPath;
    const setup_prompt = "setup";
    const skip_prompt = "skip";
    const response = await vscode.window.showWarningMessage('No LLDB instance found. Setup now?', setup_prompt, skip_prompt);

    if (response == skip_prompt) {
        return;
    }

    const downloadUrl = getLLDBDownloadUrl(context);
    const destinationDir = LLDB_OS_DIR_MAP[os.platform()];

    if (!downloadUrl || !destinationDir) {
        throw WamrLLDBNotSupportedError;
    }

    const lldbDestinationFolder = path.join(extensionPath, LLDB_RESOURCE_DIR, destinationDir);
    const lldbZipPath = path.join(lldbDestinationFolder, "bundle.zip");

    vscode.window.showInformationMessage(`Downloading LLDB...`);

    await DownloadFile(downloadUrl, lldbZipPath);

    vscode.window.showInformationMessage(`LLDB downloaded to ${lldbZipPath}. Installing...`);

    const lldbFiles = await UnzipFile(lldbZipPath, filename => getLLDBUnzipFilePath(lldbDestinationFolder, filename));
    // Allow execution of lldb
    lldbFiles.forEach(file => fs.chmodSync(file, "0775"));

    vscode.window.showInformationMessage(`LLDB installed at ${lldbDestinationFolder}`);

    // Remove the bundle.zip
    fs.unlink(lldbZipPath, () => {});
}


