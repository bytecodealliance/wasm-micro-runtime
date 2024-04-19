#!/bin/bash

echo "====== Interpreter"
out/native-stack-overflow out/wasm-apps/testapp.wasm

echo
echo "====== AOT"
out/native-stack-overflow out/wasm-apps/testapp.wasm.aot

echo
echo "====== AOT WAMR_DISABLE_HW_BOUND_CHECK=1"
out/native-stack-overflow.WAMR_DISABLE_HW_BOUND_CHECK out/wasm-apps/testapp.wasm.aot.bounds-checks
