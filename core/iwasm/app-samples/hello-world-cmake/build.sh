#!/bin/bash

mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../../../test-tools/toolchain/wamr_toolchain.cmake
make