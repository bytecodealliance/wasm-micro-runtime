/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { add as renameAdd } from './export_normal';
import { add } from '../export_func';
import { declareAdd, declareVar } from './export_declare';

const invokeImportedRenameFuncRes = renameAdd(1, 2);
const invokeImportedFuncRes = add(1, 2);
const invokeDeclareFuncRes = declareAdd(5, 6);
const importedDeclaredVar = declareVar;

const importedDeclaredFunc = declareAdd;
importedDeclaredFunc(7, 8);
