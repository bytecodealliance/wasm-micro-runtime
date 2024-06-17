#!/bin/bash
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

if [[ $2 == "--sgx" ]];then
    readonly IWASM_CMD="../../../product-mini/platforms/linux-sgx/enclave-sample/iwasm"
else
    readonly IWASM_CMD="../../../product-mini/platforms/linux/build/iwasm"
fi
readonly WAMRC_CMD="../../../wamr-compiler/build/wamrc"

if [[ $1 != "--aot" ]]; then
    echo "============> run cowsay.wasm"
    ${IWASM_CMD} cowsay.wasm hello
    ${IWASM_CMD} cowsay.wasm -f tux "This is a test run"
else
    echo "============> compile cowsay.wasm to aot"
    [[ $2 == "--sgx" ]] && ${WAMRC_CMD} -sgx -o cowsay.aot cowsay.wasm \
                        || ${WAMRC_CMD} -o cowsay.aot cowsay.wasm
    echo "============> run cowsay.aot"
    ${IWASM_CMD} cowsay.aot hello
    ${IWASM_CMD} cowsay.aot -f tux "This is a test run"
fi

