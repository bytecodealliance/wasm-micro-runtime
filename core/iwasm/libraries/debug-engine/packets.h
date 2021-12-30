/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef PACKETS_H
#define PACKETS_H

#include "gdbserver.h"

bool
read_packet(WASMGDBServer *gdbserver);

void
write_packet(WASMGDBServer *gdbserver, const char *data);

void
inbuf_erase_head(WASMGDBServer *gdbserver, ssize_t end);

#endif
