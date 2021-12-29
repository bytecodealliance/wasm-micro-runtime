#!/bin/bash -e

rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32c3.cmake -DIDF_TARGET=esp32c3 -DCMAKE_BUILD_TYPE=Release -GNinja
cp ../sdkconfig .
cmake --build .
ninja flash
