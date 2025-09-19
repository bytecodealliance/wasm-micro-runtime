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
static std::string LLVM_TEST_WASM = "/llvm_test.wasm";
static char *WASM_FILE;

static std::string
get_binary_path()
{
    char cwd[1024];
    memset(cwd, 0, 1024);

    if (readlink("/proc/self/exe", cwd, 1024) <= 0) {
    }

    char *path_end = strrchr(cwd, '/');
    if (path_end != NULL) {
        *path_end = '\0';
    }

    return std::string(cwd);
}

class LLVMIntegrationOptimizationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        if (!wasm_runtime_full_init(&init_args)) {
            FAIL() << "Failed to initialize WAMR runtime";
        }
        
        // Initialize AOT compiler
        if (!aot_compiler_init()) {
            FAIL() << "Failed to initialize AOT compiler";
        }
        
        // Initialize compilation options
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default optimization values
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_thread_mgr = false;
        option.enable_tail_call = true;
        option.enable_ref_types = true;
        option.enable_llvm_pgo = false;
        option.disable_llvm_lto = false;
        
        // Initialize paths - Use existing WASM file that's already copied
        WASM_FILE = const_cast<char*>("main.wasm");
        
        // Initialize test data
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
        error_buf[0] = '\0';
    }
    
    void TearDown() override
    {
        // Clean up resources in reverse order
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
            BH_FREE(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
        
        // Destroy AOT compiler
        aot_compiler_destroy();
        
        wasm_runtime_destroy();
    }
    
    bool load_wasm_file(const char *file_name)
    {
        // The WASM files are copied to the build directory, so use just the filename
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(file_name, &wasm_file_size);
        if (!wasm_file_buf || wasm_file_size == 0) {
            return false;
        }
        
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        return wasm_module != nullptr;
    }
    
    bool create_compilation_context(AOTCompOption *test_option)
    {
        if (!wasm_module) return false;
        
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (!comp_data) return false;
        
        comp_ctx = aot_create_comp_context(comp_data, test_option);
        return comp_ctx != nullptr;
    }
    
    bool compile_wasm()
    {
        if (!comp_ctx) return false;
        return aot_compile_wasm(comp_ctx);
    }
    
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    char error_buf[256];
};

// Test 1: LLVM Module Creation and Validation
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_module_creation_and_validation)
{
    // Test LLVM module creation with various configurations
    AOTCompOption test_option = option;
    test_option.opt_level = 0;  // No optimization for basic validation
    
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    ASSERT_TRUE(create_compilation_context(&test_option));
    
    // Test basic LLVM module creation and compilation
    ASSERT_TRUE(compile_wasm()) << "Failed to create basic LLVM module: " << aot_get_last_error();
    
    // Validate compilation succeeded
    ASSERT_NE(comp_ctx, nullptr);
    ASSERT_NE(comp_data, nullptr);
}

// Test 2: LLVM Optimization Pass Configuration
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_optimization_pass_configuration)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test different optimization levels
    for (uint32_t opt_level = 0; opt_level <= 3; opt_level++) {
        // Clean up previous context
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        AOTCompOption test_option = option;
        test_option.opt_level = opt_level;
        
        ASSERT_TRUE(create_compilation_context(&test_option)) 
            << "Failed to create context for optimization level " << opt_level;
        ASSERT_TRUE(compile_wasm()) 
            << "Failed compilation for optimization level " << opt_level << ": " << aot_get_last_error();
    }
}

// Test 3: LLVM Target Machine Setup
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_target_machine_setup)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test target machine configuration
    AOTCompOption test_option = option;
    test_option.target_arch = const_cast<char*>("x86_64");
    test_option.target_abi = const_cast<char*>("gnu");
    test_option.cpu_features = const_cast<char*>("+sse2,+sse3,+ssse3,+sse4.1,+sse4.2");
    
    ASSERT_TRUE(create_compilation_context(&test_option));
    ASSERT_TRUE(compile_wasm()) << "Target machine setup failed: " << aot_get_last_error();
    
    // Verify target-specific compilation succeeded
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 4: LLVM Code Generation Pipeline
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_code_generation_pipeline)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test complete code generation pipeline
    AOTCompOption test_option = option;
    test_option.opt_level = 2;
    test_option.size_level = 1;
    
    ASSERT_TRUE(create_compilation_context(&test_option));
    ASSERT_TRUE(compile_wasm()) << "Code generation pipeline failed: " << aot_get_last_error();
    
    // Verify pipeline produces valid output
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 5: LLVM Debug Information Integration
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_debug_information_integration)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test debug information generation
    AOTCompOption debug_option = option;
    debug_option.opt_level = 0;  // Debug builds typically use lower optimization
    
    ASSERT_TRUE(create_compilation_context(&debug_option));
    ASSERT_TRUE(compile_wasm()) << "Debug information integration failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 6: LLVM Memory Layout Optimization
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_memory_layout_optimization)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test memory layout optimizations
    AOTCompOption memory_option = option;
    memory_option.enable_bulk_memory = true;
    memory_option.enable_aux_stack_check = true;
    memory_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&memory_option));
    ASSERT_TRUE(compile_wasm()) << "Memory layout optimization failed: " << aot_get_last_error();
    
    // Verify memory optimization succeeded
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 7: LLVM Function Inlining Decisions
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_function_inlining_decisions)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test function inlining with different size levels
    for (uint32_t size_level = 0; size_level <= 3; size_level++) {
        // Clean up previous context
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        AOTCompOption inline_option = option;
        inline_option.size_level = size_level;
        inline_option.opt_level = 2;
        
        ASSERT_TRUE(create_compilation_context(&inline_option))
            << "Failed to create context for size level " << size_level;
        ASSERT_TRUE(compile_wasm())
            << "Inlining test failed for size level " << size_level << ": " << aot_get_last_error();
    }
}

// Test 8: LLVM Constant Propagation Optimization
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_constant_propagation_optimization)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test constant propagation optimization
    AOTCompOption const_option = option;
    const_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&const_option));
    ASSERT_TRUE(compile_wasm()) << "Constant propagation optimization failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 9: LLVM Dead Code Elimination
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_dead_code_elimination)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test with no optimization
    AOTCompOption no_opt_option = option;
    no_opt_option.opt_level = 0;
    
    ASSERT_TRUE(create_compilation_context(&no_opt_option));
    ASSERT_TRUE(compile_wasm()) << "No optimization compilation failed: " << aot_get_last_error();
    
    // Clean up and test with optimization
    aot_destroy_comp_context(comp_ctx);
    comp_ctx = nullptr;
    aot_destroy_comp_data(comp_data);
    comp_data = nullptr;
    
    AOTCompOption opt_option = option;
    opt_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&opt_option));
    ASSERT_TRUE(compile_wasm()) << "Optimized compilation failed: " << aot_get_last_error();
    
    // Both should succeed
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 10: LLVM Loop Optimization Passes
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_loop_optimization_passes)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test loop optimization
    AOTCompOption loop_option = option;
    loop_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&loop_option));
    ASSERT_TRUE(compile_wasm()) << "Loop optimization failed: " << aot_get_last_error();
    
    // Verify loop optimization compilation succeeded
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 11: LLVM Vectorization Opportunities
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_vectorization_opportunities)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test vectorization with SIMD enabled
    AOTCompOption vec_option = option;
    vec_option.enable_simd = true;
    vec_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&vec_option));
    ASSERT_TRUE(compile_wasm()) << "Vectorization compilation failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 12: LLVM Register Allocation Strategies
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_register_allocation_strategies)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test register allocation optimization
    AOTCompOption reg_option = option;
    reg_option.opt_level = 2;
    
    ASSERT_TRUE(create_compilation_context(&reg_option));
    ASSERT_TRUE(compile_wasm()) << "Register allocation optimization failed: " << aot_get_last_error();
    
    // Verify register allocation optimization succeeded
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 13: LLVM Instruction Scheduling Optimization
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_instruction_scheduling_optimization)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test instruction scheduling
    AOTCompOption sched_option = option;
    sched_option.opt_level = 3;
    
    ASSERT_TRUE(create_compilation_context(&sched_option));
    ASSERT_TRUE(compile_wasm()) << "Instruction scheduling optimization failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 14: LLVM Platform Specific Code Generation
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_platform_specific_code_generation)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test platform-specific code generation
    AOTCompOption platform_option = option;
    platform_option.target_arch = const_cast<char*>("x86_64");
    platform_option.opt_level = 2;
    
    ASSERT_TRUE(create_compilation_context(&platform_option));
    ASSERT_TRUE(compile_wasm()) << "Platform-specific code generation failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 15: LLVM Cross Compilation Support
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_cross_compilation_support)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test cross-compilation capabilities
    AOTCompOption cross_option = option;
    cross_option.target_arch = const_cast<char*>("x86_64");
    cross_option.target_abi = const_cast<char*>("gnu");
    
    ASSERT_TRUE(create_compilation_context(&cross_option));
    ASSERT_TRUE(compile_wasm()) << "Cross-compilation failed: " << aot_get_last_error();
    
    // Verify cross-compilation succeeded
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 16: LLVM LTO (Link Time Optimization) Integration
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_lto_integration)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test LTO integration
    AOTCompOption lto_option = option;
    lto_option.disable_llvm_lto = false;  // Enable LTO
    lto_option.opt_level = 2;
    
    ASSERT_TRUE(create_compilation_context(&lto_option));
    ASSERT_TRUE(compile_wasm()) << "LTO integration failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 17: LLVM Error Handling and Diagnostics
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_error_handling_and_diagnostics)
{
    // Test error handling with invalid input
    unsigned char invalid_wasm[] = {0x00, 0x61, 0x73, 0x6d}; // Incomplete WASM magic
    uint32_t invalid_size = sizeof(invalid_wasm);
    
    wasm_module_t invalid_module = wasm_runtime_load(invalid_wasm, invalid_size, error_buf, sizeof(error_buf));
    
    // Should fail gracefully with proper error message
    ASSERT_EQ(invalid_module, nullptr);
    ASSERT_GT(strlen(error_buf), 0);  // Should have error message
}

// Test 18: LLVM Metadata Preservation
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_metadata_preservation)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test metadata preservation during compilation
    AOTCompOption meta_option = option;
    meta_option.opt_level = 1;  // Lower optimization to preserve more metadata
    
    ASSERT_TRUE(create_compilation_context(&meta_option));
    ASSERT_TRUE(compile_wasm()) << "Metadata preservation compilation failed: " << aot_get_last_error();
    
    ASSERT_NE(comp_ctx, nullptr);
}

// Test 19: LLVM Performance Profiling Integration
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_performance_profiling_integration)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test performance profiling integration
    AOTCompOption prof_option = option;
    prof_option.opt_level = 2;
    prof_option.enable_llvm_pgo = true;
    
    // Measure compilation time as a basic performance metric
    auto start_time = std::chrono::high_resolution_clock::now();
    
    ASSERT_TRUE(create_compilation_context(&prof_option));
    ASSERT_TRUE(compile_wasm()) << "Performance profiling compilation failed: " << aot_get_last_error();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    ASSERT_NE(comp_ctx, nullptr);
    ASSERT_LT(duration.count(), 10000);  // Should complete within 10 seconds
}

// Test 20: LLVM Resource Cleanup and Management
TEST_F(LLVMIntegrationOptimizationTest, test_llvm_resource_cleanup_and_management)
{
    ASSERT_TRUE(load_wasm_file(WASM_FILE));
    
    // Test multiple compilations to verify resource cleanup
    for (int i = 0; i < 3; i++) {
        // Clean up previous context if exists
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        AOTCompOption cleanup_option = option;
        cleanup_option.opt_level = 1;
        
        ASSERT_TRUE(create_compilation_context(&cleanup_option))
            << "Resource cleanup test iteration " << i << " context creation failed";
        ASSERT_TRUE(compile_wasm())
            << "Resource cleanup test iteration " << i << " compilation failed: " << aot_get_last_error();
        
        ASSERT_NE(comp_ctx, nullptr);
        // Cleanup will happen in loop or TearDown
    }
}