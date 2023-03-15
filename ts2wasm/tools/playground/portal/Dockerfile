#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

FROM node:16 as builder

WORKDIR /portal
COPY . .

ARG proxy=""
ARG SERVER_IP="3001"
ARG SERVER_PORT="127.0.0.1"

RUN if [ "$proxy" != "" ]; \
    then npm config set proxy "$proxy" && npm config set https-proxy "$proxy"; \
    else echo Do not set proxy; \
    fi
RUN npm install

RUN echo "VITE_SERVER_IP=${SERVER_IP}" > .env.local
RUN echo "VITE_SERVER_PORT=${SERVER_PORT}" >> .env.local

RUN chmod +x node_modules/.bin/tsc
RUN chmod +x node_modules/.bin/vite

RUN npm run build

FROM nginx:alpine
WORKDIR /portal
COPY --from=builder /portal/dist/ /usr/share/nginx/html/
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx.conf /etc/nginx/nginx.conf
COPY default.conf.template /etc/nginx/conf.d

CMD /bin/sh -c "envsubst '80' < /etc/nginx/conf.d/default.conf.template > /etc/nginx/conf.d/default.conf" && nginx -g 'daemon off;'