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

set(lv_name lvgl)
set(lv_drivers_name lv_drivers)
set(LVGL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${lv_name})
set(LVGL_DRIVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${lv_drivers_name})

if ((NOT EXISTS ${LVGL_SOURCE_DIR}) OR (NOT EXISTS ${LVGL_DRIVER_DIR}))
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl/CMakeLists.txt)

        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl )
        if(result)
                message(FATAL_ERROR "CMake step for lvgl failed: ${result}")
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl )
        if(result)
                message(FATAL_ERROR "Build step for lvgl failed: ${result}")
        endif()
endif()

#################################
set(CMAKE_C_STANDARD 11)#C11
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE INCLUDES "${LVGL_DRIVER_DIR}/*.h" "${LVGL_SOURCE_DIR}/*.h"  "./*.h" )
file(GLOB_RECURSE SOURCES  "${LVGL_DRIVER_DIR}/*.c" "${LVGL_SOURCE_DIR}/*.c" )

add_executable(lvgl_native_ui_app main.c get_time.c ${SOURCES} ${INCLUDES})
target_link_libraries(lvgl_native_ui_app PRIVATE SDL2 )

