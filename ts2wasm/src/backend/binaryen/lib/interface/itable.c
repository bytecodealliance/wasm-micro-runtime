/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

/*
 name: field name
 flag: field type, function or property
 index: field index in shape
 */
typedef struct ItableField {
    char *name;
    int flag; // 0: field, 1: method 2: getter 3: setter
    int index;
} ItableField;

/*
 id: type id
 size: field size
 itable_field: field array
*/
typedef struct Itable {
    int id;
    int size;
    ItableField fields[0];
} Itable;

/* find field index based on prop_name*/
int find_index(Itable *table, char *prop_name, int flag) {
    for (int i = 0; i < table->size; i++) {
        if (strcmp(table->fields[i].name, prop_name) == 0 && table->fields[i].flag == flag) {
            return table->fields[i].index;
        }
    }
    return -1;
}
