# Copyright (C) 2021 Ant Group.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (CKPT_RESTORE_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_CHECKPOINT_RESTORE=1)

include_directories(${CKPT_RESTORE_DIR})

file (GLOB source_all ${CKPT_RESTORE_DIR}/*.cpp)

set (CKPT_RESTORE_SOURCE ${source_all})