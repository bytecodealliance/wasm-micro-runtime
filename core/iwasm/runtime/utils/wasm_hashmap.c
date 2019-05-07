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

#include "wasm_hashmap.h"
#include "wasm_log.h"
#include "wasm_thread.h"
#include "wasm_memory.h"


typedef struct HashMapElem {
    void *key;
    void *value;
    struct HashMapElem *next;
} HashMapElem;

struct HashMap {
    /* size of element array */
    uint32 size;
    /* lock for elements */
    korp_mutex *lock;
    /* hash function of key */
    HashFunc hash_func;
    /* key equal function */
    KeyEqualFunc key_equal_func;
    KeyDestroyFunc key_destroy_func;
    ValueDestroyFunc value_destroy_func;
    HashMapElem *elements[1];
};

HashMap*
wasm_hash_map_create(uint32 size, bool use_lock,
                     HashFunc hash_func,
                     KeyEqualFunc key_equal_func,
                     KeyDestroyFunc key_destroy_func,
                     ValueDestroyFunc value_destroy_func)
{
    HashMap *map;
    uint32 total_size;

    if (size > HASH_MAP_MAX_SIZE) {
        LOG_ERROR("HashMap create failed: size is too large.\n");
        return NULL;
    }

    if (!hash_func || !key_equal_func) {
        LOG_ERROR("HashMap create failed: hash function or key equal function "
                " is NULL.\n");
        return NULL;
    }

    total_size = offsetof(HashMap, elements) +
                 sizeof(HashMapElem) * size +
                 (use_lock ? sizeof(korp_mutex) : 0);

    if (!(map = wasm_malloc(total_size))) {
        LOG_ERROR("HashMap create failed: alloc memory failed.\n");
        return NULL;
    }

    memset(map, 0, total_size);

    if (use_lock) {
        map->lock = (korp_mutex*)
                    ((uint8*)map + offsetof(HashMap, elements) + sizeof(HashMapElem) * size);
        if (ws_mutex_init(map->lock, false)) {
            LOG_ERROR("HashMap create failed: init map lock failed.\n");
            wasm_free(map);
            return NULL;
        }
    }

    map->size = size;
    map->hash_func = hash_func;
    map->key_equal_func = key_equal_func;
    map->key_destroy_func = key_destroy_func;
    map->value_destroy_func = value_destroy_func;
    return map;
}

bool
wasm_hash_map_insert(HashMap *map, void *key, void *value)
{
    uint32 index;
    HashMapElem *elem;

    if (!map || !key) {
        LOG_ERROR("HashMap insert elem failed: map or key is NULL.\n");
        return false;
    }

    if (map->lock) {
        ws_mutex_lock(map->lock);
    }

    index = map->hash_func(key) % map->size;
    elem = map->elements[index];
    while (elem) {
        if (map->key_equal_func(elem->key, key)) {
            LOG_ERROR("HashMap insert elem failed: duplicated key found.\n");
            goto fail;
        }
        elem = elem->next;
    }

    if (!(elem = wasm_malloc(sizeof(HashMapElem)))) {
        LOG_ERROR("HashMap insert elem failed: alloc memory failed.\n");
        goto fail;
    }

    elem->key = key;
    elem->value = value;
    elem->next = map->elements[index];
    map->elements[index] = elem;

    if (map->lock) {
        ws_mutex_unlock(map->lock);
    }
    return true;

fail:
    if (map->lock) {
        ws_mutex_unlock(map->lock);
    }
    return false;
}

void*
wasm_hash_map_find(HashMap *map, void *key)
{
    uint32 index;
    HashMapElem *elem;
    void *value;

    if (!map || !key) {
        LOG_ERROR("HashMap find elem failed: map or key is NULL.\n");
        return NULL;
    }

    if (map->lock) {
        ws_mutex_lock(map->lock);
    }

    index = map->hash_func(key) % map->size;
    elem = map->elements[index];

    while (elem) {
        if (map->key_equal_func(elem->key, key)) {
            value = elem->value;
            if (map->lock) {
                ws_mutex_unlock(map->lock);
            }
            return value;
        }
        elem = elem->next;
    }

    if (map->lock) {
        ws_mutex_unlock(map->lock);
    }
    return NULL;
}

bool
wasm_hash_map_update(HashMap *map, void *key, void *value,
                     void **p_old_value)
{
    uint32 index;
    HashMapElem *elem;

    if (!map || !key) {
        LOG_ERROR("HashMap update elem failed: map or key is NULL.\n");
        return false;
    }

    if (map->lock) {
        ws_mutex_lock(map->lock);
    }

    index = map->hash_func(key) % map->size;
    elem = map->elements[index];

    while (elem) {
        if (map->key_equal_func(elem->key, key)) {
            if (p_old_value)
                *p_old_value = elem->value;
            elem->value = value;
            if (map->lock) {
                ws_mutex_unlock(map->lock);
            }
            return true;
    }
    elem = elem->next;
    }

    if (map->lock) {
        ws_mutex_unlock(map->lock);
    }
    return false;
}

bool
wasm_hash_map_remove(HashMap *map, void *key,
                     void **p_old_key, void **p_old_value)
{
    uint32 index;
    HashMapElem *elem, *prev;

    if (!map || !key) {
        LOG_ERROR("HashMap remove elem failed: map or key is NULL.\n");
        return false;
    }

    if (map->lock) {
        ws_mutex_lock(map->lock);
    }

    index = map->hash_func(key) % map->size;
    prev = elem = map->elements[index];

    while (elem) {
        if (map->key_equal_func(elem->key, key)) {
            if (p_old_key)
                *p_old_key = elem->key;
            if (p_old_value)
                *p_old_value = elem->value;

            if (elem == map->elements[index])
                map->elements[index] = elem->next;
            else
                prev->next = elem->next;

            wasm_free(elem);

            if (map->lock) {
                ws_mutex_unlock(map->lock);
            }
            return true;
        }

        prev = elem;
        elem = elem->next;
    }

    if (map->lock) {
        ws_mutex_unlock(map->lock);
    }
    return false;
}

bool
wasm_hash_map_destroy(HashMap *map)
{
    uint32 index;
    HashMapElem *elem, *next;

    if (!map) {
        LOG_ERROR("HashMap destroy failed: map is NULL.\n");
        return false;
    }

    if (map->lock) {
        ws_mutex_lock(map->lock);
    }

    for (index = 0; index < map->size; index++) {
        elem = map->elements[index];
        while (elem) {
            next = elem->next;

            if (map->key_destroy_func) {
                map->key_destroy_func(elem->key);
            }
            if (map->value_destroy_func) {
                map->value_destroy_func(elem->value);
            }
            wasm_free(elem);

            elem = next;
        }
    }

    if (map->lock) {
        ws_mutex_unlock(map->lock);
        ws_mutex_destroy(map->lock);
    }
    wasm_free(map);
    return true;
}
