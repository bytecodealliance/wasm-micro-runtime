/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WASM_VECTOR_H
#define _WASM_VECTOR_H

#include "wasm_platform.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_VECTOR_INIT_SIZE 8

typedef struct Vector {
    /* size of each element */
    uint32 size_elem;
    /* max element number */
    uint32 max_elements;
    /* current element num */
    uint32 num_elements;
    /* vector data allocated */
    uint8 *data;
} Vector;

/**
 * Initialize vector
 *
 * @param vector the vector to init
 * @param init_length the initial length of the vector
 * @param size_elem size of each element
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_init(Vector *vector, uint32 init_length, uint32 size_elem);

/**
 * Set element of vector
 *
 * @param vector the vector to set
 * @param index the index of the element to set
 * @param elem_buf the element buffer which stores the element data
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_set(Vector *vector, uint32 index, const void *elem_buf);

/**
 * Get element of vector
 *
 * @param vector the vector to get
 * @param index the index of the element to get
 * @param elem_buf the element buffer to store the element data,
 *                 whose length must be no less than element size
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_get(const Vector *vector, uint32 index, void *elem_buf);

/**
 * Insert element of vector
 *
 * @param vector the vector to insert
 * @param index the index of the element to insert
 * @param elem_buf the element buffer which stores the element data
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_insert(Vector *vector, uint32 index, const void *elem_buf);

/**
 * Append element to the end of vector
 *
 * @param vector the vector to append
 * @param elem_buf the element buffer which stores the element data
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_append(Vector *vector, const void *elem_buf);

/**
 * Remove element from vector
 *
 * @param vector the vector to remove element
 * @param index the index of the element to remove
 * @param old_elem_buf if not NULL, copies the element data to the buffer
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_remove(Vector *vector, uint32 index, void *old_elem_buf);

/**
 * Return the size of the vector
 *
 * @param vector the vector to get size
 *
 * @return return the size of the vector
 */
uint32
wasm_vector_size(const Vector *vector);

/**
 * Destroy the vector
 *
 * @param vector the vector to destroy
 *
 * @return true if success, false otherwise
 */
bool
wasm_vector_destroy(Vector *vector);

#ifdef __cplusplus
}
#endif

#endif /* endof _WASM_VECTOR_H */

