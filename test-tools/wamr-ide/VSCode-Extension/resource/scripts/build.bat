@REM Copyright (C) 2019 Intel Corporation.  All rights reserved.
@REM SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

@echo off

@REM start a container, mount current project path to container/mnt
docker run --name=wasm-toolchain-ctr ^
                -it -v "%cd%":/mnt ^
                --env=PROJ_PATH="%cd%" ^
                wasm-toolchain:1.0  ^
                /bin/bash -c "./build_wasm.sh %1"

@REM stop and remove wasm-toolchain-ctr container
docker stop wasm-toolchain-ctr>nul 2>nul
docker rm wasm-toolchain-ctr>nul 2>nul