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

#ifndef _WASM_EXPORT_API_H
#define _WASM_EXPORT_API_H

#include <inttypes.h>
#include <stdbool.h>

/**
 * API exported to WASM application
 */

#ifdef __cplusplus
extern "C" {
#endif

void
wasm_runtime_get_current_module_inst1(uint64_t *p_module_inst);

bool
wasm_runtime_validate_app_addr1(uint32_t module_inst_part0,
                                uint32_t module_inst_part1,
                                int32_t app_offset, uint32_t size);
bool
wasm_runtime_validate_native_addr1(uint32_t module_inst_part0,
                                   uint32_t module_inst_part1,
                                   uint32_t native_ptr_part0,
                                   uint32_t native_ptr_part1,
                                   uint32_t size);
bool
wasm_runtime_addr_app_to_native1(uint32_t module_inst_part0,
                                 uint32_t module_inst_part1,
                                 int32_t app_offset,
                                 uint64_t *p_native_ptr);

int32_t
wasm_runtime_addr_native_to_app1(uint32_t module_inst_part0,
                                 uint32_t module_inst_part1,
                                 uint32_t native_ptr_part0,
                                 uint32_t native_ptr_part1);

/**
 * Get current WASM module instance of the current native thread
 *
 * @return current WASM module instance of the current native thread, 0
 *         if not found
 * Note: the return type is uint64_t but not pointer type, because that
 *       the we only supports WASM-32, in which the pointer type is
 *       compiled to WASM i32 type, but the pointer type in native can be
 *       32-bit and 64-bit. And if the native pointer is 64-bit, data loss
 *       occurs after converting it to WASM i32 type.
 */
static inline uint64_t
wasm_runtime_get_current_module_inst()
{
    uint64_t module_inst;
    wasm_runtime_get_current_module_inst1(&module_inst);
    return module_inst;
}

/**
 * Validate the app address, check whether it belongs to WASM module
 * instance's address space, or in its heap space or memory space.
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app address to validate, which is a relative address
 * @param size the size bytes of the app address
 *
 * @return true if success, false otherwise.
 */
static inline bool
wasm_runtime_validate_app_addr(uint64_t module_inst,
                               int32_t app_offset, uint32_t size)
{
    union { uint64_t val; uint32_t parts[2]; } u;
    u.val = module_inst;
    return wasm_runtime_validate_app_addr1(u.parts[0], u.parts[1],
                                           app_offset, size);
}

/**
 * Validate the native address, check whether it belongs to WASM module
 * instance's address space, or in its heap space or memory space.
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address to validate, which is an absolute
 *        address
 * @param size the size bytes of the app address
 *
 * @return true if success, false otherwise.
 */
static inline bool
wasm_runtime_validate_native_addr(uint64_t module_inst,
                                  uint64_t native_ptr, uint32_t size)
{
    union { uint64_t val; uint32_t parts[2]; } u1, u2;
    u1.val = module_inst;
    u2.val = native_ptr;
    return wasm_runtime_validate_native_addr1(u1.parts[0], u1.parts[1],
                                              u2.parts[0], u2.parts[1],
                                              size);
}

/**
 * Convert app address(relative address) to native address(absolute address)
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app adress
 *
 * @return the native address converted
 */
static inline uint64_t
wasm_runtime_addr_app_to_native(uint64_t module_inst,
                                int32_t app_offset)
{
    union { uint64_t val; uint32_t parts[2]; } u;
    uint64_t native_ptr;
    u.val = module_inst;
    if (!wasm_runtime_addr_app_to_native1(u.parts[0], u.parts[1],
                                          app_offset, &native_ptr))
        return 0;
    return native_ptr;
}

/**
 * Convert native address(absolute address) to app address(relative address)
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address
 *
 * @return the app address converted
 */
static inline int32_t
wasm_runtime_addr_native_to_app(uint64_t module_inst,
                                uint64_t native_ptr)
{
    union { uint64_t val; uint32_t parts[2]; } u1, u2;
    u1.val = module_inst;
    u2.val = native_ptr;
    return wasm_runtime_addr_native_to_app1(u1.parts[0], u1.parts[1],
                                            u2.parts[0], u2.parts[1]);
}

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_EXPORT_API_H */
