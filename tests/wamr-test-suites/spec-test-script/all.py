#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import argparse
import multiprocessing as mp
import os
import pathlib
import subprocess
import sys
import time

"""
The script itself has to be put under the same directory with the "spec".
To run a single non-GC case with interpreter mode:
  cd workspace
  python3 runtest.py --wast2wasm wabt/bin/wat2wasm --interpreter iwasm \
    spec/test/core/xxx.wast
To run a single non-GC case with aot mode:
  cd workspace
  python3 runtest.py --aot --wast2wasm wabt/bin/wat2wasm --interpreter iwasm \
    --aot-compiler wamrc spec/test/core/xxx.wast
To run a single GC case:
  cd workspace
  python3 runtest.py --wast2wasm spec/interpreter/wasm --interpreter iwasm \
    --aot-compiler wamrc --gc spec/test/core/xxx.wast
"""

PLATFORM_NAME = os.uname().sysname.lower()
IWASM_CMD = "../../../product-mini/platforms/" + PLATFORM_NAME + "/build/iwasm"
IWASM_SGX_CMD = "../../../product-mini/platforms/linux-sgx/enclave-sample/iwasm"
IWASM_QEMU_CMD = "iwasm"
SPEC_TEST_DIR = "spec/test/core"
WAST2WASM_CMD = "./wabt/out/gcc/Release/wat2wasm"
SPEC_INTERPRETER_CMD = "spec/interpreter/wasm"
WAMRC_CMD = "../../../wamr-compiler/build/wamrc"

class TargetAction(argparse.Action):
    TARGET_MAP = {
        "ARMV7_VFP": "armv7",
        "RISCV32": "riscv32_ilp32",
        "RISCV32_ILP32": "riscv32_ilp32",
        "RISCV32_ILP32D": "riscv32_ilp32d",
        "RISCV64": "riscv64_lp64",
        "RISCV64_LP64": "riscv64_lp64",
        "RISCV64_LP64D": "riscv64_lp64",
        "THUMBV7_VFP": "thumbv7",
        "X86_32": "i386",
        "X86_64": "x86_64",
    }

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, self.TARGET_MAP.get(values, "x86_64"))


def ignore_the_case(
    case_name,
    target,
    aot_flag=False,
    sgx_flag=False,
    multi_module_flag=False,
    multi_thread_flag=False,
    simd_flag=False,
    gc_flag=False,
    xip_flag=False,
    eh_flag=False,
    qemu_flag=False
):
    # print(f"case_name {case_name}\n")
    if eh_flag and case_name in [ "tag", "try_catch", "rethrow", "try_delegate" ]:
        return False
    else:
        return True


    if case_name in ["comments", "inline-module", "names"]:
        return True

    if not multi_module_flag and case_name in ["imports", "linking"]:
        return True

    # Note: x87 doesn't preserve sNaN and makes some relevant tests fail.
    if "i386" == target and case_name in ["float_exprs", "conversions"]:
        return True

    if gc_flag:
        if case_name in ["type-canon", "type-equivalence", "type-rec"]:
            return True;

    if sgx_flag:
        if case_name in ["conversions", "f32_bitwise", "f64_bitwise"]:
            return True

        if aot_flag and case_name in [
            "call_indirect",
            "call",
            "fac",
            "skip-stack-guard-page",
        ]:
            return True

    if qemu_flag:
        if case_name in ["f32_bitwise", "f64_bitwise", "loop", "f64", "f64_cmp",
                         "conversions", "f32", "f32_cmp", "float_exprs",
                         "float_misc", "select", "memory_grow"]:
            return True

    return False


def preflight_check(aot_flag, eh_flag):
    if eh_flag:
        SPEC_TEST_DIR="exception-handling/test/core";
    else: #resetting it to default
        SPEC_TEST_DIR="spec/test/core";    

    if not pathlib.Path(SPEC_TEST_DIR).resolve().exists():
        print(f"Can not find {SPEC_TEST_DIR}")
        return False

    if not pathlib.Path(WAST2WASM_CMD).resolve().exists():
        print(f"Can not find {WAST2WASM_CMD}")
        return False

    if aot_flag and not pathlib.Path(WAMRC_CMD).resolve().exists():
        print(f"Can not find {WAMRC_CMD}")
        return False

    return True


def test_case(
    case_path,
    target,
    aot_flag=False,
    sgx_flag=False,
    multi_module_flag=False,
    multi_thread_flag=False,
    simd_flag=False,
    xip_flag=False,
    eh_flag=False,
    clean_up_flag=True,
    verbose_flag=True,
    gc_flag=False,
    qemu_flag=False,
    qemu_firmware='',
    log='',
):
    case_path = pathlib.Path(case_path).resolve()
    case_name = case_path.stem

    if ignore_the_case(
        case_name,
        target,
        aot_flag,
        sgx_flag,
        multi_module_flag,
        multi_thread_flag,
        simd_flag,
        gc_flag,
        xip_flag,
        eh_flag,
        qemu_flag
    ):
        return True

    CMD = ["python3", "runtest.py"]
    CMD.append("--wast2wasm")
    CMD.append(WAST2WASM_CMD if not gc_flag else SPEC_INTERPRETER_CMD)
    CMD.append("--interpreter")
    if sgx_flag:
        CMD.append(IWASM_SGX_CMD)
    elif qemu_flag:
        CMD.append(IWASM_QEMU_CMD)
    else:
        CMD.append(IWASM_CMD)
    CMD.append("--aot-compiler")
    CMD.append(WAMRC_CMD)

    if aot_flag:
        CMD.append("--aot")

    CMD.append("--target")
    CMD.append(target)

    if multi_module_flag:
        CMD.append("--multi-module")

    if multi_thread_flag:
        CMD.append("--multi-thread")

    if sgx_flag:
        CMD.append("--sgx")

    if simd_flag:
        CMD.append("--simd")

    if xip_flag:
        CMD.append("--xip")

    if eh_flag:
        CMD.append("--eh")

    if qemu_flag:
        CMD.append("--qemu")
        CMD.append("--qemu-firmware")
        CMD.append(qemu_firmware)

    if not clean_up_flag:
        CMD.append("--no_cleanup")

    if gc_flag:
        CMD.append("--gc")

    if log != '':
        CMD.append("--log-dir")
        CMD.append(log)

    CMD.append(case_path)
    print(f"============> run {case_name} ", end="")
    with subprocess.Popen(
        CMD,
        bufsize=1,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        try:
            case_last_words = []
            while not p.poll():
                output = p.stdout.readline()

                if not output:
                    break

                if verbose_flag:
                    print(output, end="")
                else:
                    if len(case_last_words) == 16:
                        case_last_words.pop(0)
                    case_last_words.append(output)

            p.wait(60)

            if p.returncode:
                print(f"failed with a non-zero return code {p.returncode}")
                if not verbose_flag:
                    print(
                        f"\n==================== LAST LOG of {case_name} ====================\n"
                    )
                    print("".join(case_last_words))
                    print("\n==================== LAST LOG END ====================\n")
                raise Exception(case_name)
            else:
                print("successful")
                return True
        except subprocess.CalledProcessError:
            print("failed with CalledProcessError")
            raise Exception(case_name)
        except subprocess.TimeoutExpired:
            print("failed with TimeoutExpired")
            raise Exception(case_name)


def test_suite(
    target,
    aot_flag=False,
    sgx_flag=False,
    multi_module_flag=False,
    multi_thread_flag=False,
    simd_flag=False,
    xip_flag=False,
    eh_flag=False,
    clean_up_flag=True,
    verbose_flag=True,
    gc_flag=False,
    parl_flag=False,
    qemu_flag=False,
    qemu_firmware='',
    log='',
):
    if eh_flag:
        SPEC_TEST_DIR="exception-handling/test/core";
    else: #resetting it to default
        SPEC_TEST_DIR="spec/test/core";
    
    suite_path = pathlib.Path(SPEC_TEST_DIR).resolve()
    if not suite_path.exists():
        print(f"can not find spec test cases at {suite_path}")
        return False

    case_list = sorted(suite_path.glob("*.wast"))
    if simd_flag:
        simd_case_list = sorted(suite_path.glob("simd/*.wast"))
        case_list.extend(simd_case_list)

    if gc_flag:
        gc_case_list = sorted(suite_path.glob("gc/*.wast"))
        case_list.extend(gc_case_list)

    case_count = len(case_list)
    failed_case = 0
    successful_case = 0

    if parl_flag:
        print(f"----- Run the whole spec test suite on {mp.cpu_count()} cores -----")
        with mp.Pool() as pool:
            results = {}
            for case_path in case_list:
                results[case_path.stem] = pool.apply_async(
                    test_case,
                    [
                        str(case_path),
                        target,
                        aot_flag,
                        sgx_flag,
                        multi_module_flag,
                        multi_thread_flag,
                        simd_flag,
                        xip_flag,
                        eh_flag,
                        clean_up_flag,
                        verbose_flag,
                        gc_flag,
                        qemu_flag,
                        qemu_firmware,
                        log,
                    ],
                )

            for case_name, result in results.items():
                try:
                    if qemu_flag:
                        # 60 min / case, testing on QEMU may be very slow
                        result.wait(7200)
                    else:
                        # 5 min / case
                        result.wait(300)
                    if not result.successful():
                        failed_case += 1
                    else:
                        successful_case += 1
                except mp.TimeoutError:
                    print(f"{case_name} meets TimeoutError")
                    failed_case += 1
    else:
        print(f"----- Run the whole spec test suite -----")
        for case_path in case_list:
            try:
                test_case(
                    str(case_path),
                    target,
                    aot_flag,
                    sgx_flag,
                    multi_module_flag,
                    multi_thread_flag,
                    simd_flag,
                    xip_flag,
                    eh_flag,
                    clean_up_flag,
                    verbose_flag,
                    gc_flag,
                    qemu_flag,
                    qemu_firmware,
                    log,
                )
                successful_case += 1
            except Exception as e:
                failed_case += 1
                raise e

    print(
        f"IN ALL {case_count} cases: {successful_case} PASS, {failed_case} FAIL, {case_count - successful_case - failed_case} SKIP"
    )

    return 0 == failed_case


def main():
    parser = argparse.ArgumentParser(description="run the whole spec test suite")

    parser.add_argument(
        "-M",
        action="store_true",
        default=False,
        dest="multi_module_flag",
        help="Running with the Multi-Module feature",
    )
    parser.add_argument(
        "-m",
        action=TargetAction,
        choices=list(TargetAction.TARGET_MAP.keys()),
        type=str,
        dest="target",
        default="X86_64",
        help="Specify Target ",
    )
    parser.add_argument(
        "-p",
        action="store_true",
        default=False,
        dest="multi_thread_flag",
        help="Running with the Multi-Thread feature",
    )
    parser.add_argument(
        "-S",
        action="store_true",
        default=False,
        dest="simd_flag",
        help="Running with the SIMD feature",
    )
    parser.add_argument(
        "-X",
        action="store_true",
        default=False,
        dest="xip_flag",
        help="Running with the XIP feature",
    )
    # added to support WASM_ENABLE_EXCE_HANDLING
    parser.add_argument(
        "-e",
        action="store_true",
        default=False,
        dest="eh_flag",
        help="Running with the exception-handling feature",
    )
    parser.add_argument(
        "-t",
        action="store_true",
        default=False,
        dest="aot_flag",
        help="Running with AOT mode",
    )
    parser.add_argument(
        "-x",
        action="store_true",
        default=False,
        dest="sgx_flag",
        help="Running with SGX environment",
    )
    parser.add_argument(
        "--no_clean_up",
        action="store_false",
        default=True,
        dest="clean_up_flag",
        help="Does not remove tmpfiles. But it will be enabled while running parallelly",
    )
    parser.add_argument(
        "--parl",
        action="store_true",
        default=False,
        dest="parl_flag",
        help="To run whole test suite parallelly",
    )
    parser.add_argument(
        "--qemu",
        action="store_true",
        default=False,
        dest="qemu_flag",
        help="To run whole test suite in qemu",
    )
    parser.add_argument(
        "--qemu-firmware",
        default="",
        dest="qemu_firmware",
        help="Firmware required by qemu",
    )
    parser.add_argument(
        "--log",
        default='',
        dest="log",
        help="Log directory",
    )
    parser.add_argument(
        "--quiet",
        action="store_false",
        default=True,
        dest="verbose_flag",
        help="Close real time output while running cases, only show last words of failed ones",
    )
    parser.add_argument(
        "--gc",
        action="store_true",
        default=False,
        dest="gc_flag",
        help="Running with GC feature",
    )
    parser.add_argument(
        "cases",
        metavar="path_to__case",
        type=str,
        nargs="*",
        help=f"Specify all wanted cases. If not the script will go through all cases under {SPEC_TEST_DIR}",
    )

    options = parser.parse_args()
    print(options)


    if not preflight_check(options.aot_flag, options.eh_flag):
        return False

    if not options.cases:
        if options.parl_flag:
            # several cases might share the same workspace/tempfile at the same time
            # so, disable it while running parallelly
            options.clean_up_flag = False
            options.verbose_flag = False

        start = time.time_ns()
        ret = test_suite(
            options.target,
            options.aot_flag,
            options.sgx_flag,
            options.multi_module_flag,
            options.multi_thread_flag,
            options.simd_flag,
            options.xip_flag,
            options.eh_flag,
            options.clean_up_flag,
            options.verbose_flag,
            options.gc_flag,
            options.parl_flag,
            options.qemu_flag,
            options.qemu_firmware,
            options.log,
        )
        end = time.time_ns()
        print(
            f"It takes {((end - start) / 1000000):,} ms to run test_suite {'parallelly' if options.parl_flag else ''}"
        )
    else:
        try:
            for case in options.cases:
                test_case(
                    case,
                    options.target,
                    options.aot_flag,
                    options.sgx_flag,
                    options.multi_module_flag,
                    options.multi_thread_flag,
                    options.simd_flag,
                    options.xip_flag,
                    options.eh_flag,
                    options.clean_up_flag,
                    options.verbose_flag,
                    options.gc_flag,
                    options.qemu_flag,
                    options.qemu_firmware,
                    options.log
                )
            else:
                ret = True
        except Exception:
            ret = False

    return ret


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
