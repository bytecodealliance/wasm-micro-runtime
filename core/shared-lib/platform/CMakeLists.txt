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

include_directories (./include ../include ./${PLATFORM})

add_definitions (-D__POSIX__ -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=199309L -D_BSD_SOURCE)

file (GLOB_RECURSE source_all ${PLATFORM}/*.c)
add_library (supportlib ${source_all})

target_link_libraries (supportlib -pthread -lrt)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 add_library (supportlib_ut ${source_all})

 set_target_properties (supportlib_ut PROPERTIES COMPILE_DEFINITIONS BH_TEST=1)

 target_link_libraries (supportlib_ut -pthread -lrt)
endif ()

