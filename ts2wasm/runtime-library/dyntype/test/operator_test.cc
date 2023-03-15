/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include <gtest/gtest.h>

class OperatorTest : public testing::Test {
  protected:
    virtual void SetUp() {
        ctx = dyntype_context_init();
    }

    virtual void TearDown() {
        dyntype_context_destroy(ctx);
    }

    testing::AssertionResult is_type_eq(dyn_value_t lhs, dyn_value_t rhs, uint32_t l, uint32_t r) {
        if (dyntype_type_eq(ctx, lhs, rhs)) {
            return testing::AssertionSuccess() << "they are value1[" << l << "], value2[" << r << "]";
        }
        return testing::AssertionFailure() << "they are value1[" << l << "], value2[" << r << "]";
    }

    dyn_ctx_t ctx;
};

TEST_F(OperatorTest, typeof) {
    void *extobj = malloc(sizeof(uint32_t) * 10);

    dyn_value_t num = dyntype_new_number(ctx, 2147483649);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t obj = dyntype_new_object(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "string");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref_obj = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj);
    dyn_value_t extref_func = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtFunc);

    EXPECT_EQ(dyntype_typeof(ctx, num), DynNumber);
    EXPECT_EQ(dyntype_typeof(ctx, boolean), DynBoolean);
    EXPECT_EQ(dyntype_typeof(ctx, undefined), DynUndefined);
    EXPECT_EQ(dyntype_typeof(ctx, null), DynObject);
    EXPECT_EQ(dyntype_typeof(ctx, obj), DynObject);
    EXPECT_EQ(dyntype_typeof(ctx, str), DynString);
    EXPECT_EQ(dyntype_typeof(ctx, array), DynObject);
    EXPECT_EQ(dyntype_typeof(ctx, extref_obj), DynExtRefObj);
    EXPECT_EQ(dyntype_typeof(ctx, extref_func), DynExtRefFunc);


    free(extobj);
    dyntype_release(ctx, obj);
    dyntype_release(ctx, str);
    dyntype_release(ctx, array);
}

TEST_F(OperatorTest, type_eq) {
    void *extobj = malloc(sizeof(uint32_t) * 10);
    dyn_value_t value1[] = {dyntype_new_number(ctx, 2147483649), dyntype_new_boolean(ctx, true),
                            dyntype_new_undefined(ctx), dyntype_new_string(ctx, "string"),
                            dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj),
                            dyntype_new_extref(ctx, extobj, external_ref_tag::ExtFunc)};

    dyn_value_t value2[] = {dyntype_new_number(ctx, -10.00), dyntype_new_boolean(ctx, false),
                            dyntype_new_undefined(ctx), dyntype_new_string(ctx, "test"),
                            dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj),
                            dyntype_new_extref(ctx, extobj, external_ref_tag::ExtFunc),
                            dyntype_new_null(ctx), dyntype_new_object(ctx), dyntype_new_array(ctx)};

    // they are all object type
    dyn_value_t value3[] = {dyntype_new_null(ctx), dyntype_new_object(ctx), dyntype_new_array(ctx)};
    uint32_t len1 = sizeof(value1) / sizeof(dyn_value_t);
    uint32_t len2 = sizeof(value2) / sizeof(dyn_value_t);
    uint32_t len3 = sizeof(value3) / sizeof(dyn_value_t);

    for (uint32_t i = 0; i < len1; i++) {
        for (uint32_t j = 0; j < len2; j++) {
            if (i == j) {
                EXPECT_TRUE(is_type_eq(value1[i], value2[j], i, j));
                continue;
            }
            EXPECT_FALSE(is_type_eq(value1[i], value2[j], i, j));
        }
    }
    // null, arary, object types
    for (uint32_t i = 8; i < len2; i++) {
        for (uint32_t j = 8; j < len3; j++) {
                EXPECT_TRUE(is_type_eq(value2[i], value3[j], i, j));
        }
    }

    free(extobj);
    for (uint32_t i = 0; i < len1; i++) {
        dyntype_release(ctx, value1[i]);
    }
    for (uint32_t i = 0; i < len2; i++) {
        dyntype_release(ctx, value2[i]);
    }
    for (uint32_t i = 0; i < len3; i++) {
        dyntype_release(ctx, value3[i]);
    }
}
