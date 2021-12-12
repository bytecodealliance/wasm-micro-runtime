rm -rf build && mkdir build && cd build
cp ../sdkconfig .
cmake .. -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-esp32c3.cmake -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build .
ninja flash
