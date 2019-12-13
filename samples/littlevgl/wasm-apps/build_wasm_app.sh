# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/sh

WAMR_DIR=${PWD}/../../..

if [ -z $KW_BUILD ] || [ -z $KW_OUT_FILE ];then
    echo "Local Build Env"
    makewrap="make"
else
    echo "Klocwork Build Env"
    makewrap="kwinject -o $KW_OUT_FILE make"
fi

if [ ! -d "lvgl" ]; then
    git clone https://github.com/littlevgl/lvgl.git --branch v5.3
fi
$makewrap -f Makefile_wasm_app

