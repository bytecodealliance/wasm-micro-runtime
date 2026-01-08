#!/bin/bash
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Test for x18 register reservation on macOS ARM64 (aarch64).
#
# On macOS ARM64, x18 is reserved by Apple for TLS (Thread Local Storage).
# Without the +reserve-x18 LLVM flag, the AOT compiler may generate code
# that uses x18, causing random SIGSEGV crashes when run on macOS.
#
# This test compiles a WASM module that stresses register allocation
# (forcing x18 usage without the fix) and runs it 1000 times to verify
# no crashes occur.
#

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
WAMR_DIR="${SCRIPT_DIR}/../../.."

# Detect platform
UNAME_S=$(uname -s)
UNAME_M=$(uname -m)

# Only run this test on macOS ARM64
if [[ "${UNAME_S}" != "Darwin" ]] || [[ "${UNAME_M}" != "arm64" ]]; then
    echo "Skipping x18 reserve test: only applicable on macOS ARM64"
    echo "Current platform: ${UNAME_S} ${UNAME_M}"
    exit 0
fi

# Determine iwasm path based on platform
if [[ "${UNAME_S}" == "Darwin" ]]; then
    IWASM_CMD="${WAMR_DIR}/product-mini/platforms/darwin/build/iwasm"
else
    IWASM_CMD="${WAMR_DIR}/product-mini/platforms/linux/build/iwasm"
fi

WAMRC_CMD="${WAMR_DIR}/wamr-compiler/build/wamrc"

# Check if required binaries exist
if [[ ! -x "${IWASM_CMD}" ]]; then
    echo "Error: iwasm not found at ${IWASM_CMD}"
    echo "Please build iwasm first"
    exit 1
fi

if [[ ! -x "${WAMRC_CMD}" ]]; then
    echo "Error: wamrc not found at ${WAMRC_CMD}"
    echo "Please build wamrc first"
    exit 1
fi

cd "${SCRIPT_DIR}"

# Find wat2wasm (check CI path first, then system PATH)
if [[ -x "/opt/wabt/bin/wat2wasm" ]]; then
    WAT2WASM="/opt/wabt/bin/wat2wasm"
elif command -v wat2wasm &> /dev/null; then
    WAT2WASM="wat2wasm"
else
    echo "Error: wat2wasm not found"
    echo "Please install wabt tools"
    exit 1
fi

# Compile WAT to WASM if needed
if [[ ! -f stress_registers.wasm ]] || [[ stress_registers.wat -nt stress_registers.wasm ]]; then
    echo "Compiling stress_registers.wat to WASM..."
    if ! ${WAT2WASM} stress_registers.wat -o stress_registers.wasm; then
        echo "Error: Failed to compile WAT to WASM"
        exit 1
    fi
fi

if [[ $1 != "--aot" ]]; then
    echo "============> run stress_registers.wasm (interpreter mode)"
    echo "Running 100 iterations in interpreter mode..."
    for i in $(seq 1 100); do
        if ! ${IWASM_CMD} stress_registers.wasm 2>&1; then
            echo "FAILED: Crash at iteration $i"
            exit 1
        fi
    done
    echo "PASSED: 100 iterations completed without crash"
else
    echo "============> compile stress_registers.wasm to AOT"

    # Compile to AOT - the fix should add +reserve-x18 automatically on macOS ARM64
    if ! ${WAMRC_CMD} --opt-level=3 -o stress_registers.aot stress_registers.wasm; then
        echo "Error: Failed to compile WASM to AOT"
        exit 1
    fi

    echo "============> run stress_registers.aot"
    echo "Running 1000 iterations to verify x18 is properly reserved..."
    echo "(Without the fix, this would crash ~80% of the time)"

    failed=0
    for i in $(seq 1 1000); do
        if ! ${IWASM_CMD} stress_registers.aot 2>&1 > /dev/null; then
            echo "FAILED: Crash at iteration $i"
            failed=1
            break
        fi
        # Progress indicator every 100 iterations
        if [[ $((i % 100)) -eq 0 ]]; then
            echo "  Progress: $i/1000 iterations completed"
        fi
    done

    if [[ ${failed} -eq 0 ]]; then
        echo "PASSED: 1000 iterations completed without crash"
        echo "The +reserve-x18 fix is working correctly"
        exit 0
    else
        echo "FAILED: x18 register corruption detected"
        echo "The +reserve-x18 fix may not be applied correctly"
        exit 1
    fi
fi
