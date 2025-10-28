from pycparser import c_parser, c_ast, parse_file
import os


def collect_typedefs(ast):
    """Collect all typedefs in the AST."""
    typedefs = {}
    for node in ast.ext:
        if not isinstance(node, c_ast.Typedef):
            continue

        if node.name in typedefs:
            raise Exception(f"Duplicate typedef found: {node.name}")

        typedef_name = node.name
        typedef_type = node.type
        typedefs[typedef_name] = typedef_type

    return typedefs


def resolve_typedef(typedefs, type_name):
    """Resolve a typedef to its underlying type."""

    def resolve_base_type(ptr_decl):
        # handle cases like: typedef int******* ptr;
        cur_type = ptr_decl
        pointer_type_name = ""

        while isinstance(cur_type, c_ast.PtrDecl):
            cur_type = cur_type.type
            pointer_type_name += "*"

        assert isinstance(cur_type, c_ast.TypeDecl)
        if isinstance(cur_type.type, c_ast.IdentifierType):
            base_type_name = " ".join(cur_type.type.names)
            pointer_type_name = base_type_name + pointer_type_name
        else:
            pointer_type_name = "".join(cur_type.type.name) + pointer_type_name

        return pointer_type_name

    resolved_type = typedefs.get(type_name)

    if resolved_type is None:
        return type_name

    print(f"\n\nResolving typedef {type_name}:")

    if isinstance(resolved_type, c_ast.TypeDecl):
        if isinstance(resolved_type.type, c_ast.Enum):
            print(f"Resolved enum typedef {type_name}")
            return type_name

        if isinstance(resolved_type.type, c_ast.Struct):
            print(f"Resolved struct typedef {type_name}")
            return type_name

        if isinstance(resolved_type.type, c_ast.Union):
            print(f"Resolved union typedef {type_name}")
            return type_name

        if isinstance(resolved_type.type, c_ast.IdentifierType):
            base_type_name = " ".join(resolved_type.type.names)
            print(f"Resolved base typedef {type_name} to {base_type_name}")
            return type_name

        raise Exception(f"Unhandled TypeDecl typedef {type_name}")
    elif isinstance(resolved_type, c_ast.PtrDecl):
        pointer_type_name = resolve_base_type(resolved_type)
        print(f"Resolved pointer typedef {type_name} to {pointer_type_name}")
        return pointer_type_name
    else:
        resolved_type.show()
        raise Exception(f"Unhandled typedef {type_name}")


def generate_checked_function(func, typedefs):
    func_name = func.name  # Access the name directly from Decl
    new_func_name = f"{func_name}_checked"

    # Extract parameters
    params = func.type.args.params if func.type.args else []

    # Determine the return type
    return_pointer = False
    return_type = "void"  # Default to void if no return type is specified
    if isinstance(func.type.type, c_ast.TypeDecl):
        return_type = " ".join(func.type.type.type.names)

        resolved_type = resolve_typedef(typedefs, return_type)
        if resolved_type.endswith("*"):
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

    # Collect all typedefs
    typedefs = collect_typedefs(ast)

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
            # resolved_type = resolve_typedef(typedefs, return_type)
            return_types.add(return_type)

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
