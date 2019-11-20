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

# Build as ARM_32 by default, change to "X86_32", "MIPS_32" or "XTENSA_32"
# if we want to support x86, mips or xtensa
if (NOT BUILD_TARGET)
  set (BUILD_TARGET "ARM_32")
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
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")
set (IWASM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/core/iwasm)
set (APP_MGR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/core/app-mgr)
set (SHARED_LIB_ROOT ${IWASM_ROOT}/../shared-lib)

target_include_directories(app PRIVATE ${IWASM_ROOT}/runtime/include
    ${IWASM_ROOT}/runtime/platform/zephyr
    ${IWASM_ROOT}/runtime/platform/include
    ${IWASM_ROOT}/runtime/utils
    ${IWASM_ROOT}/runtime/vmcore-wasm
    ${IWASM_ROOT}/lib/native/base
    ${IWASM_ROOT}/lib/native/libc
    ${IWASM_ROOT}/lib/native/extension/sensor
    ${IWASM_ROOT}/lib/native/extension/connection
    ${IWASM_ROOT}/lib/native-interface
    ${APP_MGR_ROOT}/app-manager
    ${APP_MGR_ROOT}/app-mgr-shared
    ${SHARED_LIB_ROOT}/include
    ${SHARED_LIB_ROOT}/platform/include
    ${SHARED_LIB_ROOT}/platform/zephyr
    ${SHARED_LIB_ROOT}/mem-alloc/ems
    ${SHARED_LIB_ROOT}/utils
    ${SHARED_LIB_ROOT}/coap/er-coap
    ${SHARED_LIB_ROOT}/coap/extension
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr
    )

set (IWASM_SRCS
    ${IWASM_ROOT}/runtime/platform/zephyr/wasm_native.c
    ${IWASM_ROOT}/runtime/utils/wasm_dlfcn.c
    ${IWASM_ROOT}/runtime/utils/wasm_hashmap.c
    ${IWASM_ROOT}/runtime/utils/wasm_log.c
    ${IWASM_ROOT}/runtime/utils/wasm_vector.c
    ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_application.c
    ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_interp.c
    ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_loader.c
    ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_runtime.c
    ${IWASM_ROOT}/runtime/vmcore-wasm/invokeNative_general.c
    ${IWASM_ROOT}/lib/native/base/base_lib_export.c
    ${IWASM_ROOT}/lib/native/base/request_response.c
    ${IWASM_ROOT}/lib/native/base/timer_wrapper.c
    ${IWASM_ROOT}/lib/native/libc/libc_wrapper.c
    ${IWASM_ROOT}/lib/native/extension/sensor/runtime_sensor.c
    ${IWASM_ROOT}/lib/native/extension/connection/connection_wrapper.c
    ${IWASM_ROOT}/lib/native/extension/connection/zephyr/connection_lib_impl.c
    ${IWASM_ROOT}/lib/native-interface/attr_container.c
    ${IWASM_ROOT}/lib/native-interface/restful_utils.c
    ${APP_MGR_ROOT}/app-manager/app_manager.c
    ${APP_MGR_ROOT}/app-manager/app_manager_host.c
    ${APP_MGR_ROOT}/app-manager/ble_msg.c
    ${APP_MGR_ROOT}/app-manager/event.c
    ${APP_MGR_ROOT}/app-manager/message.c
    ${APP_MGR_ROOT}/app-manager/module_jeff.c
    ${APP_MGR_ROOT}/app-manager/module_utils.c
    ${APP_MGR_ROOT}/app-manager/module_wasm_app.c
    ${APP_MGR_ROOT}/app-manager/module_wasm_lib.c
    ${APP_MGR_ROOT}/app-manager/resource_reg.c
    ${APP_MGR_ROOT}/app-manager/watchdog.c
    ${APP_MGR_ROOT}/app-manager/platform/zephyr/app_mgr_zephyr.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_assert.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_definition.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform_log.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_thread.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_time.c
    ${SHARED_LIB_ROOT}/platform/zephyr/bh_math.c
    ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c
    ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c
    ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c
    ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c
    ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c
    ${SHARED_LIB_ROOT}/mem-alloc/tlsf/tlsf.c
    ${SHARED_LIB_ROOT}/utils/bh_list.c
    ${SHARED_LIB_ROOT}/utils/bh_log.c
    ${SHARED_LIB_ROOT}/utils/bh_queue.c
    ${SHARED_LIB_ROOT}/utils/runtime_timer.c
    ${SHARED_LIB_ROOT}/coap/er-coap/er-coap.c
    ${SHARED_LIB_ROOT}/coap/extension/coap_conversion.c
    ${SHARED_LIB_ROOT}/coap/extension/coap_over_tcp.c
    )

    set (LVGL_DRV_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340_adafruit_1480.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_ili9340.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/display_indev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/XPT2046.c
    )

target_sources(app PRIVATE ${IWASM_SRCS}  
    ${LVGL_DRV_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/zephyr/iwasm_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ext_lib_export.c)
