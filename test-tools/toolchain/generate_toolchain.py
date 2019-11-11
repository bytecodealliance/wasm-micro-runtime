#!/usr/bin/python3

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import os
import sys
import shutil
from optparse import OptionParser

import re

optParser = OptionParser()
optParser.add_option("-o", "", dest="output_dir", help="the output path of sysroot")
optParser.add_option("-f", "--api_file", dest="api_file_name",
        help="append user defined APIs to toolchain")
(options, args) = optParser.parse_args()
optParser.usage = "%prog [options] output_dir"

sysroot_path = os.path.join(os.getcwd(), "sysroot")
if not os.path.isdir(sysroot_path):
    print("Error: No sysroot folder in current path.")
    exit(0)

if options.output_dir == None:
    out_dir = sysroot_path
else:
    out_dir = os.path.join(os.path.abspath(options.output_dir), "sysroot")

def check_sysroot():
    if out_dir != sysroot_path and not os.path.isdir(out_dir):
        try:
            shutil.copytree(sysroot_path,out_dir)
        except:
            print('Error while copy sysroot')

def Search_API(pattern, file):
    f = open(file, 'r')
    content = f.read()
    f.close()
    p = re.compile(pattern, re.S | re.M)
    return p.findall(content)

def fill_defined_symbols():
    wamr_root = os.path.join(os.getcwd(), "../..")
    user_lib_dir = os.path.join(wamr_root, "core/iwasm/lib/native/extension")

    defined_symbols = []
    # WAMR extension APIs
    for lib in os.listdir(user_lib_dir):
        for file in os.listdir(os.path.join(user_lib_dir, lib)):
            if re.match(r'.*.inl', file):
                defined_symbols += Search_API(r'EXPORT_WASM_API[(](.*?)[)]', os.path.join(user_lib_dir, lib, file))

    # Base lib APIs
    defined_symbols += Search_API(r'EXPORT_WASM_API[(](.*?)[)]', 
                        os.path.join(wamr_root, "core/iwasm/lib/native/base", "base_lib_export.c"))

    # libc
    defined_symbols += Search_API(r'REG_NATIVE_FUNC[(]env, _(.*?)[)]', 
                        os.path.join(wamr_root, "core/iwasm/lib/native/libc", "libc_wrapper.c"))

    # User defined APIs
    if options.api_file_name != None:
        api_file = open(options.api_file_name)
        APIs = api_file.read().split('\n')
        if APIs != None and APIs != []:
            defined_symbols += APIs
    
    defined_symbols = [i for i in defined_symbols if len(i) != 0]
    symbol_file = open(os.path.join(out_dir, "share/defined-symbols.txt"), 'w')
    symbol_file.write('\n'.join(defined_symbols))
    symbol_file.close()

def generate_toolchain_file():
    cmake_content = """
    SET(CMAKE_SYSTEM_NAME Linux)
    SET(CMAKE_SYSTEM_PROCESSOR wasm32)
    SET (CMAKE_SYSROOT                {})

    SET (CMAKE_C_FLAGS                "-nostdlib" CACHE INTERNAL "")
    SET (CMAKE_C_COMPILER_TARGET      "wasm32")
    SET (CMAKE_C_COMPILER             "clang-8")

    SET (CMAKE_CXX_FLAGS                "-nostdlib" CACHE INTERNAL "")
    SET (CMAKE_CXX_COMPILER_TARGET      "wasm32")
    SET (CMAKE_CXX_COMPILER             "clang++-8")

    SET (CMAKE_EXE_LINKER_FLAGS "-Wl,--no-entry,--export-all,--allow-undefined-file=${{CMAKE_SYSROOT}}/share/defined-symbols.txt" CACHE INTERNAL "")
    SET (CMAKE_LINKER  "/usr/bin/wasm-ld-8" CACHE INTERNAL "")

    SET (CMAKE_AR      "/usr/bin/llvm-ar-8" CACHE INTERNAL "")
    SET (CMAKE_NM      "/usr/bin/llvm-nm-8" CACHE INTERNAL "")
    SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump-8" CACHE INTERNAL "")
    SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib-8" CACHE INTERNAL "")
    """.format(out_dir)
    f = open(os.path.join(out_dir, "..", "wamr_toolchain.cmake"), 'w')
    f.write(cmake_content)
    f.close()

def main():
    check_sysroot()
    fill_defined_symbols()
    generate_toolchain_file()

    print("Successfully generate wamr toolchain")
    print("Now you can use this command to build your cmake based project:")
    print("cmake /path/to/CMakeLists.txt -DCMAKE_TOOLCHAIN_FILE={}"
            .format(os.path.abspath(os.path.join(out_dir, "..", "wamr_toolchain.cmake"))))

if __name__ == '__main__':
    main()

