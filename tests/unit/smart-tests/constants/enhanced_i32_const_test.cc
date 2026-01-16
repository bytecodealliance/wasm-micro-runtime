/*
* Copyright (C) 2019 Intel Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::string CWD;
static std::string WASM_FILE;

static int app_argc;
static char **app_argv;

/**
* Test fixture for i32.const opcode validation
*
* This class provides comprehensive testing infrastructure for the i32.const WebAssembly opcode,
* ensuring proper constant loading functionality across different execution modes (interpreter and AOT).
* Tests validate that i32.const correctly pushes immediate 32-bit signed integer values onto
* the execution stack without consuming any stack operands.
*/
class I32ConstTest : public testing::TestWithParam<RunningMode>
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
    * Set up test environment for i32.const opcode testing
    *
    * Initializes WAMR runtime with appropriate configuration for testing i32.const operations.
    * Configures memory allocation, execution mode, and loads the i32.const test module.
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
    * Clean up test environment after i32.const opcode testing
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
    * Execute i32.const test function and return the loaded constant value
    *
    * @param func_name Name of the WASM function to execute (must return i32)
    * @return The i32 constant value loaded by the function
    */
    int32_t call_const_func(const char* func_name)
    {
        wasm_function_inst_t func_inst = wasm_runtime_lookup_function(module_inst, func_name);
        EXPECT_NE(func_inst, nullptr) << "Failed to lookup function: " << func_name;

        uint32_t argv[1] = {0};

        bool success = wasm_runtime_call_wasm(exec_env, func_inst, 0, argv);
        EXPECT_TRUE(success) << "Failed to call function: " << func_name
                        << " - " << wasm_runtime_get_exception(module_inst);

        return static_cast<int32_t>(argv[0]);
    }
};

/**
* @test BasicConstantLoading_ReturnsCorrectValues
* @brief Validates i32.const produces correct values for typical integer inputs
* @details Tests fundamental constant loading operation with positive, negative, and zero values.
*          Verifies that i32.const correctly pushes immediate values onto the execution stack.
* @test_category Main - Basic functionality validation
* @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:i32_const_operation
* @input_conditions Standard integer values: 1, -1, 42, -42, 100, -100, 0
* @expected_behavior Returns exact constant values: 1, -1, 42, -42, 100, -100, 0
* @validation_method Direct comparison of WASM function result with expected constant values
*/
TEST_P(I32ConstTest, BasicConstantLoading_ReturnsCorrectValues)
{
    // Test positive constant values
    ASSERT_EQ(1, call_const_func("const_positive_one"))
        << "i32.const failed to load positive constant 1";
    ASSERT_EQ(42, call_const_func("const_positive_42"))
        << "i32.const failed to load positive constant 42";
    ASSERT_EQ(100, call_const_func("const_positive_100"))
        << "i32.const failed to load positive constant 100";

    // Test negative constant values
    ASSERT_EQ(-1, call_const_func("const_negative_one"))
        << "i32.const failed to load negative constant -1";
    ASSERT_EQ(-42, call_const_func("const_negative_42"))
        << "i32.const failed to load negative constant -42";
    ASSERT_EQ(-100, call_const_func("const_negative_100"))
        << "i32.const failed to load negative constant -100";

    // Test zero constant value
    ASSERT_EQ(0, call_const_func("const_zero"))
        << "i32.const failed to load zero constant";
}





// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, I32ConstTest,
                        testing::Values(Mode_Interp, Mode_LLVM_JIT),
                        [](const testing::TestParamInfo<I32ConstTest::ParamType> &info) {
                            return info.param == Mode_Interp ? "INTERP" : "AOT";
                        });

int
main(int argc, char **argv)
{
    char *cwd = getcwd(nullptr, 0);
    if (cwd != nullptr) {
        CWD = std::string(cwd);
        free(cwd);
    } else {
        CWD = ".";
    }

    WASM_FILE = CWD + "/wasm-apps/i32_const_test.wasm";

    app_argc = argc;
    app_argv = argv;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
 