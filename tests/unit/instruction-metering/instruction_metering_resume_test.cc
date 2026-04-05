#include "gtest/gtest.h"
#include "test_helper.h"

#include <vector>

#include "bh_read_file.h"
#include "wasm_export.h"

class instruction_metering_resume_test_suite : public testing::Test
{
  protected:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

static std::vector<uint8_t>
load_wasm_file(const std::string &path)
{
    unsigned size = 0;
    uint8_t *buf = (uint8_t *)bh_read_file_to_buffer(path.c_str(), &size);
    EXPECT_NE(buf, nullptr);
    std::vector<uint8_t> out(buf, buf + size);
    wasm_runtime_free(buf);
    return out;
}

TEST_F(instruction_metering_resume_test_suite,
       resume_after_instruction_limit_continues_execution)
{
    std::string wasm_path = get_test_binary_dir() + "/resume_counter.wasm";
    auto wasm = load_wasm_file(wasm_path);

    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(wasm.data(), (uint32_t)wasm.size(),
                                             error_buf, sizeof(error_buf));
    ASSERT_NE(module, nullptr) << error_buf;

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 16 * 1024, 16 * 1024, error_buf, sizeof(error_buf));
    ASSERT_NE(inst, nullptr) << error_buf;

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 16 * 1024);
    ASSERT_NE(exec_env, nullptr);

    wasm_function_inst_t fn = wasm_runtime_lookup_function(inst, "countdown");
    ASSERT_NE(fn, nullptr);

    uint32_t argv[1] = { 5000 };

    wasm_runtime_set_instruction_count_limit(exec_env, 2);
    bool ok = wasm_runtime_call_wasm(exec_env, fn, 1, argv);
    EXPECT_FALSE(ok);
    ASSERT_NE(wasm_runtime_get_exception(inst), nullptr);
    EXPECT_NE(std::string(wasm_runtime_get_exception(inst))
                  .find("instruction limit exceeded"),
              std::string::npos);

    wasm_runtime_clear_exception(inst);
    wasm_runtime_set_instruction_count_limit(exec_env, 200000);
    ok = wasm_runtime_call_wasm(exec_env, fn, 1, argv);

    EXPECT_TRUE(ok);
    EXPECT_EQ(argv[0], 0u);

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
}

TEST_F(instruction_metering_resume_test_suite,
       reject_different_function_while_resume_pending)
{
    std::string wasm_path = get_test_binary_dir() + "/resume_counter.wasm";
    auto wasm = load_wasm_file(wasm_path);

    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(wasm.data(), (uint32_t)wasm.size(),
                                             error_buf, sizeof(error_buf));
    ASSERT_NE(module, nullptr) << error_buf;

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 16 * 1024, 16 * 1024, error_buf, sizeof(error_buf));
    ASSERT_NE(inst, nullptr) << error_buf;

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 16 * 1024);
    ASSERT_NE(exec_env, nullptr);

    wasm_function_inst_t countdown =
        wasm_runtime_lookup_function(inst, "countdown");
    wasm_function_inst_t noop = wasm_runtime_lookup_function(inst, "noop");
    ASSERT_NE(countdown, nullptr);
    ASSERT_NE(noop, nullptr);

    uint32_t argv_countdown[1] = { 1000 };
    wasm_runtime_set_instruction_count_limit(exec_env, 10);
    bool ok =
        wasm_runtime_call_wasm(exec_env, countdown, 1, argv_countdown);
    EXPECT_FALSE(ok);
    ASSERT_NE(wasm_runtime_get_exception(inst), nullptr);
    EXPECT_NE(std::string(wasm_runtime_get_exception(inst))
                  .find("instruction limit exceeded"),
              std::string::npos);

    wasm_runtime_clear_exception(inst);
    uint32_t argv_noop[1] = { 0 };
    ok = wasm_runtime_call_wasm(exec_env, noop, 0, argv_noop);

    EXPECT_FALSE(ok);
    ASSERT_NE(wasm_runtime_get_exception(inst), nullptr);
    EXPECT_NE(
        std::string(wasm_runtime_get_exception(inst))
            .find("cannot call different function while metering resume is "
                  "pending"),
        std::string::npos);

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
}

TEST_F(instruction_metering_resume_test_suite,
       resume_nested_call_from_same_export_continues_execution)
{
    std::string wasm_path = get_test_binary_dir() + "/resume_nested.wasm";
    auto wasm = load_wasm_file(wasm_path);

    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(wasm.data(), (uint32_t)wasm.size(),
                                             error_buf, sizeof(error_buf));
    ASSERT_NE(module, nullptr) << error_buf;

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 16 * 1024, 16 * 1024, error_buf, sizeof(error_buf));
    ASSERT_NE(inst, nullptr) << error_buf;

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 16 * 1024);
    ASSERT_NE(exec_env, nullptr);

    wasm_function_inst_t install = wasm_runtime_lookup_function(inst, "install");
    ASSERT_NE(install, nullptr);

    uint32_t argv[1] = { 5000 };

    wasm_runtime_set_instruction_count_limit(exec_env, 2);
    bool ok = wasm_runtime_call_wasm(exec_env, install, 1, argv);
    EXPECT_FALSE(ok);
    ASSERT_NE(wasm_runtime_get_exception(inst), nullptr);
    EXPECT_NE(std::string(wasm_runtime_get_exception(inst))
                  .find("instruction limit exceeded"),
              std::string::npos);

    wasm_runtime_clear_exception(inst);
    wasm_runtime_set_instruction_count_limit(exec_env, 200000);
    ok = wasm_runtime_call_wasm(exec_env, install, 1, argv);
    if (!ok) {
        const char *ex = wasm_runtime_get_exception(inst);
        if (ex) {
            fprintf(stderr, "nested resume failure exception: %s\n", ex);
        }
    }

    EXPECT_TRUE(ok);
    EXPECT_EQ(argv[0], 0u);

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
}

TEST_F(instruction_metering_resume_test_suite,
       resume_api_continues_nested_execution_without_recalling_export)
{
    std::string wasm_path = get_test_binary_dir() + "/resume_nested.wasm";
    auto wasm = load_wasm_file(wasm_path);

    char error_buf[128] = { 0 };
    wasm_module_t module = wasm_runtime_load(wasm.data(), (uint32_t)wasm.size(),
                                             error_buf, sizeof(error_buf));
    ASSERT_NE(module, nullptr) << error_buf;

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 16 * 1024, 16 * 1024, error_buf, sizeof(error_buf));
    ASSERT_NE(inst, nullptr) << error_buf;

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 16 * 1024);
    ASSERT_NE(exec_env, nullptr);

    wasm_function_inst_t install = wasm_runtime_lookup_function(inst, "install");
    ASSERT_NE(install, nullptr);

    uint32_t argv[1] = { 5000 };

    wasm_runtime_set_instruction_count_limit(exec_env, 20);
    bool ok = wasm_runtime_call_wasm(exec_env, install, 1, argv);
    EXPECT_FALSE(ok);
    ASSERT_NE(wasm_runtime_get_exception(inst), nullptr);
    EXPECT_NE(std::string(wasm_runtime_get_exception(inst))
                  .find("instruction limit exceeded"),
              std::string::npos);

    wasm_runtime_clear_exception(inst);
    wasm_runtime_set_instruction_count_limit(exec_env, 200000);
    ok = wasm_runtime_resume_wasm(exec_env);
    if (!ok) {
        const char *ex = wasm_runtime_get_exception(inst);
        if (ex) {
            fprintf(stderr, "resume api failure exception: %s\n", ex);
        }
    }

    EXPECT_TRUE(ok);

    if (ok) {
        uint32_t verify_argv[1] = { 5000 };
        ok = wasm_runtime_call_wasm(exec_env, install, 1, verify_argv);
        EXPECT_TRUE(ok);
        if (ok) {
            EXPECT_EQ(verify_argv[0], 0u);
        }
    }

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
}
