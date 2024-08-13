/*
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _DYNLINK_SECTION_LOADER_H
#define _DYNLINK_SECTION_LOADER_H

#include "wasm.h"
#include "dynlink_types.h"

bool
dynlink_try_load_dylink0_section(const uint8 *buf, const uint8 *buf_end,
                                 uint32 section_name_len, WASMModule *module,
                                 bool is_load_from_file_buf, char *error_buf,
                                 uint32 error_buf_size);
#endif