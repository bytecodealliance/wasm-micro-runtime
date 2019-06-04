/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MODULE_CONFIG_H_
#define _MODULE_CONFIG_H_

#define ENABLE_MODULE_JEFF 0
#define ENABLE_MODULE_WASM_APP 1
#define ENABLE_MODULE_WASM_LIB 1

#ifdef ENABLE_MODULE_JEFF
#include "module_jeff.h"
#endif
#ifdef ENABLE_MODULE_WASM_APP
#include "module_wasm_app.h"
#endif
#ifdef ENABLE_MODULE_WASM_LIB
#include "module_wasm_lib.h"
#endif

#endif /* _MODULE_CONFIG_H_ */
