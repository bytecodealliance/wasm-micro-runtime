#!/bin/bash

PROJECT_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build
LV_CFG_PATH=${PROJECT_DIR}/lv_config



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


cd ${WAMR_DIR}/core/shared/mem-alloc
if [ ! -d "tlsf" ]; then
    git clone https://github.com/mattconte/tlsf
fi

cd ${WAMR_DIR}/core/deps
if [ ! -d "lvgl" ]; then
        git clone https://github.com/littlevgl/lvgl.git --branch v6.0.1
fi

echo "##################### 0. build wamr-sdk littlevgl start#####################"
cd ${WAMR_DIR}/wamr-sdk
./build_sdk.sh -n littlevgl -x ${PROJECT_DIR}/wamr_config_littlevgl.cmake -e ${LV_CFG_PATH}
[ $? -eq 0 ] || exit $?
echo "#####################build wamr-sdk littlevgl success"

echo -e "\n\n"
echo "##################### 1. build native-ui-app start#####################"
cd $BUILD_DIR
mkdir -p vgl-native-ui-app
cd vgl-native-ui-app
$cmakewrap ${PROJECT_DIR}/vgl-native-ui-app
$makewrap
if [ $? != 0 ];then
    echo "BUILD_FAIL native-ui-app $?\n"
    exit 2
fi
echo $PWD
cp  vgl_native_ui_app ${OUT_DIR}
echo "#####################build native-ui-app success"

echo -e "\n\n"
echo "##################### 2. build littlevgl wasm runtime start#####################"
cd $BUILD_DIR
mkdir -p vgl-wasm-runtime
cd vgl-wasm-runtime
$cmakewrap ${PROJECT_DIR}/vgl-wasm-runtime
$makewrap
[ $? -eq 0 ] || exit $?
cp vgl_wasm_runtime ${OUT_DIR}/

echo "##################### build littlevgl wasm runtime end#####################"

echo -e "\n\n"
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

echo -e "\n\n"
echo "##################### 3. build wasm ui app start#####################"
cd ${PROJECT_DIR}/wasm-apps
if [ ! -d "${PROJECT_DIR}/wasm-apps/lvgl" ]; then
    if [ -d "$BUILD_DIR/vgl-native-ui-app/lvgl" ]; then
        cp -fr $BUILD_DIR/vgl-native-ui-app/lvgl ${PROJECT_DIR}/wasm-apps
    fi
fi
./build_wasm_app.sh
mv ui_app.wasm ${OUT_DIR}/
mv ui_app_no_wasi.wasm ${OUT_DIR}/
rm -fr ${PROJECT_DIR}/wasm-apps/lvgl
echo "#####################  build wasm ui app end#####################"
