#!/bin/bash

#
# Copyright (C) 2023 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

readonly MODE=$1
readonly TARGET=$2

readonly WORK_DIR=$PWD

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    readonly PLATFORM=windows
    readonly PYTHON_EXE=python
    # see https://github.com/pypa/virtualenv/commit/993ba1316a83b760370f5a3872b3f5ef4dd904c1
    readonly VENV_BIN_DIR=Scripts
    readonly IWASM_EXE=$(cygpath -m "${WORK_DIR}/../../../../product-mini/platforms/${PLATFORM}/build/RelWithDebInfo/iwasm.exe")
else
    readonly PLATFORM=$(uname -s | tr A-Z a-z)
    readonly VENV_BIN_DIR=bin
    readonly PYTHON_EXE=python3
    readonly IWASM_EXE="${WORK_DIR}/../../../../product-mini/platforms/${PLATFORM}/build/iwasm"
fi

readonly WAMR_DIR="${WORK_DIR}/../../../.."
readonly IWASM_CMD="${IWASM_EXE} \
    --allow-resolve=google-public-dns-a.google.com \
    --addr-pool=::1/128,127.0.0.1/32"

readonly IWASM_CMD_STRESS="${IWASM_CMD} --max-threads=8"
readonly WAMRC_CMD="${WORK_DIR}/../../../../wamr-compiler/build/wamrc"
readonly C_TESTS="tests/c/testsuite/"
readonly ASSEMBLYSCRIPT_TESTS="tests/assemblyscript/testsuite/"
readonly THREAD_PROPOSAL_TESTS="tests/proposals/wasi-threads/"
readonly THREAD_INTERNAL_TESTS="${WAMR_DIR}/core/iwasm/libraries/lib-wasi-threads/test/"
readonly LIB_SOCKET_TESTS="${WAMR_DIR}/core/iwasm/libraries/lib-socket/test/"
readonly STRESS_TESTS=("spawn_stress_test.wasm" "stress_test_threads_creation.wasm")

run_aot_tests () {
    local tests=("$@")
    local iwasm="${IWASM_CMD}"
    for test_wasm in ${tests[@]}; do
        local extra_stress_flags=""
        for stress_test in "${STRESS_TESTS[@]}"; do
            if [ "$test_wasm" == "$stress_test" ]; then
                iwasm="${IWASM_CMD_STRESS}"
            fi
        done

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

        ${IWASM_CMD} $extra_stress_flags $test_aot
        ret=${PIPESTATUS[0]}

        echo "expected=$expected, actual=$ret"
        if [[ $expected != "" ]] && [[ $expected != $ret ]];then
            exit_code=1
        fi
    done
}

if [[ $MODE != "aot" ]];then
    $PYTHON_EXE -m venv wasi-env && source wasi-env/${VENV_BIN_DIR}/activate
    $PYTHON_EXE -m pip install -r test-runner/requirements.txt

    # Stress tests require max-threads=8 so they're executed separately
    for stress_test in "${STRESS_TESTS[@]}"; do
        if [[ -e "${THREAD_INTERNAL_TESTS}${stress_test}" ]]; then
            echo "${stress_test}" is a stress test
            ${IWASM_CMD_STRESS} ${THREAD_INTERNAL_TESTS}${stress_test}
            ret=${PIPESTATUS[0]}
            if [ "${ret}" -ne 0 ]; then
                echo "Stress test ${stress_test} FAILED with code " ${ret}
                exit_code=${ret}
            fi
        fi
    done

    TEST_RUNTIME_EXE="${IWASM_CMD}" $PYTHON_EXE test-runner/wasi_test_runner.py \
            -r adapters/wasm-micro-runtime.py \
            -t \
                ${C_TESTS} \
                ${ASSEMBLYSCRIPT_TESTS} \
                ${THREAD_PROPOSAL_TESTS} \
                ${THREAD_INTERNAL_TESTS} \
                ${LIB_SOCKET_TESTS} \
            --exclude-filter "${THREAD_INTERNAL_TESTS}skip.json"

    ret=${PIPESTATUS[0]}
    if [ "${ret}" -ne 0 ]; then
        exit_code=${ret}
    fi
    deactivate
else
    target_option=""
    if [[ $TARGET == "X86_32" ]];then
        target_option="--target=i386"
    fi

    exit_code=0
    for testsuite in ${THREAD_PROPOSAL_TESTS} ${THREAD_INTERNAL_TESTS}; do
        tests=$(ls ${testsuite}*.wasm)
        tests_array=($tests)
        run_aot_tests "${tests_array[@]}"
    done
fi

exit ${exit_code}
