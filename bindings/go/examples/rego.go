package main

import (
	"gitlab.alipay-inc.com/TNT_Runtime/ant-runtime/bindings/go/wamr"
	"io/ioutil"
	"fmt"
	"time"
)

var DATA_JSON = "testdata/data.json"
var INPUT_JSON = "testdata/input.json"
var POLICY_AOT = "testdata/policy.aot"
var POLICY_WASM = "testdata/policy.wasm"

type Rego struct {
	_instance *wamr.Instance
	baseHeapPtr    uint32
	baseHeapTopPtr uint32
	dataPtr       uint32
	dataHeapPtr    uint32
}

func NewRego(instance *wamr.Instance) (*Rego, error) {
	self := &Rego{
		_instance: instance,
	}

	dataPtr, err := self.loadJson("{}")
	if err != nil {
		return nil, err
	}
	self.dataPtr = dataPtr

	argv := make([]uint32, 1)
	err2 := self._instance.CallFunc("opa_heap_ptr_get", 0, argv)
	if err2 != nil {
		return nil, err
	}

	self.baseHeapPtr = argv[0]
	self.dataHeapPtr = argv[0]

	return self, nil
}

func (self *Rego) loadJson(data string) (uint32, error) {
	argv := make([]uint32, 2)
	_len := uint32(len(data))
	argv[0] = _len + 1 // c string need '\0'

	err := self._instance.CallFunc("opa_malloc", 1, argv)
	if err != nil {
		return 0, err
	}

	addr := argv[0]
	memory := self._instance.GetMemoryData(0)[addr:]
	for i := uint32(0); i < _len; i++ {
		memory[i] = data[i]
	}
	memory[_len] = 0

	argv[0] = addr
	// argv[1] = _len + 1 // c string need '\0'
	argv[1] = _len // c string need '\0'
	err2 := self._instance.CallFunc("opa_json_parse", 2, argv)
	if err2 != nil {
		return 0, err2
	}

	return argv[0], nil
}

func (self *Rego) dumpJson(resultAddr uint32) (string, error) {
	argv := make([]uint32, 1)
	argv[0] = resultAddr
	errDump := self._instance.CallFunc("opa_json_dump", 1, argv)
	if errDump != nil {
		return "", errDump
	}
	ptr := argv[0]

	memory := self._instance.GetMemoryData(0)[ptr:]

	var contents []byte
	for i := uint32(0); memory[i] != 0; i++ {
		contents = append(contents, memory[i])
	}

	return string(contents), nil
}

func (self *Rego) setData(data string) error {
	argv := make([]uint32, 2)
	argv[0] = self.baseHeapPtr;

	err := self._instance.CallFunc("opa_heap_ptr_set", 1, argv)
	if err != nil {
		fmt.Println(err, self._instance.GetException())
		return err
	}

	dataPtr, err2 := self.loadJson(data)
	if err2 != nil {
		fmt.Println(err, 2)
		return err2
	}
	self.dataPtr = dataPtr

	err3 := self._instance.CallFunc("opa_heap_ptr_get", 0, argv)
	if err3 != nil {
		fmt.Println(err, 3)
		return err3
	}
	self.dataHeapPtr = argv[0]

	return nil
}

func (self *Rego) eval(input string) (string, error) {
	t1 := time.Now();
	argv := make([]uint32, 2)
	argv[0] = self.dataHeapPtr
	errPtrSet := self._instance.CallFunc("opa_heap_ptr_set", 1, argv)
	if errPtrSet != nil {
		return "", errPtrSet
	}

	inputAddr, errLoadInput := self.loadJson(input)
	if errLoadInput != nil {
		return "", errLoadInput
	}

	errCtxNew := self._instance.CallFunc("opa_eval_ctx_new", 0, argv)
	if errCtxNew != nil {
		return "", errCtxNew
	}
	ctxAddr := argv[0]

	argv[0] = ctxAddr
	argv[1] = self.dataPtr
	errCtxSetData := self._instance.CallFunc("opa_eval_ctx_set_data", 2, argv)
	if errCtxSetData != nil {
		return "", errCtxSetData
	}

	argv[0] = ctxAddr
	argv[1] = inputAddr
	errSetInput := self._instance.CallFunc("opa_eval_ctx_set_input", 2, argv)
	if errSetInput != nil {
		return "", errSetInput
	}
	t2 := time.Now();
	fmt.Println("before eval time cost = %v", time.Since(t1))

	argv[0] = ctxAddr
	errEval := self._instance.CallFunc("eval", 1, argv)
	if errEval != nil {
		return "", errEval
	}
	fmt.Println("eval time cost = %v", time.Since(t2))
	t3 := time.Now();

	argv[0] = ctxAddr
	errGetResult := self._instance.CallFunc("opa_eval_ctx_get_result", 1, argv)
	if errGetResult != nil {
		return "", errGetResult
	}
	resultAddr := argv[0]
	result, errDumpJson := self.dumpJson(resultAddr)
	if errDumpJson != nil {
		return "", errDumpJson
	}
	fmt.Println("after eval time cost = %v", time.Since(t3))

	return result, nil
}

func main() {
	t1 := time.Now();

	_runtime := wamr.NewWamrRuntime()
	_runtime.SetLogLevel(wamr.LOG_LEVEL_FATAL)
	// register import, TODO

	err := _runtime.FullInit()
	if err != nil {
		return
	}

	//wasmBytes, errPolicy := ioutil.ReadFile(POLICY_WASM)
	wasmBytes, errPolicy := ioutil.ReadFile(POLICY_AOT)
	if errPolicy != nil {
		fmt.Println(errPolicy, 1)
		return
	}

	module, err2 := wamr.NewModule(wasmBytes)
	if err2 != nil {
		fmt.Println(err2, 2)
		return
	}

	instance, err3 := wamr.NewInstance(module)
	if err3 != nil {
		fmt.Println(err3, 3)
		return
	}

	rego, err4 := NewRego(instance)
	if err4 != nil {
		fmt.Println(err4, 4)
		return
	}

	dataBytes, errData := ioutil.ReadFile(DATA_JSON)
	if errData != nil {
		fmt.Println(errData, 5)
		return
	}

	errSetData := rego.setData(string(dataBytes))
	if errSetData != nil {
		fmt.Println(errSetData, 6)
		return
	}

	inputBytes, errInput := ioutil.ReadFile(INPUT_JSON)
	if errInput != nil {
		return
	}
	fmt.Println("cold start time cost = %v", time.Since(t1))
	t2 := time.Now();

	for i := 0; i < 1000; i++ {
		//result, errEval := rego.eval(string(inputBytes))
		_, errEval := rego.eval(string(inputBytes))
		if errEval != nil {
			fmt.Println(errEval, 7)
			return
		}
	}
	fmt.Println("eval time cost = %v", time.Since(t2))

	return
}
