#!/usr/bin/env bash

sudo apt install build-essential cmake g++-multilib libgcc-9-dev lib32gcc-9-dev ccache

cd product-mini/platforms/linux/
mkdir build && cd build
cmake ..
make
# iwasm is generated under current directory
