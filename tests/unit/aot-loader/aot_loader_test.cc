/*
 * Copyright (C) 2024 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_export.h"
#include "test_helper.h"
#include "aot_runtime.h"

#include <cstring>
#include <cstdint>
#include <vector>

static void
append_u16_le(std::vector<uint8_t> &buf, uint16_t val)
{
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
}

static void
append_u32_le(std::vector<uint8_t> &buf, uint32_t val)
{
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 24) & 0xFF);
}

static void
append_u64_le(std::vector<uint8_t> &buf, uint64_t val)
{
    for (int i = 0; i < 8; i++)
        buf.push_back((val >> (i * 8)) & 0xFF);
}

/* Build a target info section body matching current platform. */
static std::vector<uint8_t>
build_target_info_body()
{
    std::vector<uint8_t> body;

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    uint16_t bin_type = 0x0002;
    uint16_t e_machine = 62;
    const char *arch = "x86-64";
#elif defined(BUILD_TARGET_AARCH64)
    uint16_t bin_type = 0x0002;
    uint16_t e_machine = 183;
    const char *arch = "aarch64";
#elif defined(BUILD_TARGET_X86_32)
    uint16_t bin_type = 0x0000;
    uint16_t e_machine = 3;
    const char *arch = "i386";
#elif defined(BUILD_TARGET_ARM)
    uint16_t bin_type = 0x0000;
    uint16_t e_machine = 40;
    const char *arch = "armv7";
#else
    uint16_t bin_type = 0x0002;
    uint16_t e_machine = 62;
    const char *arch = "x86-64";
#endif

    append_u16_le(body, bin_type);
    append_u16_le(body, 0);     /* abi_type */
    append_u16_le(body, 1);     /* e_type = E_TYPE_REL */
    append_u16_le(body, e_machine);
    append_u32_le(body, 1);     /* e_version */
    append_u32_le(body, 0);     /* e_flags */
    append_u64_le(body, 0);     /* feature_flags */
    append_u64_le(body, 0);     /* reserved */

    char arch_buf[16] = { 0 };
    strncpy(arch_buf, arch, 15);
    body.insert(body.end(), arch_buf, arch_buf + 16);

    return body;
}

/* Build a minimal valid INIT_DATA section body (all counts=0). */
static std::vector<uint8_t>
build_minimal_init_data_body()
{
    std::vector<uint8_t> body;

    /* load_memory_info */
    append_u32_le(body, 0); /* import_memory_count */
    append_u32_le(body, 0); /* memory_count */
    append_u32_le(body, 0); /* mem_init_data_count */

    /* load_table_info */
    append_u32_le(body, 0); /* import_table_count */
    append_u32_le(body, 0); /* table_count */
    append_u32_le(body, 0); /* table_init_data_count */

    /* load_type_info */
    append_u32_le(body, 0); /* type_count */

    /* load_import_global_info */
    append_u32_le(body, 0); /* import_global_count */

    /* load_global_info */
    append_u32_le(body, 0); /* global_count */

    /* load_import_func_info */
    append_u32_le(body, 0); /* import_func_count */

    /* func_count and start_func_index */
    append_u32_le(body, 0);          /* func_count */
    append_u32_le(body, 0xFFFFFFFF); /* start_func_index = invalid */

    /* aux data fields */
    append_u32_le(body, 0); /* aux_data_end_global_index */
    append_u64_le(body, 0); /* aux_data_end */
    append_u32_le(body, 0); /* aux_heap_base_global_index */
    append_u64_le(body, 0); /* aux_heap_base */
    append_u32_le(body, 0); /* aux_stack_top_global_index */
    append_u64_le(body, 0); /* aux_stack_bottom */
    append_u32_le(body, 0); /* aux_stack_size */

    /* load_object_data_sections_info */
    append_u32_le(body, 0); /* data_section_count */

    return body;
}

static wasm_section_t *
alloc_section(int type, uint8_t *body, uint32_t size)
{
    wasm_section_t *sec = (wasm_section_t *)malloc(sizeof(wasm_section_t));
    memset(sec, 0, sizeof(wasm_section_t));
    sec->section_type = type;
    sec->section_body = body;
    sec->section_body_size = size;
    sec->next = NULL;
    return sec;
}

static void
free_section_list(wasm_section_t *list)
{
    while (list) {
        wasm_section_t *next = list->next;
        free(list);
        list = next;
    }
}

class AotLoaderTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        memset(error_buf, 0, sizeof(error_buf));
    }

    void TearDown() override { wasm_runtime_destroy(); }

  public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    char error_buf[256];
};

/*
 * Test that load_text_section rejects a TEXT section whose literal_size
 * exceeds the remaining section body. Before the fix, literal_size was not
 * validated, causing module->code to point past buf_end and code_size to
 * underflow.
 */
TEST_F(AotLoaderTest, reject_text_section_with_oversized_literal_size)
{
    /* TARGET_INFO section */
    auto ti_body = build_target_info_body();
    wasm_section_t *ti_sec =
        alloc_section(0 /* TARGET_INFO */, ti_body.data(),
                      (uint32_t)ti_body.size());

    /* INIT_DATA section (minimal, all counts = 0) */
    auto id_body = build_minimal_init_data_body();
    wasm_section_t *id_sec =
        alloc_section(1 /* INIT_DATA */, id_body.data(),
                      (uint32_t)id_body.size());

    /* TEXT section: literal_size = 0x10000 but body is only 8 bytes */
    std::vector<uint8_t> text_body;
    append_u32_le(text_body, 0x10000); /* literal_size = 64K */
    append_u32_le(text_body, 0);       /* padding */

    wasm_section_t *text_sec =
        alloc_section(2 /* TEXT */, text_body.data(),
                      (uint32_t)text_body.size());

    /* CUSTOM section to satisfy post-loop "section missing" check */
    std::vector<uint8_t> custom_body;
    append_u32_le(custom_body, 0xFF); /* sub_section_type = unknown (default) */

    wasm_section_t *custom_sec =
        alloc_section(100 /* CUSTOM */, custom_body.data(),
                      (uint32_t)custom_body.size());

    /* Link sections: TARGET_INFO -> INIT_DATA -> TEXT -> CUSTOM */
    ti_sec->next = id_sec;
    id_sec->next = text_sec;
    text_sec->next = custom_sec;

    AOTModule *module =
        aot_load_from_sections(ti_sec, error_buf, sizeof(error_buf));

    if (module) {
        aot_unload(module);
        FAIL() << "Expected aot_load_from_sections to reject TEXT section "
                  "with literal_size (0x10000) exceeding body size (8)";
    }

    std::string err(error_buf);

    /* Skip if target info mismatch on this platform */
    if (err.find("machine type") != std::string::npos
        || err.find("endian") != std::string::npos
        || err.find("bit width") != std::string::npos) {
        free_section_list(ti_sec);
        GTEST_SKIP() << "Target info mismatch: " << err;
    }

    /* The fix should produce an error about invalid literal size */
    EXPECT_TRUE(err.find("literal size") != std::string::npos
                || err.find("literal_size") != std::string::npos)
        << "Expected error about literal size, got: " << err;

    free_section_list(ti_sec);
}

/*
 * Test that a valid TEXT section with literal_size=0 still loads correctly.
 */
TEST_F(AotLoaderTest, accept_text_section_with_zero_literal_size)
{
    auto ti_body = build_target_info_body();
    wasm_section_t *ti_sec =
        alloc_section(0, ti_body.data(), (uint32_t)ti_body.size());

    auto id_body = build_minimal_init_data_body();
    wasm_section_t *id_sec =
        alloc_section(1, id_body.data(), (uint32_t)id_body.size());

    /* TEXT section with literal_size = 0 (valid: all bytes are code) */
    std::vector<uint8_t> text_body;
    append_u32_le(text_body, 0); /* literal_size = 0 */

    wasm_section_t *text_sec =
        alloc_section(2, text_body.data(), (uint32_t)text_body.size());

    std::vector<uint8_t> custom_body;
    append_u32_le(custom_body, 0xFF);
    wasm_section_t *custom_sec =
        alloc_section(100, custom_body.data(), (uint32_t)custom_body.size());

    ti_sec->next = id_sec;
    id_sec->next = text_sec;
    text_sec->next = custom_sec;

    AOTModule *module =
        aot_load_from_sections(ti_sec, error_buf, sizeof(error_buf));

    std::string err(error_buf);

    /* Skip if target info mismatch */
    if (!module
        && (err.find("machine type") != std::string::npos
            || err.find("endian") != std::string::npos
            || err.find("bit width") != std::string::npos)) {
        free_section_list(ti_sec);
        GTEST_SKIP() << "Target info mismatch: " << err;
    }

    /* literal_size=0 should not trigger the new bounds check */
    if (!module) {
        EXPECT_TRUE(err.find("literal size") == std::string::npos
                    && err.find("literal_size") == std::string::npos)
            << "literal_size=0 should not be rejected, but got: " << err;
    }
    else {
        aot_unload(module);
    }

    free_section_list(ti_sec);
}
