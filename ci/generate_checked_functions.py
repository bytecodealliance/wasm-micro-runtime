from pycparser import c_parser, c_ast, parse_file
import os
from pprint import pprint


# Updated generate_checked_function to dynamically update Result definition for new return types


def collect_typedefs(ast):
    typedefs = {}
    for node in ast.ext:
        if isinstance(node, c_ast.Typedef):
            typedefs[node.name] = node.type
    return typedefs

def resolve_typedef(typedefs, type_name):
    resolved_type = typedefs.get(type_name)

    # Return the original type name if not a typedef
    if not resolved_type:
        return type_name

    print(f"Resolving typedef for {type_name}: {resolved_type}\n")

    if isinstance(resolved_type, c_ast.TypeDecl):
        # Base case: Return the type name
        return " ".join(resolved_type.declname)
    elif isinstance(resolved_type, c_ast.PtrDecl):
        # Handle pointer typedefs
        resolved_type.show()
        base_type = " ".join(resolved_type.type.type.name)
        return f"{base_type} *"
    elif isinstance(resolved_type, c_ast.ArrayDecl):
        # Handle array typedefs
        base_type = resolve_typedef(resolved_type.type.declname, typedefs)
        return f"{base_type} *"


def generate_checked_function(func, typedefs):
    func_name = func.name  # Access the name directly from Decl
    new_func_name = f"{func_name}_checked"

    # Extract parameters
    params = func.type.args.params if func.type.args else []

    # Determine the return type
    return_type = "void"  # Default to void if no return type is specified
    if isinstance(func.type.type, c_ast.TypeDecl):
        return_type = " ".join(func.type.type.type.names)
        resolved_type = resolve_typedef(typedefs, return_type)
        return_type = resolved_type

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
    has_variadic = False
    for param in params:
        if isinstance(param, c_ast.EllipsisParam):
            # Restructure to use va_list
            new_func.append("    va_list args;")
            has_variadic = True
        elif isinstance(param.type, c_ast.PtrDecl):
            new_func.append(f"    if ({param.name} == NULL) {{")
            new_func.append(f"        Result res = {{ .error_code = -1 }};")
            new_func.append(f"        return res;")
            new_func.append(f"    }}")

    # Call the original function
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

    # Handle result return from the original function
    new_func.append(f"    Result res;")
    # if it is bool type
    if return_type == "_Bool":
        new_func.append(f"    if (original_result == 1) {{")
        new_func.append(f"        res.error_code = 0;")
        new_func.append(f"        res.value._Bool_value = original_result;")
        new_func.append(f"    }} else {{")
        new_func.append(f"        res.error_code = -1;")
        new_func.append(f"    }}")
    # if it is void type
    elif return_type == "void":
        new_func.append(f"    res.error_code = 0;")
    else:
        if isinstance(func.type.type, c_ast.PtrDecl):
            new_func.append(f"    if (original_result != NULL) {{")
            new_func.append(f"        res.error_code = 0;")
            new_func.append(f"        res.value.{return_type}_value = original_result;")
            new_func.append(f"    }} else {{")
            new_func.append(f"        res.error_code = -1;")
            new_func.append(f"    }}")
        else:
            new_func.append(f"    res.error_code = 0;")
            new_func.append(f"    res.value.{return_type}_value = original_result;")

    new_func.append(f"    return res;")
    new_func.append("}")

    return "\n".join(new_func)


# Updated process_header to scan all return types and create a proper Result type


def process_header():
    # Define the Result struct as a string
    RESULT_STRUCT = """
    typedef struct {
        int error_code; // Error code (0 for success, non-zero for errors)
        union {
            // Add other types as needed
        } value;
    } Result;
    """

    # Based on current file location, adjust the path to the header file
    input_header = os.path.join(
        os.path.dirname(__file__), "../core/iwasm/include/wasm_export.h"
    )
    output_header = input_header.replace("wasm_export.h", "wasm_export_checked.h")

    # Parse the header file with preprocessing
    ast = parse_file(
        input_header,
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

    # Collect typedefs
    typedefs = collect_typedefs(ast)
    # pprint(typedefs)

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
            resolved_type = resolve_typedef(typedefs, return_type)
            return_types.add(resolved_type)

    # Update the Result struct with all return types
    for return_type in return_types:
        if return_type == "void":
            continue  # No need to add void type

        RESULT_STRUCT = RESULT_STRUCT.replace(
            "// Add other types as needed",
            f"    {return_type} {return_type}_value;\n        // Add other types as needed",
        )

    # Generate the new header file
    with open(output_header, "w") as f:
        f.write("#ifndef WASM_EXPORT_CHECKED_H\n#define WASM_EXPORT_CHECKED_H\n\n")

        # necessary headers
        f.write("#include <stdbool.h>\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <stdlib.h>\n")
        f.write("\n")
        f.write('#include "wasm_export.h"\n')
        f.write('#include "lib_export.h"\n')
        f.write("\n")

        # Write the updated Result struct
        f.write(RESULT_STRUCT + "\n")

        for func in functions:
            new_func = generate_checked_function(func, typedefs)
            f.write(new_func + "\n\n")

        f.write("#endif // WASM_EXPORT_CHECKED_H\n")


if __name__ == "__main__":
    process_header()
