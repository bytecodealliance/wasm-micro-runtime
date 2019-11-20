/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_log.h"
#include "wasm_vector.h"
#include "wasm_memory.h"


static uint8*
alloc_vector_data(uint32 length, uint32 size_elem)
{
    uint64 total_size = ((uint64)size_elem) * length;
    uint8 *data;

    if (total_size > UINT32_MAX) {
        return NULL;
    }

    if ((data = wasm_malloc((uint32)total_size))) {
        memset(data, 0, (uint32)total_size);
    }

    return data;
}

static bool
extend_vector(Vector *vector, uint32 length)
{
    uint8 *data;

    if (length <= vector->max_elements)
        return true;

    if (length < vector->size_elem * 3 / 2)
        length = vector->size_elem * 3 / 2;

    if (!(data = alloc_vector_data(length, vector->size_elem))) {
        return false;
    }

    memcpy(data, vector->data, vector->size_elem * vector->max_elements);
    wasm_free(vector->data);
    vector->data = data;
    vector->max_elements = length;
    return true;
}

bool
wasm_vector_init(Vector *vector, uint32 init_length, uint32 size_elem)
{
    if (!vector) {
        LOG_ERROR("Init vector failed: vector is NULL.\n");
        return false;
    }

    if (init_length == 0) {
        init_length = 4;
    }

    if (!(vector->data = alloc_vector_data(init_length, size_elem))) {
        LOG_ERROR("Init vector failed: alloc memory failed.\n");
        return false;
    }

    vector->size_elem = size_elem;
    vector->max_elements = init_length;
    vector->num_elements = 0;
    return true;
}

bool
wasm_vector_set(Vector *vector, uint32 index, const void *elem_buf)
{
    if (!vector || !elem_buf) {
        LOG_ERROR("Set vector elem failed: vector or elem buf is NULL.\n");
        return false;
    }

    if (index >= vector->num_elements) {
        LOG_ERROR("Set vector elem failed: invalid elem index.\n");
        return false;
    }

    memcpy(vector->data + vector->size_elem * index,
            elem_buf, vector->size_elem);
    return true;
}

bool wasm_vector_get(const Vector *vector, uint32 index, void *elem_buf)
{
    if (!vector || !elem_buf) {
        LOG_ERROR("Get vector elem failed: vector or elem buf is NULL.\n");
        return false;
    }

    if (index >= vector->num_elements) {
        LOG_ERROR("Get vector elem failed: invalid elem index.\n");
        return false;
    }

    memcpy(elem_buf, vector->data + vector->size_elem * index,
           vector->size_elem);
    return true;
}

bool wasm_vector_insert(Vector *vector, uint32 index, const void *elem_buf)
{
    uint32 i;
    uint8 *p;

    if (!vector || !elem_buf) {
        LOG_ERROR("Insert vector elem failed: vector or elem buf is NULL.\n");
        return false;
    }

    if (index >= vector->num_elements) {
        LOG_ERROR("Insert vector elem failed: invalid elem index.\n");
        return false;
    }

    if (!extend_vector(vector, vector->num_elements + 1)) {
        LOG_ERROR("Insert vector elem failed: extend vector failed.\n");
        return false;
    }

    p = vector->data + vector->size_elem * vector->num_elements;
    for (i = vector->num_elements - 1; i > index; i--) {
        memcpy(p, p - vector->size_elem, vector->size_elem);
        p -= vector->size_elem;
    }

    memcpy(p, elem_buf, vector->size_elem);
    vector->num_elements++;
    return true;
}

bool wasm_vector_append(Vector *vector, const void *elem_buf)
{
    if (!vector || !elem_buf) {
        LOG_ERROR("Append vector elem failed: vector or elem buf is NULL.\n");
        return false;
    }

    if (!extend_vector(vector, vector->num_elements + 1)) {
        LOG_ERROR("Append ector elem failed: extend vector failed.\n");
        return false;
    }

    memcpy(vector->data + vector->size_elem * vector->num_elements,
           elem_buf, vector->size_elem);
    vector->num_elements++;
    return true;
}

bool
wasm_vector_remove(Vector *vector, uint32 index, void *old_elem_buf)
{
    uint32 i;
    uint8 *p;

    if (!vector) {
        LOG_ERROR("Remove vector elem failed: vector is NULL.\n");
        return false;
    }

    if (index >= vector->num_elements) {
        LOG_ERROR("Remove vector elem failed: invalid elem index.\n");
        return false;
    }

    p = vector->data + vector->size_elem * index;

    if (old_elem_buf) {
        memcpy(old_elem_buf, p, vector->size_elem);
    }

    for (i = index; i < vector->num_elements - 1; i++) {
        memcpy(p, p + vector->size_elem, vector->size_elem);
        p += vector->size_elem;
    }

    vector->num_elements--;
    return true;
}

uint32
wasm_vector_size(const Vector *vector)
{
    return vector ? vector->num_elements : 0;
}

bool
wasm_vector_destroy(Vector *vector)
{
    if (!vector) {
        LOG_ERROR("Destroy vector elem failed: vector is NULL.\n");
        return false;
    }

    if (vector->data)
        wasm_free(vector->data);
    memset(vector, 0, sizeof(Vector));
    return true;
}
