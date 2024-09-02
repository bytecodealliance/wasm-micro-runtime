/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __AOT_COMP_OPTION_H__
#define __AOT_COMP_OPTION_H__

typedef struct {
    /* Enables or disables bounds checks for stack frames. When enabled, the AOT
     * compiler generates code to check if the stack pointer is within the
     * bounds of the current stack frame (and if not, traps). */
    bool bounds_checks;

    /*  Enables or disables instruction pointer (IP) tracking.*/
    bool ip;

    /* Enables or disables tracking instruction pointer of a trap. Only takes
     * effect when `ip` is enabled.*/
    bool trap_ip;

    /* Enables or disables parameters, locals and stack operands. */
    bool values;
} AOTCallStackFeatures;

typedef struct AOTCompOption {
    bool is_jit_mode;
    bool is_indirect_mode;
    char *target_arch;
    char *target_abi;
    char *target_cpu;
    char *cpu_features;
    bool is_sgx_platform;
    bool enable_bulk_memory;
    bool enable_thread_mgr;
    bool enable_tail_call;
    bool enable_simd;
    bool enable_ref_types;
    bool enable_gc;
    bool enable_aux_stack_check;
    bool enable_aux_stack_frame;
    AOTCallStackFeatures call_stack_features;
    bool enable_perf_profiling;
    bool enable_memory_profiling;
    bool disable_llvm_intrinsics;
    bool disable_llvm_lto;
    bool enable_llvm_pgo;
    bool enable_stack_estimation;
    bool quick_invoke_c_api_import;
    char *use_prof_file;
    uint32_t opt_level;
    uint32_t size_level;
    uint32_t output_format;
    uint32_t bounds_checks;
    uint32_t stack_bounds_checks;
    uint32_t segue_flags;
    char **custom_sections;
    uint32_t custom_sections_count;
    const char *stack_usage_file;
    const char *llvm_passes;
    const char *builtin_intrinsics;
} AOTCompOption, *aot_comp_option_t;

#endif
