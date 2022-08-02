# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/bash
docker pull gcc:9.3.0
docker pull ubuntu:20.04
docker build -t wasm-toolchain:1.0 .

# delete intermediate docker image
docker image prune -f
