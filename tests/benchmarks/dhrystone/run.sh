#!/bin/bash

readonly IWASM_CMD="../../../product-mini/platforms/linux/build/iwasm"

echo "============> run dhrystone native"
./dhrystone_native

echo "============> run dhrystone.aot"
${IWASM_CMD} dhrystone.aot

echo "============> run dhrystone_segue.aot"
${IWASM_CMD} dhrystone_segue.aot
