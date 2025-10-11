from pycparser import c_parser, c_ast, parse_file
import os

# Define the Result struct as a string
RESULT_STRUCT = """
typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        bool bool_value;
        void *ptr_value;
        int int_value;
        // Add other types as needed
    } value;
} Result;
"""

# Input and output file paths
INPUT_HEADER = "core/iwasm/include/wasm_export.h"
OUTPUT_HEADER = "core/iwasm/include/wasm_export_checked.h"


# Helper function to determine if a parameter is a pointer
def is_pointer(param):
    return isinstance(param.type, c_ast.PtrDecl)


# Updated generate_checked_function to dynamically update Result definition for new return types


def generate_checked_function(func):
    global RESULT_STRUCT  # Access the global Result definition

    func_name = func.name  # Access the name directly from Decl
    new_func_name = f"{func_name}_checked"

    # Extract parameters
    params = func.type.args.params if func.type.args else []

    # Determine the return type
    return_type = "void"  # Default to void if no return type is specified
    if isinstance(func.type.type, c_ast.TypeDecl):
        return_type = " ".join(func.type.type.type.names)

    # Check if the return type is already in Result, if not, add it
    if return_type not in ["bool", "void*", "int", "uint32_t"]:
        # Add a new field to the Result struct dynamically
        RESULT_STRUCT = RESULT_STRUCT.replace(
            "// Add other types as needed",
            f"    {return_type} {return_type}_value;\n        // Add other types as needed",
        )

    # Start building the new function
    new_func = [f"Result {new_func_name}("]
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
    for param in params:
        if isinstance(param, c_ast.EllipsisParam):
            continue  # Skip variadic arguments
        if is_pointer(param):
            new_func.append(f"    if ({param.name} == NULL) {{")
            new_func.append(f"        Result res = {{ .error_code = -1 }};")
            new_func.append(f"        return res;")
            new_func.append(f"    }}")

    # Call the original function
    if return_type == "void":
        new_func.append(f"    {func_name}({', '.join(param_list)});")
        new_func.append(f"    Result res = {{ .error_code = 0 }};")
    else:
        new_func.append(
            f"    {return_type} original_result = {func_name}({', '.join(param_list)});"
        )
        new_func.append(f"    Result res;")
        new_func.append(f"    if (original_result == 0) {{")
        new_func.append(f"        res.error_code = 0;")
        if return_type == "bool":
            new_func.append(f"        res.value.bool_value = original_result;")
        elif return_type == "void*":
            new_func.append(f"        res.value.ptr_value = original_result;")
        elif return_type == "uint32_t":
            new_func.append(f"        res.value.int_value = original_result;")
        else:
            new_func.append(
                f"        res.value.{return_type}_value = original_result;"
            )
        new_func.append(f"    }} else {{")
        new_func.append(f"        res.error_code = -2;")
        new_func.append(f"    }}")

    new_func.append(f"    return res;")
    new_func.append("}")

    return "\n".join(new_func)


# Updated process_header to scan all return types and create a proper Result type

def process_header():
    global RESULT_STRUCT  # Access the global Result definition

    # Parse the header file with preprocessing
    ast = parse_file(
        INPUT_HEADER,
        use_cpp=True,
        cpp_path="gcc",
        cpp_args=[
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
        ],
    )

    # Collect all function declarations
    functions = [
        node
        for node in ast.ext
        if isinstance(node, c_ast.Decl) and isinstance(node.type, c_ast.FuncDecl)
    ]

    # Scan all return types and update Result struct
    return_types = set()
    for func in functions:
        if isinstance(func.type.type, c_ast.TypeDecl):
            return_type = " ".join(func.type.type.type.names)
            return_types.add(return_type)

    # Update the Result struct with all return types
    for return_type in return_types:
        if return_type not in ["bool", "void*", "int", "uint32_t"]:
            RESULT_STRUCT = RESULT_STRUCT.replace(
                "// Add other types as needed",
                f"    {return_type} {return_type}_value;\n        // Add other types as needed",
            )

    # Generate the new header file
    with open(OUTPUT_HEADER, "w") as f:
        f.write("#ifndef WASM_EXPORT_CHECKED_H\n#define WASM_EXPORT_CHECKED_H\n\n")

        # Write the updated Result struct
        f.write(RESULT_STRUCT + "\n")

        for func in functions:
            new_func = generate_checked_function(func)
            f.write(new_func + "\n\n")

        f.write("#endif // WASM_EXPORT_CHECKED_H\n")


if __name__ == "__main__":
    process_header()
