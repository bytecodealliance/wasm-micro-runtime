import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys


def get_code_section_start(wasm_objdump: Path, wasm_file: Path) -> int:
    """
    Find the start offset of Code section in a wasm file.

    if the code section header likes:
      Code start=0x0000017c end=0x00004382 (size=0x00004206) count: 47

    the start offset is 0x0000017c
    """
    cmd = f"{wasm_objdump} -h {wasm_file}"
    p = subprocess.run(
        shlex.split(cmd),
        check=True,
        capture_output=True,
        text=True,
        universal_newlines=True,
    )
    outputs = p.stdout.split(os.linesep)

    # if there is no .debug section, return -1
    for line in outputs:
        line = line.strip()
        if ".debug_info" in line:
            break
    else:
        print(f"No .debug_info section found {wasm_file}")
        return -1

    for line in outputs:
        line = line.strip()
        if "Code" in line:
            return int(line.split()[1].split("=")[1], 16)

    return -1


def get_line_info(dwarf_dump: Path, wasm_file: Path, offset: int) -> str:
    """
    Find the location info of a given offset in a wasm file.
    """
    cmd = f"{dwarf_dump} --lookup={offset} {wasm_file}"
    p = subprocess.run(
        shlex.split(cmd),
        check=False,
        capture_output=True,
        text=True,
        universal_newlines=True,
    )
    outputs = p.stdout.split(os.linesep)

    capture_name = False
    for line in outputs:
        line = line.strip()

        if "DW_TAG_subprogram" in line:
            capture_name = True
            continue

        if "DW_AT_name" in line and capture_name:
            PATTERN = r"DW_AT_name\s+\(\"(\S+)\"\)"
            m = re.match(PATTERN, line)
            assert m is not None

            function_name = m.groups()[0]

        if line.startswith("Line info"):
            location = line
            return (function_name, location)

    return ()


def parse_line_info(line_info: str) -> ():
    """
    line_info -> [file, line, column]
    """
    PATTERN = r"Line info: file \'(.+)\', line ([0-9]+), column ([0-9]+)"
    m = re.search(PATTERN, line_info)
    assert m is not None

    file, line, column = m.groups()
    return (file, int(line), int(column))


def parse_call_stack_line(line: str) -> ():
    """
    #00: 0x0a04 - $f18   => (00, 0x0a04, $f18)
    """
    PATTERN = r"#([0-9]+): 0x([0-9a-f]+) - (\S+)"
    m = re.match(PATTERN, line)
    return m.groups() if m else None


def main():
    parser = argparse.ArgumentParser(description="addr2line for wasm")
    parser.add_argument("--wasi-sdk", type=Path, help="path to wasi-sdk")
    parser.add_argument("--wabt", type=Path, help="path to wabt")
    parser.add_argument("--wasm-file", type=Path, help="path to wasm file")
    parser.add_argument("call_stack_file", type=Path, help="path to a call stack file")
    args = parser.parse_args()

    wasm_objdump = args.wabt.joinpath("bin/wasm-objdump")
    assert wasm_objdump.exists()

    llvm_dwarf_dump = args.wasi_sdk.joinpath("bin/llvm-dwarfdump")
    assert llvm_dwarf_dump.exists()

    code_section_start = get_code_section_start(wasm_objdump, args.wasm_file)
    if code_section_start == -1:
        return -1

    assert args.call_stack_file.exists()
    with open(args.call_stack_file, "rt", encoding="ascii") as f:
        for line in f:
            line = line.strip()

            if not line:
                continue

            splitted = parse_call_stack_line(line)
            if splitted is None:
                print(line)
                continue

            _, offset, _ = splitted

            offset = int(offset, 16)
            offset = offset - code_section_start
            line_info = get_line_info(llvm_dwarf_dump, args.wasm_file, offset)
            if not line_info:
                print(line)
                continue

            function_name, line_info = line_info
            src_file, src_line, src_column = parse_line_info(line_info)
            print(
                f"{line} (FILE:{src_file} LINE:{src_line:5} COLUMN:{src_column:3} FUNC:{function_name})"
            )

    return 0


if __name__ == "__main__":
    sys.exit(main())
