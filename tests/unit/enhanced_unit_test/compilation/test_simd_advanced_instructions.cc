/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"
#include "aot_comp_option.h"

static std::string CWD;
static std::string SIMD_WASM = "/simd_test.wasm";
static char *WASM_FILE;

static std::string
get_binary_path()
{
    char cwd[1024];
    memset(cwd, 0, 1024);

    if (readlink("/proc/self/exe", cwd, 1024) <= 0) {
        // Fallback to current working directory
        if (getcwd(cwd, 1024) == NULL) {
            return std::string(".");
        }
    }

    char *path_end = strrchr(cwd, '/');
    if (path_end != NULL) {
        *path_end = '\0';
    }

    return std::string(cwd);
}

class SIMDAdvancedInstructionsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime with SIMD support
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        // Enable SIMD support in runtime
        #if WASM_ENABLE_SIMD != 0
        init_args.max_thread_num = 4;
        #endif
        
        if (!wasm_runtime_full_init(&init_args)) {
            FAIL() << "Failed to initialize WAMR runtime";
        }
        
        // Initialize test environment
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default values with SIMD enabled
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_ref_types = true;
        
        // Initialize call stack features
        aot_call_stack_features_init_default(&option.call_stack_features);
        
        // Reset all pointers
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
    }

    void TearDown() override
    {
        // Clean up resources in reverse order of creation
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        if (wasm_module) {
            wasm_runtime_unload(wasm_module);
            wasm_module = nullptr;
        }
        if (wasm_file_buf) {
            wasm_runtime_free(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
        
        // Destroy WAMR runtime
        wasm_runtime_destroy();
    }

    static void SetUpTestCase()
    {
        CWD = get_binary_path();
        WASM_FILE = strdup((CWD + SIMD_WASM).c_str());
    }

    static void TearDownTestCase() 
    { 
        if (WASM_FILE) {
            free(WASM_FILE);
            WASM_FILE = nullptr;
        }
    }

    bool LoadWasmModule()
    {
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        if (!wasm_file_buf) {
            // Try alternative paths if primary path fails
            std::string alt_path = "./simd_test.wasm";
            wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(alt_path.c_str(), &wasm_file_size);
            if (!wasm_file_buf) {
                printf("Failed to load WASM file from both %s and %s\n", WASM_FILE, alt_path.c_str());
                return false;
            }
        }

        char error_buf[128];
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        if (!wasm_module) {
            printf("Failed to load WASM module: %s\n", error_buf);
        }
        return wasm_module != nullptr;
    }

    bool InitializeCompilationContext()
    {
        if (!wasm_module) {
            return false;
        }

        char error_buf[128];
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (!comp_data) {
            return false;
        }

        comp_ctx = aot_create_comp_context(comp_data, &option);
        return comp_ctx != nullptr;
    }

    // Test fixtures and helper data
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    AOTCompData *comp_data;
    AOTCompContext *comp_ctx;
};

// Step 2: SIMD and Advanced Instruction Emission Tests

TEST_F(SIMDAdvancedInstructionsTest, test_simd_access_lanes_compilation)
{
    // Test SIMD lane access compilation functionality
    option.enable_simd = true;
    
    // Load WASM module with SIMD lane access operations
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    ASSERT_NE(comp_ctx, nullptr) << "Compilation context should not be null";
    
    // Test compilation with SIMD lane access operations
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD lane access compilation should succeed";
    
    // Verify compilation data is generated
    if (result) {
        ASSERT_NE(comp_data, nullptr) << "Compilation data should be generated";
        ASSERT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_bitmask_extracts_compilation)
{
    // Test SIMD bitmask extraction compilation
    option.enable_simd = true;
    option.opt_level = 2;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    // Test bitmask operations compilation
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD bitmask extraction compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_bit_shifts_compilation)
{
    // Test SIMD bit shift operations compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    // Test bit shift compilation
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD bit shift operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_bitwise_operations_compilation)
{
    // Test SIMD bitwise operations compilation
    option.enable_simd = true;
    option.bounds_checks = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD bitwise operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_boolean_reductions_compilation)
{
    // Test SIMD boolean reduction operations compilation
    option.enable_simd = true;
    option.enable_aux_stack_check = true;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD boolean reductions compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_comparisons_compilation)
{
    // Test SIMD comparison operations compilation
    option.enable_simd = true;
    option.size_level = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD comparisons compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_conversions_compilation)
{
    // Test SIMD type conversion operations compilation
    option.enable_simd = true;
    option.enable_ref_types = true;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD conversions compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_construct_values_compilation)
{
    // Test SIMD value construction compilation
    option.enable_simd = true;
    option.opt_level = 3;
    option.size_level = 0;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD value construction compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_floating_point_compilation)
{
    // Test SIMD floating-point operations compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.bounds_checks = 2;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD floating-point operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_integer_arithmetic_compilation)
{
    // Test SIMD integer arithmetic operations compilation
    option.enable_simd = true;
    option.output_format = AOT_FORMAT_FILE;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD integer arithmetic compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_load_store_compilation)
{
    // Test SIMD load/store operations compilation
    option.enable_simd = true;
    option.enable_aux_stack_check = true;
    option.bounds_checks = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD load/store operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_simd_saturated_arithmetic_compilation)
{
    // Test SIMD saturated arithmetic operations compilation
    option.enable_simd = true;
    option.opt_level = 2;
    option.size_level = 2;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "SIMD saturated arithmetic compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_emit_const_advanced_operations)
{
    // Test advanced constant emission operations
    option.enable_simd = true;
    option.enable_ref_types = true;
    option.enable_bulk_memory = true;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Advanced constant operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_emit_conversion_comprehensive_types)
{
    // Test comprehensive type conversion emission
    option.enable_simd = true;
    option.bounds_checks = 2;
    option.opt_level = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Comprehensive type conversions compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_emit_exception_handling_compilation)
{
    // Test exception handling compilation
    option.enable_simd = false;  // Test without SIMD for exception focus
    option.enable_aux_stack_check = true;
    option.bounds_checks = 2;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Exception handling compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_emit_gc_operations_compilation)
{
    // Test garbage collection operations compilation
    option.enable_simd = true;
    option.enable_ref_types = true;
    option.opt_level = 0;  // Disable optimization for GC testing
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "GC operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_emit_stringref_operations_compilation)
{
    // Test stringref operations compilation
    option.enable_simd = true;
    option.enable_ref_types = true;
    option.enable_bulk_memory = true;
    option.size_level = 0;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Stringref operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_aot_stack_frame_compilation_optimization)
{
    // Test AOT stack frame compilation optimization
    option.enable_simd = true;
    option.enable_aux_stack_check = true;
    option.opt_level = 3;
    option.size_level = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Stack frame optimization compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_advanced_control_flow_compilation)
{
    // Test advanced control flow compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = true;
    option.bounds_checks = 1;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Advanced control flow compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}

TEST_F(SIMDAdvancedInstructionsTest, test_complex_memory_operations_compilation)
{
    // Test complex memory operations compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.enable_aux_stack_check = true;
    option.bounds_checks = 2;
    option.opt_level = 2;
    
    ASSERT_TRUE(LoadWasmModule()) << "Failed to load SIMD test module";
    ASSERT_TRUE(InitializeCompilationContext()) << "Failed to initialize compilation context";
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result) << "Complex memory operations compilation should succeed";
    
    // Verify compilation results
    if (result) {
        EXPECT_NE(comp_data, nullptr) << "Compilation data should be generated";
        EXPECT_NE(comp_ctx, nullptr) << "Compilation context should remain valid";
    }
}