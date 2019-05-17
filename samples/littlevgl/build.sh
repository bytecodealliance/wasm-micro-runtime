#!/bin/bash

PROJECT_DIR=$PWD
ROOT_DIR=${PWD}/../../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build

if [ ! -d $BUILD_DIR ]; then
    mkdir ${BUILD_DIR}
fi

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}


cd ${ROOT_DIR}/wamr/core/shared-lib/mem-alloc
if [ ! -d "tlsf" ]; then
    git clone https://github.com/mattconte/tlsf
fi

echo "##################### 1. build native-ui-app start#####################"
cd $BUILD_DIR
mkdir -p vgl-native-ui-app
cd vgl-native-ui-app
cmake ${PROJECT_DIR}/vgl-native-ui-app
make
if [ $? != 0 ];then
    echo "BUILD_FAIL native-ui-app $?\n"
    exit 2
fi
echo $PWD
cp  vgl_native_ui_app ${OUT_DIR}
echo "#####################build native-ui-app success"

echo "#####################build host-tool"
cd $BUILD_DIR
mkdir -p host-tool
cd host-tool
cmake ${ROOT_DIR}/wamr/test-tools/host-tool
make
if [ $? != 0 ];then
        echo "BUILD_FAIL host tool exit as $?\n"
        exit 2
fi
cp host_tool ${OUT_DIR}
echo "#####################build host-tool success"


echo "##################### 2. build littlevgl wasm runtime start#####################"
cd $BUILD_DIR
mkdir -p vgl-wasm-runtime
cd vgl-wasm-runtime
cmake ${PROJECT_DIR}/vgl-wasm-runtime
make
cp vgl_wasm_runtime ${OUT_DIR}/

echo "##################### build littlevgl wasm runtime end#####################"


echo "##################### 3. build wasm ui app start#####################"
cd ${PROJECT_DIR}/wasm-apps
if [ ! -d "${PROJECT_DIR}/wasm-apps/lvgl" ]; then
    if [ -d "$BUILD_DIR/vgl-native-ui-app/lvgl" ]; then
        cp -fr $BUILD_DIR/vgl-native-ui-app/lvgl ${PROJECT_DIR}/wasm-apps
    fi
fi
./build_wasm_app.sh
cp ui_app.wasm ${OUT_DIR}/
echo "#####################  build wasm ui app end#####################"
