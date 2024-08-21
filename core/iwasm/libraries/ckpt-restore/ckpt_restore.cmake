# Copyright (C) 2021 Ant Group.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (CKPT_RESTORE_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_CHECKPOINT_RESTORE=1)

include(FetchContent)
message ("-- Fetching yalantinglibs ..")
FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG 3b73dfa989b440851dfd7b7265ac09847b68e732
)
FetchContent_Populate(yalantinglibs)
FetchContent_GetProperties(yalantinglibs)
include_directories("${yalantinglibs_SOURCE_DIR}/include")

include_directories(${CKPT_RESTORE_DIR})

file (GLOB source_all ${CKPT_RESTORE_DIR}/*.cpp)

set (CKPT_RESTORE_SOURCE ${source_all})