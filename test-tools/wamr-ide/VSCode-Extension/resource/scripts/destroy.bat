@REM Copyright (C) 2019 Intel Corporation.  All rights reserved.
@REM SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

@echo off

call docker inspect wasm-toolchain-ctr>nul 2>nul
IF %ERRORLEVEL%==0 (
    echo "Stopping and removing wasm-toolchain-ctr container..."
    docker stop wasm-toolchain-ctr>nul 2>nul
    docker rm wasm-toolchain-ctr>nul 2>nul
    echo "Done."
)

call docker inspect wasm-debug-server-ctr>nul 2>nul
IF %ERRORLEVEL%==0 (
    echo "Stopping and removing wasm-debug-server-ctr container..."
    docker stop wasm-debug-server-ctr>nul 2>nul
    docker rm wasm-debug-server-ctr>nul 2>nul
    echo "Done."
)