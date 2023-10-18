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
#include "../compilation/aot_compiler.h"
#include "../interpreter/wasm_opcode.h"

#include "llvm-c/Types.h"
#endif /* WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0 */

#include "aot_trace_exec.h"

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
struct trace_exec_op_info {
    const char *opcode_name;
    enum trace_exec_opcode_kind kind;
};

static const struct trace_exec_op_info simd_info[0xff + 1] = {
    [SIMD_v128_load] = { "v128.load", IMM_memarg_OP_i32 },
    [SIMD_v128_store] = { "v128.store", IMM_memarg_OP_i32_v128 },
    [SIMD_v128_const] = { "v128.const", IMM_v128_OP_0 },
    [SIMD_i8x16_replace_lane] = { "i8x16.replace_lane", IMM_i8_OP_v128_i32 },
    [SIMD_v128_load32_zero] = { "v128.load32_zero", IMM_memarg_OP_i32 },
    [SIMD_f32x4_abs] = { "f32x4.abs", IMM_0_OP_v128 },
    [SIMD_f32x4_min] = { "f32x4.min", IMM_0_OP_v128_v128 },
    [SIMD_f32x4_max] = { "f32x4.max", IMM_0_OP_v128_v128 },
};

static const struct trace_exec_op_info opcode_info[0xff + 1] = {
    [WASM_OP_CALL] = { "call", IMM_i32_OP_0 },
};

/*
 * value_ptr points to imms or opds in struct trace_exec_instruction
 * value is a imm or opd value
 */
static void
aot_trace_exec_fill_in_value(AOTCompContext *comp_ctx,
                             LLVMTypeRef struct_value_type,
                             LLVMValueRef struct_value_ptr,
                             enum trace_exec_value_kind kind,
                             LLVMValueRef value)
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
aot_trace_exec_get_struct_value_type(AOTCompContext *comp_ctx)
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
aot_trace_exec_get_struct_instruction_type(AOTCompContext *comp_ctx)
{
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMTypeRef instr_elem_types[5] = {
        INT8_TYPE, INT8_TYPE, I32_TYPE, ptr_type, ptr_type,
    };
    LLVMTypeRef struct_instr_type = LLVMStructTypeInContext(
        comp_ctx->context, instr_elem_types, ARR_SIZE(instr_elem_types), false);
    return struct_instr_type;
}

static LLVMTypeRef
aot_trace_exec_get_helper_func_type(AOTCompContext *comp_ctx)
{
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMTypeRef param_types[4] = { I32_TYPE, I64_TYPE, ptr_type, ptr_type };
    LLVMTypeRef func_type =
        LLVMFunctionType(VOID_TYPE, param_types, ARR_SIZE(param_types), false);
    return func_type;
}

static bool
aot_trace_exec_assemble_imm_1(AOTCompContext *comp_ctx,
                              enum trace_exec_value_kind imm_value_kind,
                              LLVMValueRef imm, LLVMValueRef instr_imms_ptr)
{
    LLVMTypeRef struct_value_type =
        aot_trace_exec_get_struct_value_type(comp_ctx);

    /* struct trace_exec_value */
    LLVMValueRef imm_ptr =
        LLVMBuildAlloca(comp_ctx->builder, struct_value_type, "imms_ptr");

    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, imm_ptr,
                                 imm_value_kind, imm);

    LLVMValueRef imm_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, imm_ptr, I64_TYPE, "imm_decay");
    LLVMBuildStore(comp_ctx->builder, imm_decay, instr_imms_ptr);

    return true;
}

static bool
aot_trace_exec_assemble_imm_i8(AOTCompContext *comp_ctx, uint8 *ip,
                               LLVMValueRef instr_imms_ptr)
{
    uint32 imm_value = *ip;
    LLVMValueRef imm = LLVMConstInt(I32_TYPE, imm_value, false);

    return aot_trace_exec_assemble_imm_1(comp_ctx, TRACE_V_I8, imm,
                                         instr_imms_ptr);
}

static bool
aot_trace_exec_assemble_imm_i32(AOTCompContext *comp_ctx, uint8 *ip,
                                LLVMValueRef instr_imms_ptr)
{
    uint32 imm_value;
    read_leb_uint32(ip, imm_value);
    LLVMValueRef imm = LLVMConstInt(I32_TYPE, imm_value, false);

    return aot_trace_exec_assemble_imm_1(comp_ctx, TRACE_V_I32, imm,
                                         instr_imms_ptr);
}

static bool
aot_trace_exec_assemble_imm_v128(AOTCompContext *comp_ctx, uint8 *ip,
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

    return aot_trace_exec_assemble_imm_1(comp_ctx, TRACE_V_V128, imm,
                                         instr_imms_ptr);
}

static bool
aot_trace_exec_assemble_imm_memarg(AOTCompContext *comp_ctx, uint8 *ip,
                                   LLVMValueRef instr_imms_ptr)
{
    uint32 align, offset;
    read_leb_uint32(ip, align);
    read_leb_uint32(ip, offset);

    LLVMTypeRef struct_value_type =
        aot_trace_exec_get_struct_value_type(comp_ctx);

    /* struct trace_exec_value[2] */
    LLVMTypeRef imms_type = LLVMArrayType(struct_value_type, 2);
    LLVMValueRef imms_ptr =
        LLVMBuildAlloca(comp_ctx->builder, imms_type, "imms_ptr");

    /* imm1:align*/
    LLVMValueRef indices[2] = { I32_ZERO, I32_ZERO };
    LLVMValueRef imms_0_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_0_ptr");

    LLVMValueRef imms_0 = LLVMConstInt(I32_TYPE, align, false);
    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_0_ptr,
                                 TRACE_V_I32, imms_0);

    /* imm2:offset*/
    indices[1] = I32_ONE;
    LLVMValueRef imms_1_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, imms_type, imms_ptr, indices, 2, "imms_1_ptr");

    LLVMValueRef imms_1 = LLVMConstInt(I32_TYPE, offset, false);
    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, imms_1_ptr,
                                 TRACE_V_I32, imms_1);

    LLVMValueRef imms_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, imms_ptr, I64_TYPE, "imms_decay");
    LLVMBuildStore(comp_ctx->builder, imms_decay, instr_imms_ptr);

    return true;
}

static bool
aot_trace_exec_assemble_opd_1(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx,
                              enum trace_exec_value_kind opd_value_kind,
                              LLVMValueRef instr_opds_ptr)
{
    LLVMTypeRef struct_value_type =
        aot_trace_exec_get_struct_value_type(comp_ctx);

    LLVMValueRef opd_ptr =
        LLVMBuildAlloca(comp_ctx->builder, struct_value_type, "opd_ptr");

    LLVMValueRef opd =
        aot_value_stack_peek(&func_ctx->block_stack.block_list_end->value_stack,
                             0)
            ->value;
    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, opd_ptr,
                                 opd_value_kind, opd);

    LLVMValueRef opd_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, opd_ptr, I64_TYPE, "opd_decay");
    LLVMBuildStore(comp_ctx->builder, opd_decay, instr_opds_ptr);
    return true;
}

static bool
aot_trace_exec_assemble_opd_i32(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx,
                                LLVMValueRef instr_opds_ptr)
{
    return aot_trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_I32,
                                         instr_opds_ptr);
}

static bool
aot_trace_exec_assemble_opd_v128(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx,
                                 LLVMValueRef instr_opds_ptr)
{
    return aot_trace_exec_assemble_opd_1(comp_ctx, func_ctx, TRACE_V_V128,
                                         instr_opds_ptr);
}

static bool
aot_trace_exec_assemble_opd_2(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx,
                              enum trace_exec_value_kind opd1_value_kind,
                              enum trace_exec_value_kind opd2_value_kind,
                              LLVMValueRef instr_opds_ptr)
{
    LLVMTypeRef struct_value_type =
        aot_trace_exec_get_struct_value_type(comp_ctx);

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
    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, opds_0_ptr,
                                 opd1_value_kind, op1);

    indices[1] = I32_ONE;
    LLVMValueRef opds_1_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, opds_type, opds_ptr, indices, 2, "opds_1_ptr");
    LLVMValueRef op2 =
        aot_value_stack_peek(&func_ctx->block_stack.block_list_end->value_stack,
                             0)
            ->value;
    aot_trace_exec_fill_in_value(comp_ctx, struct_value_type, opds_1_ptr,
                                 opd2_value_kind, op2);

    LLVMValueRef opds_decay =
        LLVMBuildPtrToInt(comp_ctx->builder, opds_ptr, I64_TYPE, "opds_decay");
    LLVMBuildStore(comp_ctx->builder, opds_decay, instr_opds_ptr);
    return true;
}

static bool
aot_trace_exec_assemble_opd_i32_v128(AOTCompContext *comp_ctx,
                                     AOTFuncContext *func_ctx,
                                     LLVMValueRef instr_opds_ptr)
{
    return aot_trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_I32,
                                         TRACE_V_V128, instr_opds_ptr);
}

static bool
aot_trace_exec_assemble_opd_v128_i32(AOTCompContext *comp_ctx,
                                     AOTFuncContext *func_ctx,
                                     LLVMValueRef instr_opds_ptr)
{
    return aot_trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_V128,
                                         TRACE_V_I32, instr_opds_ptr);
}

static bool
aot_trace_exec_assemble_opd_v128_v128(AOTCompContext *comp_ctx,
                                      AOTFuncContext *func_ctx,
                                      LLVMValueRef instr_opds_ptr)
{
    return aot_trace_exec_assemble_opd_2(comp_ctx, func_ctx, TRACE_V_V128,
                                         TRACE_V_V128, instr_opds_ptr);
}

static bool
aot_trace_exec_build_helper_func_args(AOTCompContext *comp_ctx,
                                      AOTFuncContext *func_ctx, uint32 func_idx,
                                      uint8 *ip, uint8 opcode, uint8 ext_opcode,
                                      const char *opcode_name,
                                      enum trace_exec_opcode_kind kind,
                                      LLVMValueRef *args, uint32 *args_num)
{
    /* func_idx */
    args[0] = LLVMConstInt(I32_TYPE, func_idx, false);
    /* offset = ip - len(two opcodes)*/
    args[1] = LLVMConstInt(
        I64_TYPE, (uintptr_t)ip - (uintptr_t)func_ctx->aot_func->code - 2,
        false);
    /* opcode_name */
    args[2] =
        LLVMBuildGlobalStringPtr(comp_ctx->builder, opcode_name, "opcode_name");
    /* instr */
    args[3] = LLVMBuildAlloca(
        comp_ctx->builder, aot_trace_exec_get_struct_instruction_type(comp_ctx),
        "instr");

    LLVMTypeRef struct_instr_type =
        aot_trace_exec_get_struct_instruction_type(comp_ctx);

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

    switch (kind) {
        case IMM_0_OP_v128:
        {
            aot_trace_exec_assemble_opd_v128(comp_ctx, func_ctx,
                                             instr_opds_ptr);
            break;
        }
        case IMM_0_OP_v128_v128:
        {
            aot_trace_exec_assemble_opd_v128_v128(comp_ctx, func_ctx,
                                                  instr_opds_ptr);
            break;
        }
        case IMM_i32_OP_0:
        {
            aot_trace_exec_assemble_imm_i32(comp_ctx, ip, instr_imms_ptr);
            break;
        }
        case IMM_v128_OP_0:
        {
            aot_trace_exec_assemble_imm_v128(comp_ctx, ip, instr_imms_ptr);
            break;
        }
        case IMM_i8_OP_v128_i32:
        {
            aot_trace_exec_assemble_imm_i8(comp_ctx, ip, instr_imms_ptr);
            aot_trace_exec_assemble_opd_v128_i32(comp_ctx, func_ctx,
                                                 instr_opds_ptr);
            break;
        }
        case IMM_memarg_OP_i32:
        {
            aot_trace_exec_assemble_imm_memarg(comp_ctx, ip, instr_imms_ptr);
            aot_trace_exec_assemble_opd_i32(comp_ctx, func_ctx, instr_opds_ptr);
            break;
        }
        case IMM_memarg_OP_i32_v128:
        {
            aot_trace_exec_assemble_imm_memarg(comp_ctx, ip, instr_imms_ptr);
            aot_trace_exec_assemble_opd_i32_v128(comp_ctx, func_ctx,
                                                 instr_opds_ptr);
            break;
        }
        default:
        {
            LOG_ERROR("not implement 0x%02x 0x%02x", opcode, ext_opcode);
            *args_num = 0;
            return false;
        }
    }

    *args_num = 4;
    return true;
}

bool
aot_trace_exec_build_call_helper(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx, uint32 func_idx,
                                 uint8 opcode, uint8 ext_opcode, uint8 *ip)
{
    // func
    LLVMTypeRef func_type;
    LLVMValueRef func;
    LLVMTypeRef ptr_type = LLVMPointerTypeInContext(comp_ctx->context, 0);
    LLVMTypeRef param_types[4] = { I32_TYPE, I64_TYPE, ptr_type, ptr_type };
    LLVMTypeRef ret_type = VOID_TYPE;
    LLVMTypeRef func_ptr_type;
    LLVMValueRef value;

    GET_AOT_FUNCTION(aot_trace_exec_helper, ARR_SIZE(param_types));

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

    /* not imported func index -> all func index */
    func_idx += comp_ctx->comp_data->wasm_module->import_function_count;
    LLVMValueRef args[4] = { 0 };
    uint32 args_num = 0;
    bool ret = aot_trace_exec_build_helper_func_args(
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
            "[TRACE EXEC] llvm build aot_trace_exec_op_v128() failed.");
        return false;
    }

    return true;
fail:
    return false;
}

#endif /* WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0 */

/* ============================== execution ============================== */
#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_AOT != 0
static void
pprint_i8(uint8 operand)
{
    os_printf("0x%02x", operand);
}

static void
pprint_i32(uint32 operand)
{
    os_printf("0x%08x", operand);
}

static void
pprint_i64(uint64 operand)
{
    os_printf("0x%016lx", operand);
}

static void
pprint_f32(float32 operand)
{
    os_printf("0x%08x", operand);
}

static void
pprint_f64(float64 operand)
{
    os_printf("0x%016lx", operand);
}

static void
pprint_v128(uint64 *operand)
{
    os_printf("0x%016lx 0x%016lx", operand[0], operand[1]);
}

static inline void
pprint_prelude(uint32 func_idx, uint64 offset, const char *opcode_name)
{
    os_printf("| #%08u | %08u | %s ", func_idx, offset, opcode_name);
}

static inline void
pprint_epilogue()
{
    os_printf("\n");
}

static void
pprint_imms(enum trace_exec_opcode_kind kind,
            struct trace_exec_value *imms)
{
    switch (kind) {
        case IMM_0_OP_v128:
        case IMM_0_OP_v128_v128:
        {
            break;
        }
        case IMM_i8_OP_v128_i32:
        {
            os_printf("[lane %u], ", imms[0].of.i8);
            break;
        }
        case IMM_i32_OP_0:
        {
            pprint_i32(imms[0].of.i32);
            break;
        }
        case IMM_v128_OP_0:
        {
            pprint_v128(imms[0].of.v128);
            break;
        }
        case IMM_memarg_OP_i32:
        case IMM_memarg_OP_i32_v128:
        {
            os_printf("[align %u, offset %u] ", imms[0].of.i32, imms[1].of.i32);
            break;
        }
        default:
        {
            os_printf("not implement !");
        }
    }
}

static void
pprint_opd(struct trace_exec_value *opd)
{
    switch (opd->kind) {
        case TRACE_V_I32:
        {
            pprint_i32(opd->of.i32);
            break;
        }
        case TRACE_V_I64:
        {
            pprint_i64(opd->of.i64);
            break;
        }
        case TRACE_V_F32:
        {
            pprint_f32(opd->of.f32);
            break;
        }
        case TRACE_V_F64:
        {
            pprint_f64(opd->of.f64);
            break;
        }
        case TRACE_V_V128:
        {
            pprint_v128(opd->of.v128);
            break;
        }
        default:
            bh_assert(!"unexpected kind");
    }
}

static void
pprint_opds(enum trace_exec_opcode_kind kind,
            struct trace_exec_value *opds)
{
    switch (kind) {
        /* no operands */
        case IMM_i32_OP_0:
        case IMM_v128_OP_0:
        {
            break;
        }
        /* 1 operands */
        case IMM_0_OP_v128:
        case IMM_memarg_OP_i32:
        {
            pprint_opd(&opds[0]);
            break;
        }
        /* 2 operands */
        case IMM_0_OP_v128_v128:
        case IMM_i8_OP_v128_i32:
        case IMM_memarg_OP_i32_v128:
        {
            pprint_opd(&opds[0]);
            os_printf(", ");
            pprint_opd(&opds[1]);
            break;
        }
        default:
        {
            os_printf("not implement !");
        }
    }
}

void
aot_trace_exec_helper(uint32 func_idx, uint64 offset, const char *opcode_name,
                      struct trace_exec_instruction *instr)
{
    pprint_prelude(func_idx, offset, opcode_name);

    /* print imms */
    pprint_imms(instr->kind, instr->imms);
    os_printf(" ");
    /* print opds */
    pprint_opds(instr->kind, instr->opds);

    pprint_epilogue();
}

#endif