/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_LLVM_H_
#define _AOT_LLVM_H_

#include "aot.h"
#include "llvm-c/Types.h"
#include "llvm-c/Target.h"
#include "llvm-c/Core.h"
#include "llvm-c/Object.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Transforms/Scalar.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Value in the WASM operation stack, each stack element
 * is an LLVM value
 */
typedef struct AOTValue {
  struct AOTValue *next;
  struct AOTValue *prev;
  LLVMValueRef value;
  /* VALUE_TYPE_I32/I64/F32/F64/VOID */
  uint8 type;
} AOTValue;

/**
 * Value stack, represents stack elements in a WASM block
 */
typedef struct AOTValueStack {
  AOTValue *value_list_head;
  AOTValue *value_list_end;
} AOTValueStack;

typedef struct AOTBlock {
  struct AOTBlock *next;
  struct AOTBlock *prev;

  /* Block index */
  uint32 block_index;
  /* BLOCK_TYPE_BLOCK/LOOP/IF/FUNCTION */
  uint32 block_type;
  /* VALUE_TYPE_I32/I64/F32/F64/VOID */
  uint8 return_type;
  /* Whether it is reachable */
  bool is_reachable;
  /* Whether skip translation of wasm else branch */
  bool skip_wasm_code_else;

  /* code of else opcode of this block, if it is a IF block  */
  uint8 *wasm_code_else;
  /* code end of this block */
  uint8 *wasm_code_end;

  /* LLVM label points to code begin */
  LLVMBasicBlockRef llvm_entry_block;
  /* LLVM label points to code else */
  LLVMBasicBlockRef llvm_else_block;
  /* LLVM label points to code end */
  LLVMBasicBlockRef llvm_end_block;

  /* WASM operation stack */
  AOTValueStack value_stack;

  /* Return value of this block, a PHI node */
  LLVMValueRef return_value_phi;
} AOTBlock;

/**
 * Block stack, represents WASM block stack elements
 */
typedef struct AOTBlockStack {
  AOTBlock *block_list_head;
  AOTBlock *block_list_end;
  /* Current block index of each block type */
  uint32 block_index[3];
} AOTBlockStack;

typedef struct AOTFuncContext {
  AOTFunc *aot_func;
  LLVMValueRef func;
  AOTBlockStack block_stack;

  LLVMValueRef aot_inst;
  LLVMValueRef table_base;

  LLVMValueRef mem_data_size;
  LLVMValueRef mem_base_addr;
  LLVMValueRef mem_bound_1_byte;
  LLVMValueRef mem_bound_2_bytes;
  LLVMValueRef mem_bound_4_bytes;
  LLVMValueRef mem_bound_8_bytes;

  LLVMValueRef heap_base_offset;
  LLVMValueRef heap_base_addr;
  LLVMValueRef heap_data_size;
  LLVMValueRef heap_bound_1_byte;
  LLVMValueRef heap_bound_2_bytes;
  LLVMValueRef heap_bound_4_bytes;
  LLVMValueRef heap_bound_8_bytes;

#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
  LLVMValueRef ext_mem_base_offset;
  LLVMValueRef ext_mem_base_addr;
  LLVMValueRef ext_mem_data_size;
#endif

  LLVMValueRef cur_exception;

  bool mem_space_unchanged;

  LLVMBasicBlockRef *exception_blocks;
  LLVMBasicBlockRef got_exception_block;
  LLVMBasicBlockRef func_return_block;
  LLVMValueRef exception_id_phi;
  LLVMValueRef func_ptrs;
  LLVMValueRef func_type_indexes;
  LLVMValueRef locals[1];
} AOTFuncContext;

typedef struct AOTLLVMTypes {
  LLVMTypeRef int1_type;
  LLVMTypeRef int8_type;
  LLVMTypeRef int16_type;
  LLVMTypeRef int32_type;
  LLVMTypeRef int64_type;
  LLVMTypeRef float32_type;
  LLVMTypeRef float64_type;
  LLVMTypeRef void_type;

  LLVMTypeRef int8_ptr_type;
  LLVMTypeRef int16_ptr_type;
  LLVMTypeRef int32_ptr_type;
  LLVMTypeRef int64_ptr_type;
  LLVMTypeRef float32_ptr_type;
  LLVMTypeRef float64_ptr_type;
  LLVMTypeRef void_ptr_type;

  LLVMTypeRef meta_data_type;
} AOTLLVMTypes;

typedef struct AOTLLVMConsts {
    LLVMValueRef i8_zero;
    LLVMValueRef i32_zero;
    LLVMValueRef i64_zero;
    LLVMValueRef f32_zero;
    LLVMValueRef f64_zero;
    LLVMValueRef i32_one;
    LLVMValueRef i32_two;
    LLVMValueRef i32_four;
    LLVMValueRef i32_eight;
    LLVMValueRef i32_neg_one;
    LLVMValueRef i64_neg_one;
    LLVMValueRef i32_min;
    LLVMValueRef i64_min;
    LLVMValueRef i32_31;
    LLVMValueRef i32_32;
    LLVMValueRef i64_63;
    LLVMValueRef i64_64;
} AOTLLVMConsts;

/**
 * Compiler context
 */
typedef struct AOTCompContext {
  AOTCompData *comp_data;

  /* LLVM variables required to emit LLVM IR */
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMTargetMachineRef target_machine;
  char *target_cpu;
  char target_arch[16];

  /* LLVM execution engine required by JIT */
  LLVMExecutionEngineRef exec_engine;
  bool is_jit_mode;

  /* Whether optimize the JITed code */
  bool optimize;

  /* Whether use fastcall calling convention */
  bool is_fast_call_conv;

  /* LLVM pass manager to optimize the JITed code */
  LLVMPassManagerRef pass_mgr;

  /* LLVM floating-point rounding mode metadata */
  LLVMValueRef fp_rounding_mode;

  /* LLVM floating-point exception behavior metadata */
  LLVMValueRef fp_exception_behavior;

  /* LLVM data types */
  AOTLLVMTypes basic_types;
  LLVMTypeRef aot_inst_type;

  /* LLVM const values */
  AOTLLVMConsts llvm_consts;

  /* Function contexts */
  AOTFuncContext **func_ctxes;
  uint32 func_ctx_count;
} AOTCompContext;

enum {
    AOT_FORMAT_FILE,
    AOT_OBJECT_FILE,
    AOT_LLVMIR_UNOPT_FILE,
    AOT_LLVMIR_OPT_FILE,
};

typedef struct AOTCompOption{
    bool is_jit_mode;
    char *target_triple;
    char *target_cpu;
    char *cpu_features;
    uint32 opt_level;
    uint32 output_format;
} AOTCompOption, *aot_comp_option_t;

AOTCompContext *
aot_create_comp_context(AOTCompData *comp_data,
                        aot_comp_option_t option);

void
aot_destroy_comp_context(AOTCompContext *comp_ctx);

bool
aot_compile_wasm(AOTCompContext *comp_ctx);

uint8*
aot_emit_elf_file(AOTCompContext *comp_ctx, uint32 *p_elf_file_size);

void
aot_destroy_elf_file(uint8 *elf_file);

void
aot_value_stack_push(AOTValueStack *stack, AOTValue *value);

AOTValue *
aot_value_stack_pop(AOTValueStack *stack);

void
aot_value_stack_destroy(AOTValueStack *stack);

void
aot_block_stack_push(AOTBlockStack *stack, AOTBlock *block);

AOTBlock *
aot_block_stack_pop(AOTBlockStack *stack);

void
aot_block_stack_destroy(AOTBlockStack *stack);

void
aot_block_destroy(AOTBlock *block);

LLVMTypeRef
wasm_type_to_llvm_type(AOTLLVMTypes *llvm_types, uint8 wasm_type);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_LLVM_H_ */

