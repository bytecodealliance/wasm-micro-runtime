/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_OPCODE_H
#define _WASM_OPCODE_H

#include "wasm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WASMOpcode {
    /* control instructions */
    WASM_OP_UNREACHABLE   = 0x00, /* unreachable */
    WASM_OP_NOP           = 0x01, /* nop */
    WASM_OP_BLOCK         = 0x02, /* block */
    WASM_OP_LOOP          = 0x03, /* loop */
    WASM_OP_IF            = 0x04, /* if */
    WASM_OP_ELSE          = 0x05, /* else */

    WASM_OP_UNUSED_0x06   = 0x06,
    WASM_OP_UNUSED_0x07   = 0x07,
    WASM_OP_UNUSED_0x08   = 0x08,
    WASM_OP_UNUSED_0x09   = 0x09,
    WASM_OP_UNUSED_0x0a   = 0x0a,

    WASM_OP_END           = 0x0b, /* end */
    WASM_OP_BR            = 0x0c, /* br */
    WASM_OP_BR_IF         = 0x0d, /* br if */
    WASM_OP_BR_TABLE      = 0x0e, /* br table */
    WASM_OP_RETURN        = 0x0f, /* return */
    WASM_OP_CALL          = 0x10, /* call */
    WASM_OP_CALL_INDIRECT = 0x11, /* call_indirect */

    WASM_OP_UNUSED_0x12   = 0x12,
    WASM_OP_UNUSED_0x13   = 0x13,
    WASM_OP_UNUSED_0x14   = 0x14,
    WASM_OP_UNUSED_0x15   = 0x15,
    WASM_OP_UNUSED_0x16   = 0x16,
    WASM_OP_UNUSED_0x17   = 0x17,
    WASM_OP_UNUSED_0x18   = 0x18,
    WASM_OP_UNUSED_0x19   = 0x19,

    /* parametric instructions */
    WASM_OP_DROP          = 0x1a, /* drop */
    WASM_OP_SELECT        = 0x1b, /* select */

    WASM_OP_UNUSED_0x1c   = 0x1c,
    WASM_OP_UNUSED_0x1d   = 0x1d,
    WASM_OP_UNUSED_0x1e   = 0x1e,
    WASM_OP_UNUSED_0x1f   = 0x1f,

    /* variable instructions */
    WASM_OP_GET_LOCAL     = 0x20, /* get_local */
    WASM_OP_SET_LOCAL     = 0x21, /* set_local */
    WASM_OP_TEE_LOCAL     = 0x22, /* tee_local */
    WASM_OP_GET_GLOBAL    = 0x23, /* get_global */
    WASM_OP_SET_GLOBAL    = 0x24, /* set_global */

    WASM_OP_UNUSED_0x25   = 0x25,
    WASM_OP_UNUSED_0x26   = 0x26,
    WASM_OP_UNUSED_0x27   = 0x27,

    /* memory instructions */
    WASM_OP_I32_LOAD      = 0x28, /* i32.load */
    WASM_OP_I64_LOAD      = 0x29, /* i64.load */
    WASM_OP_F32_LOAD      = 0x2a, /* f32.load */
    WASM_OP_F64_LOAD      = 0x2b, /* f64.load */
    WASM_OP_I32_LOAD8_S   = 0x2c, /* i32.load8_s */
    WASM_OP_I32_LOAD8_U   = 0x2d, /* i32.load8_u */
    WASM_OP_I32_LOAD16_S  = 0x2e, /* i32.load16_s */
    WASM_OP_I32_LOAD16_U  = 0x2f, /* i32.load16_u */
    WASM_OP_I64_LOAD8_S   = 0x30, /* i64.load8_s */
    WASM_OP_I64_LOAD8_U   = 0x31, /* i64.load8_u */
    WASM_OP_I64_LOAD16_S  = 0x32, /* i64.load16_s */
    WASM_OP_I64_LOAD16_U  = 0x33, /* i64.load16_u */
    WASM_OP_I64_LOAD32_S  = 0x34, /* i32.load32_s */
    WASM_OP_I64_LOAD32_U  = 0x35, /* i32.load32_u */
    WASM_OP_I32_STORE     = 0x36, /* i32.store */
    WASM_OP_I64_STORE     = 0x37, /* i64.store */
    WASM_OP_F32_STORE     = 0x38, /* f32.store */
    WASM_OP_F64_STORE     = 0x39, /* f64.store */
    WASM_OP_I32_STORE8    = 0x3a, /* i32.store8 */
    WASM_OP_I32_STORE16   = 0x3b, /* i32.store16 */
    WASM_OP_I64_STORE8    = 0x3c, /* i64.store8 */
    WASM_OP_I64_STORE16   = 0x3d, /* i64.sotre16 */
    WASM_OP_I64_STORE32   = 0x3e, /* i64.store32 */
    WASM_OP_MEMORY_SIZE   = 0x3f, /* memory.size */
    WASM_OP_MEMORY_GROW   = 0x40, /* memory.grow */

    /* constant instructions */
    WASM_OP_I32_CONST     = 0x41, /* i32.const */
    WASM_OP_I64_CONST     = 0x42, /* i64.const */
    WASM_OP_F32_CONST     = 0x43, /* f32.const */
    WASM_OP_F64_CONST     = 0x44, /* f64.const */

    /* comparison instructions */
    WASM_OP_I32_EQZ       = 0x45, /* i32.eqz */
    WASM_OP_I32_EQ        = 0x46, /* i32.eq */
    WASM_OP_I32_NE        = 0x47, /* i32.ne */
    WASM_OP_I32_LT_S      = 0x48, /* i32.lt_s */
    WASM_OP_I32_LT_U      = 0x49, /* i32.lt_u */
    WASM_OP_I32_GT_S      = 0x4a, /* i32.gt_s */
    WASM_OP_I32_GT_U      = 0x4b, /* i32.gt_u */
    WASM_OP_I32_LE_S      = 0x4c, /* i32.le_s */
    WASM_OP_I32_LE_U      = 0x4d, /* i32.le_u */
    WASM_OP_I32_GE_S      = 0x4e, /* i32.ge_s */
    WASM_OP_I32_GE_U      = 0x4f, /* i32.ge_u */

    WASM_OP_I64_EQZ       = 0x50, /* i64.eqz */
    WASM_OP_I64_EQ        = 0x51, /* i64.eq */
    WASM_OP_I64_NE        = 0x52, /* i64.ne */
    WASM_OP_I64_LT_S      = 0x53, /* i64.lt_s */
    WASM_OP_I64_LT_U      = 0x54, /* i64.lt_u */
    WASM_OP_I64_GT_S      = 0x55, /* i64.gt_s */
    WASM_OP_I64_GT_U      = 0x56, /* i64.gt_u */
    WASM_OP_I64_LE_S      = 0x57, /* i64.le_s */
    WASM_OP_I64_LE_U      = 0x58, /* i64.le_u */
    WASM_OP_I64_GE_S      = 0x59, /* i64.ge_s */
    WASM_OP_I64_GE_U      = 0x5a, /* i64.ge_u */

    WASM_OP_F32_EQ        = 0x5b, /* f32.eq */
    WASM_OP_F32_NE        = 0x5c, /* f32.ne */
    WASM_OP_F32_LT        = 0x5d, /* f32.lt */
    WASM_OP_F32_GT        = 0x5e, /* f32.gt */
    WASM_OP_F32_LE        = 0x5f, /* f32.le */
    WASM_OP_F32_GE        = 0x60, /* f32.ge */

    WASM_OP_F64_EQ        = 0x61, /* f64.eq */
    WASM_OP_F64_NE        = 0x62, /* f64.ne */
    WASM_OP_F64_LT        = 0x63, /* f64.lt */
    WASM_OP_F64_GT        = 0x64, /* f64.gt */
    WASM_OP_F64_LE        = 0x65, /* f64.le */
    WASM_OP_F64_GE        = 0x66, /* f64.ge */

    /* numeric operators */
    WASM_OP_I32_CLZ       = 0x67, /* i32.clz */
    WASM_OP_I32_CTZ       = 0x68, /* i32.ctz */
    WASM_OP_I32_POPCNT    = 0x69, /* i32.popcnt */
    WASM_OP_I32_ADD       = 0x6a, /* i32.add */
    WASM_OP_I32_SUB       = 0x6b, /* i32.sub */
    WASM_OP_I32_MUL       = 0x6c, /* i32.mul */
    WASM_OP_I32_DIV_S     = 0x6d, /* i32.div_s */
    WASM_OP_I32_DIV_U     = 0x6e, /* i32.div_u */
    WASM_OP_I32_REM_S     = 0x6f, /* i32.rem_s */
    WASM_OP_I32_REM_U     = 0x70, /* i32.rem_u */
    WASM_OP_I32_AND       = 0x71, /* i32.and */
    WASM_OP_I32_OR        = 0x72, /* i32.or */
    WASM_OP_I32_XOR       = 0x73, /* i32.xor */
    WASM_OP_I32_SHL       = 0x74, /* i32.shl */
    WASM_OP_I32_SHR_S     = 0x75, /* i32.shr_s */
    WASM_OP_I32_SHR_U     = 0x76, /* i32.shr_u */
    WASM_OP_I32_ROTL      = 0x77, /* i32.rotl */
    WASM_OP_I32_ROTR      = 0x78, /* i32.rotr */

    WASM_OP_I64_CLZ       = 0x79, /* i64.clz */
    WASM_OP_I64_CTZ       = 0x7a, /* i64.ctz */
    WASM_OP_I64_POPCNT    = 0x7b, /* i64.popcnt */
    WASM_OP_I64_ADD       = 0x7c, /* i64.add */
    WASM_OP_I64_SUB       = 0x7d, /* i64.sub */
    WASM_OP_I64_MUL       = 0x7e, /* i64.mul */
    WASM_OP_I64_DIV_S     = 0x7f, /* i64.div_s */
    WASM_OP_I64_DIV_U     = 0x80, /* i64.div_u */
    WASM_OP_I64_REM_S     = 0x81, /* i64.rem_s */
    WASM_OP_I64_REM_U     = 0x82, /* i64.rem_u */
    WASM_OP_I64_AND       = 0x83, /* i64.and */
    WASM_OP_I64_OR        = 0x84, /* i64.or */
    WASM_OP_I64_XOR       = 0x85, /* i64.xor */
    WASM_OP_I64_SHL       = 0x86, /* i64.shl */
    WASM_OP_I64_SHR_S     = 0x87, /* i64.shr_s */
    WASM_OP_I64_SHR_U     = 0x88, /* i64.shr_u */
    WASM_OP_I64_ROTL      = 0x89, /* i64.rotl */
    WASM_OP_I64_ROTR      = 0x8a, /* i64.rotr */

    WASM_OP_F32_ABS       = 0x8b, /* f32.abs */
    WASM_OP_F32_NEG       = 0x8c, /* f32.neg */
    WASM_OP_F32_CEIL      = 0x8d, /* f32.ceil */
    WASM_OP_F32_FLOOR     = 0x8e, /* f32.floor */
    WASM_OP_F32_TRUNC     = 0x8f, /* f32.trunc */
    WASM_OP_F32_NEAREST   = 0x90, /* f32.nearest */
    WASM_OP_F32_SQRT      = 0x91, /* f32.sqrt */
    WASM_OP_F32_ADD       = 0x92, /* f32.add */
    WASM_OP_F32_SUB       = 0x93, /* f32.sub */
    WASM_OP_F32_MUL       = 0x94, /* f32.mul */
    WASM_OP_F32_DIV       = 0x95, /* f32.div */
    WASM_OP_F32_MIN       = 0x96, /* f32.min */
    WASM_OP_F32_MAX       = 0x97, /* f32.max */
    WASM_OP_F32_COPYSIGN  = 0x98, /* f32.copysign */

    WASM_OP_F64_ABS       = 0x99, /* f64.abs */
    WASM_OP_F64_NEG       = 0x9a, /* f64.neg */
    WASM_OP_F64_CEIL      = 0x9b, /* f64.ceil */
    WASM_OP_F64_FLOOR     = 0x9c, /* f64.floor */
    WASM_OP_F64_TRUNC     = 0x9d, /* f64.trunc */
    WASM_OP_F64_NEAREST   = 0x9e, /* f64.nearest */
    WASM_OP_F64_SQRT      = 0x9f, /* f64.sqrt */
    WASM_OP_F64_ADD       = 0xa0, /* f64.add */
    WASM_OP_F64_SUB       = 0xa1, /* f64.sub */
    WASM_OP_F64_MUL       = 0xa2, /* f64.mul */
    WASM_OP_F64_DIV       = 0xa3, /* f64.div */
    WASM_OP_F64_MIN       = 0xa4, /* f64.min */
    WASM_OP_F64_MAX       = 0xa5, /* f64.max */
    WASM_OP_F64_COPYSIGN  = 0xa6, /* f64.copysign */

    /* conversions */
    WASM_OP_I32_WRAP_I64      = 0xa7, /* i32.wrap/i64 */
    WASM_OP_I32_TRUNC_S_F32   = 0xa8, /* i32.trunc_s/f32 */
    WASM_OP_I32_TRUNC_U_F32   = 0xa9, /* i32.trunc_u/f32 */
    WASM_OP_I32_TRUNC_S_F64   = 0xaa, /* i32.trunc_s/f64 */
    WASM_OP_I32_TRUNC_U_F64   = 0xab, /* i32.trunc_u/f64 */

    WASM_OP_I64_EXTEND_S_I32  = 0xac, /* i64.extend_s/i32 */
    WASM_OP_I64_EXTEND_U_I32  = 0xad, /* i64.extend_u/i32 */
    WASM_OP_I64_TRUNC_S_F32   = 0xae, /* i64.trunc_s/f32 */
    WASM_OP_I64_TRUNC_U_F32   = 0xaf, /* i64.trunc_u/f32 */
    WASM_OP_I64_TRUNC_S_F64   = 0xb0, /* i64.trunc_s/f64 */
    WASM_OP_I64_TRUNC_U_F64   = 0xb1, /* i64.trunc_u/f64 */

    WASM_OP_F32_CONVERT_S_I32 = 0xb2, /* f32.convert_s/i32 */
    WASM_OP_F32_CONVERT_U_I32 = 0xb3, /* f32.convert_u/i32 */
    WASM_OP_F32_CONVERT_S_I64 = 0xb4, /* f32.convert_s/i64 */
    WASM_OP_F32_CONVERT_U_I64 = 0xb5, /* f32.convert_u/i64 */
    WASM_OP_F32_DEMOTE_F64    = 0xb6, /* f32.demote/f64 */

    WASM_OP_F64_CONVERT_S_I32 = 0xb7, /* f64.convert_s/i32 */
    WASM_OP_F64_CONVERT_U_I32 = 0xb8, /* f64.convert_u/i32 */
    WASM_OP_F64_CONVERT_S_I64 = 0xb9, /* f64.convert_s/i64 */
    WASM_OP_F64_CONVERT_U_I64 = 0xba, /* f64.convert_u/i64 */
    WASM_OP_F64_PROMOTE_F32   = 0xbb, /* f64.promote/f32 */

    /* reinterpretations */
    WASM_OP_I32_REINTERPRET_F32   = 0xbc, /* i32.reinterpret/f32 */
    WASM_OP_I64_REINTERPRET_F64   = 0xbd, /* i64.reinterpret/f64 */
    WASM_OP_F32_REINTERPRET_I32   = 0xbe, /* f32.reinterpret/i32 */
    WASM_OP_F64_REINTERPRET_I64   = 0xbf, /* f64.reinterpret/i64 */

    /* drop/select specified types*/
    WASM_OP_DROP_64               = 0xc0,
    WASM_OP_SELECT_64             = 0xc1,

    /* extend op code */
    EXT_OP_GET_LOCAL_FAST         = 0xc2,
    EXT_OP_SET_LOCAL_FAST_I64     = 0xc3,
    EXT_OP_SET_LOCAL_FAST         = 0xc4,
    EXT_OP_TEE_LOCAL_FAST         = 0xc5,
    EXT_OP_TEE_LOCAL_FAST_I64     = 0xc6,
    EXT_OP_COPY_STACK_TOP         = 0xc7,
    EXT_OP_COPY_STACK_TOP_I64     = 0xc8,

    WASM_OP_IMPDEP                = 0xc9
} WASMOpcode;

#ifdef __cplusplus
}
#endif

/*
 * Macro used to generate computed goto tables for the C interpreter.
 */
#define WASM_INSTRUCTION_NUM 256

#define DEFINE_GOTO_TABLE(type, _name)                       \
static type _name[WASM_INSTRUCTION_NUM] = {                  \
  HANDLE_OPCODE (WASM_OP_UNREACHABLE),   /* 0x00 */          \
  HANDLE_OPCODE (WASM_OP_NOP),           /* 0x01 */          \
  HANDLE_OPCODE (WASM_OP_BLOCK),         /* 0x02 */          \
  HANDLE_OPCODE (WASM_OP_LOOP),          /* 0x03 */          \
  HANDLE_OPCODE (WASM_OP_IF),            /* 0x04 */          \
  HANDLE_OPCODE (WASM_OP_ELSE),          /* 0x05 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x06),   /* 0x06 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x07),   /* 0x07 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x08),   /* 0x08 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x09),   /* 0x09 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x0a),   /* 0x0a */          \
  HANDLE_OPCODE (WASM_OP_END),           /* 0x0b */          \
  HANDLE_OPCODE (WASM_OP_BR),            /* 0x0c */          \
  HANDLE_OPCODE (WASM_OP_BR_IF),         /* 0x0d */          \
  HANDLE_OPCODE (WASM_OP_BR_TABLE),      /* 0x0e */          \
  HANDLE_OPCODE (WASM_OP_RETURN),        /* 0x0f */          \
  HANDLE_OPCODE (WASM_OP_CALL),          /* 0x10 */          \
  HANDLE_OPCODE (WASM_OP_CALL_INDIRECT), /* 0x11 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x12),   /* 0x12 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x13),   /* 0x13 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x14),   /* 0x14 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x15),   /* 0x15 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x16),   /* 0x16 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x17),   /* 0x17 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x18),   /* 0x18 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x19),   /* 0x19 */          \
  HANDLE_OPCODE (WASM_OP_DROP),          /* 0x1a */          \
  HANDLE_OPCODE (WASM_OP_SELECT),        /* 0x1b */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x1c),   /* 0x1c */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x1d),   /* 0x1d */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x1e),   /* 0x1e */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x1f),   /* 0x1f */          \
  HANDLE_OPCODE (WASM_OP_GET_LOCAL),     /* 0x20 */          \
  HANDLE_OPCODE (WASM_OP_SET_LOCAL),     /* 0x21 */          \
  HANDLE_OPCODE (WASM_OP_TEE_LOCAL),     /* 0x22 */          \
  HANDLE_OPCODE (WASM_OP_GET_GLOBAL),    /* 0x23 */          \
  HANDLE_OPCODE (WASM_OP_SET_GLOBAL),    /* 0x24 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x25),   /* 0x25 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x26),   /* 0x26 */          \
  HANDLE_OPCODE (WASM_OP_UNUSED_0x27),   /* 0x27 */          \
  HANDLE_OPCODE (WASM_OP_I32_LOAD),      /* 0x28 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD),      /* 0x29 */          \
  HANDLE_OPCODE (WASM_OP_F32_LOAD),      /* 0x2a */          \
  HANDLE_OPCODE (WASM_OP_F64_LOAD),      /* 0x2b */          \
  HANDLE_OPCODE (WASM_OP_I32_LOAD8_S),   /* 0x2c */          \
  HANDLE_OPCODE (WASM_OP_I32_LOAD8_U),   /* 0x2d */          \
  HANDLE_OPCODE (WASM_OP_I32_LOAD16_S),  /* 0x2e */          \
  HANDLE_OPCODE (WASM_OP_I32_LOAD16_U),  /* 0x2f */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD8_S),   /* 0x30 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD8_U),   /* 0x31 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD16_S),  /* 0x32 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD16_U),  /* 0x33 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD32_S),  /* 0x34 */          \
  HANDLE_OPCODE (WASM_OP_I64_LOAD32_U),  /* 0x35 */          \
  HANDLE_OPCODE (WASM_OP_I32_STORE),     /* 0x36 */          \
  HANDLE_OPCODE (WASM_OP_I64_STORE),     /* 0x37 */          \
  HANDLE_OPCODE (WASM_OP_F32_STORE),     /* 0x38 */          \
  HANDLE_OPCODE (WASM_OP_F64_STORE),     /* 0x39 */          \
  HANDLE_OPCODE (WASM_OP_I32_STORE8),    /* 0x3a */          \
  HANDLE_OPCODE (WASM_OP_I32_STORE16),   /* 0x3b */          \
  HANDLE_OPCODE (WASM_OP_I64_STORE8),    /* 0x3c */          \
  HANDLE_OPCODE (WASM_OP_I64_STORE16),   /* 0x3d */          \
  HANDLE_OPCODE (WASM_OP_I64_STORE32),   /* 0x3e */          \
  HANDLE_OPCODE (WASM_OP_MEMORY_SIZE),   /* 0x3f */          \
  HANDLE_OPCODE (WASM_OP_MEMORY_GROW),   /* 0x40 */          \
  HANDLE_OPCODE (WASM_OP_I32_CONST),     /* 0x41 */          \
  HANDLE_OPCODE (WASM_OP_I64_CONST),     /* 0x42 */          \
  HANDLE_OPCODE (WASM_OP_F32_CONST),     /* 0x43 */          \
  HANDLE_OPCODE (WASM_OP_F64_CONST),     /* 0x44 */          \
  HANDLE_OPCODE (WASM_OP_I32_EQZ),       /* 0x45 */          \
  HANDLE_OPCODE (WASM_OP_I32_EQ),        /* 0x46 */          \
  HANDLE_OPCODE (WASM_OP_I32_NE),        /* 0x47 */          \
  HANDLE_OPCODE (WASM_OP_I32_LT_S),      /* 0x48 */          \
  HANDLE_OPCODE (WASM_OP_I32_LT_U),      /* 0x49 */          \
  HANDLE_OPCODE (WASM_OP_I32_GT_S),      /* 0x4a */          \
  HANDLE_OPCODE (WASM_OP_I32_GT_U),      /* 0x4b */          \
  HANDLE_OPCODE (WASM_OP_I32_LE_S),      /* 0x4c */          \
  HANDLE_OPCODE (WASM_OP_I32_LE_U),      /* 0x4d */          \
  HANDLE_OPCODE (WASM_OP_I32_GE_S),      /* 0x4e */          \
  HANDLE_OPCODE (WASM_OP_I32_GE_U),      /* 0x4f */          \
  HANDLE_OPCODE (WASM_OP_I64_EQZ),       /* 0x50 */          \
  HANDLE_OPCODE (WASM_OP_I64_EQ),        /* 0x51 */          \
  HANDLE_OPCODE (WASM_OP_I64_NE),        /* 0x52 */          \
  HANDLE_OPCODE (WASM_OP_I64_LT_S),      /* 0x53 */          \
  HANDLE_OPCODE (WASM_OP_I64_LT_U),      /* 0x54 */          \
  HANDLE_OPCODE (WASM_OP_I64_GT_S),      /* 0x55 */          \
  HANDLE_OPCODE (WASM_OP_I64_GT_U),      /* 0x56 */          \
  HANDLE_OPCODE (WASM_OP_I64_LE_S),      /* 0x57 */          \
  HANDLE_OPCODE (WASM_OP_I64_LE_U),      /* 0x58 */          \
  HANDLE_OPCODE (WASM_OP_I64_GE_S),      /* 0x59 */          \
  HANDLE_OPCODE (WASM_OP_I64_GE_U),      /* 0x5a */          \
  HANDLE_OPCODE (WASM_OP_F32_EQ),        /* 0x5b */          \
  HANDLE_OPCODE (WASM_OP_F32_NE),        /* 0x5c */          \
  HANDLE_OPCODE (WASM_OP_F32_LT),        /* 0x5d */          \
  HANDLE_OPCODE (WASM_OP_F32_GT),        /* 0x5e */          \
  HANDLE_OPCODE (WASM_OP_F32_LE),        /* 0x5f */          \
  HANDLE_OPCODE (WASM_OP_F32_GE),        /* 0x60 */          \
  HANDLE_OPCODE (WASM_OP_F64_EQ),        /* 0x61 */          \
  HANDLE_OPCODE (WASM_OP_F64_NE),        /* 0x62 */          \
  HANDLE_OPCODE (WASM_OP_F64_LT),        /* 0x63 */          \
  HANDLE_OPCODE (WASM_OP_F64_GT),        /* 0x64 */          \
  HANDLE_OPCODE (WASM_OP_F64_LE),        /* 0x65 */          \
  HANDLE_OPCODE (WASM_OP_F64_GE),        /* 0x66 */          \
  HANDLE_OPCODE (WASM_OP_I32_CLZ),       /* 0x67 */          \
  HANDLE_OPCODE (WASM_OP_I32_CTZ),       /* 0x68 */          \
  HANDLE_OPCODE (WASM_OP_I32_POPCNT),    /* 0x69 */          \
  HANDLE_OPCODE (WASM_OP_I32_ADD),       /* 0x6a */          \
  HANDLE_OPCODE (WASM_OP_I32_SUB),       /* 0x6b */          \
  HANDLE_OPCODE (WASM_OP_I32_MUL),       /* 0x6c */          \
  HANDLE_OPCODE (WASM_OP_I32_DIV_S),     /* 0x6d */          \
  HANDLE_OPCODE (WASM_OP_I32_DIV_U),     /* 0x6e */          \
  HANDLE_OPCODE (WASM_OP_I32_REM_S),     /* 0x6f */          \
  HANDLE_OPCODE (WASM_OP_I32_REM_U),     /* 0x70 */          \
  HANDLE_OPCODE (WASM_OP_I32_AND),       /* 0x71 */          \
  HANDLE_OPCODE (WASM_OP_I32_OR),        /* 0x72 */          \
  HANDLE_OPCODE (WASM_OP_I32_XOR),       /* 0x73 */          \
  HANDLE_OPCODE (WASM_OP_I32_SHL),       /* 0x74 */          \
  HANDLE_OPCODE (WASM_OP_I32_SHR_S),     /* 0x75 */          \
  HANDLE_OPCODE (WASM_OP_I32_SHR_U),     /* 0x76 */          \
  HANDLE_OPCODE (WASM_OP_I32_ROTL),      /* 0x77 */          \
  HANDLE_OPCODE (WASM_OP_I32_ROTR),      /* 0x78 */          \
  HANDLE_OPCODE (WASM_OP_I64_CLZ),       /* 0x79 */          \
  HANDLE_OPCODE (WASM_OP_I64_CTZ),       /* 0x7a */          \
  HANDLE_OPCODE (WASM_OP_I64_POPCNT),    /* 0x7b */          \
  HANDLE_OPCODE (WASM_OP_I64_ADD),       /* 0x7c */          \
  HANDLE_OPCODE (WASM_OP_I64_SUB),       /* 0x7d */          \
  HANDLE_OPCODE (WASM_OP_I64_MUL),       /* 0x7e */          \
  HANDLE_OPCODE (WASM_OP_I64_DIV_S),     /* 0x7f */          \
  HANDLE_OPCODE (WASM_OP_I64_DIV_U),     /* 0x80 */          \
  HANDLE_OPCODE (WASM_OP_I64_REM_S),     /* 0x81 */          \
  HANDLE_OPCODE (WASM_OP_I64_REM_U),     /* 0x82 */          \
  HANDLE_OPCODE (WASM_OP_I64_AND),       /* 0x83 */          \
  HANDLE_OPCODE (WASM_OP_I64_OR),        /* 0x84 */          \
  HANDLE_OPCODE (WASM_OP_I64_XOR),       /* 0x85 */          \
  HANDLE_OPCODE (WASM_OP_I64_SHL),       /* 0x86 */          \
  HANDLE_OPCODE (WASM_OP_I64_SHR_S),     /* 0x87 */          \
  HANDLE_OPCODE (WASM_OP_I64_SHR_U),     /* 0x88 */          \
  HANDLE_OPCODE (WASM_OP_I64_ROTL),      /* 0x89 */          \
  HANDLE_OPCODE (WASM_OP_I64_ROTR),      /* 0x8a */          \
  HANDLE_OPCODE (WASM_OP_F32_ABS),       /* 0x8b */          \
  HANDLE_OPCODE (WASM_OP_F32_NEG),       /* 0x8c */          \
  HANDLE_OPCODE (WASM_OP_F32_CEIL),      /* 0x8d */          \
  HANDLE_OPCODE (WASM_OP_F32_FLOOR),     /* 0x8e */          \
  HANDLE_OPCODE (WASM_OP_F32_TRUNC),     /* 0x8f */          \
  HANDLE_OPCODE (WASM_OP_F32_NEAREST),   /* 0x90 */          \
  HANDLE_OPCODE (WASM_OP_F32_SQRT),      /* 0x91 */          \
  HANDLE_OPCODE (WASM_OP_F32_ADD),       /* 0x92 */          \
  HANDLE_OPCODE (WASM_OP_F32_SUB),       /* 0x93 */          \
  HANDLE_OPCODE (WASM_OP_F32_MUL),       /* 0x94 */          \
  HANDLE_OPCODE (WASM_OP_F32_DIV),       /* 0x95 */          \
  HANDLE_OPCODE (WASM_OP_F32_MIN),       /* 0x96 */          \
  HANDLE_OPCODE (WASM_OP_F32_MAX),       /* 0x97 */          \
  HANDLE_OPCODE (WASM_OP_F32_COPYSIGN),  /* 0x98 */          \
  HANDLE_OPCODE (WASM_OP_F64_ABS),       /* 0x99 */          \
  HANDLE_OPCODE (WASM_OP_F64_NEG),       /* 0x9a */          \
  HANDLE_OPCODE (WASM_OP_F64_CEIL),      /* 0x9b */          \
  HANDLE_OPCODE (WASM_OP_F64_FLOOR),     /* 0x9c */          \
  HANDLE_OPCODE (WASM_OP_F64_TRUNC),     /* 0x9d */          \
  HANDLE_OPCODE (WASM_OP_F64_NEAREST),   /* 0x9e */          \
  HANDLE_OPCODE (WASM_OP_F64_SQRT),      /* 0x9f */          \
  HANDLE_OPCODE (WASM_OP_F64_ADD),       /* 0xa0 */          \
  HANDLE_OPCODE (WASM_OP_F64_SUB),       /* 0xa1 */          \
  HANDLE_OPCODE (WASM_OP_F64_MUL),       /* 0xa2 */          \
  HANDLE_OPCODE (WASM_OP_F64_DIV),       /* 0xa3 */          \
  HANDLE_OPCODE (WASM_OP_F64_MIN),       /* 0xa4 */          \
  HANDLE_OPCODE (WASM_OP_F64_MAX),       /* 0xa5 */          \
  HANDLE_OPCODE (WASM_OP_F64_COPYSIGN),  /* 0xa6 */          \
  HANDLE_OPCODE (WASM_OP_I32_WRAP_I64),      /* 0xa7 */      \
  HANDLE_OPCODE (WASM_OP_I32_TRUNC_S_F32),   /* 0xa8 */      \
  HANDLE_OPCODE (WASM_OP_I32_TRUNC_U_F32),   /* 0xa9 */      \
  HANDLE_OPCODE (WASM_OP_I32_TRUNC_S_F64),   /* 0xaa */      \
  HANDLE_OPCODE (WASM_OP_I32_TRUNC_U_F64),   /* 0xab */      \
  HANDLE_OPCODE (WASM_OP_I64_EXTEND_S_I32),  /* 0xac */      \
  HANDLE_OPCODE (WASM_OP_I64_EXTEND_U_I32),  /* 0xad */      \
  HANDLE_OPCODE (WASM_OP_I64_TRUNC_S_F32),   /* 0xae */      \
  HANDLE_OPCODE (WASM_OP_I64_TRUNC_U_F32),   /* 0xaf */      \
  HANDLE_OPCODE (WASM_OP_I64_TRUNC_S_F64),   /* 0xb0 */      \
  HANDLE_OPCODE (WASM_OP_I64_TRUNC_U_F64),   /* 0xb1 */      \
  HANDLE_OPCODE (WASM_OP_F32_CONVERT_S_I32), /* 0xb2 */      \
  HANDLE_OPCODE (WASM_OP_F32_CONVERT_U_I32), /* 0xb3 */      \
  HANDLE_OPCODE (WASM_OP_F32_CONVERT_S_I64), /* 0xb4 */      \
  HANDLE_OPCODE (WASM_OP_F32_CONVERT_U_I64), /* 0xb5 */      \
  HANDLE_OPCODE (WASM_OP_F32_DEMOTE_F64),    /* 0xb6 */      \
  HANDLE_OPCODE (WASM_OP_F64_CONVERT_S_I32), /* 0xb7 */      \
  HANDLE_OPCODE (WASM_OP_F64_CONVERT_U_I32), /* 0xb8 */      \
  HANDLE_OPCODE (WASM_OP_F64_CONVERT_S_I64), /* 0xb9 */      \
  HANDLE_OPCODE (WASM_OP_F64_CONVERT_U_I64), /* 0xba */      \
  HANDLE_OPCODE (WASM_OP_F64_PROMOTE_F32),   /* 0xbb */      \
  HANDLE_OPCODE (WASM_OP_I32_REINTERPRET_F32),   /* 0xbc */  \
  HANDLE_OPCODE (WASM_OP_I64_REINTERPRET_F64),   /* 0xbd */  \
  HANDLE_OPCODE (WASM_OP_F32_REINTERPRET_I32),   /* 0xbe */  \
  HANDLE_OPCODE (WASM_OP_F64_REINTERPRET_I64),   /* 0xbf */  \
  HANDLE_OPCODE (WASM_OP_DROP_64),           /* 0xc0 */      \
  HANDLE_OPCODE (WASM_OP_SELECT_64),         /* 0xc1 */      \
  HANDLE_OPCODE (EXT_OP_GET_LOCAL_FAST),     /* 0xc2 */      \
  HANDLE_OPCODE (EXT_OP_SET_LOCAL_FAST_I64), /* 0xc3 */      \
  HANDLE_OPCODE (EXT_OP_SET_LOCAL_FAST),     /* 0xc4 */      \
  HANDLE_OPCODE (EXT_OP_TEE_LOCAL_FAST),     /* 0xc5 */      \
  HANDLE_OPCODE (EXT_OP_TEE_LOCAL_FAST_I64), /* 0xc6 */      \
  HANDLE_OPCODE (EXT_OP_COPY_STACK_TOP),     /* 0xc7 */      \
  HANDLE_OPCODE (EXT_OP_COPY_STACK_TOP_I64), /* 0xc8 */      \
  HANDLE_OPCODE (WASM_OP_IMPDEP),            /* 0xc9 */      \
}

#endif /* end of _WASM_OPCODE_H */
