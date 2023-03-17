/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include <gtest/gtest.h>

class ObjectPropertyTest : public testing::Test {
  protected:
    virtual void SetUp() {
        ctx = dyntype_context_init();
    }

    virtual void TearDown() {
        dyntype_context_destroy(ctx);
    }

    dyn_ctx_t ctx;
};

TEST_F(ObjectPropertyTest, object_set_and_has_and_get_property) {
    dyn_value_t obj = dyntype_new_object(ctx);

    void *extobj = malloc(sizeof(uint32_t) * 10);

    dyn_value_t num = dyntype_new_number(ctx, 2147483649);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "string");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop1", num), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop2", boolean), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop3", undefined), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop4", null), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop5", str), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop6", array), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop7", extref), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop8", obj1), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    dyn_value_t num_v = dyntype_get_property(ctx, obj, "prop1");
    double v = 0;
    dyntype_to_number(ctx, num_v, &v);
    EXPECT_EQ(v, 2147483649);
    dyntype_release(ctx, num_v);

    dyn_value_t boolean_v = dyntype_get_property(ctx, obj, "prop2");
    bool v1 = false;
    dyntype_to_bool(ctx, boolean_v, &v1);
    EXPECT_EQ(v1, true);

    dyn_value_t undefined_v = dyntype_get_property(ctx, obj, "prop3");
    EXPECT_TRUE(dyntype_is_undefined(ctx, undefined_v));

    dyn_value_t null_v = dyntype_get_property(ctx, obj, "prop4");
    EXPECT_TRUE(dyntype_is_null(ctx, null_v));

    dyn_value_t str_v = dyntype_get_property(ctx, obj, "prop5");
    char const *target = "string";
    char *v2 = nullptr;
    dyntype_to_cstring(ctx, str_v, &v2);
    EXPECT_STREQ(v2, target);
    dyntype_release(ctx, str_v);

    dyn_value_t array_v = dyntype_get_property(ctx, obj, "prop6");
    EXPECT_TRUE(dyntype_is_array(ctx, array_v));
    dyntype_release(ctx, array_v);

    dyn_value_t extref_v = dyntype_get_property(ctx, obj, "prop7");
    EXPECT_TRUE(dyntype_is_extref(ctx, extref_v));

    dyn_value_t obj1_v = dyntype_get_property(ctx, obj, "prop8");
    EXPECT_TRUE(dyntype_is_object(ctx, obj1_v));
    dyntype_release(ctx, obj1_v);


    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    dyntype_dump_value(ctx, obj);
    char *buffer = new char[1024 * 10];
    dyntype_dump_value_buffer(ctx, obj, buffer, 1024 * 10);
    printf("%s\n", buffer);

    delete buffer;

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    free(extref_prop);
    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
    dyntype_release(ctx, undefined);
    dyntype_release(ctx, null);
}

TEST_F(ObjectPropertyTest, object_define_and_has_and_get_property) {
    dyn_value_t obj = dyntype_new_object(ctx);
    void *extobj = malloc(sizeof(uint32_t) * 10);

    dyn_value_t num = dyntype_new_number(ctx, -10.1);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "  ");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    dyn_value_t desc1 = dyntype_new_object(ctx);
    dyn_value_t desc1_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc1, "configurable", desc1_v);
    dyntype_set_property(ctx, desc1, "value", num);

    dyn_value_t desc2 = dyntype_new_object(ctx);
    dyn_value_t desc2_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc2, "writable", desc2_v);
    dyntype_set_property(ctx, desc2, "value", boolean);

    dyn_value_t desc3 = dyntype_new_object(ctx);
    dyn_value_t desc3_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc3, "enumerable", desc3_v);
    dyntype_set_property(ctx, desc3, "value", undefined);

    dyn_value_t desc4 = dyntype_new_object(ctx);
    dyn_value_t desc4_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc4, "configurable", desc4_v);
    dyntype_set_property(ctx, desc4, "value", null);

    dyn_value_t desc5 = dyntype_new_object(ctx);
    dyn_value_t desc5_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc5, "writable", desc5_v);
    dyntype_set_property(ctx, desc5, "value", str);

    dyn_value_t desc6 = dyntype_new_object(ctx);
    dyn_value_t desc6_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc6, "enumerable", desc6_v);
    dyntype_set_property(ctx, desc6, "value", array);

    dyn_value_t desc7 = dyntype_new_object(ctx);
    dyn_value_t desc7_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc7, "configurable", desc7_v);
    dyntype_set_property(ctx, desc7, "value", extref);

    dyn_value_t desc8 = dyntype_new_object(ctx);
    dyn_value_t desc8_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc8, "writable", desc8_v);
    dyntype_set_property(ctx, desc8, "value", obj1);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop1", desc1), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop2", desc2), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop3", desc3), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop4", desc4), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop5", desc5), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop6", desc6), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop7", desc7), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop8", desc8), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop not a object", dyntype_new_boolean(ctx, false)),
              -DYNTYPE_TYPEERR);

    dyn_value_t num_v = dyntype_get_property(ctx, obj, "prop1");
    double v = 0;
    dyntype_to_number(ctx, num_v, &v);
    EXPECT_EQ(v, -10.1);
    dyntype_release(ctx, num_v);

    dyn_value_t boolean_v = dyntype_get_property(ctx, obj, "prop2");
    bool v1 = false;
    dyntype_to_bool(ctx, boolean_v, &v1);
    EXPECT_EQ(v1, true);

    dyn_value_t undefined_v = dyntype_get_property(ctx, obj, "prop3");
    EXPECT_TRUE(dyntype_is_undefined(ctx, undefined_v));

    dyn_value_t null_v = dyntype_get_property(ctx, obj, "prop4");
    EXPECT_TRUE(dyntype_is_null(ctx, null_v));

    dyn_value_t str_v = dyntype_get_property(ctx, obj, "prop5");
    char const *target = "  ";
    char *v2 = nullptr;
    dyntype_to_cstring(ctx, str_v, &v2);
    EXPECT_STREQ(v2, target);
    dyntype_release(ctx, str_v);

    dyn_value_t array_v = dyntype_get_property(ctx, obj, "prop6");
    EXPECT_TRUE(dyntype_is_array(ctx, array_v));
    dyntype_release(ctx, array_v);

    dyn_value_t extref_v = dyntype_get_property(ctx, obj, "prop7");
    EXPECT_TRUE(dyntype_is_extref(ctx, extref_v));

    dyn_value_t obj1_v = dyntype_get_property(ctx, obj, "prop8");
    EXPECT_TRUE(dyntype_is_object(ctx, obj1_v));
    dyntype_release(ctx, obj1_v);

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    free(extref_prop);

    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
    dyntype_release(ctx, undefined);
    dyntype_release(ctx, null);

    dyntype_release(ctx, desc1);
    dyntype_release(ctx, desc2);
    dyntype_release(ctx, desc3);
    dyntype_release(ctx, desc4);
    dyntype_release(ctx, desc5);
    dyntype_release(ctx, desc6);
    dyntype_release(ctx, desc7);
    dyntype_release(ctx, desc8);
}

TEST_F(ObjectPropertyTest, object_set_and_delete_property) {
   dyn_value_t obj = dyntype_new_object(ctx);

    void *extobj = malloc(sizeof(uint32_t) * 10);

    dyn_value_t num = dyntype_new_number(ctx, 2147483649);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "string");
    dyn_value_t array = dyntype_new_array(ctx);
    EXPECT_TRUE(dyntype_is_array(ctx, array));
    dyn_value_t extref = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop1", num), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop2", boolean), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop3", undefined), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop4", null), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop5", str), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop6", array), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop7", extref), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop8", obj1), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop1"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop2"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop3"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop4"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop5"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop6"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop7"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop8"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop9"), DYNTYPE_FALSE);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), DYNTYPE_FALSE);

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    free(extref_prop);

    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
    dyntype_release(ctx, undefined);
    dyntype_release(ctx, null);
}

TEST_F(ObjectPropertyTest, object_define_and_delete_property) {
    dyn_value_t obj = dyntype_new_object(ctx);
    void *extobj = malloc(sizeof(uint32_t) * 10);

    dyn_value_t num = dyntype_new_number(ctx, -10.1);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "  ");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, extobj, external_ref_tag::ExtObj);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    dyn_value_t desc1 = dyntype_new_object(ctx);
    dyn_value_t desc1_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc1, "configurable", desc1_v);
    dyntype_set_property(ctx, desc1, "value", num);

    dyn_value_t desc2 = dyntype_new_object(ctx);
    dyn_value_t desc2_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc2, "writable", desc2_v);
    dyntype_set_property(ctx, desc2, "value", boolean);

    dyn_value_t desc3 = dyntype_new_object(ctx);
    dyn_value_t desc3_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc3, "enumerable", desc3_v);
    dyntype_set_property(ctx, desc3, "value", undefined);

    dyn_value_t desc4 = dyntype_new_object(ctx);
    dyn_value_t desc4_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc4, "configurable", desc4_v);
    dyntype_set_property(ctx, desc4, "value", null);

    dyn_value_t desc5 = dyntype_new_object(ctx);
    dyn_value_t desc5_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc5, "writable", desc5_v);
    dyntype_set_property(ctx, desc5, "value", str);

    dyn_value_t desc6 = dyntype_new_object(ctx);
    dyn_value_t desc6_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc6, "enumerable", desc6_v);
    dyntype_set_property(ctx, desc6, "value", array);

    dyn_value_t desc7 = dyntype_new_object(ctx);
    dyn_value_t desc7_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc7, "configurable", desc7_v);
    dyntype_set_property(ctx, desc7, "value", extref);

    dyn_value_t desc8 = dyntype_new_object(ctx);
    dyn_value_t desc8_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc8, "writable", desc8_v);
    dyntype_set_property(ctx, desc8, "value", obj1);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop1", desc1), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop2", desc2), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop3", desc3), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop4", desc4), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop5", desc5), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop6", desc6), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop7", desc7), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop8", desc8), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop1"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop2"), DYNTYPE_FALSE); // writable
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop3"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop4"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop5"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop6"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop7"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop8"), DYNTYPE_FALSE);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), DYNTYPE_TRUE);

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    free(extref_prop);

    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
    dyntype_release(ctx, undefined);
    dyntype_release(ctx, null);

    dyntype_release(ctx, desc1);
    dyntype_release(ctx, desc2);
    dyntype_release(ctx, desc3);
    dyntype_release(ctx, desc4);
    dyntype_release(ctx, desc5);
    dyntype_release(ctx, desc6);
    dyntype_release(ctx, desc7);
    dyntype_release(ctx, desc8);
}
