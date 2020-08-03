#!/bin/sh

# Copyright (C) 2020 Microsoft.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

rm -fr build && mkdir build
cd build
cmake .. -DWAMR_BUILD_JIT=1 -DENABLE_SNMALLOC=1 -DWAMR_BUILD_SNMALLOC_SHARED_MEMORY=1
make
cd ..
