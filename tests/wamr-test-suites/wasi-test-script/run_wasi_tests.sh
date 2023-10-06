#!/bin/bash

#
# Copyright (C) 2023 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

THIS_DIR=$(cd $(dirname $0) && pwd -P)

readonly MODE=$1
readonly TARGET=$2

readonly WORK_DIR=$PWD
readonly PLATFORM=$(uname -s | tr A-Z a-z)
readonly WAMR_DIR="${WORK_DIR}/../../../.."
readonly IWASM_CMD="${WORK_DIR}/../../../../product-mini/platforms/${PLATFORM}/build/iwasm \
    --allow-resolve=google-public-dns-a.google.com \
    --addr-pool=::1/128,127.0.0.1/32"

readonly IWASM_CMD_STRESS="${IWASM_CMD} --max-threads=12"
readonly WAMRC_CMD="${WORK_DIR}/../../../../wamr-compiler/build/wamrc"
readonly C_TESTS="tests/c/testsuite/"
readonly RUST_TESTS="tests/rust/testsuite/"
readonly ASSEMBLYSCRIPT_TESTS="tests/assemblyscript/testsuite/"
readonly THREAD_PROPOSAL_TESTS="tests/proposals/wasi-threads/"
readonly THREAD_INTERNAL_TESTS="${WAMR_DIR}/core/iwasm/libraries/lib-wasi-threads/test/"
readonly THREAD_STRESS_TESTS="${WAMR_DIR}/core/iwasm/libraries/lib-wasi-threads/stress-test/"
readonly LIB_SOCKET_TESTS="${WAMR_DIR}/core/iwasm/libraries/lib-socket/test/"

run_aot_tests () {
    local tests=("$@")
    for test_wasm in ${tests[@]}; do
        local iwasm="${IWASM_CMD}"
        if [[ $test_wasm =~ "stress" ]]; then
            iwasm="${IWASM_CMD_STRESS}"
        fi

        test_aot="${test_wasm%.wasm}.aot"
        test_json="${test_wasm%.wasm}.json"
 
        if [ -f ${test_wasm} ]; then
            expected=$(jq .exit_code ${test_json})
        fi

        echo "Compiling $test_wasm to $test_aot"
        ${WAMRC_CMD} --enable-multi-thread ${target_option} \
            -o ${test_aot} ${test_wasm}

        echo "Running $test_aot"
        expected=0
        if [ -f ${test_json} ]; then
            expected=$(jq .exit_code ${test_json})
        fi

        python3 ${THIS_DIR}/pipe.py | ${iwasm} $test_aot
        ret=${PIPESTATUS[1]}

        echo "expected=$expected, actual=$ret"
        if [[ $expected != "" ]] && [[ $expected != $ret ]];then
            exit_code=1
        fi
    done
}

if [[ $MODE != "aot" ]];then
    python3 -m venv wasi-env && source wasi-env/bin/activate
    python3 -m pip install -r test-runner/requirements.txt

    export TEST_RUNTIME_EXE="${IWASM_CMD}"
    python3 ${THIS_DIR}/pipe.py | python3 test-runner/wasi_test_runner.py \
            -r adapters/wasm-micro-runtime.py \
            -t \
                ${C_TESTS} \
                ${RUST_TESTS} \
                ${ASSEMBLYSCRIPT_TESTS} \
                ${THREAD_PROPOSAL_TESTS} \
                ${THREAD_INTERNAL_TESTS} \
                ${LIB_SOCKET_TESTS} \

    ret=${PIPESTATUS[1]}

    TEST_RUNTIME_EXE="${IWASM_CMD_STRESS}" python3 test-runner/wasi_test_runner.py \
            -r adapters/wasm-micro-runtime.py \
            -t \
                ${THREAD_STRESS_TESTS}

    if [ "${ret}" -eq 0 ]; then
        ret=${PIPESTATUS[0]}
    fi
    
    exit_code=${ret}
    
    deactivate
else
    target_option=""
    if [[ $TARGET == "X86_32" ]];then
        target_option="--target=i386"
    fi

    exit_code=0
    for testsuite in ${THREAD_STRESS_TESTS} ${THREAD_PROPOSAL_TESTS} ${THREAD_INTERNAL_TESTS}; do
        tests=$(ls ${testsuite}*.wasm)
        tests_array=($tests)
        run_aot_tests "${tests_array[@]}"
    done
fi

exit ${exit_code}
