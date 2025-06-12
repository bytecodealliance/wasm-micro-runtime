#! /bin/sh

# Copyright (C) 2025 Midokura Japan KK.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

PREFIX=/tmp/wamr
WASI_SDK=${WASI_SDK:-/opt/wasi-sdk}

cmake -B build-lib \
-DCMAKE_TOOLCHAIN_FILE=${WASI_SDK}/share/cmake/wasi-sdk.cmake \
-DCMAKE_INSTALL_PREFIX=${PREFIX} \
.
cmake --build build-lib -t install

cmake -B build-app-nn \
-DCMAKE_TOOLCHAIN_FILE=${WASI_SDK}/share/cmake/wasi-sdk.cmake \
-DCMAKE_PREFIX_PATH=${PREFIX} \
samples/nn
cmake --build build-app-nn
