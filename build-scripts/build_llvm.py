#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import argparse
import os
import pathlib
import shlex
import shutil
import subprocess
import sys


def clone_llvm(dst_dir, llvm_repo, llvm_branch):
    """
    any error will raise CallProcessError
    """
    llvm_dir = dst_dir.joinpath("llvm").resolve()

    if not llvm_dir.exists():
        print(f"Clone llvm to {llvm_dir} ...")
        GIT_CLONE_CMD = f"git clone --depth 1 --branch {llvm_branch} {llvm_repo} llvm"
        subprocess.check_output(shlex.split(GIT_CLONE_CMD), cwd=dst_dir)
    else:
        print(f"There is an LLVM local repo in {llvm_dir}, keep using it")

    return llvm_dir


def build_llvm(llvm_dir, platform, backends):
    LLVM_COMPILE_OPTIONS = [
        '-DCMAKE_BUILD_TYPE:STRING="Release"',
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DLLVM_APPEND_VC_REV:BOOL=ON",
        "-DLLVM_BUILD_BENCHMARKS:BOOL=OFF",
        "-DLLVM_BUILD_DOCS:BOOL=OFF",
        "-DLLVM_BUILD_EXAMPLES:BOOL=OFF",
        "-DLLVM_BUILD_LLVM_DYLIB:BOOL=OFF",
        "-DLLVM_BUILD_TESTS:BOOL=OFF",
        "-DLLVM_CCACHE_BUILD:BOOL=OFF",
        "-DLLVM_ENABLE_BINDINGS:BOOL=OFF",
        "-DLLVM_ENABLE_IDE:BOOL=OFF",
        "-DLLVM_ENABLE_LIBXML2:BOOL=OFF",
        "-DLLVM_ENABLE_TERMINFO:BOOL=OFF",
        "-DLLVM_ENABLE_ZLIB:BOOL=OFF",
        "-DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF",
        "-DLLVM_INCLUDE_DOCS:BOOL=OFF",
        "-DLLVM_INCLUDE_EXAMPLES:BOOL=OFF",
        "-DLLVM_INCLUDE_UTILS:BOOL=OFF",
        "-DLLVM_INCLUDE_TESTS:BOOL=OFF",
        "-DLLVM_INCLUDE_TOOLS:BOOL=OFF",
        "-DLLVM_OPTIMIZED_TABLEGEN:BOOL=ON",
    ]

    LLVM_EXTRA_COMPILER_OPTIONS = {
        "arc": [
            '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD:STRING="ARC"',
            "-DLLVM_ENABLE_LIBICUUC:BOOL=OFF",
            "-DLLVM_ENABLE_LIBICUDATA:BOOL=OFF",
        ],
        "xtensa": [
            '-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD:STRING="Xtensa"',
        ],
        "windows": [
            "-DCMAKE_INSTALL_PREFIX=LLVM-install",
        ],
        "default": [],
    }

    LLVM_TARGETS_TO_BUILD = [
        "-DLLVM_TARGETS_TO_BUILD:STRING=" + ";".join(backends)
        if backends
        else '-DLLVM_TARGETS_TO_BUILD:STRING="AArch64;ARM;Mips;RISCV;X86"'
    ]

    if not llvm_dir.exists():
        raise Exception(f"{llvm_dir} doesn't exist")

    build_dir = llvm_dir.joinpath(
        "win32build" if "windows" == platform else "build"
    ).resolve()
    build_dir.mkdir(exist_ok=True)

    lib_llvm_core_library = build_dir.joinpath("lib/libLLVMCore.a").resolve()
    if lib_llvm_core_library.exists():
        print(f"Please remove {build_dir} manually and try again")
        return

    compile_options = " ".join(
        LLVM_COMPILE_OPTIONS
        + LLVM_EXTRA_COMPILER_OPTIONS.get(
            platform, LLVM_EXTRA_COMPILER_OPTIONS["default"]
        )
        + LLVM_TARGETS_TO_BUILD
    )

    CONFIG_CMD = f"cmake {compile_options} ../llvm "
    subprocess.check_call(shlex.split(CONFIG_CMD), cwd=build_dir)

    BUILD_CMD = f"cmake --build . --parallel {os.cpu_count()}" + (
        " --config Release" if "windows" == platform else ""
    )

    subprocess.check_call(shlex.split(BUILD_CMD), cwd=build_dir)

    return build_dir


def main():
    parser = argparse.ArgumentParser(description="build necessary LLVM libraries")
    parser.add_argument(
        "--platform",
        type=str,
        choices=["android", "arc", "darwin", "linux", "windows", "xtensa"],
        help="identify current platform",
    )
    parser.add_argument(
        "--arch",
        nargs="+",
        type=str,
        choices=["AArch64", "ARC", "ARM", "Mips", "RISCV", "X86", "Xtensa"],
        help="identify LLVM supported backends, separate by space, like '--arch ARM Mips X86'",
    )
    options = parser.parse_args()

    # if the "platform" is not identified in the command line option,
    # detect it
    if not options.platform:
        if sys.platform.startswith("win32") or sys.platform.startswith("msys"):
            platform = "windows"
        elif sys.platform.startswith("darwin"):
            platform = "darwin"
        else:
            platform = "linux"
    else:
        platform = options.platform

    print(f"========== Build LLVM for {platform} ==========\n")

    llvm_repo_and_branch = {
        "arc": {
            "repo": "https://github.com/llvm/llvm-project.git",
            "branch": "release/13.x"
        },
        "xtensa": {
            "repo": "https://github.com/espressif/llvm-project.git",
            "branch": "xtensa_release_11.0.0",
        },
        "default": {
            "repo": "https://github.com/llvm/llvm-project.git",
            "branch": "release/13.x",
        },
    }

    # retrieve the real file
    current_file = pathlib.Path(__file__)
    if current_file.is_symlink():
        current_file = pathlib.Path(os.readlink(current_file))

    current_dir = current_file.parent.resolve()
    deps_dir = current_dir.joinpath("../core/deps").resolve()

    print(f"==================== CLONE LLVM ====================")
    llvm_info = llvm_repo_and_branch.get(platform, llvm_repo_and_branch["default"])
    llvm_dir = clone_llvm(deps_dir, llvm_info["repo"], llvm_info["branch"])

    print()
    print(f"==================== BUILD LLVM ====================")
    build_llvm(llvm_dir, platform, options.arch)

    print()


if __name__ == "__main__":
    main()
