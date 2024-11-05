# Copyright (C) 2024 Amazon Inc.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# simde is a header only library

set (LIB_SIMDE_DIR ${CMAKE_CURRENT_LIST_DIR})

if (WAMR_BUILD_TARGET MATCHES "AARCH64.*" OR "ARM.*")
  add_definitions (-DWASM_ENABLE_SIMDE=1)
endif ()

include_directories(${LIB_SIMDE_DIR} ${LIB_SIMDE_DIR}/simde)

include(FetchContent)

FetchContent_Declare(
    simde
    GIT_REPOSITORY  https://github.com/simd-everywhere/simde
    GIT_TAG v0.8.2
)

message("-- Fetching simde ..")
FetchContent_MakeAvailable(simde)
include_directories("${simde_SOURCE_DIR}")
