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
#

cmake_minimum_required (VERSION 2.8.3)
project (host-agent)

if (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

if (NOT TARGET_PLATFORM)
  set (TARGET_PLATFORM "linux")
endif (NOT TARGET_PLATFORM)

message ("TARGET_PLATFORM = " ${TARGET_PLATFORM})

set(REPO_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(WASM_DIR  ${REPO_ROOT_DIR}/core/iwasm)
set(APP_MGR_DIR ${REPO_ROOT_DIR}/core/app-mgr)
set(SHARED_DIR ${REPO_ROOT_DIR}/core/shared-lib)
#TODO: use soft-plc/tools/iec-runtime/external/cJSON instead
set(CJSON_DIR ${CMAKE_CURRENT_LIST_DIR}/external/cJSON)
set(SSG_LIB_DIR ${REPO_ROOT_DIR}/../host-agent/lib)

include (${WASM_DIR}/lib/native-interface/native_interface.cmake)
include (${APP_MGR_DIR}/app-mgr-shared/app_mgr_shared.cmake)
include (${SHARED_DIR}/platform/${TARGET_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${CJSON_DIR}/cjson.cmake)
include (${SHARED_DIR}/coap/lib_coap.cmake)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif ()

add_definitions(-Wall -Wno-pointer-sign -DMALLOC_MEMORY_FROM_SYSTEM)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${WASM_DIR}/runtime/include
)

file (GLOB_RECURSE HOST_TOOL_SRC src/*.c)


SET(SOURCES
    ${HOST_TOOL_SRC}
    ${PLATFORM_SHARED_SOURCE}
    ${UTILS_SHARED_SOURCE}
    ${MEM_ALLOC_DIR}/bh_memory.c
    ${NATIVE_INTERFACE_SOURCE}
    ${CJSON_SOURCE}
    ${LIB_HOST_AGENT_SOURCE}
    )
    
add_executable(host_tool ${SOURCES})
target_link_libraries(host_tool pthread)
