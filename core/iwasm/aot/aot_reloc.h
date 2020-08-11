/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_runtime.h"

typedef struct {
    const char *symbol_name;
    void *symbol_addr;
} SymbolMap;

#define REG_SYM(symbol) { #symbol, (void*)symbol }

#if WASM_ENABLE_BULK_MEMORY != 0
#define REG_BULK_MEMORY_SYM()             \
    REG_SYM(aot_memory_init),             \
    REG_SYM(aot_data_drop),
#else
#define REG_BULK_MEMORY_SYM()
#endif

#if WASM_ENABLE_SHARED_MEMORY != 0
#include "wasm_shared_memory.h"
#define REG_ATOMIC_WAIT_SYM()             \
    REG_SYM(wasm_runtime_atomic_wait),    \
    REG_SYM(wasm_runtime_atomic_notify),
#else
#define REG_ATOMIC_WAIT_SYM()
#endif

#if (defined(_WIN32) || defined(_WIN32_)) && defined(NDEBUG)
#define REG_COMMON_SYMBOLS                \
    REG_SYM(aot_set_exception_with_id),   \
    REG_SYM(aot_invoke_native),           \
    REG_SYM(aot_call_indirect),           \
    REG_SYM(wasm_runtime_enlarge_memory), \
    REG_SYM(wasm_runtime_set_exception),  \
    REG_BULK_MEMORY_SYM()                 \
    REG_ATOMIC_WAIT_SYM()
#else /* else of (defined(_WIN32) || defined(_WIN32_)) && defined(NDEBUG) */
#define REG_COMMON_SYMBOLS                \
    REG_SYM(aot_set_exception_with_id),   \
    REG_SYM(aot_invoke_native),           \
    REG_SYM(aot_call_indirect),           \
    REG_SYM(wasm_runtime_enlarge_memory), \
    REG_SYM(wasm_runtime_set_exception),  \
    REG_SYM(fmin),                        \
    REG_SYM(fminf),                       \
    REG_SYM(fmax),                        \
    REG_SYM(fmaxf),                       \
    REG_SYM(ceil),                        \
    REG_SYM(ceilf),                       \
    REG_SYM(floor),                       \
    REG_SYM(floorf),                      \
    REG_SYM(trunc),                       \
    REG_SYM(truncf),                      \
    REG_SYM(rint),                        \
    REG_SYM(rintf),                       \
    REG_BULK_MEMORY_SYM()                 \
    REG_ATOMIC_WAIT_SYM()
#endif /* end of (defined(_WIN32) || defined(_WIN32_)) && defined(NDEBUG) */

#define CHECK_RELOC_OFFSET(data_size) do {                                  \
    if (!check_reloc_offset(target_section_size, reloc_offset, data_size,   \
                            error_buf, error_buf_size))                     \
        return false;                                                       \
  } while (0)

SymbolMap *
get_target_symbol_map(uint32 *sym_num);

uint32
get_plt_table_size();

void
init_plt_table(uint8 *plt);

void
get_current_target(char *target_buf, uint32 target_buf_size);

bool
apply_relocation(AOTModule *module,
                 uint8 *target_section_addr, uint32 target_section_size,
                 uint64 reloc_offset, uint64 reloc_addend,
                 uint32 reloc_type, void *symbol_addr, int32 symbol_index,
                 char *error_buf, uint32 error_buf_size);

