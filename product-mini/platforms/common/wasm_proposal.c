/*
 * Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>

void
wasm_proposal_print_status(void)
{
    printf("Phase4 Wasm Proposals Status:\n");
    printf("  Always-on:\n");
    printf("    - Extended Constant Expressions\n");
    printf("    - Multi-value\n");
    printf("    - Non-trapping float-to-int conversions\n");
    printf("    - Sign-extension operators\n");
    printf("    - WebAssembly C and C++ API\n");
    printf("  Configurable:\n");
    printf("    - Bulk Memory Operation: %u\n", WASM_ENABLE_BULK_MEMORY);
    printf("    - Fixed-Width SIMD: %u\n", WASM_ENABLE_SIMD);
    printf("    - Garbage Collection: %u\n", WASM_ENABLE_GC);
    printf("    - Legacy Exception Handling: %u\n", WASM_ENABLE_EXCE_HANDLING);
    printf("    - Memory64: %u\n", WASM_ENABLE_MEMORY64);
    printf("    - Multiple Memory: %u\n", WASM_ENABLE_MULTI_MEMORY);
    printf("    - Reference Types: %u\n", WASM_ENABLE_REF_TYPES);
    printf("    - Reference-Typed Strings: %u\n", WASM_ENABLE_REF_TYPES);
    printf("    - Tail Call: %u\n", WASM_ENABLE_TAIL_CALL);
    printf("    - Threads: %u\n", WASM_ENABLE_SHARED_MEMORY);
    printf("    - Typed Function References: %u\n", WASM_ENABLE_GC);
    printf("  Unsupported:\n");
    printf("    - Branch Hinting\n");
    printf("    - Custom Annotation Syntax in the Text Format\n");
    printf("    - Exception handling\n");
    printf("    - Import/Export of Mutable Globals\n");
    printf("    - JS String Builtins\n");
    printf("    - Relaxed SIMD\n");
}