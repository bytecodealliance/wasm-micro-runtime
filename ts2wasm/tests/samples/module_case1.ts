/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { module_case2_var1, module_case2_func1 } from './module_case2';

const module_case1_var1 = module_case2_var1;
const module_case1_func1 = module_case2_func1;
const module_case1_var2 = module_case1_func1();
