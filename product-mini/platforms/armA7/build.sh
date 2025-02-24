rm -fr build && mkdir build
cd build
export TOOL_CHAIN_FILE=../toolchain.cmake

cmake .. -DCMAKE_TOOLCHAIN_FILE="$TOOL_CHAIN_FILE" \
         -DWAMR_BUILD_PLATFORM=linux \
         -DWAMR_BUILD_LIBC_BUILTIN=1 \
         -DWAMR_BUILD_TARGET=ARM
make
cd ..
