# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wamr_user_mode LANGUAGES C)

# Option: use pre-built library integration path.
# The library is still compiled from source under lib-wamr-zephyr/, but instead
# of relying on zephyr_library_app_memory() to register the partition, the
# resulting .a is imported as a pre-built library and the partition is registered
# manually via set_property(). This demonstrates how to integrate a WAMR library
# that was built externally (e.g. copied from another build).
#
# Usage: west build ... -- -DWAMR_USE_PREBUILT_LIB=1
option(WAMR_USE_PREBUILT_LIB
       "Use pre-built library approach for app_smem partition registration" OFF)

# Always build the library from source, but conditionally skip
# zephyr_library_app_memory() inside the subdirectory when using pre-built mode.
add_subdirectory(lib-wamr-zephyr)

if(WAMR_USE_PREBUILT_LIB)
  # Manually replicate what zephyr_library_app_memory(wamr_partition) does:
  # tell gen_app_partitions.py to place this library's globals into wamr_partition.
  set_property(TARGET zephyr_property_target
               APPEND PROPERTY COMPILE_OPTIONS
               "-l" "libwamr_lib.a" "wamr_partition")

  message(STATUS "WAMR: using pre-built library approach for partition registration")
endif()

# Link the wamr library to the app target
target_link_libraries(app PRIVATE wamr_lib)

target_sources(app PRIVATE src/main.c)
