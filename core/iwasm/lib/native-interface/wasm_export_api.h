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

#ifndef _WASM_EXPORT_H
#define _WASM_EXPORT_H

#include <inttypes.h>
#include <stdbool.h>

/**
 * API exported to WASM application
 */

#ifdef __cplusplus
extern "C" {
#endif

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
uint64_t
wasm_runtime_get_current_module_inst();

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
bool
wasm_runtime_validate_app_addr(uint64_t module_inst,
                               int32_t app_offset, uint32_t size);

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
bool
wasm_runtime_validate_native_addr(uint64_t module_inst,
                                  uint64_t native_ptr, uint32_t size);

/**
 * Convert app address(relative address) to native address(absolute address)
 *
 * @param module_inst the WASM module instance
 * @param app_offset the app adress
 *
 * @return the native address converted
 */
uint64_t
wasm_runtime_addr_app_to_native(uint64_t module_inst,
                                int32_t app_offset);

/**
 * Convert native address(absolute address) to app address(relative address)
 *
 * @param module_inst the WASM module instance
 * @param native_ptr the native address
 *
 * @return the app address converted
 */
int32_t
wasm_runtime_addr_native_to_app(uint64_t module_inst,
                                uint64_t native_ptr);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_EXPORT_H */
