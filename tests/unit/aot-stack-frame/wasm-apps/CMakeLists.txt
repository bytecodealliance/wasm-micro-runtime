# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.9)

project(wasm-apps-aot-stack-frame)

add_custom_target(aot-stack-frame-test-wasm ALL
    COMMAND cmake -B ${CMAKE_CURRENT_BINARY_DIR}/build-wamrc
                  -S ${WAMR_ROOT_DIR}/wamr-compiler
            && cmake --build ${CMAKE_CURRENT_BINARY_DIR}/build-wamrc
            && /opt/wabt/bin/wat2wasm
                  -o ${CMAKE_CURRENT_BINARY_DIR}/test.wasm
                  ${CMAKE_CURRENT_LIST_DIR}/test.wast
            && ${CMAKE_CURRENT_BINARY_DIR}/build-wamrc/wamrc
                  --enable-dump-call-stack --bounds-checks=1
                  -o ${CMAKE_CURRENT_BINARY_DIR}/test.aot
                  ${CMAKE_CURRENT_BINARY_DIR}/test.wasm
            && cmake -B ${CMAKE_CURRENT_BINARY_DIR}/build-binarydump
                  -S ${WAMR_ROOT_DIR}/test-tools/binarydump-tool
            && cmake --build ${CMAKE_CURRENT_BINARY_DIR}/build-binarydump
            && ${CMAKE_CURRENT_BINARY_DIR}/build-binarydump/binarydump
                  -o ${CMAKE_CURRENT_LIST_DIR}/test_aot.h -n test_aot
                  ${CMAKE_CURRENT_BINARY_DIR}/test.aot

)

