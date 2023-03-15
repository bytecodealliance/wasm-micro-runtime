#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

FROM node:16

WORKDIR /ts2wasm
COPY . .

ARG proxy=""

RUN if [ "$proxy" != "" ]; \
    then npm config set proxy "$proxy" && npm config set https-proxy "$proxy"; \
    else echo Do not set proxy; \
    fi

RUN npm install

WORKDIR /ts2wasm/tools/playground/server
RUN npm install

RUN npm run build

CMD node /ts2wasm/tools/playground/server/dist/tools/playground/server/index.js
