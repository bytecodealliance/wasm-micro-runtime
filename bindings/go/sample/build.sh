#!/usr/bin/env bash

PLATFORM=$(uname -s | tr A-Z a-z)
CUR_DIR=$PWD
WAMR_DIR=$PWD/../../..
WAMR_GO_DIR=$PWD/../wamr

cp -a ${WAMR_DIR}/core/iwasm/include/*.h ${WAMR_GO_DIR}/packaged/include

mkdir -p build && cd build
cmake ${WAMR_DIR}/product-mini/platforms/${PLATFORM} \
    -DWAMR_BUILD_LIB_PTHREAD=1 -DWAMR_BUILD_DUMP_CALL_STACK=1 \
    -DWAMR_BUILD_MEMORY_PROFILING=1
make -j ${nproc}
cp -a libvmlib.a ${WAMR_GO_DIR}/packaged/lib/${PLATFORM}-amd64

cd ${CUR_DIR}
go build rego.go
./rego
