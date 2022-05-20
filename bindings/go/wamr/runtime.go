package wamr

/*
#include <stdlib.h>
#include <string.h>

#include <wasm_export.h>

void
bh_log_set_verbose_level(uint32_t level);

bool
init_wamr_runtime(bool alloc_with_pool, uint8_t *heap_buf,
                  uint32_t heap_size, uint32_t max_thread_num)
{
    RuntimeInitArgs init_args;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    if (alloc_with_pool) {
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = heap_buf;
        init_args.mem_alloc_option.pool.heap_size = heap_size;
    }
    else {
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
    }

    return wasm_runtime_full_init(&init_args);
}
*/
import "C"
import (
    "fmt"
    "unsafe"
)

type LogLevel int
const (
    LOG_LEVEL_FATAL LogLevel = 0
    LOG_LEVEL_ERROR
    LOG_LEVEL_WARNING
    LOG_LEVEL_DEBUG
    LOG_LEVEL_VERBOSE
)

type Runtime struct {
    initialized bool
}

var runtime_singleton *Runtime

func CreateRuntime() (*Runtime) {
    if (runtime_singleton != nil) {
        return runtime_singleton
    }

    self := &Runtime{}
    runtime_singleton = self
    return self;
}

func (self *Runtime) DestroyRuntime() {
    if (self.initialized) {
        C.wasm_runtime_destroy()
    }
    runtime_singleton = nil
}

func (self *Runtime) FullInit(alloc_with_pool bool,
                              heap_buf []byte, heap_size uint,
                              max_thread_num uint) error {
    if (self.initialized) {
        return nil
    }

    var heap_buf_C *C.uchar
    if (alloc_with_pool) {
        if (heap_buf == nil) {
            return fmt.Errorf("Failed to init WAMR runtime")
        }
        heap_buf_C = (*C.uchar)(unsafe.Pointer(&heap_buf[0]))
    }

    if (!C.init_wamr_runtime((C.bool)(alloc_with_pool), heap_buf_C,
                             (C.uint)(heap_size), (C.uint)(max_thread_num))) {
        return fmt.Errorf("Failed to init WAMR runtime")
    }

    return nil
}

func (self *Runtime) Init() error {
    return self.FullInit(false, nil, 0, 4)
}

func (self *Runtime) SetLogLevel(level LogLevel) {
    C.bh_log_set_verbose_level(C.uint32_t(level))
}
