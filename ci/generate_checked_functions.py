"""
This script generates "checked" versions of functions from the specified header files.

Usage:
    python3 generate_checked_functions.py --headers <header1.h> <header2.h> ...

Arguments:
    --headers: A list of header file paths to process. Each header file will be parsed, and a corresponding
               "_checked.h" file will be generated with additional null pointer checks and error handling.

Example:
    python3 generate_checked_functions.py --headers core/iwasm/include/wasm_export.h

Description:
    The script parses the provided header files using `pycparser` to extract function declarations and typedefs.
    For each function, it generates a "checked" version that includes:
      - Null pointer checks for pointer parameters.
      - Error handling using a `Result` struct.

    The generated "_checked.h" files include the original header file and define the `Result` struct, which
    encapsulates the return value and error codes.

Dependencies:
    - pycparser: Install it using `pip install pycparser`.

Output:
    For each input header file, a corresponding "_checked.h" file is created in the same directory.
"""

from pycparser import c_ast, parse_file
import argparse
from pathlib import Path

# Constants for repeated strings
CPP_ARGS = [
    "-E",
    "-D__attribute__(x)=",
    "-D__asm__(x)=",
    "-D__asm(x)=",
    "-D__builtin_va_list=int",
    "-D__extension__=",
    "-D__inline__=",
    "-D__restrict=",
    "-D__restrict__=",
    "-D_Static_assert(x, y)=",
    "-D__signed=",
    "-D__volatile__(x)=",
    "-Dstatic_assert(x, y)=",
]

RESULT_STRUCT_TEMPLATE = """
    typedef struct {
        int error_code; // Error code (0 for success, non-zero for errors)
        union {
            // Add other types as needed
        } value;
    } Result;
"""

COPYRIGHT = """
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
 """

INCLUDE_HEADERS = ["<stdbool.h>", "<stdint.h>", "<stdlib.h>"]


def extract_typedefs(ast):
    """Extract all typedefs from the AST."""
    return {node.name: node.type for node in ast.ext if isinstance(node, c_ast.Typedef)}


def generate_result_struct(return_types):
    """Generate the Result struct based on return types."""
    result_struct = RESULT_STRUCT_TEMPLATE
    for return_type in return_types:
        if return_type == "void":
            continue

        result_struct = result_struct.replace(
            "// Add other types as needed",
            f"    {return_type} {return_type}_value;\n        // Add other types as needed",
        )
    return result_struct


def write_checked_header(output_path, result_struct, functions, typedefs):
    """Write the checked header file."""

    with open(output_path, "w") as f:
        # copyright
        f.write(COPYRIGHT)
        f.write("\n")

        f.write("/*\n")
        f.write(" * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!\n")
        f.write(" */\n")

        # include guard
        f.write(
            f"#ifndef {output_path.stem.upper()}_H\n#define {output_path.stem.upper()}_H\n\n"
        )

        for header in INCLUDE_HEADERS:
            f.write(f"#include {header}\n")
        f.write("\n")
        # include original header
        original_header = output_path.stem.replace("_checked", "") + ".h"
        f.write(f'#include "{original_header}"\n')
        f.write("\n")

        f.write(result_struct + "\n")

        for func in functions:
            new_func = generate_checked_function(func, typedefs)
            f.write(new_func + "\n\n")

        f.write(f"#endif // {output_path.stem.upper()}_H\n")


def generate_checked_function(func, typedefs):
    """Generate a checked version of the given function."""
    func_name = func.name  # Access the name directly from Decl
    new_func_name = f"{func_name}_checked"

    # Extract parameters
    params = func.type.args.params if func.type.args else []

    # Determine the return type
    return_pointer = False
    return_type = "void"  # Default to void if no return type is specified
    if isinstance(func.type.type, c_ast.TypeDecl):
        return_type = " ".join(func.type.type.type.names)

        resolved_type = typedefs.get(return_type, return_type)
        if isinstance(resolved_type, c_ast.PtrDecl):
            return_pointer = True

    # Start building the new function
    new_func = [f"static inline Result {new_func_name}("]
    param_list = []
    for param in params:
        if isinstance(param, c_ast.EllipsisParam):
            # Handle variadic arguments (e.g., ...)
            param_list.append("...")
            new_func.append("    ...,")
            continue

        param_name = param.name if param.name else ""
        param_list.append(param_name)
        param_type = (
            " ".join(param.type.type.names)
            if isinstance(param.type, c_ast.TypeDecl)
            else "void*"
        )
        new_func.append(f"    {param_type} {param_name},")
    if param_list:
        new_func[-1] = new_func[-1].rstrip(",")  # Remove trailing comma
    new_func.append(") {")

    # Add null checks for pointer parameters
    new_func.append(f"    Result res;")
    has_variadic = False
    for param in params:
        if isinstance(param, c_ast.EllipsisParam):
            # Restructure to use va_list
            new_func.append("    va_list args;")
            has_variadic = True
        elif isinstance(param.type, c_ast.PtrDecl):
            new_func.append(f"    // Check for null pointer parameter: {param.name}")
            new_func.append(f"    if ({param.name} == NULL) {{")
            new_func.append(f"        res.error_code = -1;")
            new_func.append(f"        return res;")
            new_func.append(f"    }}")

    # Call the original function
    new_func.append(f"    // Execute the original function")
    if return_type == "void":
        new_func.append(f"    {func_name}({', '.join(param_list)});")
    elif has_variadic:
        new_func.append("    va_start(args, " + param_list[-2] + ");")
        new_func.append(
            f"    {return_type} original_result = {func_name}({', '.join(param_list[:-1])}, args);"
        )
        new_func.append("    va_end(args);")
    else:
        new_func.append(
            f"    {return_type} original_result = {func_name}({', '.join(param_list)});"
        )

    # Handle returned values
    new_func.append(f"    // Assign return value and error code")
    if return_type == "void":
        new_func.append(f"    res.error_code = 0;")
    elif return_type == "_Bool":
        new_func.append(f"    res.error_code = original_result ? 0 : -2;")
        new_func.append(f"    res.value._Bool_value = original_result;")
    # if return type is a pointer or typedef from pointer
    elif return_pointer:
        new_func.append(f"    if (original_result != NULL) {{")
        new_func.append(f"        res.error_code = 0;")
        new_func.append(f"        res.value.{return_type}_value = original_result;")
        new_func.append(f"    }} else {{")
        new_func.append(f"        res.error_code = -2;")
        new_func.append(f"    }}")
    else:
        new_func.append(f"    if (original_result == 0) {{")
        new_func.append(f"        res.error_code = 0;")
        new_func.append(f"        res.value.{return_type}_value = original_result;")
        new_func.append(f"    }} else {{")
        new_func.append(f"        res.error_code = -2;")
        new_func.append(f"    }}")

    new_func.append(f"    return res;")
    new_func.append(f"}}")
    return "\n".join(new_func)


def parse_arguments():
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Generate checked functions from header files."
    )
    parser.add_argument(
        "--headers",
        nargs="+",
        required=True,
        help="List of header file paths to process.",
    )
    return parser.parse_args()


def generate_checked_headers(header_paths):
    """Process each header file and generate checked versions."""
    for input_header in header_paths:
        input_path = Path(input_header)
        output_path = input_path.with_name(input_path.stem + "_checked.h")

        ast = parse_file(
            str(input_path),
            use_cpp=True,
            cpp_path="gcc",
            cpp_args=CPP_ARGS,
        )

        typedefs = extract_typedefs(ast)
        functions = [
            node
            for node in ast.ext
            if isinstance(node, c_ast.Decl) and isinstance(node.type, c_ast.FuncDecl)
        ]

        return_types = {
            " ".join(func.type.type.type.names)
            for func in functions
            if isinstance(func.type.type, c_ast.TypeDecl)
        }

        result_struct = generate_result_struct(return_types)
        write_checked_header(output_path, result_struct, functions, typedefs)


def main():
    args = parse_arguments()
    generate_checked_headers(args.headers)


if __name__ == "__main__":
    main()
