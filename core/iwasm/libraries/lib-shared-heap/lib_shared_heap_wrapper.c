#include "bh_common.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "../interpreter/wasm.h"

/* clang-format off */
#define get_module_inst(exec_env) \
    wasm_runtime_get_module_inst(exec_env)

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define validate_app_str_addr(offset) \
    wasm_runtime_validate_app_str_addr(module_inst, offset)

#define validate_native_addr(addr, size) \
    wasm_runtime_validate_native_addr(module_inst, addr, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

#define module_shared_malloc(size, p_native_addr) \
    wasm_runtime_module_shared_malloc(module_inst, size, p_native_addr)
#define module_shared_free(offset) \
    wasm_runtime_module_shared_free(module_inst, offset)
/* clang-format on */


static uint32
shared_malloc_wrapper(wasm_exec_env_t exec_env, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    return (uint32)module_shared_malloc((uint64)size, NULL);
}

static void
shared_free_wrapper(wasm_exec_env_t exec_env, void *ptr)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!validate_native_addr(ptr, (uint64)sizeof(uint32)))
        return;

    module_shared_free(addr_native_to_app(ptr));
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_shared_heap[] = {
    REG_NATIVE_FUNC(shared_malloc, "(i)i"),
    REG_NATIVE_FUNC(shared_free, "(*)"),
};

uint32
get_lib_shared_heap_export_apis(NativeSymbol **p_shared_heap_apis)
{
    *p_shared_heap_apis = native_symbols_shared_heap;
    return sizeof(native_symbols_shared_heap) / sizeof(NativeSymbol);
}