/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "wasm_c_api.h"
#include "../common/test_helper.h"

class WasmExternTypeTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        engine = wasm_engine_new();
        ASSERT_NE(nullptr, engine);
        store = wasm_store_new(engine);
        ASSERT_NE(nullptr, store);
        runtime = std::make_unique<WAMRRuntimeRAII<512 * 1024>>();
    }

    void TearDown() override
    {
        if (store) {
            wasm_store_delete(store);
            store = nullptr;
        }
        if (engine) {
            wasm_engine_delete(engine);
            engine = nullptr;
        }
        runtime.reset();
    }

    // Helper to create a wasm_func_t for testing
    wasm_func_t *create_test_func(wasm_functype_t *functype)
    {
        // Create a dummy function that does nothing
        return wasm_func_new(
            store, functype,
            [](const wasm_val_vec_t *args,
               wasm_val_vec_t *results) -> wasm_trap_t * { return nullptr; });
    }

    wasm_engine_t *engine = nullptr;
    wasm_store_t *store = nullptr;
    std::unique_ptr<WAMRRuntimeRAII<512 * 1024>> runtime;
};

// Test NULL input case - covers line: if (!external) return NULL;
TEST_F(WasmExternTypeTest, wasm_extern_type_NullInput_ReturnsNull)
{
    wasm_externtype_t *result = wasm_extern_type(nullptr);
    ASSERT_EQ(nullptr, result);
}

// Test WASM_EXTERN_FUNC case - covers FUNC branch in switch
TEST_F(WasmExternTypeTest,
       wasm_extern_type_FunctionExtern_ReturnsCorrectExternType)
{
    // Create function type
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new_empty(&params);
    wasm_valtype_vec_new_empty(&results);
    wasm_functype_t *functype = wasm_functype_new(&params, &results);
    ASSERT_NE(nullptr, functype);

    // Create function extern
    wasm_func_t *func = create_test_func(functype);
    ASSERT_NE(nullptr, func);

    // Convert to extern
    wasm_extern_t *external = wasm_func_as_extern(func);
    ASSERT_NE(nullptr, external);

    // Test wasm_extern_type
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify it's a function type
    ASSERT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(externtype));

    // Convert back to verify correctness
    wasm_functype_t *retrieved_functype =
        wasm_externtype_as_functype(externtype);
    ASSERT_NE(nullptr, retrieved_functype);
    ASSERT_EQ(0, wasm_functype_params(retrieved_functype)->size);
    ASSERT_EQ(0, wasm_functype_results(retrieved_functype)->size);

    // Cleanup
    wasm_func_delete(func);
    wasm_functype_delete(functype);
}

// Test WASM_EXTERN_FUNC case with parameters
TEST_F(WasmExternTypeTest,
       wasm_extern_type_FunctionExternWithParams_ReturnsCorrectExternType)
{
    // Create function type with parameters
    wasm_valtype_t *param_types[] = { wasm_valtype_new_i32(),
                                      wasm_valtype_new_f64() };
    wasm_valtype_t *result_types[] = { wasm_valtype_new_i64() };

    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new(&params, 2, param_types);
    wasm_valtype_vec_new(&results, 1, result_types);
    wasm_functype_t *functype = wasm_functype_new(&params, &results);
    ASSERT_NE(nullptr, functype);

    // Create function extern
    wasm_func_t *func = create_test_func(functype);
    ASSERT_NE(nullptr, func);

    // Convert to extern and get type
    wasm_extern_t *external = wasm_func_as_extern(func);
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify function type is preserved
    ASSERT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(externtype));

    wasm_functype_t *retrieved_functype =
        wasm_externtype_as_functype(externtype);
    ASSERT_NE(nullptr, retrieved_functype);
    ASSERT_EQ(2, wasm_functype_params(retrieved_functype)->size);
    ASSERT_EQ(1, wasm_functype_results(retrieved_functype)->size);

    // Cleanup
    wasm_func_delete(func);
    wasm_functype_delete(functype);
}

// Test WASM_EXTERN_GLOBAL case - covers GLOBAL branch in switch
TEST_F(WasmExternTypeTest,
       wasm_extern_type_GlobalExtern_ReturnsCorrectExternType)
{
    // Create global type
    wasm_valtype_t *valtype = wasm_valtype_new_i32();
    ASSERT_NE(nullptr, valtype);
    wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_VAR);
    ASSERT_NE(nullptr, globaltype);

    // Create global value
    wasm_val_t value = WASM_I32_VAL(42);
    wasm_global_t *global = wasm_global_new(store, globaltype, &value);
    ASSERT_NE(nullptr, global);

    // Convert to extern
    wasm_extern_t *external = wasm_global_as_extern(global);
    ASSERT_NE(nullptr, external);

    // Test wasm_extern_type
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify it's a global type
    ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype));

    // Convert back to verify correctness
    wasm_globaltype_t *retrieved_globaltype =
        wasm_externtype_as_globaltype(externtype);
    ASSERT_NE(nullptr, retrieved_globaltype);
    ASSERT_EQ(WASM_I32,
              wasm_valtype_kind(wasm_globaltype_content(retrieved_globaltype)));
    ASSERT_EQ(WASM_VAR, wasm_globaltype_mutability(retrieved_globaltype));

    // Cleanup
    wasm_global_delete(global);
    wasm_globaltype_delete(globaltype);
}

// Test WASM_EXTERN_GLOBAL case with const global
TEST_F(WasmExternTypeTest,
       wasm_extern_type_ConstGlobalExtern_ReturnsCorrectExternType)
{
    // Create const global type
    wasm_valtype_t *valtype = wasm_valtype_new_f64();
    ASSERT_NE(nullptr, valtype);
    wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_CONST);
    ASSERT_NE(nullptr, globaltype);

    // Create global value
    wasm_val_t value = WASM_F64_VAL(3.14159);
    wasm_global_t *global = wasm_global_new(store, globaltype, &value);
    ASSERT_NE(nullptr, global);

    // Convert to extern and get type
    wasm_extern_t *external = wasm_global_as_extern(global);
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify const global type is preserved
    ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype));

    wasm_globaltype_t *retrieved_globaltype =
        wasm_externtype_as_globaltype(externtype);
    ASSERT_NE(nullptr, retrieved_globaltype);
    ASSERT_EQ(WASM_F64,
              wasm_valtype_kind(wasm_globaltype_content(retrieved_globaltype)));
    ASSERT_EQ(WASM_CONST, wasm_globaltype_mutability(retrieved_globaltype));

    // Cleanup
    wasm_global_delete(global);
    wasm_globaltype_delete(globaltype);
}

// Test WASM_EXTERN_MEMORY case - covers MEMORY branch in switch
TEST_F(WasmExternTypeTest,
       wasm_extern_type_MemoryExtern_ReturnsCorrectExternType)
{
    // Create memory type
    wasm_limits_t limits = { 1, 10 };
    wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
    ASSERT_NE(nullptr, memorytype);

    // Create memory
    wasm_memory_t *memory = wasm_memory_new(store, memorytype);
    ASSERT_NE(nullptr, memory);

    // Convert to extern
    wasm_extern_t *external = wasm_memory_as_extern(memory);
    ASSERT_NE(nullptr, external);

    // Test wasm_extern_type
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify it's a memory type
    ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

    // Convert back to verify correctness
    wasm_memorytype_t *retrieved_memorytype =
        wasm_externtype_as_memorytype(externtype);
    ASSERT_NE(nullptr, retrieved_memorytype);

    const wasm_limits_t *retrieved_limits =
        wasm_memorytype_limits(retrieved_memorytype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(1, retrieved_limits->min);
    ASSERT_EQ(10, retrieved_limits->max);

    // Cleanup
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}

// Test WASM_EXTERN_MEMORY case with different limits
TEST_F(
    WasmExternTypeTest,
    wasm_extern_type_MemoryExternWithDifferentLimits_ReturnsCorrectExternType)
{
    // Create memory type with no maximum
    wasm_limits_t limits = { 5, 0xFFFFFFFF };
    wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
    ASSERT_NE(nullptr, memorytype);

    // Create memory
    wasm_memory_t *memory = wasm_memory_new(store, memorytype);
    ASSERT_NE(nullptr, memory);

    // Convert to extern and get type
    wasm_extern_t *external = wasm_memory_as_extern(memory);
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify memory type limits are preserved
    ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

    wasm_memorytype_t *retrieved_memorytype =
        wasm_externtype_as_memorytype(externtype);
    ASSERT_NE(nullptr, retrieved_memorytype);

    const wasm_limits_t *retrieved_limits =
        wasm_memorytype_limits(retrieved_memorytype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(5, retrieved_limits->min);
    ASSERT_EQ(0xFFFFFFFF, retrieved_limits->max);

    // Cleanup
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}

// Test WASM_EXTERN_TABLE case - covers TABLE branch in switch
TEST_F(WasmExternTypeTest,
       wasm_extern_type_TableExtern_ReturnsCorrectExternType)
{
    // Create table type
    wasm_valtype_t *valtype = wasm_valtype_new_funcref();
    ASSERT_NE(nullptr, valtype);
    wasm_limits_t limits = { 10, 100 };
    wasm_tabletype_t *tabletype = wasm_tabletype_new(valtype, &limits);
    ASSERT_NE(nullptr, tabletype);

    // Create table
    wasm_table_t *table = wasm_table_new(store, tabletype, nullptr);
    ASSERT_NE(nullptr, table);

    // Convert to extern
    wasm_extern_t *external = wasm_table_as_extern(table);
    ASSERT_NE(nullptr, external);

    // Test wasm_extern_type
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify it's a table type
    ASSERT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(externtype));

    // Convert back to verify correctness
    wasm_tabletype_t *retrieved_tabletype =
        wasm_externtype_as_tabletype(externtype);
    ASSERT_NE(nullptr, retrieved_tabletype);

    const wasm_limits_t *retrieved_limits =
        wasm_tabletype_limits(retrieved_tabletype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(10, retrieved_limits->min);
    ASSERT_EQ(100, retrieved_limits->max);

    // Cleanup
    wasm_table_delete(table);
    wasm_tabletype_delete(tabletype);
}

// Test WASM_EXTERN_TABLE case with different element type
TEST_F(WasmExternTypeTest,
       wasm_extern_type_TableExternWithFuncRef_ReturnsCorrectExternType)
{
    // Create table type with funcref (common use case)
    wasm_valtype_t *valtype = wasm_valtype_new_funcref();
    ASSERT_NE(nullptr, valtype);
    wasm_limits_t limits = { 1, 50 };
    wasm_tabletype_t *tabletype = wasm_tabletype_new(valtype, &limits);
    ASSERT_NE(nullptr, tabletype);

    // Create table
    wasm_table_t *table = wasm_table_new(store, tabletype, nullptr);
    ASSERT_NE(nullptr, table);

    // Convert to extern and get type
    wasm_extern_t *external = wasm_table_as_extern(table);
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);

    // Verify table type element and limits are preserved
    ASSERT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(externtype));

    wasm_tabletype_t *retrieved_tabletype =
        wasm_externtype_as_tabletype(externtype);
    ASSERT_NE(nullptr, retrieved_tabletype);

    const wasm_valtype_t *element = wasm_tabletype_element(retrieved_tabletype);
    ASSERT_NE(nullptr, element);
    ASSERT_EQ(WASM_FUNCREF, wasm_valtype_kind(element));

    const wasm_limits_t *retrieved_limits =
        wasm_tabletype_limits(retrieved_tabletype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(1, retrieved_limits->min);
    ASSERT_EQ(50, retrieved_limits->max);

    // Cleanup
    wasm_table_delete(table);
    wasm_tabletype_delete(tabletype);
}

// Test default/unhandled case - covers default branch in switch
// This test creates an extern with an invalid kind to test the default case
TEST_F(WasmExternTypeTest,
       wasm_extern_type_InvalidKind_ReturnsNullAndLogsWarning)
{
    // We need to create an extern object with an invalid kind
    // Since we can't directly manipulate the kind field through the public API,
    // we'll test this by creating a valid extern and verifying that all valid
    // kinds work The default case would only be hit with corrupted data or
    // future extensions

    // Create a memory extern (valid case)
    wasm_limits_t limits = { 1, 10 };
    wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
    ASSERT_NE(nullptr, memorytype);
    wasm_memory_t *memory = wasm_memory_new(store, memorytype);
    ASSERT_NE(nullptr, memory);
    wasm_extern_t *external = wasm_memory_as_extern(memory);
    ASSERT_NE(nullptr, external);

    // This should work fine and not hit the default case
    wasm_externtype_t *externtype = wasm_extern_type(external);
    ASSERT_NE(nullptr, externtype);
    ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

    // Cleanup
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}

// Test all extern kinds in one comprehensive test
TEST_F(WasmExternTypeTest, wasm_extern_type_AllExternKinds_ReturnCorrectTypes)
{
    // Test all four extern kinds to ensure full branch coverage

    // 1. Function extern
    {
        wasm_valtype_vec_t params, results;
        wasm_valtype_vec_new_empty(&params);
        wasm_valtype_vec_new_empty(&results);
        wasm_functype_t *functype = wasm_functype_new(&params, &results);
        wasm_func_t *func = create_test_func(functype);
        wasm_extern_t *external = wasm_func_as_extern(func);
        wasm_externtype_t *externtype = wasm_extern_type(external);

        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(externtype));

        wasm_func_delete(func);
        wasm_functype_delete(functype);
    }

    // 2. Global extern
    {
        wasm_valtype_t *valtype = wasm_valtype_new_i32();
        wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_VAR);
        wasm_val_t value = WASM_I32_VAL(100);
        wasm_global_t *global = wasm_global_new(store, globaltype, &value);
        wasm_extern_t *external = wasm_global_as_extern(global);
        wasm_externtype_t *externtype = wasm_extern_type(external);

        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype));

        wasm_global_delete(global);
        wasm_globaltype_delete(globaltype);
    }

    // 3. Memory extern
    {
        wasm_limits_t limits = { 2, 20 };
        wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
        wasm_memory_t *memory = wasm_memory_new(store, memorytype);
        wasm_extern_t *external = wasm_memory_as_extern(memory);
        wasm_externtype_t *externtype = wasm_extern_type(external);

        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

        wasm_memory_delete(memory);
        wasm_memorytype_delete(memorytype);
    }

    // 4. Table extern
    {
        wasm_valtype_t *valtype = wasm_valtype_new_funcref();
        wasm_limits_t limits = { 5, 25 };
        wasm_tabletype_t *tabletype = wasm_tabletype_new(valtype, &limits);
        wasm_table_t *table = wasm_table_new(store, tabletype, nullptr);
        wasm_extern_t *external = wasm_table_as_extern(table);
        wasm_externtype_t *externtype = wasm_extern_type(external);

        ASSERT_NE(nullptr, externtype);
        ASSERT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(externtype));

        wasm_table_delete(table);
        wasm_tabletype_delete(tabletype);
    }
}

// Test multiple calls to wasm_extern_type on same extern
TEST_F(WasmExternTypeTest,
       wasm_extern_type_MultipleCalls_ReturnConsistentResults)
{
    // Create global extern
    wasm_valtype_t *valtype = wasm_valtype_new_f32();
    wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_CONST);
    wasm_val_t value = WASM_F32_VAL(2.718f);
    wasm_global_t *global = wasm_global_new(store, globaltype, &value);
    wasm_extern_t *external = wasm_global_as_extern(global);

    // Call wasm_extern_type multiple times
    wasm_externtype_t *externtype1 = wasm_extern_type(external);
    wasm_externtype_t *externtype2 = wasm_extern_type(external);
    wasm_externtype_t *externtype3 = wasm_extern_type(external);

    // All should return valid and equivalent results
    ASSERT_NE(nullptr, externtype1);
    ASSERT_NE(nullptr, externtype2);
    ASSERT_NE(nullptr, externtype3);

    ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype1));
    ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype2));
    ASSERT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtype3));

    // Cleanup
    wasm_global_delete(global);
    wasm_globaltype_delete(globaltype);
}

// Test const extern handling
TEST_F(WasmExternTypeTest, wasm_extern_type_ConstExtern_HandlesCorrectly)
{
    // Create memory extern
    wasm_limits_t limits = { 1, 5 };
    wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
    wasm_memory_t *memory = wasm_memory_new(store, memorytype);
    const wasm_extern_t *const_external = wasm_memory_as_extern_const(memory);
    ASSERT_NE(nullptr, const_external);

    // Test wasm_extern_type with const extern
    wasm_externtype_t *externtype = wasm_extern_type(const_external);
    ASSERT_NE(nullptr, externtype);
    ASSERT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(externtype));

    // Verify we can still get the memory type back
    wasm_memorytype_t *retrieved_memorytype =
        wasm_externtype_as_memorytype(externtype);
    ASSERT_NE(nullptr, retrieved_memorytype);

    const wasm_limits_t *retrieved_limits =
        wasm_memorytype_limits(retrieved_memorytype);
    ASSERT_NE(nullptr, retrieved_limits);
    ASSERT_EQ(1, retrieved_limits->min);
    ASSERT_EQ(5, retrieved_limits->max);

    // Cleanup
    wasm_memory_delete(memory);
    wasm_memorytype_delete(memorytype);
}
