#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

from pathlib import Path
from pprint import pprint
import re
import shlex
import shutil
import subprocess
from typing import List


def execute_tflite_birds_v1_image_once(
    runtime_bin: str, runtime_args: List[str], cwd: Path
) -> str:
    """
    execute tflite_birds_v1_image example with

    ```
    iwasm --native-lib=somewhere/libwasi-nn-tflite.so --map-dir=.:. \
        ./wasmedge-wasinn-example-tflite-bird-image.wasm  \
        lite-model_aiy_vision_classifier_birds_V1_3.tflite \
        bird.jpg
    ```

    or

    ```
    wasmedge --dir=.:. \
        ./wasmedge-wasinn-example-tflite-bird-image.wasm  \
        lite-model_aiy_vision_classifier_birds_V1_3.tflite \
        bird.jpg
    ```

    assumption:
    - under the right directory, tflite-birds_v1-image
    - every materials are ready
    """

    wasm_file = "./wasmedge-wasinn-example-tflite-bird-image.wasm"
    wasm_args = ["lite-model_aiy_vision_classifier_birds_V1_3.tflite", "bird.jpg"]

    cmd = [runtime_bin]
    cmd.extend(runtime_args)
    cmd.append(wasm_file)
    cmd.extend(wasm_args)

    try:
        p = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            check=True,
            text=True,
            universal_newlines=True,
        )
        return p.stdout
    except subprocess.CalledProcessError as e:
        print(e.stderr)
        print()
        print(e.stdout)


def filter_output_tflite_birds_v1_image(output: str) -> List[str]:
    """
    not all output is needed for comparision

    pick lines like: "   1.) [526](136)Cathartes burrovianus"
    """
    filtered = []
    PATTERN = re.compile(r"^\s+\d\.\)\s+\[\d+\]\(\d+\)\w+")
    for line in output.split("\n"):
        if PATTERN.search(line):
            filtered.append(line.strip())

    return filtered


def execute_tflite_birds_v1_image(iwasm_bin: str, wasmedge_bin: str, cwd: Path):
    iwasm_output = execute_tflite_birds_v1_image_once(
        iwasm_bin,
        [
            "--native-lib=/workspaces/wamr/product-mini/platforms/linux/build/libwasi-nn-tflite.so",
            "--map-dir=.:.",
        ],
        cwd,
    )
    iwasm_output = filter_output_tflite_birds_v1_image(iwasm_output)

    wasmedge_output = execute_tflite_birds_v1_image_once(
        wasmedge_bin, ["--dir=.:."], cwd
    )
    wasmedge_output = filter_output_tflite_birds_v1_image(wasmedge_output)

    if iwasm_output == wasmedge_output:
        print("- tflite_birds_v1_image. PASS")
        return

    print("- tflite_birds_v1_image. FAILED")
    print("------------------------------------------------------------")
    pprint(iwasm_output)
    print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
    pprint(wasmedge_output)
    print("------------------------------------------------------------")


def execute_wasmedge_wasinn_exmaples(iwasm_bin: str, wasmedge_bin: str):
    assert Path.cwd().name == "wasmedge-wasinn-examples"
    assert shutil.which(iwasm_bin)
    assert shutil.which(wasmedge_bin)

    tflite_birds_v1_image_dir = Path.cwd().joinpath("./tflite-birds_v1-image")
    execute_tflite_birds_v1_image(iwasm_bin, wasmedge_bin, tflite_birds_v1_image_dir)


if __name__ == "__main__":
    execute_wasmedge_wasinn_exmaples("iwasm", "wasmedge")
