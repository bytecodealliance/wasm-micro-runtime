package wamr

import (
	"github.com/stretchr/testify/assert"
	"testing"
	"io/ioutil"
//	"runtime"
//	"fmt"
)

func TestInstance(t *testing.T) {
	_runtime := NewWamrRuntime()
	err := _runtime.FullInit()
	assert.NoError(t, err)

	wasmBytes, _ := ioutil.ReadFile("testdata/module0.wasm")
	module, err2 := NewModule(wasmBytes)
	assert.NoError(t, err2)

	_, err3 := NewInstance(module)
	assert.NoError(t, err3)
}

func TestCallFunc(t *testing.T) {
	_runtime := NewWamrRuntime()
	err := _runtime.FullInit()
	assert.NoError(t, err)

	wasmBytes, _ := ioutil.ReadFile("testdata/simple.wasm")
	module, err2 := NewModule(wasmBytes)
	assert.NoError(t, err2)

	instance, err3 := NewInstance(module)
	assert.NoError(t, err3)

	args := []uint32{1, 2}
	err4 := instance.CallFunc("sum", 2, args)
	assert.NoError(t, err4)
//	fmt.Println(args)
	assert.Equal(t, args[0], uint32(3))
}
