#!/bin/bash

PLATFORM=$(uname -s | tr A-Z a-z)

readonly IWASM_CMD="../../../product-mini/platforms/${PLATFORM}/build/iwasm"

echo "============> run dhrystone native"
./dhrystone_native

echo "============> run dhrystone.aot"
${IWASM_CMD} dhrystone.aot
