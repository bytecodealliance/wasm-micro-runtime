/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import cp from 'child_process';
import fs from 'fs';
import path from 'path';
import os from 'os';
import { ParserContext } from '../../src/frontend.js';
import { WASMGen } from '../../src/backend/binaryen/index.js';

const doCompile = (filename: string) => {
    const compiler = new ParserContext();

    /* Compile to a temporary file */
    try {
        compiler.parse([filename]);
    } catch (e) {
        return null;
    }

    const backend = new WASMGen(compiler);
    backend.codegen({ opt: 3 });
    const output = backend.emitBinary();
    backend.dispose();

    // TODO: should cleanup the temp dir
    const tempdir = fs.mkdtempSync(path.join(os.tmpdir(), 'ts2wasm-test-'));
    const tempfile = path.join(tempdir, 'case.wasm');
    fs.writeFileSync(tempfile, output, { flag: 'w' });
    return tempfile;
};

/**
 * Compile the typescript source file, check if it success
 *
 * @param filename the source file to be tested
 * @returns true if success, false otherwise
 */
export function testCompile(filename: string): boolean {
    try {
        const wasmFile = doCompile(filename);
        if (!wasmFile) return false;
        /* TODO: check wasm file */
        return fs.existsSync(wasmFile);
    } catch {
        return false;
    }
}

/**
 * Compile and execute a typescript source file with wasm VM
 *
 * @param filename the source file to be tested, it will be compiled to wasm module and executed by wasm VM
 * @param func name of the function to be executed, null or '' for global entry function (~start)
 * @param args arguments passed to the function
 * @param flags flags passed to wasm VM
 * @returns stdout of the VM process, throw exception if failed
 */
export function testWithVM(
    filename: string,
    func = '',
    args: Array<number | string> = [],
    flags: string[] = [],
): string {
    const wasmFile = doCompile(filename);
    if (!wasmFile) {
        return '';
    }

    /* Execute with iwasm */
    const iwasmArgs = [
        '-f',
        func || '~start',
        ...flags,
        wasmFile,
        ...args.map((a) => a.toString()),
    ];
    return cp.spawnSync('iwasm', iwasmArgs).stdout.toString('utf-8');
}

// TODO: implement testWithREPL later
// export class testWithREPL {
//     process : any

//     constructor(filename: string, flags = [] = []) {
//         let wasmFile = doCompile(filename);

//         /* Execute with iwasm */
//         let iwasmArgs = ['--repl', ...flags, wasmFile];
//         this.process = cp.spawn('iwasm', iwasmArgs);
//         this.process.stdin.setEncoding('utf-8');
//     }

//     public call(func: string = '', args: Array<Number | string> = []) {
//         this.process.stdout.pipe(process.stdout);
//         let cmd = [func, ...args.map((a) => a.toString())].join(' ');
//         this.process.stdin.write(`${cmd}\n`);
//         // TODO: get stdout from child process and return to caller
//     }
// }
