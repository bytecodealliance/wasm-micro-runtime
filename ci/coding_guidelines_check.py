#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
import argparse
import json
import os
import re
import pathlib
import queue
import re
import shlex
import shutil
import subprocess
import sys

CLANG_FORMAT_CMD = "clang-format-12"
GIT_CLANG_FORMAT_CMD = "git-clang-format-12"

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
    "**/samples/wasm-c-api/src/",
    "**/samples/workload/",
    "**/test-tools/",
    "**/wamr-sdk/",
    "**/wamr-dev/",
    "**/wamr-dev-simd/",
]

C_SUFFIXES = [".c", ".cpp", ".h"]
VALID_DIR_NAME = r"([a-zA-Z0-9]+\-*)+[a-zA-Z0-9]*"
INVALID_DIR_NAME_SEGMENT = r"([a-zA-Z0-9]+\_[a-zA-Z0-9]+)"
VALID_FILE_NAME = r"\.?([a-zA-Z0-9]+\_*)+[a-zA-Z0-9]*\.*\w*"
INVALID_FILE_NAME_SEGMENT = r"([a-zA-Z0-9]+\-[a-zA-Z0-9]+)"


def locate_command(command: str) -> bool:
    if not shutil.which(command):
        print(f"Command '{command}'' not found")
        return False

    return True


def is_excluded(path: pathlib) -> bool:
    for exclude_path in EXCLUDE_PATHS:
        if path.match(exclude_path):
            return True
    return False


def pre_flight_check(root: pathlib) -> bool:
    def check_aspell(root):
        return True

    def check_clang_foramt(root: pathlib) -> bool:
        if not locate_command(CLANG_FORMAT_CMD):
            return False

        # Quick syntax check for .clang-format
        try:
            subprocess.check_output(
                shlex.split(f"{CLANG_FORMAT_CMD} --dump-config"), cwd=root
            )
        except subprocess.CalledProcessError:
            print(f"Might have a typo in .clang-format")
            return False
        return True

    def check_git_clang_format() -> bool:
        return locate_command(GIT_CLANG_FORMAT_CMD)

    return check_aspell(root) and check_clang_foramt(root) and check_git_clang_format()


def run_clang_format(file_path: pathlib, root: pathlib) -> bool:
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


def run_clang_format_diff(root: pathlib, commits: str) -> bool:
    try:
        before, after = commits.split("..")
        after = after if after else "HEAD"
        COMMAND = (
            f"{GIT_CLANG_FORMAT_CMD} -v --binary "
            f"{shutil.which(CLANG_FORMAT_CMD)} --style file "
            f"--extensions c,cpp,h --diff {before} {after}"
        )

        p = subprocess.Popen(
            shlex.split(COMMAND),
            stdout=subprocess.PIPE,
            stderr=None,
            stdin=None,
            universal_newlines=True,
        )

        stdout, _ = p.communicate()
        if not stdout.startswith("diff --git"):
            return True

        diff_content = stdout.split("\n")
        for summary in [x for x in diff_content if x.startswith("diff --git")]:
            # b/path/to/file -> path/to/file
            with_invalid_format = re.split("\s+", summary)[-1][2:]
            print(
                f"--- {with_invalid_format} failed on code style checking. "
                "it is required to format modification with "
                "'clang-format-12 --style file -i path/to/files'."
            )
        else:
            return False
    except subprocess.subprocess.CalledProcessError:
        return False


def run_aspell(file_path: pathlib, root: pathlib) -> bool:
    return True


def check_dir_name(path: pathlib, root: pathlib) -> bool:
    # since we don't want to check the path beyond root.
    # we hope "-" only will be used in a dir name as separators
    found = False
    for path_part in path.relative_to(root).parts:
        m = re.search(INVALID_DIR_NAME_SEGMENT, path_part)
        if m:
            print(
                f"--- found a character '_' in {m.groups()} in {path}, "
                "it is required to replace '_' with '-'."
            )
            found = True
    else:
        return not found


def check_file_name(path: pathlib) -> bool:
    # since we don't want to check the path beyond root.
    # we hope "_" only will be used in a file name as separators
    m = re.search(INVALID_FILE_NAME_SEGMENT, path.stem)
    if m:
        print(
            f"--- found a character '-' in {m.groups()} in {path}, "
            "it is required to replace '-' with '_'."
        )

    return not m


def parse_commits_range(root: pathlib, commits: str) -> list:
    GIT_LOG_CMD = f"git log --pretty='%H' {commits}"
    try:
        ret = subprocess.check_output(
            shlex.split(GIT_LOG_CMD), cwd=root, universal_newlines=True
        )
        return [x for x in ret.split("\n") if x]
    except subprocess.CalledProcessError:
        print(f"can not parse any commit from the range {commits}")
        return []


def analysis_new_item_name(root: pathlib, commit: str) -> bool:
    GIT_SHOW_CMD = f"git show --oneline --name-status --diff-filter A {commit}"
    try:
        invalid_items = True
        output = subprocess.check_output(
            shlex.split(GIT_SHOW_CMD), cwd=root, universal_newlines=True
        )
        if not output:
            return True

        NEW_FILE_PATTERN = "^A\s+(\S+)"
        for line_no, line in enumerate(output.split("\n")):
            # bypass the first line, usually it is the commit description
            if line_no == 0:
                continue

            if not line:
                continue

            match = re.match(NEW_FILE_PATTERN, line)
            if not match:
                continue

            new_item = match.group(1)
            new_item = pathlib.Path(new_item).resolve()
            if is_excluded(new_item):
                continue

            if new_item.is_file():
                if not check_file_name(new_item):
                    invalid_items = False
                    continue

                new_item = new_item.parent

            if not check_dir_name(new_item, root):
                invalid_items = False
                continue
        else:
            return invalid_items

    except subprocess.CalledProcessError:
        return False


def process_entire_pr(root: pathlib, commits: str) -> bool:
    if not commits:
        print("Please provide a commits range")
        return False

    commit_list = parse_commits_range(root, commits)
    if not commit_list:
        print(f"Quit since there is no commit to check with")
        return True

    print(f"there are {len(commit_list)} commits in the PR")

    found = False
    if not analysis_new_item_name(root, commits):
        found = True

    if not run_clang_format_diff(root, commits):
        found = True

    return not found


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check if change meets all coding guideline requirements"
    )
    parser.add_argument(
        "-c", "--commits", default=None, help="Commit range in the form: a..b"
    )
    options = parser.parse_args()

    wamr_root = pathlib.Path(__file__).parent.joinpath("..").resolve()

    if not pre_flight_check(wamr_root):
        return False

    return process_entire_pr(wamr_root, options.commits)


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
