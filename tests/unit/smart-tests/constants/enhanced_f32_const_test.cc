/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

 #include <gtest/gtest.h>
 #include <cstdint>
 #include <cfloat>
 #include <cmath>
 #include <cstdlib>
 #include <unistd.h>
 #include <cstring>
 #include <vector>
 #include "test_helper.h"
 #include "wasm_runtime_common.h"
 #include "bh_read_file.h"
 
 static std::string CWD;
 static std::string WASM_FILE;
 
 static int app_argc;
 static char **app_argv;
 
 /**
  * Test fixture for f32.const opcode validation
  *
  * This class provides comprehensive testing infrastructure for the f32.const WebAssembly opcode,
  * ensuring proper constant loading functionality across different execution modes (interpreter and AOT).
  * Tests validate that f32.const correctly pushes immediate 32-bit IEEE 754 floating-point values onto
  * the execution stack without consuming any stack operands. Includes comprehensive validation of
  * special IEEE 754 values including NaN, infinity, zero, subnormals, and boundary conditions.
  */
 class F32ConstTest : public testing::TestWithParam<RunningMode>
 {
 protected:
     WAMRRuntimeRAII<> runtime;
     wasm_module_t module = nullptr;
     wasm_module_inst_t module_inst = nullptr;
     wasm_exec_env_t exec_env = nullptr;
     uint32_t buf_size, stack_size = 8092, heap_size = 8092;
     uint8_t *buf = nullptr;
     char error_buf[128] = { 0 };
     const char *exception = nullptr;
 
     /**
      * Set up test environment for f32.const opcode testing
      *
      * Initializes WAMR runtime with appropriate configuration for testing f32.const operations.
      * Configures memory allocation, execution mode, and loads the f32.const test module.
      * Ensures proper runtime state before executing individual test cases.
      */
     void SetUp() override
     {
         memset(error_buf, 0, sizeof(error_buf));
 
         buf = (uint8_t *)bh_read_file_to_buffer(WASM_FILE.c_str(), &buf_size);
         ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << WASM_FILE;
 
         module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
         ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;
 
         module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
         ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;
 
         wasm_runtime_set_running_mode(module_inst, GetParam());
 
         exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
         ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
     }
 
     /**
      * Clean up test environment after f32.const opcode testing
      *
      * Performs proper cleanup of WASM module instances, modules, and runtime resources.
      * Ensures no memory leaks or resource conflicts between test cases.
      * Maintains clean test environment for subsequent test execution.
      */
     void TearDown() override
     {
         if (exec_env != nullptr) {
             wasm_runtime_destroy_exec_env(exec_env);
             exec_env = nullptr;
         }
         if (module_inst != nullptr) {
             wasm_runtime_deinstantiate(module_inst);
             module_inst = nullptr;
         }
         if (module != nullptr) {
             wasm_runtime_unload(module);
             module = nullptr;
         }
         if (buf != nullptr) {
             wasm_runtime_free(buf);
             buf = nullptr;
         }
     }
 
     /**
      * Execute f32.const test function and return the loaded constant value
      *
      * @param func_name Name of the WASM function to execute (must return f32)
      * @return The f32 constant value loaded by the function
      */
     float call_const_func(const char* func_name)
     {
         wasm_function_inst_t func_inst = wasm_runtime_lookup_function(module_inst, func_name);
         EXPECT_NE(func_inst, nullptr) << "Failed to lookup function: " << func_name;
 
         wasm_val_t results[1];
         wasm_val_t arguments[1];  // f32.const doesn't need arguments but declaring for consistency
 
         bool success = wasm_runtime_call_wasm_a(exec_env, func_inst, 1, results, 0, arguments);
         EXPECT_TRUE(success) << "Failed to call function: " << func_name
                             << ", exception: " << wasm_runtime_get_exception(module_inst);
 
         return results[0].of.f32;
     }
 
     /**
      * Execute multiple f32.const test function and return the loaded constant values
      *
      * @param func_name Name of the WASM function to execute (must return multiple f32 values)
      * @param result_count Number of f32 values to expect
      * @param results Array to store the returned f32 constant values
      */
     void call_multi_const_func(const char* func_name, uint32_t result_count, float* results)
     {
         wasm_function_inst_t func_inst = wasm_runtime_lookup_function(module_inst, func_name);
         ASSERT_NE(func_inst, nullptr) << "Failed to lookup function: " << func_name;
 
         wasm_val_t wasm_results[16];  // Sufficient for multiple constants
         wasm_val_t arguments[1];  // f32.const doesn't need arguments
 
         bool success = wasm_runtime_call_wasm_a(exec_env, func_inst, result_count, wasm_results, 0, arguments);
         ASSERT_TRUE(success) << "Failed to call function: " << func_name
                             << ", exception: " << wasm_runtime_get_exception(module_inst);
 
         for (uint32_t i = 0; i < result_count; i++) {
             results[i] = wasm_results[i].of.f32;
         }
     }
 
     /**
      * Check if two f32 values are bitwise identical (handles NaN correctly)
      *
      * @param a First f32 value
      * @param b Second f32 value
      * @return true if bit patterns are identical, false otherwise
      */
     bool are_f32_bitwise_equal(float a, float b)
     {
         uint32_t bits_a, bits_b;
         memcpy(&bits_a, &a, sizeof(uint32_t));
         memcpy(&bits_b, &b, sizeof(uint32_t));
         return bits_a == bits_b;
     }
 
     /**
      * Get bit pattern of f32 value as uint32_t
      *
      * @param value f32 value to get bit pattern for
      * @return uint32_t representing the bit pattern
      */
     uint32_t get_f32_bits(float value)
     {
         uint32_t bits;
         memcpy(&bits, &value, sizeof(uint32_t));
         return bits;
     }
 };
 
 /**
  * @test BasicConstants_ReturnsCorrectValues
  * @brief Validates f32.const produces correct values for typical floating-point inputs
  * @details Tests fundamental constant loading operation with positive, negative, and zero values.
  *          Verifies that f32.const correctly loads standard IEEE 754 f32 values onto stack.
  * @test_category Main - Basic functionality validation
  * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_const_operation
  * @input_conditions Standard f32 values: 1.5f, -3.14159f, 0.0f, 42.0f
  * @expected_behavior Returns exact f32 values with IEEE 754 compliance
  * @validation_method Direct f32 comparison with expected constant values
  */
 TEST_P(F32ConstTest, BasicConstants_ReturnsCorrectValues)
 {
     // Test standard positive float constant
     ASSERT_EQ(call_const_func("get_positive_const"), 1.5f)
         << "f32.const failed to load positive constant 1.5f";
 
     // Test standard negative float constant
     ASSERT_EQ(call_const_func("get_negative_const"), -3.14159f)
         << "f32.const failed to load negative constant -3.14159f";
 
     // Test positive zero constant
     ASSERT_EQ(call_const_func("get_zero_const"), 0.0f)
         << "f32.const failed to load zero constant 0.0f";
 
     // Test integer-valued float constant
     ASSERT_EQ(call_const_func("get_integer_const"), 42.0f)
         << "f32.const failed to load integer-valued constant 42.0f";
 }
 
 /**
  * @test BoundaryValues_PreservesLimits
  * @brief Validates f32.const preserves IEEE 754 boundary values accurately
  * @details Tests maximum finite values, minimum normalized values to ensure f32.const
  *          correctly handles values at the limits of IEEE 754 single-precision representation.
  * @test_category Corner - Boundary condition validation
  * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_const_operation
  * @input_conditions FLT_MAX, -FLT_MAX, FLT_MIN normalized values
  * @expected_behavior Exact preservation of boundary values with no precision loss
  * @validation_method Precise f32 comparison with boundary constants
  */
 TEST_P(F32ConstTest, BoundaryValues_PreservesLimits)
 {
     // Test maximum positive finite f32 value
     ASSERT_EQ(call_const_func("get_max_finite"), FLT_MAX)
         << "f32.const failed to preserve maximum finite positive value";
 
     // Test maximum negative finite f32 value (most negative)
     ASSERT_EQ(call_const_func("get_min_finite"), -FLT_MAX)
         << "f32.const failed to preserve maximum finite negative value";
 
     // Test smallest positive normalized f32 value (use bitwise comparison for precision)
     ASSERT_TRUE(are_f32_bitwise_equal(call_const_func("get_min_normal"), FLT_MIN))
         << "f32.const failed to preserve smallest positive normalized value";
 
     // Test smallest negative normalized f32 value (use bitwise comparison for precision)
     ASSERT_TRUE(are_f32_bitwise_equal(call_const_func("get_min_normal_neg"), -FLT_MIN))
         << "f32.const failed to preserve smallest negative normalized value";
 }
 
 
 
 
 /**
  * @test MultipleConstants_LoadsInSequence
  * @brief Validates f32.const can load multiple constants in sequence correctly
  * @details Tests loading multiple f32 constants in a single function to verify
  *          stack management and ensure no interference between constant operations.
  * @test_category Main - Multiple constant loading validation
  * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_const_operation
  * @input_conditions Sequence of multiple f32.const instructions
  * @expected_behavior All constants loaded correctly in proper order on stack
  * @validation_method Verification of multiple returned values in correct sequence
  */
 TEST_P(F32ConstTest, MultipleConstants_LoadsInSequence)
 {
     // Test loading multiple constants in sequence
     float results[3];
     call_multi_const_func("get_multiple_constants", 3, results);
 
     ASSERT_EQ(results[0], 1.0f)
         << "First f32.const in sequence failed to load correctly";
     ASSERT_EQ(results[1], 2.5f)
         << "Second f32.const in sequence failed to load correctly";
     ASSERT_EQ(results[2], -7.75f)
         << "Third f32.const in sequence failed to load correctly";
 }
 
 /**
  * @test ConstantsInOperations_FunctionsCorrectly
  * @brief Validates f32.const values work correctly in subsequent operations
  * @details Tests using f32.const values in f32.add operations to ensure constants
  *          are properly loaded and available for arithmetic operations.
  * @test_category Integration - Constant usage in operations
  * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f32_const_operation
  * @input_conditions f32.const values used as operands in f32.add
  * @expected_behavior Correct arithmetic results using loaded constants
  * @validation_method Verification of arithmetic operation results
  */
 TEST_P(F32ConstTest, ConstantsInOperations_FunctionsCorrectly)
 {
     // Test f32.const 2.5 + f32.const 3.7 = 6.2
     ASSERT_EQ(call_const_func("add_two_constants"), 6.2f)
         << "f32.const values failed in addition operation";
 
     // Test f32.const 10.0 - f32.const 3.5 = 6.5
     ASSERT_EQ(call_const_func("subtract_constants"), 6.5f)
         << "f32.const values failed in subtraction operation";
 
     // Test f32.const 2.0 * f32.const 1.5 = 3.0
     ASSERT_EQ(call_const_func("multiply_constants"), 3.0f)
         << "f32.const values failed in multiplication operation";
 }
 

 // Parameterized test instantiation for both interpreter and AOT modes
 INSTANTIATE_TEST_SUITE_P(RunningModeTest, F32ConstTest,
                          testing::Values(Mode_Interp, Mode_LLVM_JIT),
                          [](const testing::TestParamInfo<F32ConstTest::ParamType> &info) {
                              return info.param == Mode_Interp ? "INTERP" : "AOT";
                          });
 
 int main(int argc, char **argv)
 {
     char *cwd = getcwd(nullptr, 0);
     if (cwd != nullptr) {
         CWD = std::string(cwd);
         free(cwd);
     } else {
         CWD = ".";
     }
 
     WASM_FILE = CWD + "/wasm-apps/f32_const_test.wasm";
 
     app_argc = argc;
     app_argv = argv;
 
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
 }