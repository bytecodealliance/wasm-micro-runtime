#Copyright(C) 2019 Intel Corporation.All rights reserved.
#SPDX - License - Identifier : Apache - 2.0 WITH LLVM - exception

set (GC_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${GC_DIR})

file (GLOB_RECURSE source_all
      ${
    GC_DIR}/*.c)

set (GC_SHARED_SOURCE ${source_all})

