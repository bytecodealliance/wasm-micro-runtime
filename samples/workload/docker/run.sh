#!/bin/bash

docker run --rm -it \
  -e http_proxy=${http_proxy} \
  -e https_proxy=${https_proxy} \
  -e HTTP_PROXY=${http_proxy} \
  -e HTTPS_PROXY=${htpps_proxy} \
  --name workload_w_clang \
  --mount type=bind,source=$(pwd)/..,target=/data \
  clang_env:0.1
