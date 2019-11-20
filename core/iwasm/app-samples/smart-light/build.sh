# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

clang-8 --target=wasm32 -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -Wl,--allow-undefined, \
        -Wl,--export=main, \
        -Wl,--no-threads,--strip-all,--no-entry \
        -nostdlib -o test.wasm *.c
#./jeffdump -o ../test_wasm.h -n wasm_test_file test.wasm
