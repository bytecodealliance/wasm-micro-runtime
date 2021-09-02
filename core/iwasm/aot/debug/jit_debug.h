/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _ELF_PARSER_H_
#define _ELF_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

bool
init_jit_debug_engine();

bool
create_jit_code_entry(const uint8 *symfile_addr, uint64 symfile_size);

void
destroy_jit_code_entry(const uint8 *symfile_addr);

#ifdef __cplusplus
}
#endif

#endif