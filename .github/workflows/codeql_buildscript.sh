#!/usr/bin/env bash

sudo apt install -y build-essential cmake g++-multilib libgcc-9-dev lib32gcc-9-dev ccache ninja-build

cd wamr-compiler
./build_llvm.sh 
mkdir build && cd build
cmake .. 
make
# wamrc is generated under current directory

cd ../..

cd product-mini/platforms/linux/
mkdir build && cd build
cmake ..
make
# iwasm is generated under current directory
