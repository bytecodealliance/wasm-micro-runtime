/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import path from 'path';
import log4js from 'log4js';
import stackTrace from 'stacktrace-js';
import config from '../config/log4js.js';

export enum LoggerLevel {
    ALL = 'ALL',
    MARK = 'MARK',
    TRACE = 'TRACE',
    DEBUG = 'DEBUG',
    INFO = 'INFO',
    WARN = 'WARN',
    ERROR = 'ERROR',
    FATAL = 'FATAL',
    OFF = 'OFF',
}

log4js.configure(config);

const logger = log4js.getLogger();
logger.level = LoggerLevel.TRACE;

export const consoleLogger = log4js.getLogger('console');
logger.level = LoggerLevel.ERROR;

export class Logger {
    static trace(...args: any[]) {
        logger.trace(Logger.getStackTrace(), ...args);
    }

    static debug(...args: any[]) {
        logger.debug(Logger.getStackTrace(), ...args);
    }

    static log(...args: any[]) {
        logger.info(Logger.getStackTrace(), ...args);
    }

    static info(...args: any[]) {
        logger.info(Logger.getStackTrace(), ...args);
    }

    static warn(...args: any[]) {
        logger.warn(Logger.getStackTrace(), ...args);
    }

    static warning(...args: any[]) {
        logger.warn(Logger.getStackTrace(), ...args);
    }

    static error(...args: any[]) {
        logger.error(Logger.getStackTrace(), ...args);
    }

    static fatal(...args: any[]) {
        logger.fatal(Logger.getStackTrace(), ...args);
    }

    static getStackTrace(deep = 2): string {
        const stackList: stackTrace.StackFrame[] = stackTrace.getSync();
        const stackInfo: stackTrace.StackFrame = stackList[deep];
        const lineNumber: number = stackInfo.lineNumber!;
        const columnNumber: number = stackInfo.columnNumber!;
        const fileName: string = stackInfo.fileName!;
        const basename: string = path.basename(fileName);
        return `${basename} (line: ${lineNumber}, column: ${columnNumber}): \n`;
    }
}
