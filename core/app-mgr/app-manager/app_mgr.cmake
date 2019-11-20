# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (__APP_MGR_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${__APP_MGR_DIR})


file (GLOB source_all ${__APP_MGR_DIR}/*.c  ${__APP_MGR_DIR}/platform/${TARGET_PLATFORM}/*.c)

set (APP_MGR_SOURCE ${source_all})

