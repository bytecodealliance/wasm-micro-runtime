@REM Copyright (C) 2019 Intel Corporation.  All rights reserved.
@REM SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

@echo off

docker run -it --name=wasm-debug-server-ctr ^
           -v "%cd%":/mnt ^
           wasm-debug-server:1.0 ^
           /bin/bash -c "./run.sh %1"

@REM stop and remove wasm-debug-server-ctr
docker stop wasm-debug-server-ctr>nul 2>nul
docker rm wasm-debug-server-ctr>nul 2>nul