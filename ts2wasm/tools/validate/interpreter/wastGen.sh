#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

scriptDir=$(cd "$(dirname "$0")" && pwd)
ts2wasm=$scriptDir/../../../build/cli/ts2wasm.js
samplePath=${1:-$scriptDir/../../../tests/samples}
samples=$(ls $samplePath)
outputPath=$scriptDir/../output/wasmFiles

# any type is disabled
for sampleFile in $samples
    do
        sampleName=$(echo $sampleFile | cut -d . -f1)
        node $ts2wasm $samplePath/$sampleFile --disableAny --disableBuiltIn --output $outputPath/$sampleName.wasm --wat $outputPath/$sampleName.wat
        rm -f $outputPath/$sampleName.wasm
    done
