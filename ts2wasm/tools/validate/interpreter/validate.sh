#!/bin/bash
#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# use spec interpreter to validate

scriptDir=$(cd "$(dirname "$0")" && pwd)
samplePath=${1:-$scriptDir/../output/wasmFiles}
samples=$(ls $samplePath)
outputPath=$scriptDir/../output/watFiles
outputText=$scriptDir/../output/interpreter.txt

for sampleFile in $samples
    do
        sampleName=$(echo $sampleFile | cut -d . -f1)
        wasm $samplePath/$sampleFile -o $outputPath/$sampleName.wat >> $outputText 2>&1
    done
