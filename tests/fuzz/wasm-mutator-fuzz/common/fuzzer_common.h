// Copyright (C) 2025 Intel Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef FUZZER_COMMON_H
#define FUZZER_COMMON_H

#include "wasm_export.h"
#include <iostream>
#include <vector>

// Constants for consistent buffer sizes
constexpr size_t ERROR_BUF_SIZE = 128;
constexpr size_t MAX_ERROR_BUF_SIZE = 120; // Used in wasm_runtime_load

// Error phases for consistent reporting
enum class FuzzerErrorPhase {
    LOADING,
    INSTANTIATING,
    COMPILING,
    EXECUTION,
    CLEANUP
};

// Small inline helper functions

// Check if a value kind is supported by the fuzzer
static inline bool
is_supported_val_kind(wasm_valkind_t kind)
{
    return kind == WASM_I32 || kind == WASM_I64 || kind == WASM_F32
           || kind == WASM_F64 || kind == WASM_EXTERNREF
           || kind == WASM_FUNCREF;
}

// Generate a predefined value for a given value kind
static inline wasm_val_t
pre_defined_val(wasm_valkind_t kind)
{
    if (kind == WASM_I32) {
        return wasm_val_t{ .kind = WASM_I32, .of = { .i32 = 2025 } };
    }
    else if (kind == WASM_I64) {
        return wasm_val_t{ .kind = WASM_I64, .of = { .i64 = 168 } };
    }
    else if (kind == WASM_F32) {
        return wasm_val_t{ .kind = WASM_F32, .of = { .f32 = 3.14159f } };
    }
    else if (kind == WASM_F64) {
        return wasm_val_t{ .kind = WASM_F64, .of = { .f64 = 2.71828 } };
    }
    else if (kind == WASM_EXTERNREF) {
        return wasm_val_t{ .kind = WASM_EXTERNREF,
                           .of = { .foreign = 0xabcddead } };
    }
    // because aft is_supported_val_kind() check, so we can safely return as
    // WASM_FUNCREF
    else {
        return wasm_val_t{ .kind = WASM_FUNCREF, .of = { .ref = nullptr } };
    }
}

// Function declarations (implemented in fuzzer_common.cc)

// Print execution arguments for debugging
void
print_execution_args(const wasm_export_t &export_type,
                     const std::vector<wasm_val_t> &args, unsigned param_count);

// Execute all export functions in a module
bool
execute_export_functions(wasm_module_t module, wasm_module_inst_t inst);

// Helper for consistent error reporting
void
report_fuzzer_error(FuzzerErrorPhase phase, const char *message);

#endif // FUZZER_COMMON_H