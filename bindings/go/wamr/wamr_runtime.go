package wamr

/*
#include <wasm_export.h>
#include <stdlib.h>
#include <stddef.h>
RuntimeInitArgs init_args;
void init_wamr_runtime() {
#ifdef POOL_ALLOC
    static char global_heap_buf[256 * 1024] = { 0 };//(256 kB)

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
#elif defined(FUNC_ALLOC)
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;
#else
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
#endif
	init_args.n_native_symbols = 0;
	init_args.native_module_name = "env";
	init_args.native_symbols = NULL;
}
void bh_log_set_verbose_level(uint32_t level);
*/
import "C"
import (
//	"unsafe"
	"runtime"
	"fmt"
)

type LogLevel int
const (
    LOG_LEVEL_FATAL LogLevel = 0
    LOG_LEVEL_ERROR
    LOG_LEVEL_WARNING
    LOG_LEVEL_DEBUG
    LOG_LEVEL_VERBOSE
)

type WamrRuntime struct {
//	imports map[string]ImportFunc
}

// TODO: add import register
func NewWamrRuntime() (*WamrRuntime) {
	self := &WamrRuntime{}

	runtime.SetFinalizer(self, func(self *Module) {
		self.Destroy()
	})

	return self;
}

// TODO
func (self *WamrRuntime) RegisterNativeSymbol() {}

func (self *WamrRuntime) FullInit() error {
	C.init_wamr_runtime()

	if !C.wasm_runtime_full_init(&C.init_args) {
		return fmt.Errorf("FullInit Error")
	}

	return nil
}

func (self *WamrRuntime) SetLogLevel(level LogLevel) {
	C.bh_log_set_verbose_level(C.uint32_t(level))
}

// TODO: destroy native symbols
func (self *WamrRuntime) Destroy() {
	C.wasm_runtime_destroy()
}
