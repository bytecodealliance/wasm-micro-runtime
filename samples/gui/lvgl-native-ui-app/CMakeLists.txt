# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 2.8.2)
message ("lvgl_native_ui_app...")
project (lvgl_native_ui_app)

#################################################################
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -g)

set(THIRDPARTY_DIR ../../../core/iwasm/lib/3rdparty)
set(LVGL_SOURCE_DIR ${THIRDPARTY_DIR}/lvgl)
set(LVGL_DRIVER_DIR ${THIRDPARTY_DIR}/lv_drivers)

#################################
set(CMAKE_C_STANDARD 11)#C11
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${THIRDPARTY_DIR})

add_definitions (-DLV_CONF_INCLUDE_SIMPLE)

file(GLOB_RECURSE INCLUDES "${LVGL_DRIVER_DIR}/*.h" "${LVGL_SOURCE_DIR}/*.h"  "./*.h" )
file(GLOB_RECURSE SOURCES  "${LVGL_DRIVER_DIR}/*.c" "${LVGL_SOURCE_DIR}/*.c" )

add_executable(lvgl_native_ui_app main.c get_time.c ${SOURCES} ${INCLUDES})
target_link_libraries(lvgl_native_ui_app PRIVATE SDL2 )

