/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _ELF_PARSERE_H_
#define _ELF_PARSER_H_
void
initial_jit_debug_engine();

void
create_jit_code_entry(const uint8_t *symfile_addr, uint64_t symfile_size);

void
destroy_jit_code_entry(const uint8_t *symfile_addr);
#endif