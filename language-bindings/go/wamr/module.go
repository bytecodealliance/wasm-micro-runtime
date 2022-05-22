/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

package wamr

// #include <wasm_export.h>
import "C"
import (
    "unsafe"
    "runtime"
    "fmt"
)

type Module struct {
    module C.wasm_module_t
}

func NewModule(wasmBytes []byte) (*Module, error) {
    if (wasmBytes == nil || len(wasmBytes) == 0) {
        return nil, fmt.Errorf("NewModule error: invalid input")
    }

    wasmPtr := (*C.uint8_t)(unsafe.Pointer(&wasmBytes[0]))
    wasmLen := C.uint(len(wasmBytes))

    errorBytes := make([]byte, 128)
    errorPtr := (*C.char)(unsafe.Pointer(&errorBytes[0]))
    errorLen := C.uint(len(errorBytes))

    m := C.wasm_runtime_load(wasmPtr, wasmLen, errorPtr, errorLen)
    if (m == nil) {
        return nil, fmt.Errorf("NewModule error: %s", string(errorBytes))
    }

    self := &Module{
        module: m,
    }

    runtime.SetFinalizer(self, func(self *Module) {
        self.Destroy()
    })

    return self, nil
}

func (self *Module) Destroy() {
    runtime.SetFinalizer(self, nil)
    if (self.module != nil) {
        C.wasm_runtime_unload(self.module)
    }
}
