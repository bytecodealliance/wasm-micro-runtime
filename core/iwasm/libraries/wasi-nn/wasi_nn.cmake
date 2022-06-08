# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8...3.16)

project(wasi_nn)

add_library(${PROJECT_NAME} STATIC ${CMAKE_CURRENT_LIST_DIR}/wasi_nn.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
