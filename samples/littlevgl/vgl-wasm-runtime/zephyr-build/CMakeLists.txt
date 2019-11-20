# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

zephyr_compile_definitions (-DNVALGRIND
    -D__JLF__
    -D__ZEPHYR__
    -DWASM_ENABLE_BASE_LIB
    -Dattr_container_malloc=bh_malloc
    -Dattr_container_free=bh_free)

# Build as General by default, change to "ARM_32", "X86_32", "MIPS_32" or "XTENSA_32"
# if we want to support arm_32, x86, mips or xtensa
if (NOT DEFINED BUILD_TARGET)
  set (BUILD_TARGET "General")
endif ()

string(TOUPPER ${BUILD_TARGET} BUILD_TARGET)

if (BUILD_TARGET STREQUAL "X86_32")
  add_definitions(-DBUILD_TARGET_X86_32)
elseif (BUILD_TARGET STREQUAL "ARM_32")
  add_definitions(-DBUILD_TARGET_ARM_32)
elseif (BUILD_TARGET STREQUAL "MIPS_32")
  add_definitions(-DBUILD_TARGET_MIPS_32)
elseif (BUILD_TARGET STREQUAL "XTENSA_32")
  add_definitions(-DBUILD_TARGET_XTENSA_32)
elseif (BUILD_TARGET STREQUAL "GENERAL")
  # Will use invokeNative_general.c instead of assembly code,
  # but the maximum number of native arguments is limited to 20,
  # and there are possible issues when passing arguments to
  # native function for some cpus, e.g. int64 and double arguments
  # in arm and mips need to be 8-bytes aligned, and some arguments
  # of x86_64 are passed by registers but not stack
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")
set (WASM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core/iwasm)
set (APP_MGR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core/app-mgr)
set (SHARED_DIR ${WASM_DIR}/../shared-lib)

set (TARGET_PLATFORM "zephyr")

include (${WASM_DIR}/runtime/platform/${TARGET_PLATFORM}/platform.cmake)
include (${WASM_DIR}/runtime/utils/utils.cmake)
include (${WASM_DIR}/runtime/vmcore-wasm/vmcore.cmake)
include (${WASM_DIR}/lib/native/base/wasm_lib_base.cmake)
include (${WASM_DIR}/lib/native/libc/wasm_libc.cmake)
include (${WASM_DIR}/lib/native/extension/sensor/wasm_lib_sensor.cmake)
include (${WASM_DIR}/lib/native/extension/connection/wasm_lib_conn.cmake)
include (${WASM_DIR}/lib/native/extension/connection/${TARGET_PLATFORM}/connection_mgr.cmake)
include (${WASM_DIR}/lib/native-interface/native_interface.cmake)
include (${APP_MGR_DIR}/app-manager/app_mgr.cmake)
include (${APP_MGR_DIR}/app-mgr-shared/app_mgr_shared.cmake)
include (${SHARED_DIR}/platform/${TARGET_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/coap/lib_coap.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr)

set (LVGL_DRV_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340_adafruit_1480.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_indev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/XPT2046.c
    )

set (IWASM_SRCS
     ${WASM_PLATFORM_LIB_SOURCE}
     ${WASM_UTILS_LIB_SOURCE}
     ${VMCORE_LIB_SOURCE}
     ${WASM_LIBC_SOURCE}
     ${APP_MGR_SOURCE}
     ${WASM_LIB_BASE_SOURCE}
     ${WASM_LIB_EXT_SOURCE}
     ${WASM_LIB_SENSOR_SOURCE}
     ${WASM_LIB_CONN_SOURCE}
     ${WASM_LIB_CONN_MGR_SOURCE}
     ${PLATFORM_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${NATIVE_INTERFACE_SOURCE}
    )
target_sources(app PRIVATE
               ${IWASM_SRCS}
               ${LVGL_DRV_SRCS}
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/main.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/iwasm_main.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/ext_lib_export.c
              )
