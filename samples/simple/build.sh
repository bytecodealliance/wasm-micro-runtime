#!/bin/bash

CURR_DIR=$PWD
WAMR_DIR=${PWD}/../..
OUT_DIR=${PWD}/out
BUILD_DIR=${PWD}/build

IWASM_ROOT=${PWD}/../../core/iwasm
APP_FRAMEWORK_DIR=${PWD}/../../core/app-framework
NATIVE_LIBS=${APP_FRAMEWORK_DIR}/app-native-shared
APP_LIB_SRC="${APP_FRAMEWORK_DIR}/base/app/*.c ${APP_FRAMEWORK_DIR}/sensor/app/*.c \
             ${APP_FRAMEWORK_DIR}/connection/app/*.c ${NATIVE_LIBS}/*.c"
WASM_APPS=${PWD}/wasm-apps
CLEAN=

usage ()
{
    echo "build.sh [options]"
    echo " -p [platform]"
    echo " -t [target]"
    echo " -c, rebuild SDK"
    exit 1
}


while getopts "p:t:ch" opt
do
    case $opt in
        p)
        PLATFORM=$OPTARG
        ;;
        t)
        TARGET=$OPTARG
        ;;
        c)
        CLEAN="TRUE"
        ;;
        h)
        usage
        exit 1;
        ;;
        ?)
        echo "Unknown arg: $arg"
        usage
        exit 1
        ;;
    esac
done


rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/wasm-apps

cd ${WAMR_DIR}/core/shared/mem-alloc
if [ ! -d "tlsf" ]; then
    git clone https://github.com/mattconte/tlsf
fi

echo "#####################build wamr sdk"
cd ${WAMR_DIR}/wamr-sdk
./build_sdk.sh -n simple -x ${CURR_DIR}/wamr_config_simple.cmake $*

echo "#####################build simple project"
cd ${CURR_DIR}
mkdir -p cmake_build
cd cmake_build
cmake .. -DWAMR_BUILD_SDK_PROFILE=simple
make
if [ $? != 0 ];then
    echo "BUILD_FAIL simple exit as $?\n"
    exit 2
fi
cp -a simple ${OUT_DIR}
echo "#####################build simple project success"

echo -e "\n\n"
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

echo -e "\n\n"
echo "#####################build wasm apps"

cd ${WASM_APPS}

for i in `ls *.c`
do
APP_SRC="$i"
OUT_FILE=${i%.*}.wasm

/opt/wasi-sdk/bin/clang                                              \
        -I${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/include  \
        -L${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/lib      \
        -lapp_framework                                              \
        --target=wasm32 -O3 -z stack-size=4096 -Wl,--initial-memory=65536 \
        --sysroot=${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/libc-builtin-sysroot  \
        -Wl,--allow-undefined-file=${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/libc-builtin-sysroot/share/defined-symbols.txt \
        -Wl,--no-threads,--strip-all,--no-entry -nostdlib \
        -Wl,--export=on_init -Wl,--export=on_destroy \
        -Wl,--export=on_request -Wl,--export=on_response \
        -Wl,--export=on_sensor_event -Wl,--export=on_timer_callback \
        -Wl,--export=on_connection_data \
        -o ${OUT_DIR}/wasm-apps/${OUT_FILE} ${APP_SRC}
if [ -f ${OUT_DIR}/wasm-apps/${OUT_FILE} ]; then
        echo "build ${OUT_FILE} success"
else
        echo "build ${OUT_FILE} fail"
fi
done

echo "#####################build wasm apps done"
