/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include <gtest/gtest.h>

class PrototypeTest : public testing::Test {
  protected:
    virtual void SetUp() {
        ctx = dyntype_context_init();
    }

    virtual void TearDown() {
        dyntype_context_destroy(ctx);
    }

    dyn_ctx_t ctx;
};

TEST_F(PrototypeTest, prototype) {
    char const *name = "Jack";
    dyn_value_t obj1 = dyntype_new_object(ctx);
    EXPECT_NE(obj1, nullptr);
    dyn_value_t prop1 = dyntype_new_string(ctx, name);
    EXPECT_NE(prop1, nullptr);
    EXPECT_EQ(dyntype_set_property(ctx, obj1, "name", prop1), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_new_object_with_proto(ctx, dyntype_new_number(ctx, 10.0)), nullptr);

    dyn_value_t obj2 = dyntype_new_object_with_proto(ctx, obj1);
    EXPECT_NE(obj2, nullptr);
    EXPECT_EQ(dyntype_has_property(ctx, obj2, "name"), DYNTYPE_TRUE);
    EXPECT_FALSE(dyntype_instanceof(ctx, obj1, obj2));
    dyn_value_t value = dyntype_get_property(ctx, obj2, "name");
    EXPECT_TRUE(dyntype_is_string(ctx, value));
    char *raw_value = nullptr;
    EXPECT_EQ(dyntype_to_cstring(ctx, value, &raw_value), DYNTYPE_SUCCESS);
    EXPECT_STREQ(raw_value, name);

    EXPECT_EQ(dyntype_get_own_property(ctx, obj2, "name"), nullptr);
    dyn_value_t obj3 = dyntype_new_object(ctx);
    EXPECT_EQ(dyntype_set_prototype(ctx, obj1, obj3), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_prototype(ctx, obj1, dyntype_new_number(ctx, 10.0)), -DYNTYPE_TYPEERR);
    // TODO: cant test now
    // dyntype_instanceof(ctx)

    dyntype_release(ctx, obj1);
    dyntype_release(ctx, obj2);
    dyntype_release(ctx, obj3);
}
