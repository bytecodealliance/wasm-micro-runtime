package wamr

// #include <wasm_export.h>
// #include <stdlib.h>
// #include <stddef.h>
/*
uint8_t *
wasm_runtime_get_memory_data(const wasm_module_inst_t module_inst_comm,
                             uint32_t memory_inst_idx);

uint32_t
wasm_runtime_get_memory_data_size(const wasm_module_inst_t module_inst_comm,
                                  uint32_t memory_inst_idx);
*/
import "C"

import (
	"runtime"
	"unsafe"
	"fmt"
	"reflect"
)

type Instance struct {
	_instance C.wasm_module_inst_t
	_exec_env C.wasm_exec_env_t
	_module *Module
	_runtime *Runtime
	exportsCache map[string]C.wasm_function_inst_t
}

func NewInstance(module *Module, wasmRuntime *Runtime) (*Instance, error) {
	stackSize := 16 * 8092
	heapSize := 8092

	var errorBytes = make([]byte, 128)
	var errorPtr *C.char
	errorLen := len(errorBytes)
	if errorLen > 0 {
		errorPtr = (*C.char)(unsafe.Pointer(&errorBytes[0]))
	}

	_inst := C.wasm_runtime_instantiate(module._module, C.uint(stackSize), C.uint(heapSize), errorPtr, C.uint(errorLen))
	if (_inst == nil) {
		return nil, fmt.Errorf("wasm_runtime_instantiate Error: ", string(errorBytes))
	}

    _env := C.wasm_runtime_create_exec_env(_inst, C.uint(stackSize));
	if (_env == nil) {
		return nil, fmt.Errorf("wasm_runtime_create_exec_env Error")
	}

	self := &Instance{
		_instance: _inst,
		_exec_env: _env,
		_module: module,
		_runtime: wasmRuntime,
		exportsCache: make(map[string]C.wasm_function_inst_t),
	}

	runtime.SetFinalizer(self, func(self *Instance) {
		self.Destroy()
	})

	return self, nil
}

// wamr use uint32 to pass arguments and save results
func (self *Instance) CallFunc(funcName string, argc uint32, args []uint32) error {
	_func := self.exportsCache[funcName]
	if _func == nil {
		cName := C.CString(funcName)
		defer C.free(unsafe.Pointer(cName))

		_func = C.wasm_runtime_lookup_function(self._instance, cName, (*C.char)(C.NULL))
		if _func == nil {
			return fmt.Errorf("wasm_runtime_lookup_function Error")
		}
		self.exportsCache[funcName] = _func
	}

    var args_C *C.uint32_t
    if (argc > 0) {
        args_C = (*C.uint32_t)(unsafe.Pointer(&args[0]))
    }
    if !C.wasm_runtime_call_wasm(self._exec_env, _func, C.uint(argc), args_C) {
		return fmt.Errorf("wasm_runtime_call_wasm Error: %s", string(self.GetException()))
	}

	return nil
}

func (self *Instance) GetMemoryDataSize(memoryIdx uint32) uint32 {
	return 0//(uint32)(C.wasm_runtime_get_memory_data_size(self._instance, C.uint32_t(memoryIdx)))
}

func (self *Instance) GetMemoryData(memoryIdx uint32) []byte {
	length := int(self.GetMemoryDataSize(memoryIdx))
	data := (*C.uint8_t)(nil)//C.wasm_runtime_get_memory_data(self._instance, C.uint32_t(memoryIdx)))

	var header reflect.SliceHeader
	header = *(*reflect.SliceHeader)(unsafe.Pointer(&header))

	header.Data = uintptr(unsafe.Pointer(data))
	header.Len = length
	header.Cap = length

	return *(*[]byte)(unsafe.Pointer(&header))
}

func (self *Instance) GetException() string {
	cStr := C.wasm_runtime_get_exception(self._instance)
	goStr := C.GoString(cStr)
	return goStr
}

func (self *Instance) Destroy() {
	runtime.SetFinalizer(self, nil)
	C.wasm_runtime_deinstantiate(self._instance)
    C.wasm_runtime_destroy_exec_env(self._exec_env);
}
