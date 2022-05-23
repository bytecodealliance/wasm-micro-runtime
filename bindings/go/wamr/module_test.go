package wamr

import (
	"github.com/stretchr/testify/assert"
	"testing"
	"io/ioutil"
)

func TestModule(t *testing.T) {
	_runtime := NewWamrRuntime()
	err := _runtime.FullInit()
	assert.NoError(t, err)

	wasmBytes, _ := ioutil.ReadFile("testdata/module0.wasm")
	_, err2 := NewModule(wasmBytes)
	assert.NoError(t, err2)
}
