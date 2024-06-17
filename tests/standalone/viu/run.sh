#!/bin/bash
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# Source code: https://github.com/wapm-packages/viu

if [[ $2 == "--sgx" ]];then
    readonly IWASM_CMD="../../../product-mini/platforms/linux-sgx/enclave-sample/iwasm"
else
    readonly IWASM_CMD="../../../product-mini/platforms/linux/build/iwasm"
fi
readonly WAMRC_CMD="../../../wamr-compiler/build/wamrc"

if [[ $1 != "--aot" ]]; then
    echo "============> run viu.wasm"
    ${IWASM_CMD} --dir=. viu.wasm --once arrows_light.gif
    ${IWASM_CMD} --dir=. viu.wasm --once --mirror ellipses_dark.gif
else
    echo "============> compile viu.wasm to aot"
    [[ $2 == "--sgx" ]] && ${WAMRC_CMD} -sgx -o viu.aot viu.wasm \
                        || ${WAMRC_CMD} -o viu.aot viu.wasm
    echo "============> run viu.aot"
    ${IWASM_CMD} --dir=. viu.aot --once arrows_light.gif
    ${IWASM_CMD} --dir=. viu.aot --once --mirror ellipses_dark.gif
fi
