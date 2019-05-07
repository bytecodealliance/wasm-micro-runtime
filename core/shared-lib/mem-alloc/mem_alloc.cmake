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


set (MEM_ALLOC_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${MEM_ALLOC_DIR})

file (GLOB_RECURSE source_all
      ${MEM_ALLOC_DIR}/ems/*.c
      ${MEM_ALLOC_DIR}/tlsf/*.c
      ${MEM_ALLOC_DIR}/mem_alloc.c
      ${MEM_ALLOC_DIR}/bh_memory.c)

set (MEM_ALLOC_SHARED_SOURCE ${source_all})

