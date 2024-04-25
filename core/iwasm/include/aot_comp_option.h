/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __AOT_COMP_OPTION_H__
#define __AOT_COMP_OPTION_H__

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
    bool enable_gc;
    bool enable_ref_types;
    bool enable_aux_stack_check;
    bool enable_aux_stack_frame;
    bool enable_checkpoint;
    bool enable_loop_checkpoint;
    bool enable_br_checkpoint;
    bool enable_every_checkpoint;
    bool enable_perf_profiling;
    bool enable_memory_profiling;
    bool enable_aux_stack_dirty_bit;
    bool enable_counter_loop_checkpoint;
    bool enable_checkpoint_pgo;
    const char *aot_file_name;
    bool exp_disable_stack_commit_before_block;
    bool exp_disable_gen_fence_int3;
    bool exp_disable_commit_sp_ip;
    bool exp_disable_local_commit;
    bool exp_disable_restore_jump;
    bool disable_llvm_intrinsics;
    bool disable_llvm_lto;
    bool enable_llvm_pgo;
    bool enable_stack_estimation;
    bool quick_invoke_c_api_import;
    char *use_prof_file;
    uint32 opt_level;
    uint32 size_level;
    uint32 output_format;
    uint32 bounds_checks;
    uint32 stack_bounds_checks;
    uint32 segue_flags;
    char **custom_sections;
    uint32 custom_sections_count;
    const char *stack_usage_file;
    const char *llvm_passes;
    const char *builtin_intrinsics;
} AOTCompOption, *aot_comp_option_t;

#endif
