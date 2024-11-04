# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)
add_executable(import_table main.c ${UNCOMMON_SHARED_SOURCE})
target_link_libraries(import_table PRIVATE vmlib)

add_test(NAME Test_import_table
  COMMAND ./import-table/import_table
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)