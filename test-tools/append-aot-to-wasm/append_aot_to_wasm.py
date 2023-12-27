#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
"""
It is used to append a .aot to a .wasm as a custom section.
The custom section name is "aot".

e.g.
$ python3 append_aot_to_wasm.py --wasm quicksort.wasm --aot quicksort.aot --output quicksort.aot.wasm
"""

import argparse
from pathlib import Path
from typing import BinaryIO


def leb128_encode_uint(value: int) -> bytes:
    """
    encode unsigned int into a leb128 bytes
    """
    binary = []
    while value != 0:
        lower_7_bits = value & 0x7F
        value >>= 7

        if value != 0:
            current_byte = 0x80 | lower_7_bits
        else:
            current_byte = 0x00 | lower_7_bits

        binary.append(current_byte)

    return bytes(binary)


def leb128_decode_uint(binary: bytes) -> (int, int):
    """
    decode binary unsigned from a leb128 bytes
    """

    result = 0
    shift = 0
    for i, b in enumerate(binary):
        lower_7_bits = b & 0x7F
        result |= lower_7_bits << shift

        highest_bit = b & 0x80
        if not highest_bit:
            break

        shift += 7

    return i + 1, result


def create_custom_section(section_name: str, section_content: bytes) -> bytes:
    # custome section id 0
    full_content_bin = b"\x00"

    # custom section length
    # 1 for id
    section_length = 1 + len(section_name) + len(section_content)
    section_length_bin = leb128_encode_uint(section_length)
    full_content_bin += section_length_bin

    full_content_bin += (len(section_name) & 0xFF).to_bytes(
        1, byteorder="big", signed=False
    )

    full_content_bin += section_name.encode("ascii")

    full_content_bin += section_content

    return full_content_bin


def main(wasm_file: str, aot_file: str, output: str) -> None:
    cwd = Path.cwd()
    wasm_file = cwd.joinpath(wasm_file).resolve()
    aot_file = cwd.joinpath(aot_file).resolve()
    output = cwd.joinpath(output).resolve()

    assert wasm_file.exists()
    assert aot_file.exists()
    output.unlink(missing_ok=True)

    # read aot content
    with open(aot_file, "rb") as f:
        aot_content = f.read()

    # append to .wasm
    with open(wasm_file, "rb") as f_in, open(output, "wb") as f_out:
        wasm_content = f_in.read(1024)
        while wasm_content:
            f_out.write(wasm_content)
            wasm_content = f_in.read(1024)

        f_out.write(create_custom_section("aot", aot_content))

    print(f"{wasm_file.name} + {aot_file.name} ==> {output}")


if __name__ == "__main__":
    argparse = argparse.ArgumentParser()
    argparse.add_argument("--wasm", help="a .wasm")
    argparse.add_argument("--aot", help="a .aot")
    argparse.add_argument("-o", "--output", help="the output, still be a .wasm")

    args = argparse.parse_args()
    main(args.wasm, args.aot, args.output)
