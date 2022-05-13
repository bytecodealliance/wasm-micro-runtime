# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/bash
docker build --build-arg http_proxy=http://child-prc.intel.com:913 \
             --build-arg https_proxy=http://child-prc.intel.com:913 \
             --build-arg ftp_proxy=http://child-prc.intel.com:913 \
             --build-arg socks_proxy=http://child-prc.intel.com:913 \
              -t wasm-debug-server:1.0 .

# delete intermediate docker image
docker image prune -f