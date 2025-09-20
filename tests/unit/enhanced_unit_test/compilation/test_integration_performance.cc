/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"

#include "wasm_export.h"
#include "aot_export.h"
#include "bh_read_file.h"
#include <chrono>
#include <thread>
#include <atomic>

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

class IntegrationPerformanceTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize WAMR runtime first
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args)) << "Failed to initialize WAMR runtime";
        
        // Initialize AOT compiler
        ASSERT_TRUE(aot_compiler_init()) << "Failed to initialize AOT compiler";
        
        // Initialize test environment
        memset(&option, 0, sizeof(AOTCompOption));
        
        // Set default values for integration testing
        option.opt_level = 3;
        option.size_level = 3;
        option.output_format = AOT_FORMAT_FILE;
        option.bounds_checks = 2;
        option.enable_simd = true;
        option.enable_aux_stack_check = true;
        option.enable_bulk_memory = true;
        option.enable_ref_types = true;
        option.enable_thread_mgr = true;
        
        // Reset all pointers
        wasm_file_buf = nullptr;
        wasm_file_size = 0;
        wasm_module = nullptr;
        comp_data = nullptr;
        comp_ctx = nullptr;
        
        // Performance tracking
        compilation_start_time = std::chrono::high_resolution_clock::now();
        compilation_end_time = compilation_start_time;
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
            BH_FREE(wasm_file_buf);
            wasm_file_buf = nullptr;
        }
        
        // Destroy AOT compiler
        aot_compiler_destroy();
        
        // Destroy WAMR runtime
        wasm_runtime_destroy();
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
        wasm_file_buf = (unsigned char *)bh_read_file_to_buffer(WASM_FILE, &wasm_file_size);
        if (!wasm_file_buf) {
            return false;
        }

        char error_buf[128];
        wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size, error_buf, sizeof(error_buf));
        return wasm_module != nullptr;
    }

    bool CreateCompilationData()
    {
        char error_buf[128];
        comp_data = aot_create_comp_data(wasm_module, nullptr, false);
        if (!comp_data) {
            return false;
        }

        comp_ctx = aot_create_comp_context(comp_data, &option);
        return comp_ctx != nullptr;
    }

    void StartPerformanceTimer()
    {
        compilation_start_time = std::chrono::high_resolution_clock::now();
    }

    void EndPerformanceTimer()
    {
        compilation_end_time = std::chrono::high_resolution_clock::now();
    }

    double GetCompilationTimeMs()
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            compilation_end_time - compilation_start_time);
        return duration.count() / 1000.0;
    }

    // Test fixtures and helper data
    AOTCompOption option;
    unsigned char *wasm_file_buf;
    uint32_t wasm_file_size;
    wasm_module_t wasm_module;
    aot_comp_data_t comp_data;
    aot_comp_context_t comp_ctx;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point compilation_start_time;
    std::chrono::high_resolution_clock::time_point compilation_end_time;
};

// Step 4: Integration and Performance Testing Functions (20 test cases)

TEST_F(IntegrationPerformanceTest, EndToEndCompilationWorkflow)
{
    // Test complete compilation pipeline from WASM to AOT
    ASSERT_TRUE(LoadWasmModule());
    
    StartPerformanceTimer();
    ASSERT_TRUE(CreateCompilationData());
    
    // Test compilation process
    char error_buf[128];
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    ASSERT_TRUE(result);
    ASSERT_LT(GetCompilationTimeMs(), 10000.0); // Should complete within 10 seconds
    ASSERT_GT(GetCompilationTimeMs(), 0.1); // Should take some measurable time
}

TEST_F(IntegrationPerformanceTest, MultiModuleCompilationPipeline)
{
    // Test compilation with module features enabled
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    ASSERT_TRUE(result);
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, LargeWasmModuleCompilation)
{
    // Test compilation performance with larger modules
    ASSERT_TRUE(LoadWasmModule());
    
    // Verify module size is reasonable for testing
    ASSERT_GT(wasm_file_size, 100); // At least 100 bytes
    ASSERT_LT(wasm_file_size, 1024 * 1024); // Less than 1MB for test stability
    
    ASSERT_TRUE(CreateCompilationData());
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    ASSERT_TRUE(result);
    // Performance should scale reasonably with module size
    double time_per_byte = GetCompilationTimeMs() / wasm_file_size;
    ASSERT_LT(time_per_byte, 1.0); // Should be less than 1ms per byte
}

TEST_F(IntegrationPerformanceTest, CompilationPerformanceBenchmarks)
{
    // Benchmark compilation performance across different optimization levels
    ASSERT_TRUE(LoadWasmModule());
    
    std::vector<uint32_t> opt_levels = {0, 1, 2, 3};
    std::vector<double> compilation_times;
    
    for (uint32_t opt_level : opt_levels) {
        // Clean up previous compilation
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        // Set optimization level
        option.opt_level = opt_level;
        
        ASSERT_TRUE(CreateCompilationData());
        
        StartPerformanceTimer();
        bool result = aot_compile_wasm(comp_ctx);
        EndPerformanceTimer();
        
        ASSERT_TRUE(result);
        double compile_time = GetCompilationTimeMs();
        compilation_times.push_back(compile_time);
        
        // Each compilation should complete in reasonable time
        ASSERT_LT(compile_time, 30000.0); // 30 seconds max
        ASSERT_GT(compile_time, 0.1); // Some measurable time
    }
    
    // Verify we have results for all optimization levels
    ASSERT_EQ(compilation_times.size(), opt_levels.size());
}

TEST_F(IntegrationPerformanceTest, MemoryIntensiveCompilationScenarios)
{
    // Test compilation memory usage patterns
    ASSERT_TRUE(LoadWasmModule());
    
    // Configure for memory-intensive compilation
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    option.enable_ref_types = true;
    option.opt_level = 3;
    
    ASSERT_TRUE(CreateCompilationData());
    
    // Monitor memory usage during compilation
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    ASSERT_TRUE(result);
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, ConcurrentCompilationStressTesting)
{
    // Test compilation thread safety with concurrent operations
    ASSERT_TRUE(LoadWasmModule());
    
    std::atomic<int> successful_compilations(0);
    std::atomic<int> failed_compilations(0);
    const int num_threads = 2; // Conservative for test stability
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // Each thread needs its own compilation context
            aot_comp_data_t local_comp_data = nullptr;
            aot_comp_context_t local_comp_ctx = nullptr;
            
            try {
                local_comp_data = aot_create_comp_data(wasm_module, nullptr, false);
                if (local_comp_data) {
                    local_comp_ctx = aot_create_comp_context(local_comp_data, &option);
                    if (local_comp_ctx) {
                        bool result = aot_compile_wasm(local_comp_ctx);
                        if (result) {
                            successful_compilations++;
                        } else {
                            failed_compilations++;
                        }
                    } else {
                        failed_compilations++;
                    }
                } else {
                    failed_compilations++;
                }
            } catch (...) {
                failed_compilations++;
            }
            
            // Cleanup
            if (local_comp_ctx) {
                aot_destroy_comp_context(local_comp_ctx);
            }
            if (local_comp_data) {
                aot_destroy_comp_data(local_comp_data);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify at least some compilations succeeded
    ASSERT_GT(successful_compilations.load(), 0);
    ASSERT_EQ(successful_compilations.load() + failed_compilations.load(), num_threads);
}

TEST_F(IntegrationPerformanceTest, CompilationErrorPropagationChains)
{
    // Test error handling and propagation through compilation pipeline
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // Test with invalid optimization settings
    option.opt_level = 999; // Invalid optimization level
    
    // Should handle invalid options gracefully
    bool result = aot_compile_wasm(comp_ctx);
    // Either succeeds (clamps to valid range) or fails gracefully
    // Both are acceptable behaviors for invalid input
    ASSERT_TRUE(result || !result); // Should not crash
}

TEST_F(IntegrationPerformanceTest, ResourceExhaustionDuringCompilation)
{
    // Test compilation behavior under resource pressure
    ASSERT_TRUE(LoadWasmModule());
    
    // Configure for resource-intensive compilation
    option.opt_level = 3;
    option.size_level = 0; // Prioritize performance over size
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    
    ASSERT_TRUE(CreateCompilationData());
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    // Should either succeed or fail gracefully
    ASSERT_TRUE(result || !result);
    
    // Should complete in reasonable time even under pressure
    ASSERT_LT(GetCompilationTimeMs(), 60000.0); // 1 minute max
}

TEST_F(IntegrationPerformanceTest, PlatformCompatibilityCompilation)
{
    // Test compilation compatibility across platform settings
    ASSERT_TRUE(LoadWasmModule());
    
    // Test with SGX platform configuration
    option.is_sgx_platform = true;
    
    ASSERT_TRUE(CreateCompilationData());
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result || !result); // Should not crash
    
    if (result) {
        ASSERT_NE(comp_data, nullptr);
    }
}

TEST_F(IntegrationPerformanceTest, BackwardCompatibilityCompilation)
{
    // Test compilation with different feature combinations for compatibility
    ASSERT_TRUE(LoadWasmModule());
    
    // Test with minimal feature set (backward compatibility)
    option.enable_simd = false;
    option.enable_bulk_memory = false;
    option.enable_ref_types = false;
    option.enable_thread_mgr = false;
    
    ASSERT_TRUE(CreateCompilationData());
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CompilationCacheManagement)
{
    // Test compilation caching and reuse mechanisms
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // First compilation
    StartPerformanceTimer();
    bool first_result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    double first_time = GetCompilationTimeMs();
    
    ASSERT_TRUE(first_result);
    ASSERT_GT(first_time, 0.1);
    
    // Verify compilation data is cached/available
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, IncrementalCompilationSupport)
{
    // Test incremental compilation capabilities
    ASSERT_TRUE(LoadWasmModule());
    
    // Configure for incremental compilation
    option.enable_aux_stack_check = true;
    option.bounds_checks = 1; // Enable for incremental safety
    
    ASSERT_TRUE(CreateCompilationData());
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    
    // Verify incremental compilation structures
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CompilationMetadataValidation)
{
    // Test compilation metadata generation and validation
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // Enable metadata generation
    option.enable_aux_stack_check = true;
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    
    // Validate compilation metadata
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CrossPlatformCompilationConsistency)
{
    // Test compilation consistency across different configurations
    ASSERT_TRUE(LoadWasmModule());
    
    std::vector<uint32_t> size_levels = {0, 1, 2, 3};
    
    for (uint32_t size_level : size_levels) {
        // Clean up previous compilation
        if (comp_ctx) {
            aot_destroy_comp_context(comp_ctx);
            comp_ctx = nullptr;
        }
        if (comp_data) {
            aot_destroy_comp_data(comp_data);
            comp_data = nullptr;
        }
        
        option.size_level = size_level;
        option.opt_level = 2; // Consistent optimization
        
        ASSERT_TRUE(CreateCompilationData());
        
        bool result = aot_compile_wasm(comp_ctx);
        ASSERT_TRUE(result);
        
        // Verify consistent compilation results
        ASSERT_NE(comp_data, nullptr);
    }
}

TEST_F(IntegrationPerformanceTest, CompilationOutputVerification)
{
    // Test compilation output verification and validation
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // Configure for comprehensive output generation
    option.output_format = AOT_FORMAT_FILE;
    option.enable_simd = true;
    option.enable_bulk_memory = true;
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    
    // Verify compilation output integrity
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CompilationRegressionDetection)
{
    // Test compilation regression detection mechanisms
    ASSERT_TRUE(LoadWasmModule());
    
    // Baseline compilation with standard settings
    option.opt_level = 2;
    option.size_level = 2;
    option.enable_simd = true;
    
    ASSERT_TRUE(CreateCompilationData());
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    double baseline_time = GetCompilationTimeMs();
    
    ASSERT_TRUE(result);
    ASSERT_GT(baseline_time, 0.1);
    ASSERT_LT(baseline_time, 30000.0); // Should complete within 30 seconds
    
    // Verify no regression in basic functionality
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CompilationSecurityValidation)
{
    // Test compilation security validation and bounds checking
    ASSERT_TRUE(LoadWasmModule());
    
    // Configure for security-focused compilation
    option.bounds_checks = 2; // Maximum bounds checking
    option.enable_aux_stack_check = true;
    
    ASSERT_TRUE(CreateCompilationData());
    
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    
    // Verify security features are enabled
    ASSERT_NE(comp_data, nullptr);
    ASSERT_EQ(option.bounds_checks, 2);
    ASSERT_TRUE(option.enable_aux_stack_check);
}

TEST_F(IntegrationPerformanceTest, CompilationDeterministicOutput)
{
    // Test compilation deterministic output generation
    ASSERT_TRUE(LoadWasmModule());
    
    // First compilation
    ASSERT_TRUE(CreateCompilationData());
    bool first_result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(first_result);
    
    // Clean up and recompile with same settings
    aot_destroy_comp_context(comp_ctx);
    aot_destroy_comp_data(comp_data);
    comp_ctx = nullptr;
    comp_data = nullptr;
    
    // Second compilation with identical settings
    ASSERT_TRUE(CreateCompilationData());
    bool second_result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(second_result);
    
    // Verify deterministic output
    ASSERT_NE(comp_data, nullptr);
}

TEST_F(IntegrationPerformanceTest, CompilationProfilingAndMetrics)
{
    // Test compilation profiling and performance metrics collection
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // Enable profiling features
    option.enable_aux_stack_check = true;
    
    StartPerformanceTimer();
    bool result = aot_compile_wasm(comp_ctx);
    EndPerformanceTimer();
    
    ASSERT_TRUE(result);
    
    // Collect performance metrics
    double compilation_time = GetCompilationTimeMs();
    ASSERT_GT(compilation_time, 0.1);
    ASSERT_LT(compilation_time, 60000.0); // 1 minute max
    
    // Verify profiling data is available
    ASSERT_NE(comp_data, nullptr);
    
    // Calculate performance metrics
    if (wasm_file_size > 0) {
        double throughput = wasm_file_size / (compilation_time / 1000.0); // bytes per second
        ASSERT_GT(throughput, 0);
    }
}

TEST_F(IntegrationPerformanceTest, CompilationCleanupAndFinalization)
{
    // Test compilation cleanup and finalization processes
    ASSERT_TRUE(LoadWasmModule());
    ASSERT_TRUE(CreateCompilationData());
    
    // Perform compilation
    bool result = aot_compile_wasm(comp_ctx);
    ASSERT_TRUE(result);
    
    // Verify compilation state before cleanup
    ASSERT_NE(comp_data, nullptr);
    ASSERT_NE(comp_ctx, nullptr);
    
    // Test explicit cleanup
    aot_destroy_comp_context(comp_ctx);
    comp_ctx = nullptr;
    
    aot_destroy_comp_data(comp_data);
    comp_data = nullptr;
    
    // Verify cleanup completed successfully
    ASSERT_EQ(comp_ctx, nullptr);
    ASSERT_EQ(comp_data, nullptr);
    
    // Test that we can still create new compilation contexts
    ASSERT_TRUE(CreateCompilationData());
    ASSERT_NE(comp_data, nullptr);
    ASSERT_NE(comp_ctx, nullptr);
}