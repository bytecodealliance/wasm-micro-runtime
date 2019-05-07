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

#include "wasm-runtime.h"

void invokeNative(uint32 argv[], uint32 argc, void (*native_code)())
{
    WASMThread *self;
    switch(argc) {
        case 0:
            native_code();
            break;
        case 1:
            native_code(argv[0]);
            break;
        case 2:
            native_code(argv[0], argv[1]);
            break;
        case 3:
            native_code(argv[0], argv[1], argv[2]);
            break;
        case 4:
            native_code(argv[0], argv[1], argv[2], argv[3]);
            break;
        case 5:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4]);
            break;
        case 6:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
            break;
        case 7:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
            break;
        case 8:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
            break;
        case 9:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
            break;
        case 10:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
            break;
        case 11:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]);
            break;
        case 12:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11]);
            break;
        case 13:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]);
            break;
        case 14:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13]);
            break;
        case 15:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14]);
            break;
        case 16:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15]);
            break;
        case 17:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16]);
            break;
        case 18:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17]);
            break;
        case 19:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17], argv[18]);
            break;
        case 20:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12], argv[13], argv[14], argv[15], argv[16], argv[17], argv[18], argv[19]);
            break;
        default:
            /* FIXME: If this happen, add more cases. */
            self = wasm_runtime_get_self();
            wasm_runtime_set_exception(self->module_inst, "the argument number of native function exceeds maximum");
            return;
    }
}
