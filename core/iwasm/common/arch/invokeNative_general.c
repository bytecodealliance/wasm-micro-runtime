/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "../wasm_runtime_common.h"
#include "../wasm_exec_env.h"

int64 invokeNative(void (*_native_code)(), uint32 argv[], uint32 argc)
{
    bh_assert(argc >= sizeof(WASMExecEnv*)/sizeof(uint32));

    int64 (*native_code)() = (int64 (*)()) _native_code;
    switch(argc) {
        case 0:
            return native_code();
            break;
        case 1:
            return native_code(argv[0]);
            break;
        case 2:
            return native_code(argv[0], argv[1]);
            break;
        case 3:
            return native_code(argv[0], argv[1], argv[2]);
            break;
        case 4:
            return native_code(argv[0], argv[1], argv[2], argv[3]);
            break;
        case 5:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4]);
            break;
        case 6:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5]);
            break;
        case 7:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6]);
            break;
        case 8:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7]);
            break;
        case 9:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8]);
            break;
        case 10:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9]);
            break;
        case 11:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10]);
            break;
        case 12:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11]);
            break;
        case 13:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12]);
            break;
        case 14:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13]);
            break;
        case 15:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14]);
            break;
        case 16:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14],
                               argv[15]);
            break;
        case 17:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14],
                               argv[15], argv[16]);
            break;
        case 18:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14],
                               argv[15], argv[16], argv[17]);
            break;
        case 19:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14],
                               argv[15], argv[16], argv[17], argv[18]);
            break;
        case 20:
            return native_code(argv[0], argv[1], argv[2], argv[3], argv[4],
                               argv[5], argv[6], argv[7], argv[8], argv[9],
                               argv[10], argv[11], argv[12], argv[13], argv[14],
                               argv[15], argv[16], argv[17], argv[18], argv[19]);
            break;
        default:
        {
            /* FIXME: If this happen, add more cases. */
            WASMExecEnv *exec_env = *(WASMExecEnv**)argv;
            WASMModuleInstanceCommon *module_inst = exec_env->module_inst;
            wasm_runtime_set_exception(module_inst,
                    "the argument number of native function exceeds maximum");
            return 0;
        }
    }
}

