#!/usr/bin/env python3

#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import json
import os
import subprocess
import glob
import re
from typing import Dict

WORK_DIR = os.getcwd()
TEST_WASM_COMMAND = (
    "./build/build-{runtime}/iwasm {running_options} {running_mode} {file} {argument}"
)

COMPILE_AOT_COMMAND = "./build/build-wamrc/{compiler} {options} -o {out_file} {in_file}"
TEST_AOT_COMMAND = "./build/build-{runtime}/iwasm {running_options} {file} {argument}"

LOG_FILE = "issues_tests.log"
LOG_ENTRY = """
=======================================================
Failing issue id: {}.
run with command_lists: {}
{}
{}
=======================================================
"""


# Function to read and parse JSON file
def read_json_file(file_path):
    with open(file_path, "r") as file:
        return json.load(file)
    return None


def dump_error_log(failing_issue_id, command_lists, exit_code_cmp, stdout_cmp):
    with open(LOG_FILE, "a") as file:
        file.write(
            LOG_ENTRY.format(failing_issue_id, command_lists, exit_code_cmp, stdout_cmp)
        )


def get_issue_ids_should_test():
    # Define the path pattern
    path_pattern = "issues/issue-*"

    # Regular expression to extract the number
    pattern = r"issue-(\d+)"

    # Initialize a set to store the issue numbers
    issue_numbers = set()

    # Use glob to find directories matching the pattern
    for dir_path in glob.glob(path_pattern):
        # Extract the issue number using regular expression
        match = re.search(pattern, dir_path)
        if match:
            issue_number = match.group(1)
            issue_numbers.add(int(issue_number))

    # Print the set of issue numbers
    return issue_numbers


def get_and_check(d, key, default=None, nullable=False):
    element = d.get(key, default)

    if not nullable and element is None:
        raise Exception(f"Missing {key} in {d}")

    return element


def run_and_compare_results(
    passed_ids, failed_ids, issue_id, cmd, description, ret_code, stdout_content
):
    print(f"####################################")
    print(f"test BA issue #{issue_id} `{description}`: {cmd}")
    command_list = cmd.split()
    result = subprocess.run(
        command_list,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        errors="ignore",
    )

    actual_exit_code = result.returncode
    actual_output = result.stdout.rstrip("\n")

    exit_code_cmp = f"exit code (actual, expected) : {actual_exit_code, ret_code}"
    stdout_cmp = f"stdout (actual, expected) : {actual_output, stdout_content}"
    print(exit_code_cmp)
    print(stdout_cmp)

    if actual_exit_code == ret_code and (
        actual_output == stdout_content
        or (stdout_content == "Compile success"
            and actual_output.find(stdout_content) != -1)
        or (len(stdout_content) > 30 and actual_output.find(stdout_content) != -1)
    ):
        passed_ids.add(issue_id)
        print("== PASS ==")
    else:
        failed_ids.add(issue_id)
        print(f"== FAILED: {issue_id} ==")
        dump_error_log(
            issue_id,
            command_list,
            exit_code_cmp,
            stdout_cmp,
        )

    print("")


def run_issue_test_wamrc(
    passed_ids, failed_ids, issue_id, compile_options, stdout_only_cmp_last_line=False
):
    compiler = get_and_check(compile_options, "compiler")
    only_compile = get_and_check(compile_options, "only compile")
    in_file = get_and_check(compile_options, "in file")
    out_file = get_and_check(compile_options, "out file")
    options = get_and_check(compile_options, "options")

    expected_return = get_and_check(compile_options, "expected return")
    ret_code = get_and_check(expected_return, "ret code")
    stdout_content = get_and_check(expected_return, "stdout content")
    description = get_and_check(expected_return, "description")

    issue_path = os.path.join(WORK_DIR, f"issues/issue-{issue_id}/")
    # file maybe *.wasm or *.aot, needs to the match the exact file name
    actual_file = glob.glob(issue_path + in_file)
    assert len(actual_file) == 1
    # the absolute file path
    in_file_path = os.path.join(issue_path, actual_file[0])
    out_file_path = os.path.join(issue_path, out_file)

    cmd = COMPILE_AOT_COMMAND.format(
        compiler=compiler, options=options, out_file=out_file_path, in_file=in_file_path
    )

    run_and_compare_results(
        passed_ids, failed_ids, issue_id, cmd, description, ret_code, stdout_content
    )

    return only_compile


def run_issue_test_iwasm(passed_ids, failed_ids, issue_id, test_case):
    runtime = get_and_check(test_case, "runtime")
    mode = get_and_check(test_case, "mode")
    file = get_and_check(test_case, "file")
    options = get_and_check(test_case, "options")
    argument = get_and_check(test_case, "argument")

    expected_return = get_and_check(test_case, "expected return")
    ret_code = get_and_check(expected_return, "ret code")
    stdout_content = get_and_check(expected_return, "stdout content")
    description = get_and_check(expected_return, "description")

    issue_path = os.path.join(WORK_DIR, f"issues/issue-{issue_id}/")
    # file maybe *.wasm or *.aot, needs to the match the exact file name
    actual_file = glob.glob(issue_path + file)
    assert len(actual_file) == 1
    # the absolute file path
    file_path = os.path.join(issue_path, actual_file[0])

    if mode == "aot":
        cmd = TEST_AOT_COMMAND.format(
            runtime=runtime,
            file=file_path,
            running_options=options,
            argument=argument,
        )
    else:
        if mode == "classic-interp":
            running_mode = "--interp"
        elif mode == "fast-interp":
            running_mode = ""
        else:
            running_mode = f"--{mode}"

        cmd = TEST_WASM_COMMAND.format(
            runtime=runtime,
            running_mode=running_mode,
            file=file_path,
            running_options=options,
            argument=argument,
        )

    run_and_compare_results(
        passed_ids, failed_ids, issue_id, cmd, description, ret_code, stdout_content
    )


def process_and_run_test_cases(data: Dict[str, Dict]):
    issue_ids_should_test = get_issue_ids_should_test()

    passed_ids = set()
    failed_ids = set()

    for test_case in data.get("test cases", []):
        is_deprecated = get_and_check(test_case, "deprecated")
        issue_ids = get_and_check(test_case, "ids", default=[])

        if is_deprecated:
            print(f"test case {issue_ids} are deprecated, continue running nest one(s)")
            continue

        compile_options = get_and_check(test_case, "compile_options", nullable=True)
        for issue_id in issue_ids:
            only_compile = False
            # if this issue needs to test wamrc to compile the test case first
            if compile_options:
                only_compile = compile_options["only compile"]
                run_issue_test_wamrc(passed_ids, failed_ids, issue_id, compile_options)

            # if this issue requires to test iwasm to run the test case
            if not only_compile:
                run_issue_test_iwasm(passed_ids, failed_ids, issue_id, test_case)

            # cross out the this issue_id in the should test set
            issue_ids_should_test.remove(issue_id)

    total = len(passed_ids) + len(failed_ids)
    passed = len(passed_ids)
    failed = len(failed_ids)
    issue_ids_should_test = (
        issue_ids_should_test if issue_ids_should_test else "no more"
    )
    print(f"==== Test results ====")
    print(f"  Total: {total}")
    print(f"  Passed: {passed}")
    print(f"  Failed: {failed}")


def main():
    # Path to the JSON file
    file_path = "running_config.json"

    # Read and parse the JSON file
    data = read_json_file(file_path)

    # Check if data is successfully read
    if data is None:
        assert 0, "No data to process."

    # Remove the log file from last run if it exists
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    # Process the data
    process_and_run_test_cases(data)


if __name__ == "__main__":
    main()
