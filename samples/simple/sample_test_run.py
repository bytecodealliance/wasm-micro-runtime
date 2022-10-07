#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import argparse
import shlex
import subprocess
import sys
import time
import traceback


def start_server(cwd):
    """
    Startup the 'simple' process works in TCP server mode
    """
    app_server = subprocess.Popen(shlex.split("./simple -s "), cwd=cwd)
    return app_server


def query_installed_application(cwd):
    """
    Query all installed applications
    """
    qry_prc = subprocess.run(
        shlex.split("./host_tool -q"), cwd=cwd, check=False, capture_output=True
    )
    assert qry_prc.returncode == 69
    return qry_prc.returncode, qry_prc.stdout


def install_wasm_application(wasm_name, wasm_file, cwd):
    """
    Install a wasm application
    """
    inst_prc = subprocess.run(
        shlex.split(f"./host_tool -i {wasm_name} -f {wasm_file}"),
        cwd=cwd,
        check=False,
        capture_output=True,
    )
    assert inst_prc.returncode == 65
    return inst_prc.returncode, inst_prc.stdout


def uninstall_wasm_application(wasm_name, cwd):
    """
    Uninstall a wasm application
    """

    unst_prc = subprocess.run(
        shlex.split(f"./host_tool -u {wasm_name}"),
        cwd=cwd,
        check=False,
        capture_output=True,
    )
    assert unst_prc.returncode == 66
    return unst_prc.returncode, unst_prc.stdout


def send_get_to_wasm_application(wasm_name, url, cwd):
    """
    send a request (GET) from host to an applicaton
    """
    qry_prc = subprocess.run(
        shlex.split(f"./host_tool -r /app/{wasm_name}{url} -A GET"),
        cwd=cwd,
        check=False,
        capture_output=True,
    )
    assert qry_prc.returncode == 69
    return qry_prc.returncode, qry_prc.stdout


def main():
    """
    GO!GO!!GO!!!
    """
    parser = argparse.ArgumentParser(description="run the sample and examine outputs")
    parser.add_argument("working_directory", type=str)
    args = parser.parse_args()

    ret = 1
    app_server = None
    try:
        app_server = start_server(args.working_directory)

        # wait for a second
        time.sleep(1)

        print("--> Install timer.wasm...")
        install_wasm_application(
            "timer", "./wasm-apps/timer.wasm", args.working_directory
        )

        print("--> Install event_publisher.wasm...")
        install_wasm_application(
            "event_publisher",
            "./wasm-apps/event_publisher.wasm",
            args.working_directory,
        )

        print("--> Install event_subscriber.wasm...")
        install_wasm_application(
            "event_subscriber",
            "./wasm-apps/event_subscriber.wasm",
            args.working_directory,
        )

        print("--> Uninstall timer.wasm...")
        uninstall_wasm_application("timer", args.working_directory)

        print("--> Uninstall event_publisher.wasm...")
        uninstall_wasm_application(
            "event_publisher",
            args.working_directory,
        )

        print("--> Uninstall event_subscriber.wasm...")
        uninstall_wasm_application(
            "event_subscriber",
            args.working_directory,
        )

        print("--> Query all installed applications...")
        query_installed_application(args.working_directory)

        print("--> Install request_handler.wasm...")
        install_wasm_application(
            "request_handler",
            "./wasm-apps/request_handler.wasm",
            args.working_directory,
        )

        print("--> Query again...")
        query_installed_application(args.working_directory)

        print("--> Install request_sender.wasm...")
        install_wasm_application(
            "request_sender",
            "./wasm-apps/request_sender.wasm",
            args.working_directory,
        )

        print("--> Send GET to the Wasm application named request_handler...")
        send_get_to_wasm_application("request_handler", "/url1", args.working_directory)

        print("--> All pass")
        ret = 0
    except AssertionError:
        traceback.print_exc()
    finally:
        app_server.kill()

    return ret


if __name__ == "__main__":
    sys.exit(main())
