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
cmake ..
make
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
cmake ..
make
if [ $? != 0 ];then
        echo "BUILD_FAIL host tool exit as $?\n"
        exit 2
fi
cp host_tool ${OUT_DIR}
echo "#####################build host-tool success"


echo "#####################build wasm apps"

cd ${WASM_APPS}

for i in `ls *.c`
do
APP_SRC="$i ${APP_LIB_SRC}"
OUT_FILE=${i%.*}.wasm
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -I${APP_LIBS}/extension/connection \
     -I${APP_LIBS}/extension/gui \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback', '_on_connection_data', '_on_widget_event']" \
     -o ${OUT_DIR}/wasm-apps/${OUT_FILE} ${APP_SRC}
if [ -f ${OUT_DIR}/wasm-apps/${OUT_FILE} ]; then
        echo "build ${OUT_FILE} success"
else
        echo "build ${OUT_FILE} fail"
fi
done
echo "#####################build wasm apps done"
