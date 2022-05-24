/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

package wamr

// #include <stdlib.h>
// #include <wasm_export.h>
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

    var args_C *C.uint32_t
    if (argc > 0) {
        args_C = (*C.uint32_t)(unsafe.Pointer(&args[0]))
    }
    if (!C.wasm_runtime_call_wasm(self._exec_env, _func,
                                  C.uint(argc), args_C)) {
        return fmt.Errorf("CallFunc error: %s", string(self.GetException()))
    }

    return nil
}

func (self *Instance) GetException() string {
    cStr := C.wasm_runtime_get_exception(self._instance)
    goStr := C.GoString(cStr)
    return goStr
}
