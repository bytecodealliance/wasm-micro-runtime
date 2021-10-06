#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import json
import os
import pathlib
import queue
import re
import shlex
import shutil
import subprocess
import sys

CLANG_CMD = "clang-13"
CLANG_CPP_CMD = "clang-cpp-13"
CLANG_FORMAT_CMD = "clang-format-13"
CLANG_TIDY_CMD = "clang-tidy-13"
CMAKE_CMD = "cmake"


# glob style patterns
EXCLUDE_PATHS = [
    "**/.git/",
    "**/.github/",
    "**/.vscode/",
    "**/assembly-script/",
    "**/build/",
    "**/build-scripts/",
    "**/ci/",
    "**/core/deps/",
    "**/doc/",
    "**/samples/workload/",
    "**/test-tools/",
    "**/wamr-sdk/",
    "**/wamr-dev/",
    "**/wamr-dev-simd/",
]

C_SUFFIXES = [".c", ".h"]

VALID_DIR_NAME = r"([a-zA-Z0-9]+\-*)+[a-zA-Z0-9]*"
VALID_FILE_NAME = r"\.?([a-zA-Z0-9]+\_*)+[a-zA-Z0-9]*\.*\w*"


def locate_command(command):
    if not shutil.which(command):
        print(f"Command '{command}'' not found")
        return False

    return True


def is_excluded(path):
    for exclude_path in EXCLUDE_PATHS:
        if path.match(exclude_path):
            return True
    return False


def pre_flight_check(root):
    def check_clang_foramt(root):
        if not locate_command(CLANG_FORMAT_CMD):
            return False

        # Quick syntax check for .clang-format
        try:
            subprocess.check_call(
                shlex.split(f"{CLANG_FORMAT_CMD} --dump-config"), cwd=root
            )
        except subprocess.CalledProcessError:
            print(f"Might have a typo in .clang-format")
            return False
        return True

    def check_clang_tidy(root):
        if not locate_command(CLANG_TIDY_CMD):
            return False

        if (
            not locate_command(CLANG_CMD)
            or not locate_command(CLANG_CPP_CMD)
            or not locate_command(CMAKE_CMD)
        ):
            return False

        # Quick syntax check for .clang-format
        try:
            subprocess.check_call(
                shlex.split("{CLANG_TIDY_CMD} --dump-config"), cwd=root
            )
        except subprocess.CalledProcessError:
            print(f"Might have a typo in .clang-tidy")
            return False

        # looking for compile command database
        return True

    def check_aspell(root):
        return True

    return check_clang_foramt(root) and check_clang_tidy(root) and check_aspell(root)


def run_clang_format(file_path, root):
    try:
        subprocess.check_call(
            shlex.split(
                f"{CLANG_FORMAT_CMD} --style=file --Werror --dry-run {file_path}"
            ),
            cwd=root,
        )
        return True
    except subprocess.CalledProcessError:
        print(f"{file_path} failed the check of {CLANG_FORMAT_CMD}")
        return False


def generate_compile_commands(compile_command_database, root):
    CMD = f"{CMAKE_CMD} -DCMAKE_C_COMPILER={shutil.which(CLANG_CMD)} -DCMAKE_CXX_COMPILER={shutil.which(CLANG_CPP_CMD)} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .."

    try:
        linux_mini_build = root.joinpath("product-mini/platforms/linux/build").resolve()
        linux_mini_build.mkdir(exist_ok=True)
        if subprocess.check_call(shlex.split(CMD), cwd=linux_mini_build):
            return False

        wamrc_build = root.joinpath("wamr-compiler/build").resolve()
        wamrc_build.mkdir(exist_ok=True)

        if subprocess.check_call(shlex.split(CMD), cwd=wamrc_build):
            return False

        with open(linux_mini_build.joinpath("compile_commands.json"), "r") as f:
            iwasm_compile_commands = json.load(f)

        with open(wamrc_build.joinpath("compile_commands.json"), "r") as f:
            wamrc_compile_commands = json.load(f)

        all_compile_commands = iwasm_compile_commands + wamrc_compile_commands
        # TODO: duplication items ?
        with open(compile_command_database, "w") as f:
            json.dump(all_compile_commands, f)

        return True
    except subprocess.CalledProcessError:
        return False


def run_clang_tidy(file_path, root):
    # preparatoin
    compile_command_database = pathlib.Path("/tmp/compile_commands.json")
    if not compile_command_database.exists() and not generate_compile_commands(
        compile_command_database, root
    ):
        return False

    try:
        if subprocess.check_call(
            shlex.split(f"{CLANG_TIDY_CMD} -p={compile_command_database} {file_path}"),
            cwd=root,
        ):
            print(f"{file_path} failed the check of {CLANG_TIDY_CMD}")
    except subprocess.CalledProcessError:
        print(f"{file_path} failed the check of {CLANG_TIDY_CMD}")
        return False
    return True


def run_aspell(file_path, root):
    return True


def check_dir_name(path, root):
    # since we don't want to check the path beyond root.
    # we hope "-" only will be used in a dir name as separators
    return all(
        [
            re.match(VALID_DIR_NAME, path_part)
            for path_part in path.relative_to(root).parts
        ]
    )


def check_file_name(path):
    # since we don't want to check the path beyond root.
    # we hope "_" only will be used in a file name as separators
    return re.match(VALID_FILE_NAME, path.name) is not None


def run_pre_commit_check(path, root=None):
    path = path.resolve()
    if path.is_dir():
        if not check_dir_name(path, root):
            print(f"{path} is not a valid directory name")
            return False
        else:
            return True

    if path.is_file():
        if not check_file_name(path):
            print(f"{path} is not a valid file name")
            return False

        if not path.suffix in C_SUFFIXES:
            return True

        return (
            run_clang_format(path, root)
            and run_clang_tidy(path, root)
            and run_aspell(path, root)
        )

    print(f"{path} neither a file nor a directory")
    return False


def main():
    wamr_root = pathlib.Path(__file__).parent.joinpath("..").resolve()

    if not pre_flight_check(wamr_root):
        return False

    invalid_file, invalid_directory = 0, 0

    # in order to skip exclude directories ASAP,
    # will not yield Path.
    # since we will create every object
    dirs = queue.Queue()
    dirs.put(wamr_root)
    while not dirs.empty():
        qsize = dirs.qsize()
        while qsize:
            current_dir = dirs.get()

            for path in current_dir.iterdir():
                path = path.resolve()

                if path.is_symlink():
                    continue

                if path.is_dir() and not is_excluded(path):
                    invalid_directory += (
                        0 if run_pre_commit_check(path, wamr_root) else 1
                    )
                    dirs.put(path)

                if not path.is_file():
                    continue

                invalid_file += 0 if run_pre_commit_check(path) else 1

            else:
                qsize -= 1

    print(f"invalid_directory={invalid_directory}, invalid_file={invalid_file}")
    return True


if __name__ == "__main__":
    main()
