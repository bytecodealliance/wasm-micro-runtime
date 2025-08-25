# Copyright (C) 2024 Grenoble INP - ESISAR.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Python script to convert wasm file to byte array in a .h file
import os

CWD = os.getcwd()
CMAKE_CURRENT_BINARY_DIR = os.getenv('CMAKE_CURRENT_BINARY_DIR', CWD)
CMAKE_CURRENT_SOURCE_DIR = os.getenv('CMAKE_CURRENT_SOURCE_DIR', f'{CWD}/../src')

LICENCE_HEADER = """/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (C) 2024 Grenoble INP - ESISAR Limited
 * 
 * SPDX-License-Identifier: Apache-2.0
 */
"""

print('CMAKE_CURRENT_BINARY_DIR:', CMAKE_CURRENT_BINARY_DIR)
print('CMAKE_CURRENT_SOURCE_DIR:', CMAKE_CURRENT_SOURCE_DIR)

# Open the wasm file in binary mode and read the data
with open(f'{CWD}/wasm-apps/http_get.wasm', 'rb') as f:
    wasm_bytes = f.read()

# Convert the bytes to a comma-separated string of hex values
byte_array = ', '.join(f'0x{byte:02x}' for byte in wasm_bytes)

# Create the output string
output = f'unsigned char __aligned(4) wasm_test_file[] = {{ {byte_array} }};'

# Write the output string to the .h file
with open(f'{CWD}/src/http_get.h', 'w') as f:
    f.write(output)