# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# base/ is the default feature set and runs in every mode; dump-call-stack/
# needs one feature switch.  Both are added before the early return below, a
# return would skip every statement after it.
add_subdirectory(base)
add_subdirectory(dump-call-stack)

# basic.cc is the historical suite and only carries interpreter-format
# fixtures, so it stays restricted to classic-interp.
wamr_unit_test_suite_run_modes(wasm-c-api MODES classic-interp)
if(NOT WAMR_UNIT_TEST_SUITE_ENABLED)
  return()
endif()

include(../unit_common.cmake)

# Only the sources of this directory: the nested base/ suite globs its own
# files, and a recursive glob here would compile them into this target too.
file(GLOB source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
)

add_executable(wasm_c_api_test ${unit_test_sources})
target_link_libraries(wasm_c_api_test gtest_main)
gtest_discover_tests(wasm_c_api_test)
