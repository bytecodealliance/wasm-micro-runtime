#!/bin/bash

set -e

if [ $# -gt 1 ]; then
    echo "Usage: $0 [--aot]"
    exit 1
fi

APP=out/wasm-apps/custom_section.wasm

if [ $# -eq 1 ]; then
    if [ "$1" = "--aot" ]; then
        APP=out/wasm-apps/custom_section.aot
    else
        echo "Usage: $0 [--aot]"
        exit 1
    fi
fi

if [ ! -f ${APP} ]; then
    echo "Error: ${APP} not found"
    if [ "$APP" = "out/wasm-apps/custom_section.aot" ]; then
        echo "Run ./build.sh --aot first"
    else
        echo "Run ./build.sh first"
    fi
    exit 1
fi

out/custom_section -f ${APP}
