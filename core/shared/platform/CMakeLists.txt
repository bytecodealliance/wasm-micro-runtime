# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include_directories (./include ../include ./${WAMR_BUILD_PLATFORM})

add_definitions (-D__POSIX__ -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE)

file (GLOB_RECURSE source_all ${WAMR_BUILD_PLATFORM}/*.c)
add_library (supportlib ${source_all})

target_link_libraries (supportlib -pthread -lrt)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 add_library (supportlib_ut ${source_all})

 set_target_properties (supportlib_ut PROPERTIES COMPILE_DEFINITIONS BH_TEST=1)

 target_link_libraries (supportlib_ut -pthread -lrt)
endif ()

