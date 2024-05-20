#!/bin/bash
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#


# 1.check parameter
if [ ! $1 ]; then
	echo "Parameter is empty, please enter parameter !"
    exit 
fi

# 2.check dir
buildPath="./build"
corpusPath="$buildPath/CORPUS_DIR"
if [[ ! -d "$buildPath" ]]; then
    echo "auto create the build folder !"
    mkdir build
else # build Folder exists
    if [[ -d "$buildPath" ]]; then # CORPUS_DIR exists
        rm -rf $corpusPath
    fi
fi

# 3.change dir
# cd build && mkdir CORPUS_DIR && cd CORPUS_DIR
cd build && mkdir CORPUS_DIR && cd CORPUS_DIR

# 4.generate *.wasm file
echo "Generate $@ files according to user requirements"

for((i=1; i<($@+1); i++));
do
head -c 100 /dev/urandom | wasm-tools smith -o test_$i.wasm
done

# 5.check wasm file
dir=$(pwd)              
d=$(find . ! -name "." -type d -prune -o -type f -name "*.wasm" -print) 
#echo "current dir=$dir" 
num=0

for((i=1; i<($@+1); i++));
do
    wasmFile="test_$i.wasm"
    if [[ ! -f "$wasmFile" ]]; then
        echo "The file $wasmFile is not exists !"
    else
        let "num++"
    fi
done

echo "$@ user requirements, $num actually generated !"

if [ $num == $@ ]; then echo "Wasm file generated successfully !"
else echo "Wasm file generated faild !"
fi
