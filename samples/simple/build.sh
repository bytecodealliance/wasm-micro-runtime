#!/bin/bash

CURR_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build

IWASM_ROOT=${PWD}/../../core/iwasm
APP_LIBS=${IWASM_ROOT}/lib/app-libs
NATIVE_LIBS=${IWASM_ROOT}/lib/native-interface
APP_LIB_SRC="${APP_LIBS}/base/*.c ${APP_LIBS}/extension/sensor/*.c ${NATIVE_LIBS}/*.c"
WASM_APPS=${PWD}/wasm-apps

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/wasm-apps

cd ${WAMR_DIR}/core/shared-lib/mem-alloc
if [ ! -d "tlsf" ]; then
    git clone https://github.com/mattconte/tlsf
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

cd ${CURR_DIR}

APP_SRC="${WASM_APPS}/timer/timer.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/timer.wasm ${APP_SRC}

APP_SRC="${WASM_APPS}/request_handler/request_handler.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/request_handler.wasm ${APP_SRC}

APP_SRC="${WASM_APPS}/request_sender/request_sender.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/request_sender.wasm ${APP_SRC}

APP_SRC="${WASM_APPS}/event_publisher/event_publisher.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/event_publisher.wasm ${APP_SRC}

APP_SRC="${WASM_APPS}/event_subscriber/event_subscriber.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/event_subscriber.wasm ${APP_SRC}

APP_SRC="${WASM_APPS}/sensor/sensor.c ${APP_LIB_SRC}"
emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
     -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
     -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
     -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                             '_on_sensor_event', '_on_timer_callback']" \
     -o ${OUT_DIR}/wasm-apps/sensor.wasm ${APP_SRC}

echo "#####################build wasm apps success"
