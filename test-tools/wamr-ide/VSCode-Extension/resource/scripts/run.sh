# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/bash

docker run -it --name=wasm-debug-server-ctr \
           -v "$(pwd)":/mnt \
           wasm-debug-server:1.0 \
           /bin/bash -c "./run.sh $1"

docker stop wasm-debug-server-ctr>/dev/null
docker rm wasm-debug-server-ctr>/dev/null