#!/bin/bash
#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# use binaryen interpreter to validate

scriptDir=$(cd "$(dirname "$0")" && pwd)
samplePath=${1:-$scriptDir/../output/wasmFiles}
samples=$(ls $samplePath)
outputPath=$scriptDir/../output/watFiles
outputText=$scriptDir/../output/interpreter.txt
temp=temp.txt
total=0

echo $samples
echo "Totally sample files are " $(ls -l $samplePath |grep "^-"|wc -l ) >> $outputText 2>&1
echo ""
for sampleFile in $samples
    do
        sampleName=$(echo $sampleFile | cut -d . -f1)
        wasm-shell $samplePath/$sampleFile > $temp 2>&1
        lines=`sed -n '$=' $temp` #1: success
        if [ $lines -gt 1 ]
        then
            echo $samplePath/$sampleFile >> $outputText 2>&1
            awk 'NR<3' $temp >> $outputText 2>&1
            echo "" >> $outputText 2>&1
            total=`expr $total + 1`
        fi
    done

rm ${temp}

echo ""
echo "Totally" $total "files failed" >> $outputText 2>&1

