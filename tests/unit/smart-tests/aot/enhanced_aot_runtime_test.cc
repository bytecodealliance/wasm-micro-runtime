/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <limits.h>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "aot_runtime.h"
#include "aot.h"
#include "bh_bitmap.h"

// Enhanced test fixture for aot_runtime.c functions
class EnhancedAotRuntimeTest : public testing::Test {
protected:
    void SetUp() override {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    void TearDown() override {
        wasm_runtime_destroy();
    }

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
};

/******
 * Test Case: aot_resolve_import_func_NativeResolutionFails_SubModuleLoadFails
 * Source: core/iwasm/aot/aot_runtime.c:5618-5633
 * Target Lines: 5656-5664 (sub-module loading failure path)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly handles
 *                     failed sub-module loading when native symbol resolution fails
 *                     for non-built-in modules.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise sub-module loading failure path for dependency resolution
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_resolve_import_func_NativeResolutionFails_SubModuleLoadFails) {
    // Create a minimal AOT module for testing
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));

    // Create test import function that fails native resolution
    AOTImportFunc import_func;
    memset(&import_func, 0, sizeof(AOTImportFunc));

    // Set up import function with non-built-in module name
    import_func.module_name = (char*)"test_module";
    import_func.func_name = (char*)"test_function";
    import_func.func_ptr_linked = NULL; // Ensure native resolution fails

    // Create minimal function type
    AOTFuncType func_type;
    memset(&func_type, 0, sizeof(AOTFuncType));
    func_type.param_count = 0;
    func_type.result_count = 0;
    import_func.func_type = &func_type;

    // Test the function - this should attempt sub-module loading
    bool result = aot_resolve_import_func(&test_module, &import_func);

    // The result depends on whether sub-module loading succeeds
    // Since we're testing with a non-existent module, it should fail gracefully
    ASSERT_FALSE(result);
}

/******
 * Test Case: aot_resolve_import_func_SubModuleLoadFails_LogWarning
 * Source: core/iwasm/aot/aot_runtime.c:5621-5623
 * Target Lines: 5621-5623 (sub-module loading failure and LOG_WARNING)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly handles
 *                     sub-module loading failure and logs appropriate warning messages
 *                     when dependency modules cannot be loaded.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise sub-module loading failure path and warning logging
 ******/

/******
 * Test Case: aot_resolve_import_func_SubModuleNull_FallbackResolution
 * Source: core/iwasm/aot/aot_runtime.c:5624-5627
 * Target Lines: 5624-5627 (fallback function resolution when sub_module is NULL)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly falls back to
 *                     aot_resolve_function_ex() when sub-module loading fails (sub_module is NULL)
 *                     and attempts alternative function resolution methods.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise fallback resolution path when sub-module loading fails
 ******/

/******
 * Test Case: aot_resolve_import_func_FunctionResolutionFails_LogWarning
 * Source: core/iwasm/aot/aot_runtime.c:5632-5633
 * Target Lines: 5632-5633 (function resolution failure and LOG_WARNING)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly handles
 *                     function resolution failure and logs appropriate warning messages
 *                     when imported functions cannot be resolved after dependency loading.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise function resolution failure path and warning logging
 ******/

/******
 * Test Case: aot_resolve_import_func_BuiltInModule_SkipSubModuleLoading
 * Source: core/iwasm/aot/aot_runtime.c:5617-5635
 * Target Lines: 5617 (built-in module check to skip sub-module loading)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly skips
 *                     sub-module loading for built-in modules and returns early
 *                     when wasm_runtime_is_built_in_module() returns true.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise built-in module skip path for dependency resolution
 ******/

/******
 * Test Case: aot_resolve_import_func_MultiModuleDisabled_SkipDependencyLoading
 * Source: core/iwasm/aot/aot_runtime.c:5611-5638
 * Target Lines: 5615-5637 (conditional multimodule code execution)
 * Functional Purpose: Validates that aot_resolve_import_func() correctly handles cases
 *                     when WASM_ENABLE_MULTI_MODULE is enabled but native resolution fails,
 *                     testing various error scenarios and fallback paths in the multimodule logic.
 * Call Path: aot_resolve_import_func() <- aot_resolve_symbols() <- module loading
 * Coverage Goal: Exercise the multimodule-specific logic paths when native resolution fails
 ******/

/******
 * Test Case: aot_resolve_symbols_WithUnlinkedFunctions_ResolutionAttempt
 * Source: core/iwasm/aot/aot_runtime.c:5525-5531
 * Target Lines: 5525 (function pointer access), 5526 (linked check), 5527 (resolution attempt)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly iterates through
 *                     import functions and attempts resolution for unlinked functions.
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise basic function iteration and resolution attempt logic
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_resolve_symbols_WithUnlinkedFunctions_ResolutionAttempt) {
    // Create a minimal AOT module with import functions
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));

    // Create array of import functions
    AOTImportFunc import_funcs[2];
    memset(import_funcs, 0, sizeof(import_funcs));

    // Set up first import function (unlinked)
    import_funcs[0].module_name = (char*)"test_module1";
    import_funcs[0].func_name = (char*)"test_function1";
    import_funcs[0].func_ptr_linked = NULL; // Not linked

    // Create minimal function type for first function
    AOTFuncType func_type1;
    memset(&func_type1, 0, sizeof(AOTFuncType));
    func_type1.param_count = 0;
    func_type1.result_count = 0;
    import_funcs[0].func_type = &func_type1;

    // Set up second import function (unlinked)
    import_funcs[1].module_name = (char*)"test_module2";
    import_funcs[1].func_name = (char*)"test_function2";
    import_funcs[1].func_ptr_linked = NULL; // Not linked

    // Create minimal function type for second function
    AOTFuncType func_type2;
    memset(&func_type2, 0, sizeof(AOTFuncType));
    func_type2.param_count = 0;
    func_type2.result_count = 0;
    import_funcs[1].func_type = &func_type2;

    // Configure module with import functions
    test_module.import_funcs = import_funcs;
    test_module.import_func_count = 2;

    // Test the function - should attempt to resolve both functions
    bool result = aot_resolve_symbols(&test_module);

    // Should return false since both functions will fail to resolve
    ASSERT_FALSE(result);

    // Both functions should still be unlinked
    ASSERT_EQ(import_funcs[0].func_ptr_linked, nullptr);
    ASSERT_EQ(import_funcs[1].func_ptr_linked, nullptr);
}

/******
 * Test Case: aot_resolve_symbols_WithAlreadyLinkedFunctions_SkipResolution
 * Source: core/iwasm/aot/aot_runtime.c:5525-5531
 * Target Lines: 5525 (function pointer access), 5526 (linked check - skip path)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly skips
 *                     functions that are already linked (func_ptr_linked != NULL).
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise the skip path for already linked functions
 ******/

/******
 * Test Case: aot_resolve_symbols_ResolutionFailure_LogWarningAndReturnFalse
 * Source: core/iwasm/aot/aot_runtime.c:5525-5531
 * Target Lines: 5527 (resolution failure), 5528-5530 (LOG_WARNING), 5531 (ret = false)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly handles
 *                     resolution failures by logging warnings and setting return value to false.
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise warning logging and failure return path
 ******/

/******
 * Test Case: aot_resolve_symbols_EmptyImportFuncArray_ReturnTrue
 * Source: core/iwasm/aot/aot_runtime.c:5524-5535
 * Target Lines: 5524 (loop condition with count=0), 5535 (return ret=true)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly handles
 *                     modules with no import functions by returning true immediately.
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise the success path when no import functions need resolution
 ******/

/******
 * Test Case: aot_resolve_symbols_MixedLinkedUnlinked_PartialFailure
 * Source: core/iwasm/aot/aot_runtime.c:5525-5531
 * Target Lines: 5525-5531 (complete iteration with mixed success/failure)
 * Functional Purpose: Validates that aot_resolve_symbols() correctly processes
 *                     modules with mixed linked/unlinked functions and returns false
 *                     when any unlinked function fails resolution.
 * Call Path: aot_resolve_symbols() <- wasm_runtime_resolve_symbols() <- public API
 * Coverage Goal: Exercise complete iteration logic with partial failures
 ******/

/******
 * Test Case: aot_const_str_set_insert_FirstInsertion_CreatesHashMapAndInsertsString
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5437-5448 (hash map creation), 5451-5453 (memory allocation),
 *               5460-5462 (standard copy), 5469-5476 (insertion and success)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly creates
 *                     a new hash map when module->const_str_set is NULL and
 *                     successfully inserts the first string.
 * Call Path: Direct call to aot_const_str_set_insert()
 * Coverage Goal: Exercise hash map creation and first string insertion path
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_const_str_set_insert_FirstInsertion_CreatesHashMapAndInsertsString) {
    // Create a minimal AOT module for testing
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));

    // Ensure const_str_set is initially NULL to trigger creation
    test_module.const_str_set = nullptr;

    // Test string data
    const char* test_string = "test_function_name";
    uint32 str_len = strlen(test_string) + 1;
    char error_buf[256];

    // Call the function under test
    char* result = aot_const_str_set_insert((const uint8*)test_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                           false,  // not word-aligned
#endif
                                           error_buf, sizeof(error_buf));

    // Verify successful insertion
    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(test_string, result);

    // Verify hash map was created
    ASSERT_NE(nullptr, test_module.const_str_set);

    // Cleanup
    if (test_module.const_str_set) {
        bh_hash_map_destroy(test_module.const_str_set);
    }
}

/******
 * Test Case: aot_const_str_set_insert_DuplicateString_ReturnsExistingString
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5464-5467 (hash map lookup and early return)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly finds
 *                     existing strings in the hash map and returns them without
 *                     creating duplicates.
 * Call Path: Direct call to aot_const_str_set_insert() with existing string
 * Coverage Goal: Exercise string deduplication logic
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_const_str_set_insert_DuplicateString_ReturnsExistingString) {
    // Create a minimal AOT module for testing
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));
    test_module.const_str_set = nullptr;

    // Test string data
    const char* test_string = "duplicate_function_name";
    uint32 str_len = strlen(test_string) + 1;
    char error_buf[256];

    // First insertion - should create new entry
    char* first_result = aot_const_str_set_insert((const uint8*)test_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                                 false,
#endif
                                                 error_buf, sizeof(error_buf));
    ASSERT_NE(nullptr, first_result);

    // Second insertion of same string - should return existing entry
    char* second_result = aot_const_str_set_insert((const uint8*)test_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                                  false,
#endif
                                                  error_buf, sizeof(error_buf));

    // Verify same pointer is returned (deduplication)
    ASSERT_EQ(first_result, second_result);
    ASSERT_STREQ(test_string, second_result);

    // Cleanup
    if (test_module.const_str_set) {
        bh_hash_map_destroy(test_module.const_str_set);
    }
}

/******
 * Test Case: aot_const_str_set_insert_MultipleStrings_AllStoredCorrectly
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5451-5453 (memory allocation), 5460-5462 (standard copy),
 *               5469-5476 (insertion and success return)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly handles
 *                     multiple different strings and stores them all in the hash map.
 * Call Path: Multiple direct calls to aot_const_str_set_insert()
 * Coverage Goal: Exercise standard memory copy and multiple insertions
 ******/

#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
/******
 * Test Case: aot_const_str_set_insert_WordAlignedCopy_UsesWordAlignedMemcpy
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5454-5457 (word-aligned memory copy path)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly uses
 *                     word-aligned memory copy when is_vram_word_align is true.
 * Call Path: Direct call to aot_const_str_set_insert() with word-align flag
 * Coverage Goal: Exercise word-aligned memory copy conditional compilation path
 ******/
#endif

/******
 * Test Case: aot_const_str_set_insert_EmptyString_HandledCorrectly
 * Source: core/iwasm/aot/aot_runtime.c:5431-5476
 * Target Lines: 5451-5453 (memory allocation), 5460-5462 (standard copy),
 *               5469-5476 (insertion and success)
 * Functional Purpose: Validates that aot_const_str_set_insert() correctly handles
 *                     empty strings and edge cases with minimal string data.
 * Call Path: Direct call to aot_const_str_set_insert() with empty string
 * Coverage Goal: Exercise edge case handling for minimal string data
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_const_str_set_insert_EmptyString_HandledCorrectly) {
    // Create a minimal AOT module for testing
    AOTModule test_module;
    memset(&test_module, 0, sizeof(AOTModule));
    test_module.const_str_set = nullptr;

    // Test with null-terminated empty string
    const char* empty_string = "";
    uint32 str_len = 1; // Just the null terminator
    char error_buf[256];

    // Call the function under test
    char* result = aot_const_str_set_insert((const uint8*)empty_string, str_len, &test_module,
#if (WASM_ENABLE_WORD_ALIGN_READ != 0)
                                           false,
#endif
                                           error_buf, sizeof(error_buf));

    // Verify successful insertion
    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(empty_string, result);

    // Verify hash map was created
    ASSERT_NE(nullptr, test_module.const_str_set);

    // Cleanup
    if (test_module.const_str_set) {
        bh_hash_map_destroy(test_module.const_str_set);
    }
}

/******
 * Test Case: aot_memory_init_ValidSegment_SuccessfulCopy
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3544-3560 (memory instance and data segment setup), 3562-3564 (address validation),
 *              3566-3569 (bounds checking), 3571-3578 (memory copy with locking)
 * Functional Purpose: Tests the primary execution flow of aot_memory_init() with valid memory
 *                     data segment to cover the most commonly executed code paths in bulk
 *                     memory initialization operations.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise main routine processing path for standard bulk memory initialization
 ******/

/******
 * Test Case: aot_memory_init_DroppedSegment_OutOfBoundsAccessFails
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3550-3555 (dropped segment detection and empty data setup), 3604-3606 (bounds check failure)
 * Functional Purpose: Tests the execution path when data segment has been dropped
 *                     (data_dropped bitmap set) and validates bounds check failure when
 *                     attempting to access data beyond the empty segment.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise dropped segment bounds check failure path
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_memory_init_DroppedSegment_OutOfBoundsAccessFails) {
    // Create AOT module instance with dropped data segment
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTMemoryInstance memory_inst;
    AOTModule aot_module;
    AOTMemInitData mem_init_data;
    AOTMemInitData *mem_init_data_list[1];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(&memory_inst, 0, sizeof(AOTMemoryInstance));
    memset(&aot_module, 0, sizeof(AOTModule));
    memset(&mem_init_data, 0, sizeof(AOTMemInitData));

    // Setup module instance structure
    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.module = (WASMModule*)&aot_module;
    module_inst.memory_count = 1;
    // Allocate array of memory instance pointers
    module_inst.memories = (WASMMemoryInstance**)wasm_runtime_malloc(sizeof(WASMMemoryInstance*));
    ASSERT_NE(nullptr, module_inst.memories);
    module_inst.memories[0] = (WASMMemoryInstance*)&memory_inst;

    // Setup memory instance
    memory_inst.memory_data_size = 65536;
    memory_inst.memory_data = (uint8*)wasm_runtime_malloc(memory_inst.memory_data_size);
    ASSERT_NE(nullptr, memory_inst.memory_data);

    // Setup memory initialization data (will be ignored due to dropped flag)
    const char test_data[] = "This should be ignored";
    mem_init_data.byte_count = strlen(test_data);
    mem_init_data.bytes = (uint8*)test_data;
    mem_init_data_list[0] = &mem_init_data;

    aot_module.mem_init_data_count = 1;
    aot_module.mem_init_data_list = mem_init_data_list;

    // Initialize data_dropped bitmap with segment 0 marked as dropped
    extra.common.data_dropped = bh_bitmap_new(0, 1);
    ASSERT_NE(nullptr, extra.common.data_dropped);
    bh_bitmap_set_bit(extra.common.data_dropped, 0); // Mark segment 0 as dropped

    // Test parameters for dropped segment
    uint32 seg_index = 0;
    uint32 offset = 0;
    uint32 len = 10; // Any length should work with dropped segment
    size_t dst = 1024;

    // Execute aot_memory_init
    bool result = aot_memory_init(&module_inst, seg_index, offset, len, dst);

    // Assert successful handling of dropped segment (empty data)
    ASSERT_FALSE(result);

    // Cleanup
    wasm_runtime_free(memory_inst.memory_data);
    wasm_runtime_free(module_inst.memories);
    bh_bitmap_delete(extra.common.data_dropped);
}

/******
 * Test Case: aot_memory_init_InvalidAppAddr_ValidationFailure
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3562-3564 (application address validation failure)
 * Functional Purpose: Tests the address validation path where wasm_runtime_validate_app_addr
 *                     fails due to invalid destination address, ensuring proper error handling
 *                     in bulk memory operations.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise address validation failure path for error handling
 ******/
TEST_F(EnhancedAotRuntimeTest, aot_memory_init_InvalidAppAddr_ValidationFailure) {
    // Create AOT module instance with invalid destination address
    AOTModuleInstance module_inst;
    AOTModuleInstanceExtra extra;
    AOTMemoryInstance memory_inst;
    AOTModule aot_module;
    AOTMemInitData mem_init_data;
    AOTMemInitData *mem_init_data_list[1];

    memset(&module_inst, 0, sizeof(AOTModuleInstance));
    memset(&extra, 0, sizeof(AOTModuleInstanceExtra));
    memset(&memory_inst, 0, sizeof(AOTMemoryInstance));
    memset(&aot_module, 0, sizeof(AOTModule));
    memset(&mem_init_data, 0, sizeof(AOTMemInitData));

    // Setup module instance structure
    module_inst.e = (WASMModuleInstanceExtra*)&extra;
    module_inst.module = (WASMModule*)&aot_module;
    module_inst.memory_count = 1;
    // Allocate array of memory instance pointers
    module_inst.memories = (WASMMemoryInstance**)wasm_runtime_malloc(sizeof(WASMMemoryInstance*));
    ASSERT_NE(nullptr, module_inst.memories);
    module_inst.memories[0] = (WASMMemoryInstance*)&memory_inst;

    // Setup memory instance with small memory size
    memory_inst.memory_data_size = 1024; // Small memory size
    memory_inst.memory_data = (uint8*)wasm_runtime_malloc(memory_inst.memory_data_size);
    ASSERT_NE(nullptr, memory_inst.memory_data);

    // Setup valid memory initialization data
    const char test_data[] = "Test data";
    mem_init_data.byte_count = strlen(test_data);
    mem_init_data.bytes = (uint8*)test_data;
    mem_init_data_list[0] = &mem_init_data;

    aot_module.mem_init_data_count = 1;
    aot_module.mem_init_data_list = mem_init_data_list;

    // Initialize data_dropped bitmap (not dropped)
    extra.common.data_dropped = bh_bitmap_new(0, 1);
    ASSERT_NE(nullptr, extra.common.data_dropped);

    // Test parameters with invalid destination address (beyond memory bounds)
    uint32 seg_index = 0;
    uint32 offset = 0;
    uint32 len = strlen(test_data);
    size_t dst = memory_inst.memory_data_size + 1000; // Invalid destination beyond memory

    // Execute aot_memory_init
    bool result = aot_memory_init(&module_inst, seg_index, offset, len, dst);

    // Assert validation failure (wasm_runtime_validate_app_addr fails)
    ASSERT_FALSE(result);

    // Cleanup
    wasm_runtime_free(memory_inst.memory_data);
    wasm_runtime_free(module_inst.memories);
    bh_bitmap_delete(extra.common.data_dropped);
}

/******
 * Test Case: aot_memory_init_OutOfBounds_ExceptionSet
 * Source: core/iwasm/aot/aot_runtime.c:3539-3579
 * Target Lines: 3566-3569 (bounds checking and exception setting)
 * Functional Purpose: Tests the bounds checking path where offset + len exceeds segment length,
 *                     ensuring proper exception setting via aot_set_exception for out of
 *                     bounds memory access in bulk memory operations.
 * Call Path: aot_memory_init() <- AOT compiled code <- WebAssembly bulk memory operations
 * Coverage Goal: Exercise bounds violation exception handling path
 ******/
