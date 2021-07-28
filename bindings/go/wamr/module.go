package wamr

// #include <wasm_export.h>
import "C"
import (
	"unsafe"
	"runtime"
	"fmt"
)

type Module struct {
	_module C.wasm_module_t
}

func NewModule(wasmBytes []byte) (*Module, error) {
	var wasmPtr *C.uint8_t
	wasmLen := len(wasmBytes)
	if wasmLen > 0 {
		wasmPtr = (*C.uint8_t)(unsafe.Pointer(&wasmBytes[0]))
	}

	var errorPtr *C.char
	var errorBytes = make([]byte, 128)
	errorLen := len(errorBytes)
	if errorLen > 0 {
		errorPtr = (*C.char)(unsafe.Pointer(&errorBytes[0]))
	}

	_m := C.wasm_runtime_load(wasmPtr, C.uint(wasmLen), errorPtr, C.uint(errorLen))
	if (_m == nil) {
		return nil, fmt.Errorf("NewModule Error: %v", string(errorBytes))
	}

	self := &Module{
		_module: _m,
	}

	runtime.SetFinalizer(self, func(self *Module) {
		self.Destroy()
	})

	return self, nil
}

func (self *Module) Destroy() {
	runtime.SetFinalizer(self, nil)
	C.wasm_runtime_unload(self._module)
}
