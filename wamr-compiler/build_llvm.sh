#!/bin/sh

# Copyright (C) 2020 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

TEMP_DIR=$(mktemp -d)

cleanup() {
    local exit_code=$?
    rm -rf "$TEMP_DIR"
    exit $exit_code
}

trap cleanup EXIT INT TERM

/usr/bin/env python3 -m venv --clear "$TEMP_DIR"
. "$TEMP_DIR/bin/activate"
/usr/bin/env python3 -m pip install -r ../build-scripts/requirements.txt
/usr/bin/env python3 ../build-scripts/build_llvm.py "$@"
