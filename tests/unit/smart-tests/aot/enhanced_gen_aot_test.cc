/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <limits.h>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "aot_llvm.h"
#include "aot_intrinsic.h"
#include "aot.h"

#define G_INTRINSIC_COUNT (50u)
#define CONS(num) ("f##num##.const")

// Use external declarations to avoid multiple definitions
extern const char *llvm_intrinsic_tmp[G_INTRINSIC_COUNT];
extern uint64 g_intrinsic_flag[G_INTRINSIC_COUNT];

// Enhanced test fixture for coverage improvement
class EnhancedAOTTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    virtual void TearDown() { wasm_runtime_destroy(); }

  public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

// Enhanced test cases targeting set_error_buf_v function coverage
// Target: Lines 108-114 in aot_runtime.c set_error_buf_v function

TEST_F(EnhancedAOTTest, WasmLoader_NullErrorBuffer_HandlesGracefully) {
    // Tests WASM loader error handling when error_buf is NULL
    // Path: wasm_runtime_load -> get_package_type -> wasm_loader with NULL buffer
    
    uint8_t invalid_data[] = {0x00, 0x61, 0x73, 0x6d}; // WASM magic but incomplete
    uint32_t data_size = sizeof(invalid_data);
    
    // Load module with NULL error buffer - should handle gracefully without crash
    wasm_module_t module = wasm_runtime_load(invalid_data, data_size, NULL, 0);
    ASSERT_EQ(nullptr, module);
}

TEST_F(EnhancedAOTTest, WasmLoader_ValidErrorBuffer_FormatsErrorMessage) {
    // Tests WASM loader error formatting with valid error buffer
    // Path: wasm_runtime_load -> wasm_loader_set_error_buf -> error formatting
    
    uint8_t invalid_data[] = {0x00, 0x61, 0x73, 0x6d}; // WASM magic but incomplete
    uint32_t data_size = sizeof(invalid_data);
    char error_buf[256];
    memset(error_buf, 0, sizeof(error_buf));
    
    // Load module with valid error buffer - triggers error formatting
    wasm_module_t module = wasm_runtime_load(invalid_data, data_size, error_buf, sizeof(error_buf));
    ASSERT_EQ(nullptr, module);
    
    // Verify error message was generated
    ASSERT_GT(strlen(error_buf), 0);
}

TEST_F(EnhancedAOTTest, WasmLoader_MalformedData_GeneratesError) {
    // Tests WASM loader handling of malformed data with large error buffer
    // Path: wasm_runtime_load -> get_package_type -> validation failure
    
    // Use malformed data to trigger error handling
    uint8_t malformed_data[1024];
    memset(malformed_data, 0xFF, sizeof(malformed_data)); // Fill with invalid data
    char error_buf[512];
    memset(error_buf, 0, sizeof(error_buf));
    
    // This should trigger error handling
    wasm_module_t module = wasm_runtime_load(malformed_data, sizeof(malformed_data), error_buf, sizeof(error_buf));
    ASSERT_EQ(nullptr, module);
    
    // Verify error message was generated
    ASSERT_GT(strlen(error_buf), 0);
}

TEST_F(EnhancedAOTTest, WasmLoader_MinimalModule_LoadAndInstantiate) {
    // Tests WASM module loading and instantiation paths
    // Path: wasm_runtime_load -> wasm_runtime_instantiate
    
    uint8_t simple_wasm[] = {
        0x00, 0x61, 0x73, 0x6d, // WASM magic
        0x01, 0x00, 0x00, 0x00, // WASM version
    };
    char error_buf[256];
    memset(error_buf, 0, sizeof(error_buf));
    
    wasm_module_t module = wasm_runtime_load(simple_wasm, sizeof(simple_wasm), error_buf, sizeof(error_buf));
    
    if (module) {
        // Try to instantiate - may succeed or fail depending on module contents
        wasm_module_inst_t inst = wasm_runtime_instantiate(module, 8192, 8192, error_buf, sizeof(error_buf));
        
        if (inst) {
            wasm_runtime_deinstantiate(inst);
        }
        wasm_runtime_unload(module);
        ASSERT_TRUE(true); // Module loaded successfully
    } else {
        // Load failed - verify error message exists
        ASSERT_GT(strlen(error_buf), 0);
    }
}

/*
 * COMPREHENSIVE COVERAGE TESTS FOR aot_lookup_function_with_idx
 * TARGET: Lines 1421-1452 in aot_runtime.c
 *
 * CALL PATHS EVALUATED:
 * 1. Direct call to aot_lookup_function_with_idx() [SELECTED - Direct testing]
 *    - Depth: 1 level
 *    - Complexity: LOW (minimal setup required)
 *    - Precision: HIGH (direct targeting of specific lines)
 *    - Rating: ⭐⭐⭐⭐
 *
 * 2. aot_get_function_instance() -> aot_lookup_function_with_idx() [Alternative]
 *    - Depth: 2 levels
 *    - Complexity: MEDIUM (requires valid AOT module setup)
 *    - Precision: MEDIUM (additional code paths involved)
 *    - Rating: ⭐⭐⭐
 *
 * SELECTED STRATEGY: Use aot_lookup_function_with_idx() directly with crafted AOTModuleInstance
 * REASON: Most precise targeting of lines 1421-1452 with minimal test complexity
 */

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_NoExportFunctions_ReturnsNull) {
    // Target: Line 1418-1419: if (module_inst->export_func_count == 0) return NULL;
    // This test ensures early return when no export functions exist

    // Create a minimal AOT module instance with no export functions
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 0; // No export functions

    // Call should return NULL immediately without entering lock section
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 0);
    ASSERT_EQ(nullptr, result);
}

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_MapCreation_FindsViaMapSearch) {
    // Target: Lines 1421-1452 - Map creation and binary search path
    // Path: export_func_maps=NULL -> runtime_malloc -> qsort -> bsearch -> found

    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTFunctionInstance export_funcs[2];

    // Setup module instance with export functions
    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(export_funcs, 0, sizeof(export_funcs));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 2;
    module_inst.export_functions = (WASMExportFuncInstance*)export_funcs;

    // Setup export function data
    export_funcs[0].func_index = 100;
    export_funcs[1].func_index = 200;

    // Ensure export_func_maps is NULL to trigger map creation
    extra.export_func_maps = NULL;

    // Test finds function via map creation + binary search
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 100);
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(100, result->func_index);
}

TEST_F(EnhancedAOTTest, LookupFunctionWithIdx_BinarySearch_NotFound) {
    // Target: Binary search not finding function
    // Path: export_func_maps=NULL -> map creation -> bsearch returns NULL

    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTFunctionInstance export_funcs[2];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(export_funcs, 0, sizeof(export_funcs));

    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.export_func_count = 2;
    module_inst.export_functions = (WASMExportFuncInstance*)export_funcs;

    export_funcs[0].func_index = 100;
    export_funcs[1].func_index = 200;

    extra.export_func_maps = NULL; // Triggers map creation

    // Search for non-existent function index - binary search returns NULL
    AOTFunctionInstance* result = aot_lookup_function_with_idx(&module_inst, 999);
    ASSERT_EQ(nullptr, result);
}


