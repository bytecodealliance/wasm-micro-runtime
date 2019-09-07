/*
 * Copyright (C) 2019 Taobao (China) Inc.  All rights reserved.
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

#ifndef _WASM_APPLICATION_H
#define _WASM_APPLICATION_H

//#include "wasm_runtime.h"


#ifdef __cplusplus
extern "C" {
#endif

struct WASMModuleInstance;

/**
 * Find the unique main function from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main function is called, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_application_execute_main(struct WASMModuleInstance *module_inst,
                              int argc, char *argv[]);

/**
 * Find the specified function in argv[0] from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param name the name of the function to execute
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the specified function is called, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_application_execute_func(struct WASMModuleInstance *module_inst,
                              char *name, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_APPLICATION_H */

