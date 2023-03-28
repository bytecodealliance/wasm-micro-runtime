#!/bin/bash

#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

scriptDir=$(cd "$(dirname "$0")" && pwd)
ts2wasm=$scriptDir/../../../build/cli/ts2wasm.js
samplePath=$scriptDir/../../../tests/samples

samples=$(ls $samplePath)
dataset=validate_res.txt
result=result.txt
totalFiles=0
totalFailed=0
totalIgnore=0
successIgnore=0
optimize_level=0

if [ $# -eq 1 ]
then
    optimize_level=$1
fi

rm $result
for sampleFile in $samples
    do
        sampleName=$(echo $sampleFile | cut -d . -f1)
        line=$(grep -w $sampleName $dataset)
        # all test file must exist in validate_res.txt
        if [ -z "$line" ]
        then
            echo "Warn: $sampleName is not in validate_res.txt"
            continue
        fi
        node $ts2wasm $samplePath/$sampleFile --opt o${optimize_level} --output $sampleName.wasm

        ignore=$(echo $line | awk -F ' ' '{print $2}')
        # # not validate iff 0
        if [ $ignore -eq 0 ]
        then
            totalIgnore=`expr $totalIgnore + 1`
        fi
        totalFiles=`expr $totalFiles + 1`
        value=$(d8 --experimental-wasm-gc runWasm.js -- $line)
        res=$(echo $value | awk 'NR==1')
        if test $res == "true"
        then
            if [ $ignore -eq 0 ]
            then
                echo "[validate ignored case success] $sampleName.wasm" >> result.txt
                successIgnore=`expr $successIgnore + 1`
            else
                echo "[validate success] $sampleName.wasm" >> result.txt
            fi
        else
            echo "[validate failed] $sampleName.wasm" >> result.txt
            totalFailed=`expr $totalFailed + 1`
        fi
    done
    rm -rf *.wasm

failedIgnore=`expr $totalIgnore - $successIgnore`
echo "Totally $totalFiles files are validated, and $totalFailed files are validate failed, include ignored files  $failedIgnore" >> result.txt
