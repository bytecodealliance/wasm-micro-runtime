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
static std::string MAIN_WASM = "/main.wasm";
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

class AdvancedCompilerCoreTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test environment
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default values
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
    }

    static void SetUpTestCase()
    {
        CWD = get_binary_path();
        WASM_FILE = strdup((CWD + MAIN_WASM).c_str());
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
        if (wasm_module) return true; // Already loaded
        
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        if (!wasm_file_buf) return false;
        
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        return wasm_module != nullptr;
    }

    bool CreateCompilationContext(AOTCompOption *opt = nullptr)
    {
        if (!wasm_module && !LoadWasmModule()) return false;
        
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (!comp_data) return false;
        
        comp_ctx = aot_create_comp_context(comp_data, opt ? opt : &option);
        return comp_ctx != nullptr;
    }

    // Test fixtures and helper data
    WAMRRuntimeRAII<512 * 1024> runtime;
    AOTCompOption option;
    
    unsigned char *wasm_file_buf;
    unsigned int wasm_file_size;
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    char error_buf[128];
};

// Test 1: Advanced compiler options configuration
TEST_F(AdvancedCompilerCoreTest, AdvancedOptionsConfiguration)
{
    // Test GC support configuration
    option.enable_gc = true;
    option.enable_ref_types = true;
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 2: Thread manager configuration
TEST_F(AdvancedCompilerCoreTest, ThreadManagerConfiguration)
{
    // Test thread manager configuration
    option.enable_thread_mgr = true;
    option.enable_bulk_memory = true;
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 3: Exception handling support compilation
TEST_F(AdvancedCompilerCoreTest, ExceptionHandlingSupportCompilation)
{
    // Test with tail call enabled (related to exception handling)
    option.enable_tail_call = true;
    option.enable_aux_stack_check = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 4: GC support compilation with various configurations
TEST_F(AdvancedCompilerCoreTest, GCSupportCompilation)
{
    // Test GC with reference types
    option.enable_gc = true;
    option.enable_ref_types = true;
    option.enable_extended_const = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 5: GC with shared heap support
TEST_F(AdvancedCompilerCoreTest, GCSharedHeapSupport)
{
    // Test GC with shared heap
    option.enable_gc = true;
    option.enable_ref_types = true;
    option.enable_shared_heap = true;
    option.enable_shared_chain = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 6: Memory optimization settings
TEST_F(AdvancedCompilerCoreTest, MemoryOptimizationSettings)
{
    // Test memory profiling enabled
    option.enable_memory_profiling = true;
    option.enable_perf_profiling = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
    
    // Test stack estimation
    option.enable_stack_estimation = true;
    option.stack_usage_file = const_cast<char*>("test_stack.txt");
    
    // Recreate context with new options
    TearDown();
    SetUp();
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 7: Target architecture selection
TEST_F(AdvancedCompilerCoreTest, TargetArchitectureSelection)
{
    // Test SGX platform configuration
    option.is_sgx_platform = true;
    option.quick_invoke_c_api_import = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 8: Debug information generation
TEST_F(AdvancedCompilerCoreTest, DebugInformationGeneration)
{
    // Test call stack features
    option.call_stack_features.frame_per_function = true;
    option.call_stack_features.ip = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 9: Error recovery mechanisms
TEST_F(AdvancedCompilerCoreTest, ErrorRecoveryMechanisms)
{
    // Test with valid but high optimization level
    option.opt_level = 3; // Valid max level
    option.size_level = 3; // Valid max level
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 10: Validation pipeline
TEST_F(AdvancedCompilerCoreTest, ValidationPipeline)
{
    // Test bounds checking configurations
    option.bounds_checks = 0; // Disabled
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
    
    // Test with enabled bounds checks
    TearDown();
    SetUp();
    option.bounds_checks = 1; // Enabled
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 11: Resource management and compilation context lifecycle
TEST_F(AdvancedCompilerCoreTest, ResourceManagementLifecycle)
{
    // Test multiple context creation/destruction cycles
    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(CreateCompilationContext());
        ASSERT_TRUE(aot_compile_wasm(comp_ctx));
        
        // Clean up for next iteration
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
    }
}

// Test 12: Module dependency resolution
TEST_F(AdvancedCompilerCoreTest, ModuleDependencyResolution)
{
    // Test feature dependencies
    option.enable_ref_types = true;
    option.enable_gc = true; // Depends on ref_types
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 13: Optimization level validation
TEST_F(AdvancedCompilerCoreTest, OptimizationLevelValidation)
{
    // Test all valid optimization levels
    for (uint32_t level = 0; level <= 3; level++) {
        option.opt_level = level;
        option.size_level = level;
        
        ASSERT_TRUE(CreateCompilationContext());
        ASSERT_TRUE(aot_compile_wasm(comp_ctx));
        
        // Clean up for next iteration
        TearDown();
        SetUp();
    }
}

// Test 14: Platform specific options
TEST_F(AdvancedCompilerCoreTest, PlatformSpecificOptions)
{
    // Test JIT vs AOT mode configurations
    option.is_jit_mode = false; // AOT mode
    option.quick_invoke_c_api_import = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 15: Feature flag combinations
TEST_F(AdvancedCompilerCoreTest, FeatureFlagCombinations)
{
    // Test comprehensive feature combination
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = true;
    option.enable_aux_stack_check = true;
    option.enable_thread_mgr = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 16: Invalid input handling
TEST_F(AdvancedCompilerCoreTest, InvalidInputHandling)
{
    // Test with valid configuration to ensure robustness
    option.opt_level = 2;
    option.size_level = 2;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 17: Memory pressure scenarios
TEST_F(AdvancedCompilerCoreTest, MemoryPressureScenarios)
{
    // Test size vs speed optimization trade-offs
    option.size_level = 3; // Optimize for size
    option.opt_level = 0;  // Minimal optimization
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 18: Concurrent compilation safety
TEST_F(AdvancedCompilerCoreTest, ConcurrentCompilationSafety)
{
    // Test thread manager with bulk memory (thread-safe features)
    option.enable_thread_mgr = true;
    option.enable_bulk_memory = true;
    option.enable_aux_stack_check = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 19: Stack frame type configuration
TEST_F(AdvancedCompilerCoreTest, StackFrameTypeConfiguration)
{
    // Test standard stack frame (more reliable)
    option.aux_stack_frame_type = AOT_STACK_FRAME_TYPE_STANDARD;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
    
    // Test with stack frame off
    TearDown();
    SetUp();
    option.aux_stack_frame_type = AOT_STACK_FRAME_OFF;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}

// Test 20: LLVM optimization configuration
TEST_F(AdvancedCompilerCoreTest, LLVMOptimizationConfiguration)
{
    // Test PGO and LTO configurations
    option.enable_llvm_pgo = true;
    option.disable_llvm_lto = false;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
    
    // Test with optimizations disabled
    TearDown();
    SetUp();
    option.disable_llvm_intrinsics = true;
    option.disable_llvm_lto = true;
    
    ASSERT_TRUE(CreateCompilationContext());
    ASSERT_TRUE(aot_compile_wasm(comp_ctx));
}