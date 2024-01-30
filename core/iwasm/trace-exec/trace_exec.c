/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "platform_api_vmcore.h"
#include "platform_common.h"
#include "wasm_runtime_common.h"
#include <stdarg.h>
#include <stdint.h>

#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0
#include "aot_llvm.h"
#include "../compilation/aot_compiler.h"
#include "../interpreter/wasm_opcode.h"

#include "llvm-c/Types.h"
#endif /* WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0 */

#include "trace_exec.h"
#include "trace_exec_ops.h"

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0
/* ============================== ============================== */
// TODO: make a public helper?

/* no worried on out of range reading */
static bool
read_leb(const uint8 *buf, uint32 *p_offset, uint32 maxbits, bool sign,
         uint64 *p_result)
{
    uint64 result = 0;
    uint32 shift = 0;
    uint32 bcnt = 0;
    uint64 byte;

    while (true) {
        byte = buf[*p_offset];
        *p_offset += 1;
        result |= ((byte & 0x7f) << (shift % 64));
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
        bcnt += 1;
    }
    if (bcnt > (maxbits + 6) / 7) {
        aot_set_last_error("read leb failed: "
                           "integer representation too long");
        return false;
    }
    if (sign && (shift < maxbits) && (byte & 0x40)) {
        /* Sign extend */
        result |= (~((uint64)0)) << shift;
    }
    *p_result = result;
    return true;
}

#define read_leb_uint32(p, res)                    \
    do {                                           \
        uint32 off = 0;                            \
        uint64 res64;                              \
        if (!read_leb(p, &off, 32, false, &res64)) \
            return false;                          \
        p += off;                                  \
        res = (uint32)res64;                       \
    } while (0)

#define read_leb_int32(p, res)                    \
    do {                                          \
        uint32 off = 0;                           \
        uint64 res64;                             \
        if (!read_leb(p, &off, 32, true, &res64)) \
            return false;                         \
        p += off;                                 \
        res = (int32)res64;                       \
    } while (0)

#define read_leb_int64(p, res)                    \
    do {                                          \
        uint32 off = 0;                           \
        uint64 res64;                             \
        if (!read_leb(p, &off, 64, true, &res64)) \
            return false;                         \
        p += off;                                 \
        res = (int64)res64;                       \
    } while (0)

/* ============================== compilation ============================== */
typedef bool (*imm_assembler)(AOTCompContext *, uint8 *, LLVMValueRef);
typedef bool (*opd_assembler)(AOTCompContext *, AOTFuncContext *, LLVMValueRef);
struct trace_exec_op_assembler {
    imm_assembler imm_assembler;
    opd_assembler opd_assembler;
};

/*
 * value_ptr points to imms or opds in struct trace_exec_instruction
 * value is a imm or opd value
 */
static void
trace_exec_fill_in_value(AOTCompContext *comp_ctx,
                         LLVMTypeRef struct_value_type,
                         LLVMValueRef struct_value_ptr,
                         enum trace_exec_value_kind kind, LLVMValueRef value)
{
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMValueRef memset_func =
        LLVMGetNamedFunction(comp_ctx->module, "llvm.memset.p0.i32");

    LLVMTypeRef params_type[4] = {
        ptr_type,
        INT8_TYPE,
        I32_TYPE,
        INT1_TYPE,
    };
    LLVMTypeRef func_type =
        LLVMFunctionType(VOID_TYPE, params_type, ARR_SIZE(params_type), false);
    if (!memset_func) {
        memset_func =
            LLVMAddFunction(comp_ctx->module, "llvm.memset.p0.i32", func_type);
    }

    LLVMValueRef memset_args[4] = {
        struct_value_ptr,
        I8_ZERO,
        LLVMConstInt(I32_TYPE, sizeof(struct trace_exec_value), false),
        comp_ctx->llvm_consts.i1_zero,
    };

    LLVMBuildCall2(comp_ctx->builder, func_type, memset_func, memset_args, 4,
                   "");

    /* .kind */
    LLVMValueRef kind_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_value_type, struct_value_ptr, 0, "kind_ptr");
    LLVMBuildStore(comp_ctx->builder, LLVMConstInt(I32_TYPE, kind, false),
                   kind_ptr);

    /* .of*/
    LLVMValueRef of_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_value_type, struct_value_ptr, 4, "of_ptr");
    LLVMBuildStore(comp_ctx->builder, value, of_ptr);
}

static LLVMTypeRef
trace_exec_get_struct_value_type(AOTCompContext *comp_ctx)
{
    LLVMTypeRef struct_value_elem_types[5] = {
        I32_TYPE, I32_TYPE, I32_TYPE, I32_TYPE, V128_TYPE,
    };
    LLVMTypeRef struct_value_type =
        LLVMStructTypeInContext(comp_ctx->context, struct_value_elem_types,
                                ARR_SIZE(struct_value_elem_types), false);
    return struct_value_type;
}

static LLVMTypeRef
trace_exec_get_struct_instruction_type(AOTCompContext *comp_ctx)
{
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMTypeRef instr_elem_types[5] = {
        INT8_TYPE, INT8_TYPE, I32_TYPE, ptr_type, ptr_type,
    };
    LLVMTypeRef struct_instr_type = LLVMStructTypeInContext(
        comp_ctx->context, instr_elem_types, ARR_SIZE(instr_elem_types), false);
    return struct_instr_type;
}

static bool
trace_exec_assemble_imm_1(AOTCompContext *comp_ctx,
                          enum trace_exec_value_kind imm_value_kind,
                          LLVMValueRef imm, LLVMValueRef instr_imms_ptr)
{
    LLVMTypeRef struct_value_type = trace_exec_get_struct_value_type(comp_ctx);

    /* struct trace_exec_value */
    LLVMValueRef imm_ptr =
        LLVMBuildAlloca(comp_ctx->builder, struct_value_type, "imms_ptr");

    trace_exec_fill_in_value(comp_ctx, struct_value_type, imm_ptr,
                             imm_value_kind, imm);

    LLVMValueRef imm_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, imm_ptr, I64_TYPE, "imm_decay");
    LLVMBuildStore(comp_ctx->builder, imm_decay, instr_imms_ptr);

    return true;
}

static bool
trace_exec_assemble_imm_i8(AOTCompContext *comp_ctx, uint8 *ip,
                           LLVMValueRef instr_imms_ptr)
{
    uint32 imm_value = *ip;
    LLVMValueRef imm = LLVMConstInt(I32_TYPE, imm_value, false);

    return trace_exec_assemble_imm_1(comp_ctx, TRACE_V_I8, imm, instr_imms_ptr);
}

static bool
trace_exec_assemble_imm_i32(AOTCompContext *comp_ctx, uint8 *ip,
                            LLVMValueRef instr_imms_ptr)
{
    uint32 imm_value;
    read_leb_uint32(ip, imm_value);
    LLVMValueRef imm = LLVMConstInt(I32_TYPE, imm_value, false);

    return trace_exec_assemble_imm_1(comp_ctx, TRACE_V_I32, imm,
                                     instr_imms_ptr);
}

static bool
trace_exec_assemble_imm_f32(AOTCompContext *comp_ctx, uint8 *ip,
                            LLVMValueRef instr_imms_ptr)
{
    float32 imm_value = *(float32 *)ip;

    LLVMValueRef imm = LLVMConstReal(F32_TYPE, imm_value);

    return trace_exec_assemble_imm_1(comp_ctx, TRACE_V_F32, imm,
                                     instr_imms_ptr);
}

static bool
trace_exec_assemble_imm_v128(AOTCompContext *comp_ctx, uint8 *ip,
                             LLVMValueRef instr_imms_ptr)
{
    uint64 imm_value[2];
    wasm_runtime_read_v128(ip, &imm_value[0], &imm_value[1]);
    /* imm */
    LLVMValueRef imm_elem[2] = {
        LLVMConstInt(I64_TYPE, imm_value[0], false),
        LLVMConstInt(I64_TYPE, imm_value[1], false),
    };
    LLVMValueRef imm = LLVMConstVector(imm_elem, 2);

    return trace_exec_assemble_imm_1(comp_ctx, TRACE_V_V128, imm,
                                     instr_imms_ptr);
}

static bool
trace_exec_assemble_imm_2(AOTCompContext *comp_ctx,
                          enum trace_exec_value_kind imms_0_value_kind,
                          LLVMValueRef imms_0,
                          enum trace_exec_value_kind imms_1_value_kind,
                          LLVMValueRef imms_1, LLVMValueRef instr_imms_ptr)
{
    LLVMTypeRef struct_value_type = trace_exec_get_struct_value_type(comp_ctx);

    /* struct trace_exec_value[2] */
    LLVMTypeRef imms_type = LLVMArrayType(struct_value_type, 2);
    LLVMValueRef imms_ptr =
        LLVMBuildAlloca(comp_ctx->builder, imms_type, "imms_ptr");

    /* imm1*/
    LLVMValueRef indices[2] = { I32_ZERO, I32_ZERO };
    LLVMValueRef imms_0_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_0_ptr");
    trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_0_ptr,
                             imms_0_value_kind, imms_0);

    /* imm2*/
    indices[1] = I32_ONE;
    LLVMValueRef imms_1_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_1_ptr");
    trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_1_ptr,
                             imms_1_value_kind, imms_1);

    LLVMValueRef imms_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, imms_ptr, I64_TYPE, "imm_decay");
    LLVMBuildStore(comp_ctx->builder, imms_decay, instr_imms_ptr);

    return true;
}

static bool
trace_exec_assemble_imm_i32_i32(AOTCompContext *comp_ctx, uint8 *ip,
                                LLVMValueRef instr_imms_ptr)
{
    uint32 imm1_val, imm2_val;
    read_leb_uint32(ip, imm1_val);
    read_leb_uint32(ip, imm2_val);

    LLVMValueRef imm1 = LLVMConstInt(I32_TYPE, imm1_val, false);
    LLVMValueRef imm2 = LLVMConstInt(I32_TYPE, imm2_val, false);

    return trace_exec_assemble_imm_2(comp_ctx, TRACE_V_I32, imm1, TRACE_V_I32,
                                     imm2, instr_imms_ptr);
}

static bool
trace_exec_assemble_imm_memarg(AOTCompContext *comp_ctx, uint8 *ip,
                               LLVMValueRef instr_imms_ptr)
{
    uint32 align, offset;
    read_leb_uint32(ip, align);
    read_leb_uint32(ip, offset);

    LLVMTypeRef struct_value_type = trace_exec_get_struct_value_type(comp_ctx);

    /* struct trace_exec_value[2] */
    LLVMTypeRef imms_type = LLVMArrayType(struct_value_type, 2);
    LLVMValueRef imms_ptr =
        LLVMBuildAlloca(comp_ctx->builder, imms_type, "imms_ptr");

    /* imm1:align*/
    LLVMValueRef indices[2] = { I32_ZERO, I32_ZERO };
    LLVMValueRef imms_0_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_0_ptr");

    LLVMValueRef imms_0 = LLVMConstInt(I32_TYPE, align, false);
    trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_0_ptr,
                             TRACE_V_I32, imms_0);

    /* imm2:offset*/
    indices[1] = I32_ONE;
    LLVMValueRef imms_1_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_1_ptr");

    LLVMValueRef imms_1 = LLVMConstInt(I32_TYPE, offset, false);
    trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_1_ptr,
                             TRACE_V_I32, imms_1);

    LLVMValueRef imms_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, imms_ptr, I64_TYPE, "imms_decay");
    LLVMBuildStore(comp_ctx->builder, imms_decay, instr_imms_ptr);

    return true;
}

static bool
trace_exec_assemble_opd_1(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          enum trace_exec_value_kind opd_value_kind,
                          LLVMValueRef instr_opds_ptr)
{
    LLVMTypeRef struct_value_type = trace_exec_get_struct_value_type(comp_ctx);

    LLVMValueRef opd_ptr =
        LLVMBuildAlloca(comp_ctx->builder, struct_value_type, "opd_ptr");

    LLVMValueRef opd =
        aot_value_stack_peek(&func_ctx->block_stack.block_list_end->value_stack,
                             0)
            ->value;
    trace_exec_fill_in_value(comp_ctx, struct_value_type, opd_ptr,
                             opd_value_kind, opd);

    LLVMValueRef opd_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, opd_ptr, I64_TYPE, "opd_decay");
    LLVMBuildStore(comp_ctx->builder, opd_decay, instr_opds_ptr);
    return true;
}

static bool
trace_exec_assemble_opd_i32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_I32,
                                     instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_f32(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_F32,
                                     instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_f64(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_F64,
                                     instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_v128(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_V128,
                                     instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_2(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          enum trace_exec_value_kind opd1_value_kind,
                          enum trace_exec_value_kind opd2_value_kind,
                          LLVMValueRef instr_opds_ptr)
{
    LLVMTypeRef struct_value_type = trace_exec_get_struct_value_type(comp_ctx);

    LLVMTypeRef opds_type = LLVMArrayType(struct_value_type, 2);
    LLVMValueRef opds_ptr =
        LLVMBuildAlloca(comp_ctx->builder, opds_type, "opds_ptr");

    LLVMValueRef indices[2] = { I32_ZERO, I32_ZERO };
    LLVMValueRef opds_0_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, opds_type, opds_ptr, indices, 2, "opds_0_ptr");
    LLVMValueRef op1 =
        aot_value_stack_peek(&func_ctx->block_stack.block_list_end->value_stack,
                             1)
            ->value;
    trace_exec_fill_in_value(comp_ctx, struct_value_type, opds_0_ptr,
                             opd1_value_kind, op1);

    indices[1] = I32_ONE;
    LLVMValueRef opds_1_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, opds_type, opds_ptr, indices, 2, "opds_1_ptr");
    LLVMValueRef op2 =
        aot_value_stack_peek(&func_ctx->block_stack.block_list_end->value_stack,
                             0)
            ->value;
    trace_exec_fill_in_value(comp_ctx, struct_value_type, opds_1_ptr,
                             opd2_value_kind, op2);

    LLVMValueRef opds_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, opds_ptr, I64_TYPE, "opds_decay");
    LLVMBuildStore(comp_ctx->builder, opds_decay, instr_opds_ptr);
    return true;
}

static bool
trace_exec_assemble_opd_i32_i32(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx,
                                LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_I32,
                                     TRACE_V_I32, instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_i32_v128(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx,
                                 LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_I32,
                                     TRACE_V_V128, instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_v128_i32(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx,
                                 LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_V128,
                                     TRACE_V_I32, instr_opds_ptr);
}

static bool
trace_exec_assemble_opd_v128_v128(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx,
                                  LLVMValueRef instr_opds_ptr)
{
    return trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_V128,
                                     TRACE_V_V128, instr_opds_ptr);
}

static bool
trace_exec_build_helper_func_args(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx, uint32 func_idx,
                                  uint8 *ip, uint8 opcode, uint8 ext_opcode,
                                  const char *opcode_name,
                                  enum trace_exec_opcode_kind kind,
                                  LLVMValueRef *args, uint32 *args_num)
{
    /* func_idx */
    args[0] = LLVMConstInt(I32_TYPE, func_idx, false);
    /* offset */
    unsigned offset = (uintptr_t)ip - (uintptr_t)func_ctx->aot_func->code;
    if (opcode < WASM_OP_MISC_PREFIX) {
        /* - len(opcode) */
        offset -= 1;
    }
    else {
        /* - len(opcode + ext_opcode) */
        offset -= 2;
    }
    args[1] = LLVMConstInt(I64_TYPE, offset, false);
    /* opcode_name */
    args[2] =
        LLVMBuildGlobalStringPtr(comp_ctx->builder, opcode_name, "opcode_name");
    /* instr */
    args[3] = LLVMBuildAlloca(comp_ctx->builder,
                              trace_exec_get_struct_instruction_type(comp_ctx),
                              "instr");

    LLVMTypeRef struct_instr_type =
        trace_exec_get_struct_instruction_type(comp_ctx);

    /* instr->opcode */
    LLVMValueRef instr_opcode_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_instr_type, args[3], 0, "instr_opcode_ptr");
    LLVMBuildStore(comp_ctx->builder, LLVMConstInt(INT8_TYPE, opcode, false),
                   instr_opcode_ptr);

    /* instr->ext_opcode */
    LLVMValueRef instr_ext_opcode_ptr =
        LLVMBuildStructGEP2(comp_ctx->builder, struct_instr_type, args[3], 1,
                            "instr_ext_opcode_ptr");
    LLVMBuildStore(comp_ctx->builder,
                   LLVMConstInt(INT8_TYPE, ext_opcode, false),
                   instr_ext_opcode_ptr);

    /* instr->kind */
    LLVMValueRef instr_kind_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_instr_type, args[3], 2, "instr_kind_ptr");
    LLVMBuildStore(comp_ctx->builder, LLVMConstInt(I32_TYPE, kind, false),
                   instr_kind_ptr);

    /* instr->imms */
    LLVMValueRef instr_imms_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_instr_type, args[3], 3, "instr_imms_ptr");
    /* instr->opds */
    LLVMValueRef instr_opds_ptr = LLVMBuildStructGEP2(
        comp_ctx->builder, struct_instr_type, args[3], 4, "instr_opds_ptr");

    struct trace_exec_op_assembler assemblers[OPCODE_KIND_AMOUNT] = {
        [IMM_0_OP_0] = { NULL, NULL },
        [IMM_0_OP_i32] = { NULL, trace_exec_assemble_opd_i32 },
        [IMM_0_OP_f32] = { NULL, trace_exec_assemble_opd_f32 },
        [IMM_0_OP_f64] = { NULL, trace_exec_assemble_opd_f64 },
        [IMM_0_OP_v128] = { NULL, trace_exec_assemble_opd_v128 },
        [IMM_0_OP_i32_i32] = { NULL, trace_exec_assemble_opd_i32_i32 },
        [IMM_0_OP_v128_v128] = { NULL, trace_exec_assemble_opd_v128_v128 },
        [IMM_i32_OP_0] = { trace_exec_assemble_imm_i32, NULL },
        [IMM_f32_OP_0] = { trace_exec_assemble_imm_f32, NULL },
        [IMM_v128_OP_0] = { trace_exec_assemble_imm_v128, NULL },
        [IMM_i8_OP_v128] = { trace_exec_assemble_imm_i8,
                             trace_exec_assemble_opd_v128 },
        [IMM_i32_OP_i32] = { trace_exec_assemble_imm_i32,
                             trace_exec_assemble_opd_i32 },
        [IMM_i8_OP_v128_i32] = { trace_exec_assemble_imm_i8,
                                 trace_exec_assemble_opd_v128_i32 },
        [IMM_i32_i32_OP_i32] = { trace_exec_assemble_imm_i32_i32,
                                 trace_exec_assemble_opd_i32 },
        [IMM_ty_tbl_OP_i32] = { trace_exec_assemble_imm_i32_i32,
                                trace_exec_assemble_opd_i32 },
        [IMM_memarg_OP_i32] = { trace_exec_assemble_imm_memarg,
                                trace_exec_assemble_opd_i32 },
        [IMM_memarg_OP_i32_v128] = { trace_exec_assemble_imm_memarg,
                                     trace_exec_assemble_opd_i32_v128 },
    };

    bh_assert(kind < OPCODE_KIND_AMOUNT);

    if (assemblers[kind].imm_assembler) {
        assemblers[kind].imm_assembler(comp_ctx, ip, instr_imms_ptr);
    }

    if (assemblers[kind].opd_assembler) {
        assemblers[kind].opd_assembler(comp_ctx, func_ctx, instr_opds_ptr);
    }

    *args_num = 4;
    return true;
}

bool
trace_exec_build_call_helper(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 func_idx, uint8 opcode, uint8 ext_opcode,
                             uint8 *ip)
{
    // func
    LLVMTypeRef func_type;
    LLVMValueRef func;
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMTypeRef param_types[4] = { I32_TYPE, I64_TYPE, ptr_type, ptr_type };
    LLVMTypeRef ret_type = VOID_TYPE;
    LLVMTypeRef func_ptr_type;
    LLVMValueRef value;

    GET_AOT_FUNCTION(trace_exec_helper, ARR_SIZE(param_types));

    // build args
    const char *opcode_name;
    enum trace_exec_opcode_kind opcode_kind;
    if (opcode != WASM_OP_SIMD_PREFIX) {
        opcode_name = opcode_info[opcode].opcode_name;
        opcode_kind = opcode_info[opcode].kind;
    }
    else {
        opcode_name = simd_info[ext_opcode].opcode_name;
        opcode_kind = simd_info[ext_opcode].kind;
    }

    // FIXME: remove me when all opcodes are implemented
    if (!opcode_name) {
        // LOG_ERROR("unimplement opcde 0x%02x 0x%02x", opcode, ext_opcode);
        return false;
    }

    /* not imported func index -> all func index */
    func_idx += comp_ctx->comp_data->wasm_module->import_function_count;
    LLVMValueRef args[4] = { 0 };
    uint32 args_num = 0;
    bool ret = trace_exec_build_helper_func_args(
        comp_ctx, func_ctx, func_idx, ip, opcode, ext_opcode, opcode_name,
        opcode_kind, args, &args_num);
    if (!ret) {
        return false;
    }

    bh_assert(args_num == ARR_SIZE(args));

    // build call
    ret =
        LLVMBuildCall2(comp_ctx->builder, func_type, func, args, args_num, "");
    if (!ret) {
        aot_set_last_error(
            "[TRACE EXEC] llvm build trace_exec_op_v128() failed.");
        return false;
    }

    return true;
fail:
    return false;
}

#endif /* WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0 */

/* ============================== execution ============================== */
#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_AOT != 0
typedef void (*imm_pprint)(struct trace_exec_value *);
typedef void (*opd_pprint)(struct trace_exec_value *);

struct trace_exec_op_pprinter {
    imm_pprint imm_pprint;
    opd_pprint opd_pprint;
};

static void
pprint_i8(struct trace_exec_value *v)
{
    os_printf("i8:0x%02x", v->of.i8);
}

static void
pprint_i32(struct trace_exec_value *v)
{
    os_printf("i32:0x%08x", v->of.i32);
}

static void
pprint_i64(struct trace_exec_value *v)
{
    os_printf("i64:0x%016lx", v->of.i64);
}

static void
pprint_f32(struct trace_exec_value *v)
{
    os_printf("i32:0x%08x", v->of.f32);
}

static void
pprint_f64(struct trace_exec_value *v)
{
    os_printf("i64:0x%016lx", v->of.f64);
}

static void
pprint_v128(struct trace_exec_value *v)
{
    os_printf("v128:0x%016lx 0x%016lx", v->of.v128[0], v->of.v128[1]);
}

static void
pprint_i32_i32(struct trace_exec_value *v)
{
    pprint_i32(v);
    os_printf(", ");
    pprint_i32(v + 1);
}

static void
pprint_i32_v128(struct trace_exec_value *v)
{
    pprint_i32(v);
    os_printf(", ");
    pprint_v128(v + 1);
}

static void
pprint_v128_i32(struct trace_exec_value *v)
{
    pprint_v128(v);
    os_printf(", ");
    pprint_i32(v + 1);
}

static void
pprint_v128_v128(struct trace_exec_value *v)
{
    pprint_v128(v);
    os_printf(", ");
    pprint_v128(v + 1);
}

void
pprint_ty_tbl(struct trace_exec_value *v)
{
    os_printf("type %u, table %u", v[0].of.i32, v[1].of.i32);
}

void
pprint_memarg(struct trace_exec_value *v)
{
    os_printf("align %u, offset %u", v[0].of.i32, v[1].of.i32);
}

static inline void
pprint_prelude(uint32 func_idx, uint64 offset, const char *opcode_name)
{
    os_printf("| #%04u | %08u | %s ", func_idx, offset, opcode_name);
}

static inline void
pprint_epilogue()
{
    os_printf("\n");
}

void
trace_exec_helper(uint32 func_idx, uint64 offset, const char *opcode_name,
                  struct trace_exec_instruction *instr)
{
    struct trace_exec_op_pprinter pprinter[OPCODE_KIND_AMOUNT] = {
        [IMM_0_OP_0] = { NULL, NULL },
        [IMM_0_OP_i32] = { NULL, pprint_i32 },
        [IMM_0_OP_f32] = { NULL, pprint_f32 },
        [IMM_0_OP_f64] = { NULL, pprint_f64 },
        [IMM_0_OP_v128] = { NULL, pprint_v128 },
        [IMM_0_OP_i32_i32] = { NULL, pprint_i32_i32 },
        [IMM_0_OP_v128_v128] = { NULL, pprint_v128_v128 },
        [IMM_i32_OP_0] = { pprint_i32, NULL },
        [IMM_f32_OP_0] = { pprint_f32, NULL },
        [IMM_v128_OP_0] = { pprint_v128, NULL },
        [IMM_i8_OP_v128] = { pprint_i8, pprint_v128 },
        [IMM_i32_OP_i32] = { pprint_i32, pprint_i32 },
        [IMM_i8_OP_v128_i32] = { pprint_i8, pprint_v128_i32 },
        [IMM_i32_i32_OP_i32] = { pprint_i32_i32, pprint_i32 },
        [IMM_ty_tbl_OP_i32] = { pprint_ty_tbl, pprint_i32 },
        [IMM_memarg_OP_i32] = { pprint_memarg, pprint_i32 },
        [IMM_memarg_OP_i32_v128] = { pprint_memarg, pprint_i32_v128 },
    };

    bh_assert(instr->kind < OPCODE_KIND_AMOUNT);

    pprint_prelude(func_idx, offset, opcode_name);

    /* print imms */
    if (pprinter[instr->kind].imm_pprint) {
        pprinter[instr->kind].imm_pprint(instr->imms);
    }

    os_printf(" ");

    /* print opds */
    if (pprinter[instr->kind].opd_pprint) {
        pprinter[instr->kind].opd_pprint(instr->opds);
    }

    pprint_epilogue();
}

#endif