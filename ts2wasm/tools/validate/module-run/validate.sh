#!/bin/bash

#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

scriptDir=$(cd "$(dirname "$0")" && pwd)
ts2wasm=$scriptDir/../../../build/cli/ts2wasm.js
samplePath=${1:-$scriptDir/../../../tests/samples}

samples=$(ls $samplePath)
dataset=validate_res.txt
result=result.txt
totalFiles=0
totalFailed=0

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
            # exit 1
        fi
        node $ts2wasm $samplePath/$sampleFile --disableAny --disableInterface --output $sampleName.wasm

        rule=$(echo $line | awk -F ' ' '{print $2}')
        # not validate iff 0
        if [ $rule -eq 0 ]
        then
            continue
        else
            totalFiles=`expr $totalFiles + 1`
            # if you build V8 with snapshot, you can use this command to validate
            # d8 --snapshot_blob="/path/to/snapshot_blob.bin" --experimental-wasm-gc  validate.js -- $outputPath/$line
            value=$(d8 --experimental-wasm-gc validate.js -- $line)
            res=$(echo $value | awk 'NR==1')
            if test $res == "true"
            then
                echo "[validate success] $sampleName.wasm" >> result.txt
            else
                echo "[validate failed] $sampleName.wasm" >> result.txt
                totalFailed=`expr $totalFailed + 1`
            fi

        fi
    done
    rm -rf *.wasm

echo "Totally $totalFiles files are validated, and $totalFailed files are validate failed" >> result.txt
