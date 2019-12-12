#!/bin/bash

PROJECT_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build

if [ -z $KW_BUILD ] || [ -z $KW_OUT_FILE ];then
    echo "Local Build Env"
    cmakewrap="cmake"
    makewrap="make"
else
    echo "Klocwork Build Env"
    cmakewrap="cmake -DCMAKE_BUILD_TYPE=Debug"
    makewrap="kwinject -o $KW_OUT_FILE make"
fi

if [ ! -d $BUILD_DIR ]; then
    mkdir ${BUILD_DIR}
fi

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}


cd ${WAMR_DIR}/core/shared-lib/mem-alloc
if [ ! -d "tlsf" ]; then
    git clone https://github.com/mattconte/tlsf
fi

cd ${WAMR_DIR}/core/iwasm/lib/3rdparty
if [ ! -d "lvgl" ]; then
        git clone https://github.com/littlevgl/lvgl.git --branch v6.0.1
fi
if [ ! -d "lv_drivers" ]; then
        git clone https://github.com/littlevgl/lv_drivers.git
fi

echo "##################### 1. build native-ui-app start#####################"
cd $BUILD_DIR
mkdir -p lvgl-native-ui-app
cd lvgl-native-ui-app
$cmakewrap ${PROJECT_DIR}/lvgl-native-ui-app
$makewrap
if [ $? != 0 ];then
    echo "BUILD_FAIL native-ui-app $?\n"
    exit 2
fi
echo $PWD
cp  lvgl_native_ui_app ${OUT_DIR}
echo "#####################build native-ui-app success"


echo "##################### 2. build littlevgl wasm runtime start#####################"
cd $BUILD_DIR
mkdir -p wasm-runtime-wgl
cd wasm-runtime-wgl
$cmakewrap ${PROJECT_DIR}/wasm-runtime-wgl/linux-build
$makewrap
cp wasm_runtime_wgl ${OUT_DIR}/

echo "##################### build littlevgl wasm runtime end#####################"

echo "#####################build host-tool"
cd $BUILD_DIR
mkdir -p host-tool
cd host-tool
$cmakewrap ${WAMR_DIR}/test-tools/host-tool
$makewrap
if [ $? != 0 ];then
        echo "BUILD_FAIL host tool exit as $?\n"
        exit 2
fi
cp host_tool ${OUT_DIR}
echo "#####################build host-tool success"


echo "##################### 3. build wasm ui app start#####################"
cd ${PROJECT_DIR}/wasm-apps/wgl
$makewrap
cp ui_app.wasm ${OUT_DIR}/
cd ${PROJECT_DIR}/wasm-apps/lvgl-compatible
$makewrap
cp ui_app_lvgl_compatible.wasm ${OUT_DIR}/
echo "#####################  build wasm ui app end#####################"
