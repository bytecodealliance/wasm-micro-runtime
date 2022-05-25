/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

package wamr

/*
#include <stdlib.h>
#include <wasm_export.h>

static inline void
PUT_I64_TO_ADDR(uint32_t *addr, int64_t value)
{
    union {
        int64_t val;
        uint32_t parts[2];
    } u;
    u.val = value;
    addr[0] = u.parts[0];
    addr[1] = u.parts[1];
}

static inline void
PUT_F64_TO_ADDR(uint32_t *addr, double value)
{
    union {
        double val;
        uint32_t parts[2];
    } u;
    u.val = value;
    addr[0] = u.parts[0];
    addr[1] = u.parts[1];
}

static inline int64_t
GET_I64_FROM_ADDR(uint32_t *addr)
{
    union {
        int64_t val;
        uint32_t parts[2];
    } u;
    u.parts[0] = addr[0];
    u.parts[1] = addr[1];
    return u.val;
}

static inline double
GET_F64_FROM_ADDR(uint32_t *addr)
{
    union {
        double val;
        uint32_t parts[2];
    } u;
    u.parts[0] = addr[0];
    u.parts[1] = addr[1];
    return u.val;
}
*/
import "C"

import (
    "runtime"
    "unsafe"
    "fmt"
)

type Instance struct {
    _instance C.wasm_module_inst_t
    _exec_env C.wasm_exec_env_t
    _module *Module
    _exportsCache map[string]C.wasm_function_inst_t
}

func NewInstance(module *Module,
                 stackSize uint, heapSize uint) (*Instance, error) {
    if (module == nil) {
        return nil, fmt.Errorf("NewInstance error: invalid input")
    }

    errorBytes := make([]byte, 128)
    errorPtr := (*C.char)(unsafe.Pointer(&errorBytes[0]))
    errorLen := C.uint(len(errorBytes))

    instance := C.wasm_runtime_instantiate(module.module, C.uint(stackSize),
                                           C.uint(heapSize), errorPtr, errorLen)
    if (instance == nil) {
        return nil, fmt.Errorf("NewInstance Error: %s", string(errorBytes))
    }

    exec_env := C.wasm_runtime_create_exec_env(instance, C.uint(stackSize));
    if (exec_env == nil) {
        C.wasm_runtime_deinstantiate(instance)
        return nil, fmt.Errorf("NewInstance Error: create exec_env failed")
    }

    self := &Instance{
        _instance: instance,
        _exec_env: exec_env,
        _module: module,
        _exportsCache: make(map[string]C.wasm_function_inst_t),
    }

    runtime.SetFinalizer(self, func(self *Instance) {
        self.Destroy()
    })

    return self, nil
}

func (self *Instance) Destroy() {
    runtime.SetFinalizer(self, nil)
    if (self._instance != nil) {
        C.wasm_runtime_deinstantiate(self._instance)
    }
    if (self._exec_env != nil) {
        C.wasm_runtime_destroy_exec_env(self._exec_env)
    }
}

func (self *Instance) CallFunc(funcName string,
                               argc uint32, args []uint32) error {
    _func := self._exportsCache[funcName]
    if _func == nil {
        cName := C.CString(funcName)
        defer C.free(unsafe.Pointer(cName))

        _func = C.wasm_runtime_lookup_function(self._instance,
                                               cName, (*C.char)(C.NULL))
        if _func == nil {
            return fmt.Errorf("CallFunc error: lookup function failed")
        }
        self._exportsCache[funcName] = _func
    }

    thread_env_inited := Runtime().ThreadEnvInited()
    if (!thread_env_inited) {
        Runtime().InitThreadEnv()
    }

    var args_C *C.uint32_t
    if (argc > 0) {
        args_C = (*C.uint32_t)(unsafe.Pointer(&args[0]))
    }
    if (!C.wasm_runtime_call_wasm(self._exec_env, _func,
                                  C.uint(argc), args_C)) {
        if (!thread_env_inited) {
            Runtime().DestroyThreadEnv()
        }
        return fmt.Errorf("CallFunc error: %s", string(self.GetException()))
    }

    if (!thread_env_inited) {
        Runtime().DestroyThreadEnv()
    }
    return nil
}

func (self *Instance) CallFuncV(funcName string,
                                num_results uint32, results []interface{},
                                args ... interface{}) error {
    _func := self._exportsCache[funcName]
    if _func == nil {
        cName := C.CString(funcName)
        defer C.free(unsafe.Pointer(cName))

        _func = C.wasm_runtime_lookup_function(self._instance,
                                               cName, (*C.char)(C.NULL))
        if _func == nil {
            return fmt.Errorf("CallFunc error: lookup function failed")
        }
        self._exportsCache[funcName] = _func
    }

    param_count := uint32(C.wasm_func_get_param_count(_func, self._instance))
    result_count := uint32(C.wasm_func_get_result_count(_func, self._instance))

    if (num_results < result_count) {
        str := "CallFunc error: invalid result count %u, " +
               "must be no smaller than %u"
        return fmt.Errorf(str, num_results, result_count)
    }

    param_types := make([]C.uchar, param_count, param_count)
    result_types := make([]C.uchar, result_count, result_count)
    C.wasm_func_get_param_types(_func, self._instance,
                                (*C.uchar)(unsafe.Pointer(&param_types[0])))
    C.wasm_func_get_result_types(_func, self._instance,
                                 (*C.uchar)(unsafe.Pointer(&result_types[0])))

    argv_size := param_count * 2
    if (result_count > param_count) {
        argv_size = result_count * 2
    }
    argv := make([]uint32, argv_size, argv_size)

    var i, argc uint32
    for _, arg := range args {
        switch arg.(type) {
            case int32:
                if (param_types[i] != C.WASM_I32 &&
                    param_types[i] != C.WASM_FUNCREF &&
                    param_types[i] != C.WASM_ANYREF) {
                    str := "CallFunc error: invalid param type %u, " +
                           "expect i32 but got other"
                    return fmt.Errorf(str, param_types[i])
                }
                argv[argc] = (uint32)(arg.(int32))
                argc++
                break
            case int64:
                if (param_types[i] != C.WASM_I64) {
                    str := "CallFunc error: invalid param type %u, " +
                           "expect i64 but got other"
                    return fmt.Errorf(str, param_types[i])
                }
                addr := (*C.uint32_t)(unsafe.Pointer(&argv[argc]))
                C.PUT_I64_TO_ADDR(addr, (C.int64_t)(arg.(int64)))
                argc += 2
                break
            case float32:
                if (param_types[i] != C.WASM_F32) {
                    str := "CallFunc error: invalid param type %u, " +
                           "expect f32 but got other"
                    return fmt.Errorf(str, param_types[i])
                }
                *(*C.float)(unsafe.Pointer(&argv[argc])) = (C.float)(arg.(float32))
                argc++
                break;
            case float64:
                if (param_types[i] != C.WASM_F64) {
                    str := "CallFunc error: invalid param type %u, " +
                           "expect f64 but got other"
                    return fmt.Errorf(str, param_types[i])
                }
                addr := (*C.uint32_t)(unsafe.Pointer(&argv[argc]))
                C.PUT_F64_TO_ADDR(addr, (C.double)(arg.(float64)))
                argc += 2
                break
            default:
                return fmt.Errorf("CallFunc error: unknown param type %u",
                                  param_types[i])
        }
        i++
    }

    err := self.CallFunc(funcName, argc, argv)
    if (err != nil) {
        return err
    }

    argc = 0
    for i = 0; i < num_results; i++ {
        /* TODO: get the results */
    }

    return nil
}

func (self *Instance) GetException() string {
    cStr := C.wasm_runtime_get_exception(self._instance)
    goStr := C.GoString(cStr)
    return goStr
}
