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
EXPECTED_NUM=$1

# 2.check dir
buildPath="./build"
corpusPath="$buildPath/CORPUS_DIR"
rm -rf "${corpusPath}"
mkdir -p "${corpusPath}"

# 3.change dir
cd "${corpusPath}"

# 4.generate *.wasm file
echo "Generating $EXPECTED_NUM Wasm files for each kind as required"

# Generate wasm files with different features
# Try on and on until the generated wasm file exists
function try_generate_wasm()
{
    SMITH_OPTIONS=$1
    GENERATED_WASM_NAME=$2

    local try_i=0
    until [[ -f $GENERATED_WASM_NAME ]]; do
        head -c 100 /dev/urandom | wasm-tools smith $SMITH_OPTIONS -o $GENERATED_WASM_NAME  >/dev/null 2>&1
        try_i=$((try_i+1))
    done

    printf -- "-- output ${GENERATED_WASM_NAME} in %d retries\n" $try_i
}

# try_generate_wasm "--min-memories=1 --min-tables=1" "test_min.wasm"

for i in $(seq 1 $EXPECTED_NUM)
do
    # by default
    try_generate_wasm "" test_$i.wasm

    # with different features
    # mvp
    try_generate_wasm "--min-memories=1 --min-tables=1" test_min_$i.wasm
    try_generate_wasm "--min-memories=1 --min-tables=1 --bulk-memory-enabled true" test_bulk_$i.wasm
    try_generate_wasm "--min-memories=1 --min-tables=1 --reference-types-enabled true" test_ref_$i.wasm
    try_generate_wasm "--min-memories=1 --min-tables=1 --multi-value-enabled true" test_multi_$i.wasm
    try_generate_wasm "--min-memories=1 --min-tables=1 --simd-enabled true" test_simd_$i.wasm
    try_generate_wasm "--min-memories=1 --min-tables=1 --tail-call-enabled true " test_tail_$i.wasm

    # enable me when compiling iwasm with those features
    #try_generate_wasm "--min-memories=1 --min-tables=1 --threads-enabled true" test_thread_$i.wasm
    #try_generate_wasm "--min-memories=1 --min-tables=1 --memory64-enabled true" test_memory64_$i.wasm
    #try_generate_wasm "--min-memories=1 --min-tables=1 --exceptions-enabled true" test_exception_$i.wasm
    #try_generate_wasm "--min-memories=1 --min-tables=1 --gc-enabled true" test_gc_$i.wasm
    # with custom-section
    try_generate_wasm "--min-memories=1 --min-tables=1 --generate-custom-sections true" test_custom_$i.wasm
done

printf "Done\n"
