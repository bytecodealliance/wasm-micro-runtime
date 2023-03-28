/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import express, { Express, Request, Response } from 'express';
import fs from 'fs';
import os from 'os';
import path from 'path';
import { ParserContext } from '../../../src/frontend.js';
import sqlite3 from 'better-sqlite3';
import ts from 'typescript';
import { fileURLToPath } from 'url';
import { WASMGen } from '../../../src/backend/binaryen/index.js';

const storage_dir = path.join(os.homedir(), '.ts2wasm_playground');
const samples_dir = 'assets/samples';

const app: Express = express();
const port = process.env.PORT || 3001;

function make_storage_dir() {
    if (!fs.existsSync(storage_dir)) {
        fs.mkdirSync(storage_dir);
    }
}

function store_case(buffer: string, file_name: string, e: string) {
    const file_path = path.join(storage_dir, file_name);
    fs.writeFileSync(file_path, buffer);
    fs.appendFileSync(file_path, `\n// Error message: ${e}`);

    return file_path;
}

function get_feedback_db() {
    make_storage_dir();
    const db_dir = path.join(storage_dir, 'feedback.sqlite');

    if (!fs.existsSync(db_dir)) {
        const db = new sqlite3(db_dir);
        db.exec(`
            CREATE TABLE suggestions
            (
                ID INTEGER PRIMARY KEY AUTOINCREMENT,
                user VARCHAR(128) NOT NULL,
                suggestion VARCHAR(4096) NOT NULL
            );
        `);

        return db;
    }

    const db = new sqlite3(db_dir);
    return db;
}

app.all('*', function (req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Headers', 'X-Requested-With');
    res.header('Access-Control-Allow-Headers', 'Content-Type');
    res.header('Access-Control-Allow-Methods', 'PUT,POST,GET,DELETE,OPTIONS');
    res.header('Access-Control-Allow-Credentials', 'true');
    res.header('X-Powered-By', ' 3.2.1');

    // Disable caching for content files
    res.header('Cache-Control', 'no-cache, no-store, must-revalidate');
    res.header('Pragma', 'no-cache');
    res.header('Expires', '0');

    if (req.method == 'OPTIONS') {
        res.status(200).send();
        return;
    }

    next();
});

app.post('/feedback', async (req: Request, res: Response) => {
    let buffer = '';
    req.on('data', (chunk) => {
        buffer += chunk.toString();
    });

    req.on('end', () => {
        const content = JSON.parse(buffer);
        console.log(`[${content.user}] suggested:\n${content.suggests}`);
        const db = get_feedback_db();
        const stmt = db.prepare(
            `INSERT INTO suggestions (user, suggestion) VALUES (?, ?)`,
        );
        stmt.run([content.user, content.suggests]);
        res.status(200).send();
    });
});

app.get('/samples/:name?', (req: Request, res: Response) => {
    if (req.params.name) {
        /* Request specific file */
        const file_path = path.join(samples_dir, req.params.name);
        if (!fs.existsSync(file_path)) {
            res.status(404).send({
                error: `File ${req.params.name} not found`,
            });
            return;
        }

        res.setHeader('Content-Type', 'text/plain');
        fs.createReadStream(file_path).pipe(res);
        return;
    } else {
        /* Request list of samples */
        const files = fs.readdirSync(samples_dir);
        res.json(files);
        return;
    }
});

app.post('/compile', (req: Request, res: Response) => {
    let buffer = '';
    req.on('data', (chunk) => {
        buffer += chunk.toString();
    });

    req.on('end', () => {
        let tmpDir;
        const prefix = 'ts2wasm-playground';
        try {
            const payloadJson = JSON.parse(buffer);
            tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), prefix));
            const tempfile = path.join(tmpDir, 'index.ts');
            fs.writeFileSync(tempfile, payloadJson.code);

            console.log(`Saving data to temp file [${tempfile}]`);

            const optLevel = payloadJson.options?.opt ? 3 : 0;
            const parserCtx = new ParserContext();
            const startTime = Date.now();
            try {
                parserCtx.parse([tempfile], { opt: optLevel });
            } catch (e: any) {
                console.log(e);
                console.log(
                    `Recording as file [${store_case(
                        buffer,
                        path.basename(tmpDir) + '.ts',
                        e,
                    )}]`,
                );

                let formattedError = '';
                const syntaxErrors = parserCtx.errorMessage;
                if (syntaxErrors?.length) {
                    formattedError = ts.formatDiagnostics(
                        syntaxErrors as readonly ts.Diagnostic[],
                        {
                            getCurrentDirectory: () => {
                                return path.dirname(
                                    fileURLToPath(import.meta.url),
                                );
                            },
                            getCanonicalFileName: (fileNames) => {
                                return fileNames;
                            },
                            getNewLine: () => {
                                return '\n';
                            },
                        },
                    );
                }

                res.json({
                    error: `${e.toString()}`,
                });
                return;
            }

            const backend = new WASMGen(parserCtx);

            try {
                backend.codegen({ opt: optLevel });
            } catch (e: any) {
                res.json({
                    error: `${e.toString()}`,
                });
                return;
            }

            const resultText = backend.emitText({
                format: payloadJson.options?.format,
            });

            const wasmBinary = backend.emitBinary();

            backend.dispose();

            res.json({
                content: resultText,
                wasm: Buffer.from(wasmBinary).toString('base64'),
                duration: Date.now() - startTime,
            });
        } catch {
            res.status(500).send('Failed to create workspace');
        } finally {
            try {
                if (tmpDir) {
                    fs.rmSync(tmpDir, { recursive: true });
                }
            } catch (e) {
                console.log(
                    `An error has occurred while removing the temp folder at ${tmpDir}. Please remove it manually. Error: ${e}`,
                );
            }
        }
    });
});

app.listen(port, () => {
    console.log(`⚡️[server]: Server is running at http://localhost:${port}`);
});
