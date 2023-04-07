/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include <gtest/gtest.h>

class TypesTest : public testing::Test {
  protected:
    virtual void SetUp() {
        ctx = dyntype_context_init();
        if (ctx == NULL) {
        }
    }

    virtual void TearDown() {
        dyntype_context_destroy(ctx);
    }

    dyn_ctx_t ctx;
};

TEST_F(TypesTest, create_number_object) {
    double check_values[] = { 2147483649.1, 0, -5.48, 1111, -1, 1234.0 };

    for (int i = 0; i < sizeof(check_values) / sizeof(check_values[0]); i++) {
        double raw_number = 0;
        dyn_value_t num = dyntype_new_number(ctx, check_values[i]);
        EXPECT_NE(num, nullptr);
        dyntype_dump_value(ctx, num);

        EXPECT_EQ(dyntype_set_property(ctx, num, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_define_property(ctx, num, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_get_property(ctx, num, "not_a_object"), nullptr);
        EXPECT_EQ(dyntype_has_property(ctx, num, "not_a_object"), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_delete_property(ctx, num, "not_a_object"), -DYNTYPE_FALSE);

        EXPECT_TRUE(dyntype_is_number(ctx, num));
        EXPECT_FALSE(dyntype_is_bool(ctx, num));
        EXPECT_FALSE(dyntype_is_object(ctx, num));
        EXPECT_FALSE(dyntype_is_undefined(ctx, num));
        EXPECT_FALSE(dyntype_is_null(ctx, num));
        EXPECT_FALSE(dyntype_is_string(ctx, num));
        EXPECT_FALSE(dyntype_is_array(ctx, num));
        EXPECT_FALSE(dyntype_is_extref(ctx, num));

        bool temp;
        char *temp2;
        EXPECT_EQ(dyntype_to_bool(ctx, num, &temp), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_to_cstring(ctx, num, &temp2), -DYNTYPE_TYPEERR);

        dyntype_to_number(ctx, num, &raw_number);
        EXPECT_EQ(raw_number, check_values[i]);
    }
}

TEST_F(TypesTest, create_boolean_object) {
    bool check_values[] = { true, false, false, false, true };

    for (int i = 0; i < sizeof(check_values) / sizeof(check_values[0]); i++) {
        bool raw_value = 0;
        dyn_value_t boolean = dyntype_new_boolean(ctx, check_values[i]);
        EXPECT_NE(boolean, nullptr);
        EXPECT_EQ(dyntype_set_property(ctx, boolean, "not_a_object", dyntype_new_boolean(ctx, false)),
                  -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_define_property(ctx, boolean, "not_a_object", dyntype_new_boolean(ctx, false)),
                  -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_get_property(ctx, boolean, "not_a_object"), nullptr);
        EXPECT_EQ(dyntype_has_property(ctx, boolean, "not_a_object"), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_delete_property(ctx, boolean, "not_a_object"), -DYNTYPE_FALSE);
        EXPECT_FALSE(dyntype_is_number(ctx, boolean));
        EXPECT_TRUE(dyntype_is_bool(ctx, boolean));
        EXPECT_FALSE(dyntype_is_object(ctx, boolean));
        EXPECT_FALSE(dyntype_is_undefined(ctx, boolean));
        EXPECT_FALSE(dyntype_is_null(ctx, boolean));
        EXPECT_FALSE(dyntype_is_string(ctx, boolean));
        EXPECT_FALSE(dyntype_is_array(ctx, boolean));
        EXPECT_FALSE(dyntype_is_extref(ctx, boolean));

        double temp1;
        char *temp2;
        EXPECT_EQ(dyntype_to_number(ctx, boolean, &temp1), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_to_cstring(ctx, boolean, &temp2), -DYNTYPE_TYPEERR);

        dyntype_to_bool(ctx, boolean, &raw_value);
        EXPECT_EQ(raw_value, check_values[i]);

        dyntype_release(ctx, boolean);
    }
}

TEST_F(TypesTest, create_undefined) {
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    EXPECT_NE(undefined, nullptr);

    EXPECT_FALSE(dyntype_is_number(ctx, undefined));
    EXPECT_FALSE(dyntype_is_bool(ctx, undefined));
    EXPECT_FALSE(dyntype_is_object(ctx, undefined));
    EXPECT_TRUE(dyntype_is_undefined(ctx, undefined));
    EXPECT_FALSE(dyntype_is_null(ctx, undefined));
    EXPECT_FALSE(dyntype_is_string(ctx, undefined));
    EXPECT_FALSE(dyntype_is_array(ctx, undefined));
    EXPECT_FALSE(dyntype_is_extref(ctx, undefined));

    EXPECT_EQ(dyntype_set_prototype(ctx, undefined, dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_get_prototype(ctx, undefined), nullptr);
    EXPECT_EQ(dyntype_get_own_property(ctx, undefined, "has not property"), nullptr);

    bool temp;
    double temp1;
    char *temp2;
    EXPECT_EQ(dyntype_to_bool(ctx, undefined, &temp), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_number(ctx, undefined, &temp1), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_cstring(ctx, undefined, &temp2), -DYNTYPE_TYPEERR);

    dyntype_release(ctx, undefined);
}

TEST_F(TypesTest, create_null) {
    dyn_value_t null = dyntype_new_null(ctx);
    EXPECT_NE(null, nullptr);

    EXPECT_FALSE(dyntype_is_number(ctx, null));
    EXPECT_FALSE(dyntype_is_bool(ctx, null));
    EXPECT_FALSE(dyntype_is_object(ctx, null));
    EXPECT_FALSE(dyntype_is_undefined(ctx, null));
    EXPECT_TRUE(dyntype_is_null(ctx, null));
    EXPECT_FALSE(dyntype_is_string(ctx, null));
    EXPECT_FALSE(dyntype_is_array(ctx, null));
    EXPECT_FALSE(dyntype_is_extref(ctx, null));

    EXPECT_EQ(dyntype_set_prototype(ctx, null, dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_get_prototype(ctx, null), nullptr);
    EXPECT_EQ(dyntype_get_own_property(ctx, null, "has not property"), nullptr);

    dyntype_release(ctx, null);
}

TEST_F(TypesTest, create_string) {
    char const *check_values[] = {"", " ", "abcd", "123456", "字符串", "@#$%^&*)(*"};

    for (int i = 0; i < sizeof(check_values) / sizeof(check_values[0]); i++) {
        char *raw_value = nullptr;
        dyn_value_t str = dyntype_new_string(ctx, check_values[i]);
        EXPECT_NE(str, nullptr);
        EXPECT_EQ(dyntype_set_property(ctx, str, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_define_property(ctx, str, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_get_property(ctx, str, "not_a_object"), nullptr);
        EXPECT_EQ(dyntype_has_property(ctx, str, "not_a_object"), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_delete_property(ctx, str, "not_a_object"), -DYNTYPE_FALSE);
        EXPECT_FALSE(dyntype_is_number(ctx, str));
        EXPECT_FALSE(dyntype_is_bool(ctx, str));
        EXPECT_FALSE(dyntype_is_object(ctx, str));
        EXPECT_FALSE(dyntype_is_undefined(ctx, str));
        EXPECT_FALSE(dyntype_is_null(ctx, str));
        EXPECT_TRUE(dyntype_is_string(ctx, str));
        EXPECT_FALSE(dyntype_is_array(ctx, str));
        EXPECT_FALSE(dyntype_is_extref(ctx, str));

        dyntype_hold(ctx, str);
        dyntype_release(ctx, str);

        bool temp;
        double temp1;
        EXPECT_EQ(dyntype_to_bool(ctx, str, &temp), -DYNTYPE_TYPEERR);
        EXPECT_EQ(dyntype_to_number(ctx, str, &temp1), -DYNTYPE_TYPEERR);

        EXPECT_EQ(dyntype_to_cstring(ctx, str, &raw_value), DYNTYPE_SUCCESS);
        EXPECT_STREQ(raw_value, check_values[i]);
        dyntype_release(ctx, str);
        dyntype_free_cstring(ctx, raw_value);

    }
}

TEST_F(TypesTest, create_array) {

    dyn_value_t array = dyntype_new_array(ctx);
    EXPECT_NE(array, nullptr);

    EXPECT_FALSE(dyntype_is_number(ctx, array));
    EXPECT_FALSE(dyntype_is_bool(ctx, array));
    EXPECT_TRUE(dyntype_is_object(ctx, array));
    EXPECT_FALSE(dyntype_is_undefined(ctx, array));
    EXPECT_FALSE(dyntype_is_null(ctx, array));
    EXPECT_FALSE(dyntype_is_string(ctx, array));
    EXPECT_TRUE(dyntype_is_array(ctx, array));
    EXPECT_FALSE(dyntype_is_extref(ctx, array));

    dyntype_hold(ctx, array);
    dyntype_release(ctx, array);

    bool temp;
    double temp1;
    char *temp2;
    EXPECT_EQ(dyntype_to_bool(ctx, array, &temp), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_number(ctx, array, &temp1), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_cstring(ctx, array, &temp2), -DYNTYPE_TYPEERR);


    dyntype_release(ctx, array);
}

TEST_F(TypesTest, create_extern_ref) {
    void *obj = malloc(sizeof(uint32_t) * 10);
    void *func = malloc(sizeof(uint32_t) * 6);

    dyn_value_t extobj = dyntype_new_extref(ctx, obj, ExtObj);
    EXPECT_NE(extobj, nullptr);

    EXPECT_EQ(dyntype_set_property(ctx, extobj, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_define_property(ctx, extobj, "not_a_object", dyntype_new_boolean(ctx, false)), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_get_property(ctx, extobj, "not_a_object"), nullptr);
    EXPECT_EQ(dyntype_has_property(ctx, extobj, "not_a_object"), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_delete_property(ctx, extobj, "not_a_object"), -DYNTYPE_FALSE);

    dyn_value_t extobj1 = dyntype_new_extref(ctx, obj, (external_ref_tag)(ExtArray + 1));
    EXPECT_EQ(extobj1, nullptr);

    EXPECT_FALSE(dyntype_is_number(ctx, extobj));
    EXPECT_FALSE(dyntype_is_bool(ctx, extobj));
    EXPECT_FALSE(dyntype_is_object(ctx, extobj));
    EXPECT_FALSE(dyntype_is_undefined(ctx, extobj));
    EXPECT_FALSE(dyntype_is_null(ctx, extobj));
    EXPECT_FALSE(dyntype_is_string(ctx, extobj));
    EXPECT_FALSE(dyntype_is_array(ctx, extobj));
    EXPECT_TRUE(dyntype_is_extref(ctx, extobj));

    dyn_value_t extfunc = dyntype_new_extref(ctx, func, ExtFunc);
    EXPECT_NE(extfunc, nullptr);

    EXPECT_FALSE(dyntype_is_number(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_bool(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_object(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_undefined(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_null(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_string(ctx, extfunc));
    EXPECT_FALSE(dyntype_is_array(ctx, extfunc));
    EXPECT_TRUE(dyntype_is_extref(ctx, extfunc));
    void *temp_obj;
    EXPECT_NE(dyntype_to_extref(ctx, extobj, &temp_obj), -DYNTYPE_TYPEERR);
    EXPECT_NE(dyntype_to_extref(ctx, extfunc, &temp_obj), -DYNTYPE_TYPEERR);

    void *extref_obj = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extobj, &extref_obj), DYNTYPE_SUCCESS);
    free(extref_obj);

    void *extref_fun = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extfunc, &extref_fun), DYNTYPE_SUCCESS);
    free(extref_fun);

    dyntype_release(ctx, extobj);
    dyntype_release(ctx, extfunc);
}

TEST_F(TypesTest, create_object) {
    dyn_value_t obj = dyntype_new_object(ctx);
    EXPECT_NE(obj, nullptr);
    EXPECT_FALSE(dyntype_is_number(ctx, obj));
    EXPECT_FALSE(dyntype_is_bool(ctx, obj));
    EXPECT_TRUE(dyntype_is_object(ctx, obj));
    EXPECT_FALSE(dyntype_is_undefined(ctx, obj));
    EXPECT_FALSE(dyntype_is_null(ctx, obj));
    EXPECT_FALSE(dyntype_is_string(ctx, obj));
    EXPECT_FALSE(dyntype_is_array(ctx, obj));
    EXPECT_FALSE(dyntype_is_extref(ctx, obj));

    dyntype_hold(ctx, obj);
    dyntype_release(ctx, obj);

    bool temp;
    double temp1;
    char *temp2;
    EXPECT_EQ(dyntype_to_bool(ctx, obj, &temp), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_number(ctx, obj, &temp1), -DYNTYPE_TYPEERR);
    EXPECT_EQ(dyntype_to_cstring(ctx, obj, &temp2), -DYNTYPE_TYPEERR);

    /* Currently we need to manually release the object,
        after GC support finished, this line is not needed */
    dyntype_release(ctx, obj);
}
