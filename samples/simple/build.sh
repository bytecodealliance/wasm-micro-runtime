#!/bin/bash

CURR_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build

IWASM_ROOT=${PWD}/../../core/iwasm
APP_LIBS=${IWASM_ROOT}/lib/app-libs
NATIVE_LIBS=${IWASM_ROOT}/lib/native-interface
APP_LIB_SRC="${APP_LIBS}/base/*.c ${APP_LIBS}/extension/sensor/*.c ${APP_LIBS}/extension/connection/*.c ${APP_LIBS}/extension/gui/src/*.c ${NATIVE_LIBS}/*.c"
WASM_APPS=${PWD}/wasm-apps

if [ -z $KW_BUILD ] || [ -z $KW_OUT_FILE ];then
    echo "Local Build Env"
    cmakewrap="cmake"
    makewrap="make"
else
    echo "Klocwork Build Env"
    cmakewrap="cmake -DCMAKE_BUILD_TYPE=Debug"
    makewrap="kwinject -o $KW_OUT_FILE make"
fi

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/wasm-apps

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

echo "#####################build simple project"
cd ${CURR_DIR}
mkdir -p cmake_build
cd cmake_build
$cmakewrap -DENABLE_GUI=YES ..
$makewrap
if [ $? != 0 ];then
    echo "BUILD_FAIL simple exit as $?\n"
    exit 2
fi
cp -a simple ${OUT_DIR}
echo "#####################build simple project success"

echo "#####################build host-tool"
cd ${WAMR_DIR}/test-tools/host-tool
mkdir -p bin
cd bin
$cmakewrap ..
$makewrap
if [ $? != 0 ];then
        echo "BUILD_FAIL host tool exit as $?\n"
        exit 2
fi
cp host_tool ${OUT_DIR}
echo "#####################build host-tool success"


echo "#####################build wasm apps"

cd ${WASM_APPS}

for i in `ls *.c | grep -v wasi`
do
APP_SRC="$i ${APP_LIB_SRC}"
OUT_FILE=${i%.*}.wasm
clang-8 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
        -I${APP_LIBS}/extension/connection \
        -I${APP_LIBS}/extension/gui \
        -DENABLE_WGL=1 \
        --target=wasm32 -O3 -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined \
        -Wl,--no-threads,--strip-all,--no-entry -nostdlib \
        -Wl,--export=on_init -Wl,--export=on_destroy \
        -Wl,--export=on_request -Wl,--export=on_response \
        -Wl,--export=on_sensor_event -Wl,--export=on_timer_callback \
        -Wl,--export=on_connection_data -Wl,--export=on_widget_event \
        -o ${OUT_DIR}/wasm-apps/${OUT_FILE} ${APP_SRC}
if [ -f ${OUT_DIR}/wasm-apps/${OUT_FILE} ]; then
        echo "build ${OUT_FILE} success"
else
        echo "build ${OUT_FILE} fail"
fi
done

#build wasi app
WASI_APP_SRC="wasi.c ${APP_LIB_SRC}"
/opt/wasi-sdk/bin/clang --target=wasm32-wasi -O3 \
        -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
        -I${APP_LIBS}/extension/connection \
        -I${APP_LIBS}/extension/gui \
        -DENABLE_WGL=1 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined \
        -Wl,--export=main \
        -Wl,--export=on_init -Wl,--export=on_destroy \
        -Wl,--export=on_request -Wl,--export=on_response \
        -Wl,--export=on_sensor_event -Wl,--export=on_timer_callback \
        -Wl,--export=on_connection_data -Wl,--export=on_widget_event \
        -o ${OUT_DIR}/wasm-apps/wasi.wasm ${WASI_APP_SRC}
        echo "build wasi.wasm success"
echo "#####################build wasm apps done"
