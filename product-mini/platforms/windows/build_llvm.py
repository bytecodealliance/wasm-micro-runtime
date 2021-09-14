#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

#!/usr/bin/env python3
import os
import sys
from pathlib import Path

def clone_llvm():
    llvm_dir = Path("llvm")
    if(llvm_dir.exists() == False):
        print("Clone llvm to core/deps/ ..")
        for line in os.popen("git clone --branch release/11.x https://github.com/llvm/llvm-project.git llvm"):
            print(line)
    else:
        print("llvm source codes already existed")
    return llvm_dir

def main():
    current_os = sys.platform
    print("current OS is ", current_os)

    current_dir = Path.cwd()
    deps_dir = current_dir.joinpath( "../../../core/deps")

    os.chdir(deps_dir)
    llvm_dir = clone_llvm()
    os.chdir(llvm_dir)

    build_dir_name = "win32build"
    llvm_file = "LLVM.sln"

    Path(build_dir_name).mkdir(exist_ok = True)
    build_dir = Path(build_dir_name)
    os.chdir(build_dir)

    if ( not Path(llvm_file).exists()):
        core_number = os.cpu_count()
        print("Build llvm with", core_number, " cores")
        cmd = 'cmake ../llvm \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                -DCMAKE_BUILD_TYPE:STRING="Release" \
                -DLLVM_TARGETS_TO_BUILD:STRING="X86;ARM;AArch64;Mips" \
                -DLLVM_INCLUDE_GO_TESTS=OFF \
                -DLLVM_INCLUDE_TOOLS=OFF \
                -DLLVM_INCLUDE_UTILS=OFF \
                -DLLVM_ENABLE_TERMINFO=OFF \
                -DLLVM_BUILD_LLVM_DYLIB:BOOL=OFF \
                -DLLVM_OPTIMIZED_TABLEGEN:BOOL=ON \
                -DLLVM_ENABLE_ZLIB:BOOL=OFF \
                -DLLVM_INCLUDE_DOCS:BOOL=OFF \
                -DLLVM_INCLUDE_EXAMPLES:BOOL=OFF \
                -DLLVM_INCLUDE_TESTS:BOOL=OFF \
                -DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF \
                -DLLVM_APPEND_VC_REV:BOOL=OFF'
        print(cmd)
        for line in os.popen(cmd):
            print(line)
    else:
        print("llvm has already been Cmaked")

    print("Please open LLVM.sln in {} to build *Release* version".format(build_dir.absolute()))

    os.chdir(current_dir)

if __name__ == "__main__":
    main()
