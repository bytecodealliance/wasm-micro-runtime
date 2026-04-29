#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Unified QEMU runner for Zephyr CI.
# Extracts the QEMU command from build.ninja, runs it with a timeout,
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

NINJA_FILE="$BUILD_DIR/build.ninja"
if [ ! -f "$NINJA_FILE" ]; then
    echo "Error: build.ninja not found at $NINJA_FILE"
    exit 1
fi

# Extract the QEMU command from build.ninja.
# The run_qemu target has a COMMAND line like:
#   COMMAND = cd /path/to/build && /path/to/qemu-system-xxx <flags> -kernel <elf>
QEMU_CMD=$(awk '
    /^build zephyr\/CMakeFiles\/run_qemu /{found=1}
    found && /^  COMMAND = /{
        sub(/^  COMMAND = /, "")
        print
        exit
    }
' "$NINJA_FILE")

if [ -z "$QEMU_CMD" ]; then
    echo "Error: could not extract QEMU command from $NINJA_FILE"
    exit 1
fi

echo "=== Extracted QEMU command ==="
echo "$QEMU_CMD"
echo "=== Running with ${TIMEOUT_SECONDS}s timeout ==="

LOG_FILE=$(mktemp /tmp/qemu_output.XXXXXX)
trap "rm -f $LOG_FILE" EXIT

# Run QEMU with timeout. Exit code 124 means timeout — that's expected since
# Zephyr doesn't shut down the emulator. Any other non-zero exit is also fine
# for some boards (e.g., x86 with isa-debug-exit returns 1).
set +e
timeout "${TIMEOUT_SECONDS}s" bash -c "${QEMU_CMD}" > "$LOG_FILE" 2>&1
QEMU_EXIT=$?
set -e

echo "=== QEMU exited with code $QEMU_EXIT ==="

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
    echo "=== Full QEMU output ==="
    cat "$LOG_FILE"
    echo "=== End of QEMU output ==="
    exit 1
fi

echo "=== All expected strings found ==="
