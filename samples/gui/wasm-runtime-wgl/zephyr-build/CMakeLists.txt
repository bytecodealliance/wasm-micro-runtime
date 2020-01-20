# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

set (WAMR_BUILD_PLATFORM "zephyr")

enable_language (ASM)

zephyr_compile_definitions (-DWAMR_BUILD_GUI=1)

# Build as THUMB by default
# change to "ARM[sub]", "THUMB[sub]", "X86_32", "MIPS" or "XTENSA"
# if we want to support arm_32, x86, mips or xtensa
if (NOT DEFINED WAMR_BUILD_TARGET)
  set (WAMR_BUILD_TARGET "THUMB")
endif ()


if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Disable AOT by default.
  set (WAMR_BUILD_AOT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_JIT)
  # Disable JIT by default.
  set (WAMR_BUILD_JIT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Disable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/wamr)
set (IWASM_DIR  ${WAMR_ROOT_DIR}/core/iwasm)
set (APP_MGR_DIR ${WAMR_ROOT_DIR}/core/app-mgr)
set (SHARED_DIR ${WAMR_ROOT_DIR}/core/shared)
set (APP_FRAMEWORK_DIR ${WAMR_ROOT_DIR}/core/app-framework)

# include the build config template file
include (${WAMR_ROOT_DIR}/build-scripts/config_common.cmake)

include_directories (${SHARED_DIR}/include
                     ${IWASM_DIR}/include
                     ${APP_FRAMEWORK_DIR}/app-native-shared/bi-inc/deps)

include (${SHARED_DIR}/platform/${WAMR_BUILD_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
if (WAMR_BUILD_LIBC_BUILTIN EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-builtin/libc_builtin.cmake)
endif ()
if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-wasi/libc_wasi.cmake)
endif ()
include (${APP_FRAMEWORK_DIR}/base/native/wasm_lib_base.cmake)

include (${IWASM_DIR}/common/iwasm_common.cmake)

if (WAMR_BUILD_INTERP EQUAL 1 OR WAMR_BUILD_JIT EQUAL 1)
  include (${IWASM_DIR}/interpreter/iwasm_interp.cmake)
endif ()

if (WAMR_BUILD_AOT EQUAL 1)
  include (${IWASM_DIR}/aot/iwasm_aot.cmake)
  if (WAMR_BUILD_JIT EQUAL 1)
    include (${IWASM_DIR}/compilation/iwasm_compl.cmake)
  endif ()
endif ()

include (${APP_FRAMEWORK_DIR}/sensor/native/wasm_lib_sensor.cmake)
include (${APP_FRAMEWORK_DIR}/wgl/native/wasm_lib_gui.cmake)
include (${APP_FRAMEWORK_DIR}/connection/native/wasm_lib_conn.cmake)
include (${APP_FRAMEWORK_DIR}/connection/native/${WAMR_BUILD_PLATFORM}/connection_mgr.cmake)
include (${APP_FRAMEWORK_DIR}/app-native-shared/native_interface.cmake)
include (${APP_MGR_DIR}/app-manager/app_mgr.cmake)
include (${APP_MGR_DIR}/app-mgr-shared/app_mgr_shared.cmake)
include (${SHARED_DIR}/coap/lib_coap.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr)

set (LVGL_DRV_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340_adafruit_1480.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/XPT2046.c
    )

set (IWASM_SRCS
     ${PLATFORM_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE}
     ${LIBC_BUILTIN_SOURCE}
     ${LIBC_WASI_SOURCE}
     ${IWASM_COMMON_SOURCE}
     ${IWASM_INTERP_SOURCE}
     ${IWASM_AOT_SOURCE}
     ${IWASM_COMPL_SOURCE}
     ${WASM_LIB_BASE_SOURCE}
     ${WASM_LIB_SENSOR_SOURCE}
     ${WASM_LIB_GUI_SOURCE}
     ${WASM_LIB_CONN_SOURCE}
     ${WASM_LIB_CONN_MGR_SOURCE}
     ${NATIVE_INTERFACE_SOURCE}
     ${APP_MGR_SOURCE}
    )

target_sources(app PRIVATE
               ${IWASM_SRCS}
               ${LVGL_DRV_SRCS}
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/main.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/iwasm_main.c
               ${CMAKE_CURRENT_SOURCE_DIR}/../src/ext_lib_export.c
              )
