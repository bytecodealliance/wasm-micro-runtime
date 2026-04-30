#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Unified QEMU runner for Zephyr CI.
# Uses `west build -t run` to launch the emulator with a timeout,
# and verifies expected strings appear in the output.
#
# Usage: run_qemu_and_verify.sh <build_dir> <timeout_seconds> <expected_string> [expected_string ...]

set -euo pipefail

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 <build_dir> <timeout_seconds> <expected_string> [expected_string ...]"
    exit 1
fi

BUILD_DIR="$1"
TIMEOUT_SECONDS="$2"
shift 2
EXPECTED_STRINGS=("$@")

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: build directory not found at $BUILD_DIR"
    exit 1
fi

echo "=== Running 'west build -t run' with ${TIMEOUT_SECONDS}s timeout ==="

LOG_FILE=$(mktemp /tmp/qemu_output.XXXXXX)
trap "rm -f $LOG_FILE" EXIT

# Run the emulator via west. Exit code 124 means timeout — that's expected since
# Zephyr doesn't shut down the emulator. Any other non-zero exit is also fine
# for some boards (e.g., x86 with isa-debug-exit returns 1).
set +e
timeout "${TIMEOUT_SECONDS}s" west build -t run --build-dir "$BUILD_DIR" 2>&1 | tee "$LOG_FILE"
WEST_EXIT=${PIPESTATUS[0]}
set -e

echo "=== west exited with code $WEST_EXIT ==="

# Verify expected strings
FAILED=0
for expected in "${EXPECTED_STRINGS[@]}"; do
    if grep -qF "$expected" "$LOG_FILE"; then
        echo "PASS: found '$expected'"
    else
        echo "FAIL: missing '$expected'"
        FAILED=1
    fi
done

if [ "$FAILED" -ne 0 ]; then
    echo ""
    echo "=== Full output ==="
    cat "$LOG_FILE"
    echo "=== End of output ==="
    exit 1
fi

echo "=== All expected strings found ==="
