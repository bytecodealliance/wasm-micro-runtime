#!/bin/bash

set -e

CURR_DIR=$PWD
OUT_DIR=${PWD}/out
WASM_APPS=${PWD}/wasm-apps
WAMR_ROOT_DIR=${PWD}/../..
WAMRC_CMD=${WAMR_ROOT_DIR}/wamr-compiler/build/wamrc
BUILD_AOT=0

if [ $# -gt 1 ]; then
    echo "Usage: $0 [--aot]"
    exit 1
fi

if [ $# -eq 1 ]; then
    if [ "$1" = "--aot" ]; then
        BUILD_AOT=1
    else
        echo "Usage: $0 [--aot]"
        exit 1
    fi
fi

rm -rf ${OUT_DIR}
mkdir -p ${OUT_DIR}/wasm-apps

printf '##################### build custom-section project\n'
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j 4
cp -a custom_section ${OUT_DIR}

printf '\n##################### build wasm app\n'
cd ${WASM_APPS}
/opt/wasi-sdk/bin/clang \
    --target=wasm32 \
    -O0 \
    -nostdlib \
    -Wl,--strip-all,--no-entry \
    -Wl,--allow-undefined \
    -Wl,--export=run_demo \
    -o ${OUT_DIR}/wasm-apps/custom_section.wasm \
    custom_section.c \
    custom_section_payload.s

printf '\nbuild custom_section.wasm success\n'

if [ ${BUILD_AOT} -eq 1 ]; then
    if [ ! -x ${WAMRC_CMD} ]; then
        echo "Error: wamrc not found at ${WAMRC_CMD}"
        echo "Please build wamrc first under ${WAMR_ROOT_DIR}/wamr-compiler"
        exit 1
    fi

    printf '\n##################### build aot app\n'
    ${WAMRC_CMD} --emit-custom-sections=demo -o ${OUT_DIR}/wasm-apps/custom_section.aot ${OUT_DIR}/wasm-apps/custom_section.wasm
    printf '\nbuild custom_section.aot success\n'
fi

cd ${CURR_DIR}
