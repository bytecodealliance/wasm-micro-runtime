#!/bin/bash

PROJECT_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build
WAMR_RUNTIME_CFG=${PROJECT_DIR}/wamr_config_gui.cmake
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
if [ ! -d "lv_drivers" ]; then
        git clone https://github.com/littlevgl/lv_drivers.git
fi

echo -e "\n\n"
echo "##################### 0. build wamr-sdk gui start#####################"
cd ${WAMR_DIR}/wamr-sdk
./build_sdk.sh -n gui -x ${WAMR_RUNTIME_CFG} -e ${LV_CFG_PATH}
[ $? -eq 0 ] || exit $?

echo "#####################build wamr-sdk success"


echo -e  "\n\n"
echo "##################### 1. build native-ui-app start#####################"
cd $BUILD_DIR
mkdir -p lvgl-native-ui-app
cd lvgl-native-ui-app
$cmakewrap ${PROJECT_DIR}/lvgl-native-ui-app
[ $? -eq 0 ] || exit $?
$makewrap
if [ $? != 0 ];then
    echo "BUILD_FAIL native-ui-app $?\n"
    exit 2
fi
echo $PWD
cp  lvgl_native_ui_app ${OUT_DIR}
echo "#####################build native-ui-app success"
echo -e "\n\n"


echo "##################### 2. build wasm runtime start#####################"
cd $BUILD_DIR
mkdir -p wasm-runtime-wgl
cd wasm-runtime-wgl
$cmakewrap ${PROJECT_DIR}/wasm-runtime-wgl/linux-build -DWAMR_BUILD_SDK_PROFILE=gui
[ $? -eq 0 ] || exit $?
$makewrap
[ $? -eq 0 ] || exit $?
cp wasm_runtime_wgl ${OUT_DIR}/

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
cd ${PROJECT_DIR}/wasm-apps/wgl

rm -rf build
mkdir build && cd build
$cmakewrap .. -DCMAKE_TOOLCHAIN_FILE=${WAMR_DIR}/wamr-sdk/out/gui/app-sdk/wamr_toolchain.cmake
$makewrap
[ $? -eq 0 ] || exit $?
mv ui_app.wasm ${OUT_DIR}/

# $makewrap
# mv ui_app.wasm ${OUT_DIR}/

cd ${PROJECT_DIR}/wasm-apps/lvgl-compatible
$makewrap
[ $? -eq 0 ] || exit $?
mv ui_app_lvgl_compatible.wasm ${OUT_DIR}/
echo "#####################  build wasm ui app end#####################"
