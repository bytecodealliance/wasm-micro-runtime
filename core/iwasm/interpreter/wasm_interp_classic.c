/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_interp.h"
#include "bh_log.h"
#include "wasm_runtime.h"
#include "wasm_opcode.h"
#include "wasm_loader.h"
#include "../common/wasm_exec_env.h"

typedef int32 CellType_I32;
typedef int64 CellType_I64;
typedef float32 CellType_F32;
typedef float64 CellType_F64;

#define BR_TABLE_TMP_BUF_LEN 32

/* 64-bit Memory accessors. */
#if WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0
#define PUT_I64_TO_ADDR(addr, value) do {       \
    *(int64*)(addr) = (int64)(value);           \
  } while (0)
#define PUT_F64_TO_ADDR(addr, value) do {       \
    *(float64*)(addr) = (float64)(value);       \
  } while (0)

#define GET_I64_FROM_ADDR(addr) (*(int64*)(addr))
#define GET_F64_FROM_ADDR(addr) (*(float64*)(addr))

/* For STORE opcodes */
#define STORE_I64 PUT_I64_TO_ADDR
#define STORE_U32(addr, value) do {             \
    *(uint32*)(addr) = (uint32)(value);         \
  } while (0)
#define STORE_U16(addr, value) do {             \
    *(uint16*)(addr) = (uint16)(value);         \
  } while (0)

/* For LOAD opcodes */
#define LOAD_I64(addr) (*(int64*)(addr))
#define LOAD_F64(addr) (*(float64*)(addr))
#define LOAD_I32(addr) (*(int32*)(addr))
#define LOAD_U32(addr) (*(uint32*)(addr))
#define LOAD_I16(addr) (*(int16*)(addr))
#define LOAD_U16(addr) (*(uint16*)(addr))

#else  /* WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0 */
#define PUT_I64_TO_ADDR(addr, value) do {       \
    union { int64 val; uint32 parts[2]; } u;    \
    u.val = (int64)(value);                     \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)
#define PUT_F64_TO_ADDR(addr, value) do {       \
    union { float64 val; uint32 parts[2]; } u;  \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

static inline int64
GET_I64_FROM_ADDR(uint32 *addr)
{
    union { int64 val; uint32 parts[2]; } u;
    u.parts[0] = addr[0];
    u.parts[1] = addr[1];
    return u.val;
}

static inline float64
GET_F64_FROM_ADDR (uint32 *addr)
{
    union { float64 val; uint32 parts[2]; } u;
    u.parts[0] = addr[0];
    u.parts[1] = addr[1];
    return u.val;
}

/* For STORE opcodes */
#define STORE_I64(addr, value) do {             \
    uintptr_t addr1 = (uintptr_t)(addr);        \
    union { int64 val; uint32 u32[2];           \
            uint16 u16[4]; uint8 u8[8]; } u;    \
    if ((addr1 & (uintptr_t)7) == 0)            \
      *(int64*)(addr) = (int64)(value);         \
    else {                                      \
        u.val = (int64)(value);                 \
        if ((addr1 & (uintptr_t)3) == 0) {      \
            ((uint32*)(addr))[0] = u.u32[0];    \
            ((uint32*)(addr))[1] = u.u32[1];    \
        }                                       \
        else if ((addr1 & (uintptr_t)1) == 0) { \
            ((uint16*)(addr))[0] = u.u16[0];    \
            ((uint16*)(addr))[1] = u.u16[1];    \
            ((uint16*)(addr))[2] = u.u16[2];    \
            ((uint16*)(addr))[3] = u.u16[3];    \
        }                                       \
        else {                                  \
            int32 t;                            \
            for (t = 0; t < 8; t++)             \
                ((uint8*)(addr))[t] = u.u8[t];  \
        }                                       \
    }                                           \
  } while (0)

#define STORE_U32(addr, value) do {             \
    uintptr_t addr1 = (uintptr_t)(addr);        \
    union { uint32 val;                         \
            uint16 u16[2]; uint8 u8[4]; } u;    \
    if ((addr1 & (uintptr_t)3) == 0)            \
      *(uint32*)(addr) = (uint32)(value);       \
    else {                                      \
        u.val = (uint32)(value);                \
        if ((addr1 & (uintptr_t)1) == 0) {      \
            ((uint16*)(addr))[0] = u.u16[0];    \
            ((uint16*)(addr))[1] = u.u16[1];    \
        }                                       \
        else {                                  \
            ((uint8*)(addr))[0] = u.u8[0];      \
            ((uint8*)(addr))[1] = u.u8[1];      \
            ((uint8*)(addr))[2] = u.u8[2];      \
            ((uint8*)(addr))[3] = u.u8[3];      \
        }                                       \
    }                                           \
  } while (0)

#define STORE_U16(addr, value) do {             \
    union { uint16 val; uint8 u8[2]; } u;       \
    u.val = (uint16)(value);                    \
    ((uint8*)(addr))[0] = u.u8[0];              \
    ((uint8*)(addr))[1] = u.u8[1];              \
  } while (0)

/* For LOAD opcodes */
static inline int64
LOAD_I64(void *addr)
{
    uintptr_t addr1 = (uintptr_t)addr;
    union { int64 val; uint32 u32[2];
            uint16 u16[4]; uint8 u8[8]; } u;
    if ((addr1 & (uintptr_t)7) == 0)
        return *(int64*)addr;

    if ((addr1 & (uintptr_t)3) == 0) {
        u.u32[0] = ((uint32*)addr)[0];
        u.u32[1] = ((uint32*)addr)[1];
    }
    else if ((addr1 & (uintptr_t)1) == 0) {
        u.u16[0] = ((uint16*)addr)[0];
        u.u16[1] = ((uint16*)addr)[1];
        u.u16[2] = ((uint16*)addr)[2];
        u.u16[3] = ((uint16*)addr)[3];
    }
    else {
        int32 t;
        for (t = 0; t < 8; t++)
            u.u8[t] = ((uint8*)addr)[t];
    }
    return u.val;
}

static inline float64
LOAD_F64(void *addr)
{
    uintptr_t addr1 = (uintptr_t)addr;
    union { float64 val; uint32 u32[2];
            uint16 u16[4]; uint8 u8[8]; } u;
    if ((addr1 & (uintptr_t)7) == 0)
        return *(float64*)addr;

    if ((addr1 & (uintptr_t)3) == 0) {
        u.u32[0] = ((uint32*)addr)[0];
        u.u32[1] = ((uint32*)addr)[1];
    }
    else if ((addr1 & (uintptr_t)1) == 0) {
        u.u16[0] = ((uint16*)addr)[0];
        u.u16[1] = ((uint16*)addr)[1];
        u.u16[2] = ((uint16*)addr)[2];
        u.u16[3] = ((uint16*)addr)[3];
    }
    else {
        int32 t;
        for (t = 0; t < 8; t++)
            u.u8[t] = ((uint8*)addr)[t];
    }
    return u.val;
}

static inline int32
LOAD_I32(void *addr)
{
    uintptr_t addr1 = (uintptr_t)addr;
    union { int32 val; uint16 u16[2]; uint8 u8[4]; } u;
    if ((addr1 & (uintptr_t)3) == 0)
        return *(int32*)addr;

    if ((addr1 & (uintptr_t)1) == 0) {
        u.u16[0] = ((uint16*)addr)[0];
        u.u16[1] = ((uint16*)addr)[1];
    }
    else {
        u.u8[0] = ((uint8*)addr)[0];
        u.u8[1] = ((uint8*)addr)[1];
        u.u8[2] = ((uint8*)addr)[2];
        u.u8[3] = ((uint8*)addr)[3];
    }
    return u.val;
}

static inline int16
LOAD_I16(void *addr)
{
    union { int16 val; uint8 u8[2]; } u;
    u.u8[0] = ((uint8*)addr)[0];
    u.u8[1] = ((uint8*)addr)[1];
    return u.val;
}

#define LOAD_U32(addr) ((uint32)LOAD_I32(addr))
#define LOAD_U16(addr) ((uint16)LOAD_I16(addr))

#endif  /* WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0 */

#define CHECK_MEMORY_OVERFLOW(bytes) do {                                   \
    int32 offset1 = (int32)(offset + addr);                                 \
    uint64 offset2 = (uint64)(uint32)(offset1 - heap_base_offset);          \
    /* if (flags != 2)                                                      \
      LOG_VERBOSE("unaligned load/store, flag: %d.\n", flags); */           \
    if (offset2 + LOAD_SIZE[opcode - WASM_OP_I32_LOAD] <= total_mem_size)   \
      /* If offset1 is in valid range, maddr must also be in valid range,   \
         no need to check it again. */                                      \
      maddr = memory->memory_data + offset1;                                \
    else                                                                    \
      goto out_of_bounds;                                                   \
  } while (0)

static inline uint32
rotl32(uint32 n, uint32 c)
{
    const uint32 mask = (31);
    c = c % 32;
    c &= mask;
    return (n<<c) | (n>>( (-c)&mask ));
}

static inline uint32
rotr32(uint32 n, uint32 c)
{
    const uint32 mask = (31);
    c = c % 32;
    c &= mask;
    return (n>>c) | (n<<( (-c)&mask ));
}

static inline uint64
rotl64(uint64 n, uint64 c)
{
    const uint64 mask = (63);
    c = c % 64;
    c &= mask;
    return (n<<c) | (n>>( (-c)&mask ));
}

static inline uint64
rotr64(uint64 n, uint64 c)
{
    const uint64 mask = (63);
    c = c % 64;
    c &= mask;
    return (n>>c) | (n<<( (-c)&mask ));
}

static inline double
wa_fmax(double a, double b)
{
    double c = fmax(a, b);
    if (c==0 && a==b)
        return signbit(a) ? b : a;
    return c;
}

static inline double
wa_fmin(double a, double b)
{
    double c = fmin(a, b);
    if (c==0 && a==b)
        return signbit(a) ? a : b;
    return c;
}

static inline uint32
clz32(uint32 type)
{
    uint32 num = 0;
    if (type == 0)
        return 32;
    while (!(type & 0x80000000)) {
        num++;
        type <<= 1;
    }
    return num;
}

static inline uint32
clz64(uint64 type)
{
    uint32 num = 0;
    if (type == 0)
        return 64;
    while (!(type & 0x8000000000000000LL)) {
        num++;
        type <<= 1;
    }
    return num;
}

static inline uint32
ctz32(uint32 type)
{
    uint32 num = 0;
    if (type == 0)
        return 32;
    while (!(type & 1)) {
        num++;
        type >>= 1;
    }
    return num;
}

static inline uint32
ctz64(uint64 type)
{
    uint32 num = 0;
    if (type == 0)
        return 64;
    while (!(type & 1)) {
        num++;
        type >>= 1;
    }
    return num;
}

static inline uint32
popcount32(uint32 u)
{
    uint32 ret = 0;
    while (u) {
        u = (u & (u - 1));
        ret++;
    }
    return ret;
}

static inline uint32
popcount64(uint64 u)
{
    uint32 ret = 0;
    while (u) {
        u = (u & (u - 1));
        ret++;
    }
    return ret;
}

static uint64
read_leb(const uint8 *buf, uint32 *p_offset, uint32 maxbits, bool sign)
{
    uint64 result = 0;
    uint32 shift = 0;
    uint32 bcnt = 0;
    uint64 byte;

    while (true) {
        byte = buf[*p_offset];
        *p_offset += 1;
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
        bcnt += 1;
    }
    if (sign && (shift < maxbits) && (byte & 0x40)) {
        /* Sign extend */
        result |= - ((uint64)1 << shift);
    }
    return result;
}

#define PUSH_I32(value) do {                    \
    *(int32*)frame_sp++ = (int32)(value);       \
  } while (0)

#define PUSH_F32(value) do {                    \
    *(float32*)frame_sp++ = (float32)(value);   \
  } while (0)

#define PUSH_I64(value) do {                    \
    PUT_I64_TO_ADDR(frame_sp, value);           \
    frame_sp += 2;                              \
  } while (0)

#define PUSH_F64(value) do {                    \
    PUT_F64_TO_ADDR(frame_sp, value);           \
    frame_sp += 2;                              \
  } while (0)

#define PUSH_CSP(type, ret_type, _target_addr) do {     \
    bh_assert(frame_csp < frame->csp_boundary);         \
    frame_csp->block_type = type;                       \
    frame_csp->return_type = ret_type;                  \
    frame_csp->target_addr = _target_addr;              \
    frame_csp->frame_sp = frame_sp;                     \
    frame_csp++;                                        \
  } while (0)

#define POP_I32() (--frame_sp, *(int32*)frame_sp)

#define POP_F32() (--frame_sp, *(float32*)frame_sp)

#define POP_I64() (frame_sp -= 2, GET_I64_FROM_ADDR(frame_sp))

#define POP_F64() (frame_sp -= 2, GET_F64_FROM_ADDR(frame_sp))

#define POP_CSP_CHECK_OVERFLOW(n) do {                          \
    bh_assert(frame_csp - n >= frame->csp_bottom);              \
  } while (0)

#define POP_CSP() do {                                          \
    POP_CSP_CHECK_OVERFLOW(1);                                  \
    --frame_csp;                                                \
  } while (0)

#define POP_CSP_N(n) do {                                       \
    uint32 *frame_sp_old = frame_sp;                            \
    POP_CSP_CHECK_OVERFLOW(n + 1);                              \
    frame_csp -= n;                                             \
    frame_ip = (frame_csp - 1)->target_addr;                    \
    /* copy return value of block */                            \
    frame_sp = (frame_csp - 1)->frame_sp;                       \
    switch ((frame_csp - 1)->return_type) {                     \
      case VALUE_TYPE_I32:                                      \
      case VALUE_TYPE_F32:                                      \
        PUSH_I32(*(frame_sp_old - 1));                          \
        break;                                                  \
      case VALUE_TYPE_I64:                                      \
      case VALUE_TYPE_F64:                                      \
        PUSH_I64(GET_I64_FROM_ADDR(frame_sp_old - 2));          \
        break;                                                  \
    }                                                           \
  } while (0)

/* Pop the given number of elements from the given frame's stack.  */
#define POP(N) do {                             \
    int n = (N);                                \
    frame_sp -= n;                              \
  } while (0)

#define SYNC_ALL_TO_FRAME() do {                \
    frame->sp = frame_sp;                       \
    frame->ip = frame_ip;                       \
    frame->csp = frame_csp;                     \
  } while (0)

#define UPDATE_ALL_FROM_FRAME() do {            \
    frame_sp = frame->sp;                       \
    frame_ip = frame->ip;                       \
    frame_csp = frame->csp;                     \
  } while (0)

#define read_leb_int64(p, p_end, res) do {      \
  uint8 _val = *p;                              \
  if (!(_val & 0x80)) {                         \
    res = (int64)_val;                          \
    if (_val & 0x40)                            \
      /* sign extend */                         \
      res |= 0xFFFFFFFFFFFFFF80LL;              \
    p++;                                        \
    break;                                      \
  }                                             \
  uint32 _off = 0;                              \
  res = (int64)read_leb(p, &_off, 64, true);    \
  p += _off;                                    \
} while (0)

#define read_leb_uint32(p, p_end, res) do {     \
  uint8 _val = *p;                              \
  if (!(_val & 0x80)) {                         \
    res = _val;                                 \
    p++;                                        \
    break;                                      \
  }                                             \
  uint32 _off = 0;                              \
  res = (uint32)read_leb(p, &_off, 32, false);  \
  p += _off;                                    \
} while (0)

#define read_leb_int32(p, p_end, res) do {      \
  uint8 _val = *p;                              \
  if (!(_val & 0x80)) {                         \
    res = (int32)_val;                          \
    if (_val & 0x40)                            \
      /* sign extend */                         \
      res |= 0xFFFFFF80;                        \
    p++;                                        \
    break;                                      \
  }                                             \
  uint32 _off = 0;                              \
  res = (int32)read_leb(p, &_off, 32, true);    \
  p += _off;                                    \
} while (0)

#if WASM_ENABLE_LABELS_AS_VALUES == 0
#define RECOVER_FRAME_IP_END() \
    frame_ip_end = wasm_get_func_code_end(cur_func)
#else
#define RECOVER_FRAME_IP_END() (void)0
#endif

#define RECOVER_CONTEXT(new_frame) do {                             \
    frame = (new_frame);                                            \
    cur_func = frame->function;                                     \
    prev_frame = frame->prev_frame;                                 \
    frame_ip = frame->ip;                                           \
    RECOVER_FRAME_IP_END();                                         \
    frame_lp = frame->lp;                                           \
    frame_sp = frame->sp;                                           \
    frame_csp = frame->csp;                                         \
  } while (0)

#if WASM_ENABLE_LABELS_AS_VALUES != 0
#define GET_OPCODE() opcode = *(frame_ip - 1);
#else
#define GET_OPCODE() (void)0
#endif

#define DEF_OP_I_CONST(ctype, src_op_type) do {                     \
    ctype cval;                                                     \
    read_leb_##ctype(frame_ip, frame_ip_end, cval);                 \
    PUSH_##src_op_type(cval);                                       \
  } while (0)

#define DEF_OP_EQZ(src_op_type) do {                                \
    int32 val;                                                      \
    val = POP_##src_op_type() == 0;                                 \
    PUSH_I32(val);                                                  \
  } while (0)

#define DEF_OP_CMP(src_type, src_op_type, cond) do {                \
    uint32 res;                                                     \
    src_type val1, val2;                                            \
    val2 = (src_type)POP_##src_op_type();                           \
    val1 = (src_type)POP_##src_op_type();                           \
    res = val1 cond val2;                                           \
    PUSH_I32(res);                                                  \
  } while (0)

#define DEF_OP_BIT_COUNT(src_type, src_op_type, operation) do {     \
    src_type val1, val2;                                            \
    val1 = (src_type)POP_##src_op_type();                           \
    val2 = (src_type)operation(val1);                               \
    PUSH_##src_op_type(val2);                                       \
  } while (0)

#define DEF_OP_NUMERIC(src_type1, src_type2, src_op_type, operation) do {   \
    frame_sp -= sizeof(src_type2)/sizeof(uint32);                           \
    *(src_type1*)(frame_sp - sizeof(src_type1)/sizeof(uint32)) operation##= \
    *(src_type2*)(frame_sp);                                                \
  } while (0)

#if WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0
#define DEF_OP_NUMERIC_64 DEF_OP_NUMERIC
#else
#define DEF_OP_NUMERIC_64(src_type1, src_type2, src_op_type, operation) do {\
    src_type1 val1;                                                         \
    src_type2 val2;                                                         \
    frame_sp -= 2;                                                          \
    val1 = (src_type1)GET_##src_op_type##_FROM_ADDR(frame_sp - 2);          \
    val2 = (src_type2)GET_##src_op_type##_FROM_ADDR(frame_sp);              \
    val1 operation##= val2;                                                 \
    PUT_##src_op_type##_TO_ADDR(frame_sp - 2, val1);                        \
  } while (0)
#endif

#define DEF_OP_NUMERIC2(src_type1, src_type2, src_op_type, operation) do {  \
    frame_sp -= sizeof(src_type2)/sizeof(uint32);                           \
    *(src_type1*)(frame_sp - sizeof(src_type1)/sizeof(uint32)) operation##= \
    (*(src_type2*)(frame_sp) % 32);                                         \
  } while (0)

#define DEF_OP_NUMERIC2_64(src_type1, src_type2, src_op_type, operation) do { \
    src_type1 val1;                                                           \
    src_type2 val2;                                                           \
    frame_sp -= 2;                                                            \
    val1 = (src_type1)GET_##src_op_type##_FROM_ADDR(frame_sp - 2);            \
    val2 = (src_type2)GET_##src_op_type##_FROM_ADDR(frame_sp);                \
    val1 operation##= (val2 % 64);                                            \
    PUT_##src_op_type##_TO_ADDR(frame_sp - 2, val1);                          \
  } while (0)

#define DEF_OP_MATH(src_type, src_op_type, method) do {             \
    src_type val;                                                   \
    val = POP_##src_op_type();                                      \
    PUSH_##src_op_type(method(val));                                \
  } while (0)

#define DEF_OP_TRUNC(dst_type, dst_op_type, src_type, src_op_type,  \
                     min_cond, max_cond) do {                       \
    src_type value = POP_##src_op_type();                           \
    if (isnan(value)) {                                             \
      wasm_set_exception(module, "invalid conversion to integer");  \
      goto got_exception;                                           \
    }                                                               \
    else if (value min_cond || value max_cond) {                    \
      wasm_set_exception(module, "integer overflow");               \
      goto got_exception;                                           \
    }                                                               \
    PUSH_##dst_op_type(((dst_type)value));                          \
  } while (0)

#define DEF_OP_CONVERT(dst_type, dst_op_type,                       \
                       src_type, src_op_type) do {                  \
    dst_type value = (dst_type)(src_type)POP_##src_op_type();       \
    PUSH_##dst_op_type(value);                                      \
  } while (0)

#define GET_LOCAL_INDEX_TYPE_AND_OFFSET() do {                      \
    uint32 param_count = cur_func->param_count;                     \
    read_leb_uint32(frame_ip, frame_ip_end, local_idx);             \
    bh_assert(local_idx < param_count + cur_func->local_count);     \
    local_offset = cur_func->local_offsets[local_idx];              \
    if (local_idx < param_count)                                    \
      local_type = cur_func->param_types[local_idx];                \
    else                                                            \
      local_type = cur_func->local_types[local_idx - param_count];  \
  } while (0)

static inline int32
sign_ext_8_32(int8 val)
{
    if (val & 0x80)
        return (int32)val | (int32)0xffffff00;
    return val;
}

static inline int32
sign_ext_16_32(int16 val)
{
    if (val & 0x8000)
        return (int32)val | (int32)0xffff0000;
    return val;
}

static inline int64
sign_ext_8_64(int8 val)
{
    if (val & 0x80)
        return (int64)val | (int64)0xffffffffffffff00;
    return val;
}

static inline int64
sign_ext_16_64(int16 val)
{
    if (val & 0x8000)
        return (int64)val | (int64)0xffffffffffff0000;
    return val;
}

static inline int64
sign_ext_32_64(int32 val)
{
    if (val & (int32)0x80000000)
        return (int64)val | (int64)0xffffffff00000000;
    return val;
}

static inline void
word_copy(uint32 *dest, uint32 *src, unsigned num)
{
    for (; num > 0; num--)
        *dest++ = *src++;
}

static inline WASMInterpFrame*
ALLOC_FRAME(WASMExecEnv *exec_env, uint32 size, WASMInterpFrame *prev_frame)
{
    WASMInterpFrame *frame = wasm_exec_env_alloc_wasm_frame(exec_env, size);

    if (frame)
        frame->prev_frame = prev_frame;
    else {
        wasm_set_exception((WASMModuleInstance*)exec_env->module_inst,
                           "WASM interp failed: stack overflow.");
    }

    return frame;
}

static inline void
FREE_FRAME(WASMExecEnv *exec_env, WASMInterpFrame *frame)
{
    wasm_exec_env_free_wasm_frame(exec_env, frame);
}

static void
wasm_interp_call_func_native(WASMModuleInstance *module_inst,
                             WASMExecEnv *exec_env,
                             WASMFunctionInstance *cur_func,
                             WASMInterpFrame *prev_frame)
{
    WASMFunctionImport *func_import = cur_func->u.func_import;
    unsigned local_cell_num = 2;
    WASMInterpFrame *frame;
    uint32 argv_ret[2];
    char buf[128];
    bool ret;

    if (!(frame = ALLOC_FRAME(exec_env,
                              wasm_interp_interp_frame_size(local_cell_num),
                              prev_frame)))
        return;

    frame->function = cur_func;
    frame->ip = NULL;
    frame->sp = frame->lp + local_cell_num;

    wasm_exec_env_set_cur_frame(exec_env, frame);

    if (!func_import->func_ptr_linked) {
        snprintf(buf, sizeof(buf),
                 "fail to call unlinked import function (%s, %s)",
                 func_import->module_name, func_import->field_name);
        wasm_set_exception(module_inst, buf);
        return;
    }

    if (!func_import->call_conv_raw) {
        ret = wasm_runtime_invoke_native(exec_env, func_import->func_ptr_linked,
                                         func_import->func_type, func_import->signature,
                                         func_import->attachment,
                                         frame->lp, cur_func->param_cell_num, argv_ret);
    }
    else {
        ret = wasm_runtime_invoke_native_raw(exec_env, func_import->func_ptr_linked,
                                             func_import->func_type, func_import->signature,
                                             func_import->attachment,
                                             frame->lp, cur_func->param_cell_num, argv_ret);
    }

    if (!ret)
        return;

    if (cur_func->ret_cell_num == 1) {
        prev_frame->sp[0] = argv_ret[0];
        prev_frame->sp++;
    }
    else if (cur_func->ret_cell_num == 2) {
        prev_frame->sp[0] = argv_ret[0];
        prev_frame->sp[1] = argv_ret[1];
        prev_frame->sp += 2;
    }

    FREE_FRAME(exec_env, frame);
    wasm_exec_env_set_cur_frame(exec_env, prev_frame);
}

#if WASM_ENABLE_LABELS_AS_VALUES != 0

#define HANDLE_OP(opcode) HANDLE_##opcode
#define FETCH_OPCODE_AND_DISPATCH() goto *handle_table[*frame_ip++]
#define HANDLE_OP_END() FETCH_OPCODE_AND_DISPATCH()

#else   /* else of WASM_ENABLE_LABELS_AS_VALUES */

#define HANDLE_OP(opcode) case opcode
#define HANDLE_OP_END() continue

#endif  /* end of WASM_ENABLE_LABELS_AS_VALUES */

static void
wasm_interp_call_func_bytecode(WASMModuleInstance *module,
                               WASMExecEnv *exec_env,
                               WASMFunctionInstance *cur_func,
                               WASMInterpFrame *prev_frame)
{
  WASMMemoryInstance *memory = module->default_memory;
  int32 heap_base_offset = memory ? memory->heap_base_offset : 0;
  uint32 num_bytes_per_page = memory ? memory->num_bytes_per_page : 0;
  uint32 total_mem_size = memory ? num_bytes_per_page * memory->cur_page_count
                                   - heap_base_offset : 0;
  uint8 *global_data = memory ? memory->global_data : NULL;
  WASMTableInstance *table = module->default_table;
  WASMGlobalInstance *globals = module->globals;
  uint8 opcode_IMPDEP = WASM_OP_IMPDEP;
  WASMInterpFrame *frame = NULL;
  /* Points to this special opcode so as to jump to the call_method_from_entry.  */
  register uint8  *frame_ip = &opcode_IMPDEP; /* cache of frame->ip */
  register uint32 *frame_lp = NULL;  /* cache of frame->lp */
  register uint32 *frame_sp = NULL;  /* cache of frame->sp */
  WASMBranchBlock *frame_csp = NULL;
  WASMGlobalInstance *global;
  BlockAddr *cache_items;
  uint8 *frame_ip_end = frame_ip + 1;
  uint8 opcode, block_ret_type;
  uint32 *depths = NULL;
  uint32 depth_buf[BR_TABLE_TMP_BUF_LEN];
  uint32 i, depth, cond, count, fidx, tidx, frame_size = 0;
  uint64 all_cell_num = 0;
  int32 didx, val;
  uint8 *else_addr, *end_addr, *maddr = NULL;
  uint32 local_idx, local_offset, global_idx;
  uint8 local_type, *global_addr;
  uint32 cache_index;

#if WASM_ENABLE_LABELS_AS_VALUES != 0
  #define HANDLE_OPCODE(op) &&HANDLE_##op
  DEFINE_GOTO_TABLE (const void *, handle_table);
  #undef HANDLE_OPCODE
#endif

  /* Size of memory load.
     This starts with the first memory load operator at opcode 0x28 */
  uint32 LOAD_SIZE[] = {
    4, 8, 4, 8, 1, 1, 2, 2, 1, 1, 2, 2, 4, 4,   /* loads */
    4, 8, 4, 8, 1, 2, 1, 2, 4 };                /* stores */

#if WASM_ENABLE_LABELS_AS_VALUES == 0
  while (frame_ip < frame_ip_end) {
    opcode = *frame_ip++;
    switch (opcode) {
#else
      FETCH_OPCODE_AND_DISPATCH ();
#endif
      /* control instructions */
      HANDLE_OP (WASM_OP_UNREACHABLE):
        wasm_set_exception(module, "unreachable");
        goto got_exception;

      HANDLE_OP (WASM_OP_NOP):
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BLOCK):
        block_ret_type = *frame_ip++;

        cache_index = ((uintptr_t)frame_ip) & (uintptr_t)(BLOCK_ADDR_CACHE_SIZE - 1);
        cache_items = exec_env->block_addr_cache[cache_index];
        if (cache_items[0].start_addr == frame_ip) {
          end_addr = cache_items[0].end_addr;
        }
        else if (cache_items[1].start_addr == frame_ip) {
          end_addr = cache_items[1].end_addr;
        }
        else if (!wasm_loader_find_block_addr((BlockAddr*)exec_env->block_addr_cache,
                                              frame_ip, (uint8*)-1,
                                              BLOCK_TYPE_BLOCK,
                                              &else_addr, &end_addr,
                                              NULL, 0)) {
          wasm_set_exception(module, "find block address failed");
          goto got_exception;
        }

        PUSH_CSP(BLOCK_TYPE_BLOCK, block_ret_type, end_addr);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_LOOP):
        block_ret_type = *frame_ip++;
        PUSH_CSP(BLOCK_TYPE_LOOP, block_ret_type, frame_ip);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_IF):
        block_ret_type = *frame_ip++;

        cache_index = ((uintptr_t)frame_ip) & (uintptr_t)(BLOCK_ADDR_CACHE_SIZE - 1);
        cache_items = exec_env->block_addr_cache[cache_index];
        if (cache_items[0].start_addr == frame_ip) {
          else_addr = cache_items[0].else_addr;
          end_addr = cache_items[0].end_addr;
        }
        else if (cache_items[1].start_addr == frame_ip) {
          else_addr = cache_items[1].else_addr;
          end_addr = cache_items[1].end_addr;
        }
        else if (!wasm_loader_find_block_addr((BlockAddr*)exec_env->block_addr_cache,
                                              frame_ip, (uint8*)-1,
                                              BLOCK_TYPE_IF,
                                              &else_addr, &end_addr,
                                              NULL, 0)) {
          wasm_set_exception(module, "find block address failed");
          goto got_exception;
        }

        cond = (uint32)POP_I32();

        PUSH_CSP(BLOCK_TYPE_IF, block_ret_type, end_addr);

        /* condition of the if branch is false, else condition is met */
        if (cond == 0) {
          /* if there is no else branch, go to the end addr */
          if (else_addr == NULL) {
            POP_CSP();
            frame_ip = end_addr + 1;
          }
          /* if there is an else branch, go to the else addr */
          else
            frame_ip = else_addr + 1;
        }
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_ELSE):
        /* comes from the if branch in WASM_OP_IF */
        frame_ip = (frame_csp - 1)->target_addr;
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_END):
        if (frame_csp > frame->csp_bottom + 1) {
          POP_CSP();
        }
        else { /* end of function, treat as WASM_OP_RETURN */
          frame_sp -= cur_func->ret_cell_num;
          for (i = 0; i < cur_func->ret_cell_num; i++) {
            *prev_frame->sp++ = frame_sp[i];
          }
          goto return_func;
        }
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR):
        read_leb_uint32(frame_ip, frame_ip_end, depth);
        POP_CSP_N(depth);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR_IF):
        read_leb_uint32(frame_ip, frame_ip_end, depth);
        cond = (uint32)POP_I32();
        if (cond)
          POP_CSP_N(depth);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR_TABLE):
        read_leb_uint32(frame_ip, frame_ip_end, count);
        if (count <= BR_TABLE_TMP_BUF_LEN)
          depths = depth_buf;
        else {
          uint64 total_size = sizeof(uint32) * (uint64)count;
          if (total_size >= UINT32_MAX
              || !(depths = wasm_runtime_malloc((uint32)total_size))) {
            wasm_set_exception(module,
                               "WASM interp failed: allocate memory failed.");
            goto got_exception;
          }
        }
        for (i = 0; i < count; i++) {
          read_leb_uint32(frame_ip, frame_ip_end, depths[i]);
        }
        read_leb_uint32(frame_ip, frame_ip_end, depth);
        didx = POP_I32();
        if (didx >= 0 && (uint32)didx < count) {
          depth = depths[didx];
        }
        if (depths != depth_buf) {
          wasm_runtime_free(depths);
          depths = NULL;
        }
        POP_CSP_N(depth);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_RETURN):
        frame_sp -= cur_func->ret_cell_num;
        for (i = 0; i < cur_func->ret_cell_num; i++) {
          *prev_frame->sp++ = frame_sp[i];
        }
        goto return_func;

      HANDLE_OP (WASM_OP_CALL):
        read_leb_uint32(frame_ip, frame_ip_end, fidx);
        bh_assert(fidx < module->function_count);
        cur_func = module->functions + fidx;
        goto call_func_from_interp;

      HANDLE_OP (WASM_OP_CALL_INDIRECT):
        {
          WASMType *cur_type, *cur_func_type;

          read_leb_uint32(frame_ip, frame_ip_end, tidx);
          if (tidx >= module->module->type_count) {
            wasm_set_exception(module, "type index is overflow");
            goto got_exception;
          }
          cur_type = module->module->types[tidx];

          /* to skip 0x00 here */
          frame_ip++;
          val = POP_I32();

          if (val < 0 || val >= (int32)table->cur_size) {
            wasm_set_exception(module, "undefined element");
            goto got_exception;
          }

          fidx = ((uint32*)table->base_addr)[val];
          if (fidx == (uint32)-1) {
            wasm_set_exception(module, "uninitialized element");
            goto got_exception;
          }

          cur_func = module->functions + fidx;

          if (cur_func->is_import_func)
            cur_func_type = cur_func->u.func_import->func_type;
          else
            cur_func_type = cur_func->u.func->func_type;
          if (!wasm_type_equal(cur_type, cur_func_type)) {
            wasm_set_exception(module, "indirect call type mismatch");
            goto got_exception;
          }
          goto call_func_from_interp;
        }

      /* parametric instructions */
      HANDLE_OP (WASM_OP_DROP):
        {
          frame_sp--;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_DROP_64):
        {
          frame_sp -= 2;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SELECT):
        {
          cond = (uint32)POP_I32();
          frame_sp--;
          if (!cond)
            *(frame_sp - 1) = *frame_sp;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SELECT_64):
        {
          cond = (uint32)POP_I32();
          frame_sp -= 2;
          if (!cond) {
            *(frame_sp - 2) = *frame_sp;
            *(frame_sp - 1) = *(frame_sp + 1);
          }
          HANDLE_OP_END ();
        }

      /* variable instructions */
      HANDLE_OP (WASM_OP_GET_LOCAL):
        {
          GET_LOCAL_INDEX_TYPE_AND_OFFSET();

          switch (local_type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
              PUSH_I32(*(int32*)(frame_lp + local_offset));
              break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
              PUSH_I64(GET_I64_FROM_ADDR(frame_lp + local_offset));
              break;
            default:
              wasm_set_exception(module, "invalid local type");
              goto got_exception;
          }

          HANDLE_OP_END ();
        }

      HANDLE_OP (EXT_OP_GET_LOCAL_FAST):
        {
          local_offset = *frame_ip++;
          if (local_offset & 0x80)
            PUSH_I64(GET_I64_FROM_ADDR(frame_lp + (local_offset & 0x7F)));
          else
            PUSH_I32(*(int32*)(frame_lp + local_offset));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SET_LOCAL):
        {
          GET_LOCAL_INDEX_TYPE_AND_OFFSET();

          switch (local_type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
              *(int32*)(frame_lp + local_offset) = POP_I32();
              break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
              PUT_I64_TO_ADDR((uint32*)(frame_lp + local_offset), POP_I64());
              break;
            default:
              wasm_set_exception(module, "invalid local type");
              goto got_exception;
          }

          HANDLE_OP_END ();
        }

      HANDLE_OP (EXT_OP_SET_LOCAL_FAST):
        {
          local_offset = *frame_ip++;
          if (local_offset & 0x80)
            PUT_I64_TO_ADDR((uint32*)(frame_lp + (local_offset & 0x7F)), POP_I64());
          else
            *(int32*)(frame_lp + local_offset) = POP_I32();
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_TEE_LOCAL):
        {
          GET_LOCAL_INDEX_TYPE_AND_OFFSET();

          switch (local_type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
              *(int32*)(frame_lp + local_offset) = *(int32*)(frame_sp - 1);
              break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
              PUT_I64_TO_ADDR((uint32*)(frame_lp + local_offset),
                              GET_I64_FROM_ADDR(frame_sp - 2));
              break;
            default:
              wasm_set_exception(module, "invalid local type");
              goto got_exception;
          }

          HANDLE_OP_END ();
        }

      HANDLE_OP (EXT_OP_TEE_LOCAL_FAST):
        {
          local_offset = *frame_ip++;
          if (local_offset & 0x80)
            PUT_I64_TO_ADDR((uint32*)(frame_lp + (local_offset & 0x7F)),
                            GET_I64_FROM_ADDR(frame_sp - 2));
          else
            *(int32*)(frame_lp + local_offset) = *(int32*)(frame_sp - 1);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_GET_GLOBAL):
        {
          read_leb_uint32(frame_ip, frame_ip_end, global_idx);

          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
          global_addr = global_data + global->data_offset;

          switch (global->type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
              PUSH_I32(*(uint32*)global_addr);
              break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
              PUSH_I64(GET_I64_FROM_ADDR((uint32*)global_addr));
              break;
            default:
              wasm_set_exception(module, "invalid global type");
              goto got_exception;
          }

          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SET_GLOBAL):
        {
          read_leb_uint32(frame_ip, frame_ip_end, global_idx);

          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
          global_addr = global_data + global->data_offset;

          switch (global->type) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
              *(int32*)global_addr = POP_I32();
              break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
              PUT_I64_TO_ADDR((uint32*)global_addr, POP_I64());
              break;
            default:
              wasm_set_exception(module, "invalid global type");
              goto got_exception;
          }

          HANDLE_OP_END ();
        }

      /* memory load instructions */
      HANDLE_OP (WASM_OP_I32_LOAD):
      HANDLE_OP (WASM_OP_I64_LOAD):
      HANDLE_OP (WASM_OP_F32_LOAD):
      HANDLE_OP (WASM_OP_F64_LOAD):
      HANDLE_OP (WASM_OP_I32_LOAD8_S):
      HANDLE_OP (WASM_OP_I32_LOAD8_U):
      HANDLE_OP (WASM_OP_I32_LOAD16_S):
      HANDLE_OP (WASM_OP_I32_LOAD16_U):
      HANDLE_OP (WASM_OP_I64_LOAD8_S):
      HANDLE_OP (WASM_OP_I64_LOAD8_U):
      HANDLE_OP (WASM_OP_I64_LOAD16_S):
      HANDLE_OP (WASM_OP_I64_LOAD16_U):
      HANDLE_OP (WASM_OP_I64_LOAD32_S):
      HANDLE_OP (WASM_OP_I64_LOAD32_U):
        {
          uint32 offset, flags, addr;
          GET_OPCODE();
          read_leb_uint32(frame_ip, frame_ip_end, flags);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
          addr = (uint32)POP_I32();
          CHECK_MEMORY_OVERFLOW();
#if WASM_ENABLE_LABELS_AS_VALUES != 0
          static const void *handle_load_table[] = {
              &&HANDLE_LOAD_WASM_OP_I32_LOAD,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD,
              &&HANDLE_LOAD_WASM_OP_F32_LOAD,
              &&HANDLE_LOAD_WASM_OP_F64_LOAD,
              &&HANDLE_LOAD_WASM_OP_I32_LOAD8_S,
              &&HANDLE_LOAD_WASM_OP_I32_LOAD8_U,
              &&HANDLE_LOAD_WASM_OP_I32_LOAD16_S,
              &&HANDLE_LOAD_WASM_OP_I32_LOAD16_U,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD8_S,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD8_U,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD16_S,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD16_U,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD32_S,
              &&HANDLE_LOAD_WASM_OP_I64_LOAD32_U
          };
          #define HANDLE_OP_LOAD(opcode) HANDLE_LOAD_##opcode
          goto *handle_load_table[opcode - WASM_OP_I32_LOAD];
#else
          #define HANDLE_OP_LOAD(opcode) case opcode
          switch (opcode)
#endif
          {
            HANDLE_OP_LOAD(WASM_OP_I32_LOAD):
              PUSH_I32(LOAD_I32(maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD):
              PUSH_I64(LOAD_I64(maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_F32_LOAD):
              PUSH_I32(LOAD_I32(maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_F64_LOAD):
              PUSH_F64(LOAD_F64(maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I32_LOAD8_S):
              PUSH_I32(sign_ext_8_32(*(int8*)maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I32_LOAD8_U):
              PUSH_I32((uint32)(*(uint8*)maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I32_LOAD16_S):
              PUSH_I32(sign_ext_16_32(LOAD_I16(maddr)));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I32_LOAD16_U):
              PUSH_I32((uint32)(LOAD_U16(maddr)));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD8_S):
              PUSH_I64(sign_ext_8_64(*(int8*)maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD8_U):
              PUSH_I64((uint64)(*(uint8*)maddr));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD16_S):
              PUSH_I64(sign_ext_16_64(LOAD_I16(maddr)));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD16_U):
              PUSH_I64((uint64)(LOAD_U16(maddr)));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD32_S):
              PUSH_I64(sign_ext_32_64(LOAD_I32(maddr)));
              HANDLE_OP_END();
            HANDLE_OP_LOAD(WASM_OP_I64_LOAD32_U):
              PUSH_I64((uint64)(LOAD_U32(maddr)));
              HANDLE_OP_END();
          }
          (void)flags;
          HANDLE_OP_END ();
        }

      /* memory store instructions */
      HANDLE_OP (WASM_OP_F32_STORE):
        {
          uint32 offset, flags, addr;
          GET_OPCODE();
          read_leb_uint32(frame_ip, frame_ip_end, flags);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
          frame_sp--;
          addr = (uint32)POP_I32();
          CHECK_MEMORY_OVERFLOW();
          STORE_U32(maddr, frame_sp[1]);
          (void)flags;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_F64_STORE):
        {
          uint32 offset, flags, addr;
          GET_OPCODE();
          read_leb_uint32(frame_ip, frame_ip_end, flags);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
          frame_sp -= 2;
          addr = (uint32)POP_I32();
          CHECK_MEMORY_OVERFLOW();
          STORE_U32(maddr, frame_sp[1]);
          STORE_U32(maddr + 4, frame_sp[2]);
          (void)flags;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_STORE):
      HANDLE_OP (WASM_OP_I32_STORE8):
      HANDLE_OP (WASM_OP_I32_STORE16):
        {
          uint32 offset, flags, addr;
          uint32 sval;
          GET_OPCODE();
          read_leb_uint32(frame_ip, frame_ip_end, flags);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
          sval = (uint32)POP_I32();
          addr = (uint32)POP_I32();
          CHECK_MEMORY_OVERFLOW();
          switch (opcode) {
            case WASM_OP_I32_STORE:
              STORE_U32(maddr, sval);
              break;
            case WASM_OP_I32_STORE8:
              *(uint8*)maddr = (uint8)sval;
              break;
            case WASM_OP_I32_STORE16:
              STORE_U16(maddr, (uint16)sval);
              break;
          }
          (void)flags;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_STORE):
      HANDLE_OP (WASM_OP_I64_STORE8):
      HANDLE_OP (WASM_OP_I64_STORE16):
      HANDLE_OP (WASM_OP_I64_STORE32):
        {
          uint32 offset, flags, addr;
          uint64 sval;
          GET_OPCODE();
          read_leb_uint32(frame_ip, frame_ip_end, flags);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
          sval = (uint64)POP_I64();
          addr = (uint32)POP_I32();
          CHECK_MEMORY_OVERFLOW();
          switch (opcode) {
            case WASM_OP_I64_STORE:
              STORE_I64(maddr, sval);
              break;
            case WASM_OP_I64_STORE8:
              *(uint8*)maddr = (uint8)sval;
              break;
            case WASM_OP_I64_STORE16:
              STORE_U16(maddr, (uint16)sval);
              break;
            case WASM_OP_I64_STORE32:
              STORE_U32(maddr, (uint32)sval);
              break;
          }
          (void)flags;
          HANDLE_OP_END ();
        }

      /* memory size and memory grow instructions */
      HANDLE_OP (WASM_OP_MEMORY_SIZE):
      {
        uint32 reserved;
        read_leb_uint32(frame_ip, frame_ip_end, reserved);
        PUSH_I32(memory->cur_page_count);
        (void)reserved;
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_MEMORY_GROW):
      {
        uint32 reserved, delta, prev_page_count = memory->cur_page_count;

        read_leb_uint32(frame_ip, frame_ip_end, reserved);
        delta = (uint32)POP_I32();

        if (!wasm_enlarge_memory(module, delta)) {
          /* fail to memory.grow, return -1 */
          PUSH_I32(-1);
          if (wasm_get_exception(module)) {
            os_printf("%s\n", wasm_get_exception(module));
            wasm_set_exception(module, NULL);
          }
        }
        else {
          /* success, return previous page count */
          PUSH_I32(prev_page_count);
          /* update the memory instance ptr */
          memory = module->default_memory;
          total_mem_size = num_bytes_per_page * memory->cur_page_count
                           - heap_base_offset;
          global_data = memory->global_data;
        }

        (void)reserved;
        HANDLE_OP_END ();
      }

      /* constant instructions */
      HANDLE_OP (WASM_OP_I32_CONST):
        DEF_OP_I_CONST(int32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_CONST):
        DEF_OP_I_CONST(int64, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_CONST):
        {
          uint8 *p_float = (uint8*)frame_sp++;
          for (i = 0; i < sizeof(float32); i++)
            *p_float++ = *frame_ip++;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_F64_CONST):
        {
          uint8 *p_float = (uint8*)frame_sp++;
          frame_sp++;
          for (i = 0; i < sizeof(float64); i++)
            *p_float++ = *frame_ip++;
          HANDLE_OP_END ();
        }

      /* comparison instructions of i32 */
      HANDLE_OP (WASM_OP_I32_EQZ):
        DEF_OP_EQZ(I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_EQ):
        DEF_OP_CMP(uint32, I32, ==);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_NE):
        DEF_OP_CMP(uint32, I32, !=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_LT_S):
        DEF_OP_CMP(int32, I32, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_LT_U):
        DEF_OP_CMP(uint32, I32, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_GT_S):
        DEF_OP_CMP(int32, I32, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_GT_U):
        DEF_OP_CMP(uint32, I32, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_LE_S):
        DEF_OP_CMP(int32, I32, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_LE_U):
        DEF_OP_CMP(uint32, I32, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_GE_S):
        DEF_OP_CMP(int32, I32, >=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_GE_U):
        DEF_OP_CMP(uint32, I32, >=);
        HANDLE_OP_END ();

      /* comparison instructions of i64 */
      HANDLE_OP (WASM_OP_I64_EQZ):
        DEF_OP_EQZ(I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EQ):
        DEF_OP_CMP(uint64, I64, ==);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_NE):
        DEF_OP_CMP(uint64, I64, !=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_LT_S):
        DEF_OP_CMP(int64, I64, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_LT_U):
        DEF_OP_CMP(uint64, I64, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_GT_S):
        DEF_OP_CMP(int64, I64, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_GT_U):
        DEF_OP_CMP(uint64, I64, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_LE_S):
        DEF_OP_CMP(int64, I64, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_LE_U):
        DEF_OP_CMP(uint64, I64, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_GE_S):
        DEF_OP_CMP(int64, I64, >=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_GE_U):
        DEF_OP_CMP(uint64, I64, >=);
        HANDLE_OP_END ();

      /* comparison instructions of f32 */
      HANDLE_OP (WASM_OP_F32_EQ):
        DEF_OP_CMP(float32, F32, ==);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_NE):
        DEF_OP_CMP(float32, F32, !=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_LT):
        DEF_OP_CMP(float32, F32, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_GT):
        DEF_OP_CMP(float32, F32, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_LE):
        DEF_OP_CMP(float32, F32, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_GE):
        DEF_OP_CMP(float32, F32, >=);
        HANDLE_OP_END ();

      /* comparison instructions of f64 */
      HANDLE_OP (WASM_OP_F64_EQ):
        DEF_OP_CMP(float64, F64, ==);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_NE):
        DEF_OP_CMP(float64, F64, !=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_LT):
        DEF_OP_CMP(float64, F64, <);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_GT):
        DEF_OP_CMP(float64, F64, >);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_LE):
        DEF_OP_CMP(float64, F64, <=);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_GE):
        DEF_OP_CMP(float64, F64, >=);
        HANDLE_OP_END ();

      /* numberic instructions of i32 */
      HANDLE_OP (WASM_OP_I32_CLZ):
        DEF_OP_BIT_COUNT(uint32, I32, clz32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_CTZ):
        DEF_OP_BIT_COUNT(uint32, I32, ctz32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_POPCNT):
        DEF_OP_BIT_COUNT(uint32, I32, popcount32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_ADD):
        DEF_OP_NUMERIC(uint32, uint32, I32, +);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_SUB):
        DEF_OP_NUMERIC(uint32, uint32, I32, -);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_MUL):
        DEF_OP_NUMERIC(uint32, uint32, I32, *);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_DIV_S):
      {
        int32 a, b;

        b = POP_I32();
        a = POP_I32();
        if (a == (int32)0x80000000 && b == -1) {
          wasm_set_exception(module, "integer overflow");
          goto got_exception;
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I32(a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_DIV_U):
      {
        uint32 a, b;

        b = (uint32)POP_I32();
        a = (uint32)POP_I32();
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I32(a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_REM_S):
      {
        int32 a, b;

        b = POP_I32();
        a = POP_I32();
        if (a == (int32)0x80000000 && b == -1) {
          PUSH_I32(0);
          HANDLE_OP_END ();
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I32(a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_REM_U):
      {
        uint32 a, b;

        b = (uint32)POP_I32();
        a = (uint32)POP_I32();
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I32(a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_AND):
        DEF_OP_NUMERIC(uint32, uint32, I32, &);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_OR):
        DEF_OP_NUMERIC(uint32, uint32, I32, |);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_XOR):
        DEF_OP_NUMERIC(uint32, uint32, I32, ^);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_SHL):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC(uint32, uint32, I32, <<);
#else
        DEF_OP_NUMERIC2(uint32, uint32, I32, <<);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_SHR_S):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC(int32, uint32, I32, >>);
#else
        DEF_OP_NUMERIC2(int32, uint32, I32, >>);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_SHR_U):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC(uint32, uint32, I32, >>);
#else
        DEF_OP_NUMERIC2(uint32, uint32, I32, >>);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_ROTL):
      {
        uint32 a, b;

        b = (uint32)POP_I32();
        a = (uint32)POP_I32();
        PUSH_I32(rotl32(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_ROTR):
      {
        uint32 a, b;

        b = (uint32)POP_I32();
        a = (uint32)POP_I32();
        PUSH_I32(rotr32(a, b));
        HANDLE_OP_END ();
      }

      /* numberic instructions of i64 */
      HANDLE_OP (WASM_OP_I64_CLZ):
        DEF_OP_BIT_COUNT(uint64, I64, clz64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_CTZ):
        DEF_OP_BIT_COUNT(uint64, I64, ctz64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_POPCNT):
        DEF_OP_BIT_COUNT(uint64, I64, popcount64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_ADD):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, +);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_SUB):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, -);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_MUL):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, *);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_DIV_S):
      {
        int64 a, b;

        b = POP_I64();
        a = POP_I64();
        if (a == (int64)0x8000000000000000LL && b == -1) {
          wasm_set_exception(module, "integer overflow");
          goto got_exception;
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I64(a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_DIV_U):
      {
        uint64 a, b;

        b = (uint64)POP_I64();
        a = (uint64)POP_I64();
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I64(a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_REM_S):
      {
        int64 a, b;

        b = POP_I64();
        a = POP_I64();
        if (a == (int64)0x8000000000000000LL && b == -1) {
          PUSH_I64(0);
          HANDLE_OP_END ();
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I64(a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_REM_U):
      {
        uint64 a, b;

        b = (uint64)POP_I64();
        a = (uint64)POP_I64();
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        PUSH_I64(a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_AND):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, &);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_OR):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, |);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_XOR):
        DEF_OP_NUMERIC_64(uint64, uint64, I64, ^);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_SHL):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC_64(uint64, uint64, I64, <<);
#else
        DEF_OP_NUMERIC2_64(uint64, uint64, I64, <<);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_SHR_S):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC_64(int64, uint64, I64, >>);
#else
        DEF_OP_NUMERIC2_64(int64, uint64, I64, >>);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_SHR_U):
      {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_X86_32)
        DEF_OP_NUMERIC_64(uint64, uint64, I64, >>);
#else
        DEF_OP_NUMERIC2_64(uint64, uint64, I64, >>);
#endif
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_ROTL):
      {
        uint64 a, b;

        b = (uint64)POP_I64();
        a = (uint64)POP_I64();
        PUSH_I64(rotl64(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_ROTR):
      {
        uint64 a, b;

        b = (uint64)POP_I64();
        a = (uint64)POP_I64();
        PUSH_I64(rotr64(a, b));
        HANDLE_OP_END ();
      }

      /* numberic instructions of f32 */
      HANDLE_OP (WASM_OP_F32_ABS):
        DEF_OP_MATH(float32, F32, fabs);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_NEG):
      {
          int32 i32 = (int32)frame_sp[-1];
          int32 sign_bit = i32 & (1 << 31);
          if (sign_bit)
              frame_sp[-1] = i32 & ~(1 << 31);
          else
              frame_sp[-1] = (uint32)(i32 | (1 << 31));
          HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F32_CEIL):
        DEF_OP_MATH(float32, F32, ceil);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_FLOOR):
        DEF_OP_MATH(float32, F32, floor);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_TRUNC):
        DEF_OP_MATH(float32, F32, trunc);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_NEAREST):
        DEF_OP_MATH(float32, F32, rint);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_SQRT):
        DEF_OP_MATH(float32, F32, sqrt);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_ADD):
        DEF_OP_NUMERIC(float32, float32, F32, +);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_SUB):
        DEF_OP_NUMERIC(float32, float32, F32, -);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_MUL):
        DEF_OP_NUMERIC(float32, float32, F32, *);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_DIV):
        DEF_OP_NUMERIC(float32, float32, F32, /);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_MIN):
      {
        float32 a, b;

        b = POP_F32();
        a = POP_F32();

        if (isnan(a))
            PUSH_F32(a);
        else if (isnan(b))
            PUSH_F32(b);
        else
            PUSH_F32(wa_fmin(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F32_MAX):
      {
        float32 a, b;

        b = POP_F32();
        a = POP_F32();

        if (isnan(a))
            PUSH_F32(a);
        else if (isnan(b))
            PUSH_F32(b);
        else
            PUSH_F32(wa_fmax(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F32_COPYSIGN):
      {
        float32 a, b;

        b = POP_F32();
        a = POP_F32();
        PUSH_F32(signbit(b) ? -fabs(a) : fabs(a));
        HANDLE_OP_END ();
      }

      /* numberic instructions of f64 */
      HANDLE_OP (WASM_OP_F64_ABS):
        DEF_OP_MATH(float64, F64, fabs);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_NEG):
      {
          int64 i64 = GET_I64_FROM_ADDR(frame_sp - 2);
          int64 sign_bit = i64 & (((int64)1) << 63);
          if (sign_bit)
              PUT_I64_TO_ADDR(frame_sp - 2, ((uint64)i64 & ~(((uint64)1) << 63)));
          else
              PUT_I64_TO_ADDR(frame_sp - 2, ((uint64)i64 | (((uint64)1) << 63)));
          HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F64_CEIL):
        DEF_OP_MATH(float64, F64, ceil);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_FLOOR):
        DEF_OP_MATH(float64, F64, floor);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_TRUNC):
        DEF_OP_MATH(float64, F64, trunc);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_NEAREST):
        DEF_OP_MATH(float64, F64, rint);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_SQRT):
        DEF_OP_MATH(float64, F64, sqrt);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_ADD):
        DEF_OP_NUMERIC_64(float64, float64, F64, +);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_SUB):
        DEF_OP_NUMERIC_64(float64, float64, F64, -);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_MUL):
        DEF_OP_NUMERIC_64(float64, float64, F64, *);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_DIV):
        DEF_OP_NUMERIC_64(float64, float64, F64, /);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_MIN):
      {
        float64 a, b;

        b = POP_F64();
        a = POP_F64();

        if (isnan(a))
            PUSH_F64(a);
        else if (isnan(b))
            PUSH_F64(b);
        else
            PUSH_F64(wa_fmin(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F64_MAX):
      {
        float64 a, b;

        b = POP_F64();
        a = POP_F64();

        if (isnan(a))
            PUSH_F64(a);
        else if (isnan(b))
            PUSH_F64(b);
        else
            PUSH_F64(wa_fmax(a, b));
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F64_COPYSIGN):
      {
        float64 a, b;

        b = POP_F64();
        a = POP_F64();
        PUSH_F64(signbit(b) ? -fabs(a) : fabs(a));
        HANDLE_OP_END ();
      }

      /* conversions of i32 */
      HANDLE_OP (WASM_OP_I32_WRAP_I64):
        {
          int32 value = (int32)(POP_I64() & 0xFFFFFFFFLL);
          PUSH_I32(value);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_TRUNC_S_F32):
        /* We don't use INT32_MIN/INT32_MAX/UINT32_MIN/UINT32_MAX,
           since float/double values of ieee754 cannot precisely represent
           all int32/uint32/int64/uint64 values, e.g.:
           UINT32_MAX is 4294967295, but (float32)4294967295 is 4294967296.0f,
           but not 4294967295.0f. */
        DEF_OP_TRUNC(int32, I32, float32, F32, <= -2147483904.0f,
                                               >= 2147483648.0f);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_U_F32):
        DEF_OP_TRUNC(uint32, I32, float32, F32, <= -1.0f,
                                                >= 4294967296.0f);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_S_F64):
        DEF_OP_TRUNC(int32, I32, float64, F64, <= -2147483649.0,
                                               >= 2147483648.0);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_U_F64):
        DEF_OP_TRUNC(uint32, I32, float64, F64, <= -1.0 ,
                                                >= 4294967296.0);
        HANDLE_OP_END ();

      /* conversions of i64 */
      HANDLE_OP (WASM_OP_I64_EXTEND_S_I32):
        DEF_OP_CONVERT(int64, I64, int32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EXTEND_U_I32):
        DEF_OP_CONVERT(int64, I64, uint32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_S_F32):
        DEF_OP_TRUNC(int64, I64, float32, F32, <= -9223373136366403584.0f,
                                               >= 9223372036854775808.0f);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_U_F32):
        DEF_OP_TRUNC(uint64, I64, float32, F32, <= -1.0f,
                                                >= 18446744073709551616.0f);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_S_F64):
        DEF_OP_TRUNC(int64, I64, float64, F64, <= -9223372036854777856.0,
                                               >= 9223372036854775808.0);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_U_F64):
        DEF_OP_TRUNC(uint64, I64, float64, F64, <= -1.0,
                                                >= 18446744073709551616.0);
        HANDLE_OP_END ();

      /* conversions of f32 */
      HANDLE_OP (WASM_OP_F32_CONVERT_S_I32):
        DEF_OP_CONVERT(float32, F32, int32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_CONVERT_U_I32):
        DEF_OP_CONVERT(float32, F32, uint32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_CONVERT_S_I64):
        DEF_OP_CONVERT(float32, F32, int64, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_CONVERT_U_I64):
        DEF_OP_CONVERT(float32, F32, uint64, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_DEMOTE_F64):
        DEF_OP_CONVERT(float32, F32, float64, F64);
        HANDLE_OP_END ();

      /* conversions of f64 */
      HANDLE_OP (WASM_OP_F64_CONVERT_S_I32):
        DEF_OP_CONVERT(float64, F64, int32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_CONVERT_U_I32):
        DEF_OP_CONVERT(float64, F64, uint32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_CONVERT_S_I64):
        DEF_OP_CONVERT(float64, F64, int64, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_CONVERT_U_I64):
        DEF_OP_CONVERT(float64, F64, uint64, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_PROMOTE_F32):
        DEF_OP_CONVERT(float64, F64, float32, F32);
        HANDLE_OP_END ();

      /* reinterpretations */
      HANDLE_OP (WASM_OP_I32_REINTERPRET_F32):
      HANDLE_OP (WASM_OP_I64_REINTERPRET_F64):
      HANDLE_OP (WASM_OP_F32_REINTERPRET_I32):
      HANDLE_OP (WASM_OP_F64_REINTERPRET_I64):
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_IMPDEP):
        frame = prev_frame;
        frame_ip = frame->ip;
        frame_sp = frame->sp;
        frame_csp = frame->csp;
        goto call_func_from_entry;

#if WASM_ENABLE_LABELS_AS_VALUES == 0
      default:
        wasm_set_exception(module, "WASM interp failed: unsupported opcode.");
        goto got_exception;
    }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES != 0
    HANDLE_OP (WASM_OP_UNUSED_0x06):
    HANDLE_OP (WASM_OP_UNUSED_0x07):
    HANDLE_OP (WASM_OP_UNUSED_0x08):
    HANDLE_OP (WASM_OP_UNUSED_0x09):
    HANDLE_OP (WASM_OP_UNUSED_0x0a):
    HANDLE_OP (WASM_OP_UNUSED_0x12):
    HANDLE_OP (WASM_OP_UNUSED_0x13):
    HANDLE_OP (WASM_OP_UNUSED_0x14):
    HANDLE_OP (WASM_OP_UNUSED_0x15):
    HANDLE_OP (WASM_OP_UNUSED_0x16):
    HANDLE_OP (WASM_OP_UNUSED_0x17):
    HANDLE_OP (WASM_OP_UNUSED_0x18):
    HANDLE_OP (WASM_OP_UNUSED_0x19):
    HANDLE_OP (WASM_OP_UNUSED_0x1c):
    HANDLE_OP (WASM_OP_UNUSED_0x1d):
    HANDLE_OP (WASM_OP_UNUSED_0x1e):
    HANDLE_OP (WASM_OP_UNUSED_0x1f):
    HANDLE_OP (WASM_OP_UNUSED_0x25):
    HANDLE_OP (WASM_OP_UNUSED_0x26):
    HANDLE_OP (WASM_OP_UNUSED_0x27):
    /* Used by fast interpreter */
    HANDLE_OP (EXT_OP_SET_LOCAL_FAST_I64):
    HANDLE_OP (EXT_OP_TEE_LOCAL_FAST_I64):
    HANDLE_OP (EXT_OP_COPY_STACK_TOP):
    HANDLE_OP (EXT_OP_COPY_STACK_TOP_I64):
    {
      wasm_set_exception(module, "WASM interp failed: unsupported opcode.");
      goto got_exception;
    }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES == 0
    continue;
#else
    FETCH_OPCODE_AND_DISPATCH ();
#endif

  call_func_from_interp:
    /* Only do the copy when it's called from interpreter.  */
    {
      WASMInterpFrame *outs_area = wasm_exec_env_wasm_stack_top(exec_env);
      POP(cur_func->param_cell_num);
      SYNC_ALL_TO_FRAME();
      word_copy(outs_area->lp, frame_sp, cur_func->param_cell_num);
      prev_frame = frame;
    }

  call_func_from_entry:
    {
      if (cur_func->is_import_func) {
        wasm_interp_call_func_native(module, exec_env, cur_func, prev_frame);
        prev_frame = frame->prev_frame;
        cur_func = frame->function;
        UPDATE_ALL_FROM_FRAME();

        memory = module->default_memory;
        if (wasm_get_exception(module))
          goto got_exception;
      }
      else {
        WASMFunction *cur_wasm_func = cur_func->u.func;
        WASMType *func_type;
        uint8 ret_type;

        func_type = cur_wasm_func->func_type;

        all_cell_num = (uint64)cur_func->param_cell_num
                       + (uint64)cur_func->local_cell_num
                       + (uint64)cur_wasm_func->max_stack_cell_num
                       + ((uint64)cur_wasm_func->max_block_num) * sizeof(WASMBranchBlock) / 4;
        if (all_cell_num >= UINT32_MAX) {
            wasm_set_exception(module, "WASM interp failed: stack overflow.");
            goto got_exception;
        }

        frame_size = wasm_interp_interp_frame_size((uint32)all_cell_num);
        if (!(frame = ALLOC_FRAME(exec_env, frame_size, prev_frame))) {
          frame = prev_frame;
          goto got_exception;
        }

        /* Initialize the interpreter context. */
        frame->function = cur_func;
        frame_ip = wasm_get_func_code(cur_func);
        frame_ip_end = wasm_get_func_code_end(cur_func);
        frame_lp = frame->lp;

        frame_sp = frame->sp_bottom = frame_lp + cur_func->param_cell_num
                                               + cur_func->local_cell_num;
        frame->sp_boundary = frame->sp_bottom + cur_wasm_func->max_stack_cell_num;

        frame_csp = frame->csp_bottom = (WASMBranchBlock*)frame->sp_boundary;
        frame->csp_boundary = frame->csp_bottom + cur_wasm_func->max_block_num;

        /* Initialize the local varialbes */
        memset(frame_lp + cur_func->param_cell_num, 0,
               (uint32)(cur_func->local_cell_num * 4));

        /* Push function block as first block */
        ret_type = func_type->result_count
                   ? cur_func->param_types[func_type->param_count]
                   : VALUE_TYPE_VOID;
        PUSH_CSP(BLOCK_TYPE_FUNCTION, ret_type, frame_ip_end - 1);

        wasm_exec_env_set_cur_frame(exec_env, (WASMRuntimeFrame*)frame);
      }
      HANDLE_OP_END ();
    }

  return_func:
    {
      FREE_FRAME(exec_env, frame);
      wasm_exec_env_set_cur_frame(exec_env, (WASMRuntimeFrame*)prev_frame);

      if (!prev_frame->ip)
        /* Called from native. */
        return;

      RECOVER_CONTEXT(prev_frame);
      HANDLE_OP_END ();
    }

  out_of_bounds:
    wasm_set_exception(module, "out of bounds memory access");

  got_exception:
    return;

#if WASM_ENABLE_LABELS_AS_VALUES == 0
  }
#else
  FETCH_OPCODE_AND_DISPATCH ();
#endif
}

void
wasm_interp_call_wasm(WASMModuleInstance *module_inst,
                      WASMExecEnv *exec_env,
                      WASMFunctionInstance *function,
                      uint32 argc, uint32 argv[])
{
    WASMRuntimeFrame *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMInterpFrame *frame, *outs_area;

    /* Allocate sufficient cells for all kinds of return values.  */
    unsigned all_cell_num = 2, i;
    /* This frame won't be used by JITed code, so only allocate interp
       frame here.  */
    unsigned frame_size = wasm_interp_interp_frame_size(all_cell_num);

    if (argc != function->param_cell_num) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "invalid argument count %d, expected %d",
                 argc, function->param_cell_num);
        wasm_set_exception(module_inst, buf);
        return;
    }

    /* TODO: check stack overflow. */

    if (!(frame = ALLOC_FRAME(exec_env, frame_size, (WASMInterpFrame*)prev_frame)))
        return;

    outs_area = wasm_exec_env_wasm_stack_top(exec_env);
    frame->function = NULL;
    frame->ip = NULL;
    /* There is no local variable. */
    frame->sp = frame->lp + 0;

    if (argc > 0)
        word_copy(outs_area->lp, argv, argc);

    wasm_exec_env_set_cur_frame(exec_env, frame);

    if (function->is_import_func)
        wasm_interp_call_func_native(module_inst, exec_env, function, frame);
    else
        wasm_interp_call_func_bytecode(module_inst, exec_env, function, frame);

    /* Output the return value to the caller */
    if (!wasm_get_exception(module_inst)) {
        for (i = 0; i < function->ret_cell_num; i++)
            argv[i] = *(frame->sp + i - function->ret_cell_num);
    }

    wasm_exec_env_set_cur_frame(exec_env, prev_frame);
    FREE_FRAME(exec_env, frame);
}
