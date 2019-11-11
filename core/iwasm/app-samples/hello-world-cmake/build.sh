#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../../../test-tools/toolchain/wamr_toolchain.cmake
make