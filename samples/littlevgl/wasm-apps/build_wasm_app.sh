# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/sh
if [ ! -d "lvgl" ]; then
    git clone https://github.com/littlevgl/lvgl.git --branch v5.3
fi
make -f Makefile_wasm_app

