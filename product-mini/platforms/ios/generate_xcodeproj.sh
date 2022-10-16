#!/bin/sh

rm -rf ./iwasm-proj
git clone https://github.com/leetal/ios-cmake.git ios-cmake
cmake -Biwasm-proj -G Xcode -DDEPLOYMENT_TARGET=11.0 -DPLATFORM=OS64 -DENABLE_BITCODE=0 -DCMAKE_TOOLCHAIN_FILE=ios-cmake/ios.toolchain.cmake .
