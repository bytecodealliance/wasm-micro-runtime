#!/bin/sh

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

CUR_DIR=$(cd $(dirname $0) && pwd -P)
ROOT_DIR=${CUR_DIR}/../../..

WAMR_BUILD_PLATFORM=${WAMR_BUILD_PLATFORM:-"linux"}

cd ${ROOT_DIR}/product-mini/platforms/${WAMR_BUILD_PLATFORM}

mkdir -p build && cd build
cmake ..
make -j

cp libiwasm.so ${CUR_DIR}/../src/wamr/libs
