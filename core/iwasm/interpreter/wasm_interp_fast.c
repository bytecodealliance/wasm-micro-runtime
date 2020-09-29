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
#if WASM_ENABLE_SHARED_MEMORY != 0
#include "../common/wasm_shared_memory.h"
#endif

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
#define LOAD_F32(addr) (*(float32*)(addr))
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
#define LOAD_F32(addr) ((float32)LOAD_I32(addr))

#endif  /* WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0 */

#define CHECK_MEMORY_OVERFLOW(bytes) do {                                \
    uint64 offset1 = (uint64)offset + (uint64)addr;                      \
    if (offset1 + bytes <= (uint64)linear_mem_size)                      \
      /* If offset1 is in valid range, maddr must also be in valid range,\
         no need to check it again. */                                   \
      maddr = memory->memory_data + offset1;                             \
    else                                                                 \
      goto out_of_bounds;                                                \
  } while (0)

#define CHECK_BULK_MEMORY_OVERFLOW(start, bytes, maddr) do {             \
    uint64 offset1 = (uint32)(start);                                    \
    if (offset1 + bytes <= linear_mem_size)                              \
      /* App heap space is not valid space for bulk memory operation */  \
      maddr = memory->memory_data + offset1;                             \
    else                                                                 \
      goto out_of_bounds;                                                \
  } while (0)

#define CHECK_ATOMIC_MEMORY_ACCESS(align) do {          \
    if (((uintptr_t)maddr & (align - 1)) != 0)          \
      goto unaligned_atomic;                            \
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

#define read_uint32(p) (p += sizeof(uint32), *(uint32 *)(p - sizeof(uint32)))

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

#define GET_OFFSET() (frame_ip += 2, *(int16 *)(frame_ip - 2))

#define SET_OPERAND(type, off, value)           \
    (*(type*)(frame_lp + *(int16*)(frame_ip + off))) = value

#define GET_OPERAND(type, off) (*(type*)(frame_lp + *(int16*)(frame_ip + off)))

#define PUSH_I32(value) do {                    \
    *(int32*)(frame_lp + GET_OFFSET()) = value;    \
  } while (0)

#define PUSH_F32(value) do {                    \
    *(float32*)(frame_lp + GET_OFFSET()) = value;  \
  } while (0)

#define PUSH_I64(value) do {                    \
    *(int64*)(frame_lp + GET_OFFSET()) = value;    \
  } while (0)

#define PUSH_F64(value) do {                    \
    *(float64*)(frame_lp + GET_OFFSET()) = value;  \
  } while (0)

#define POP_I32() (*(int32*)(frame_lp + GET_OFFSET()))

#define POP_F32() (*(float32*)(frame_lp + GET_OFFSET()))

#define POP_I64() (*(int64*)(frame_lp + GET_OFFSET()))

#define POP_F64() (*(float64*)(frame_lp + GET_OFFSET()))

#define SYNC_ALL_TO_FRAME() do {                \
    frame->ip = frame_ip;                       \
  } while (0)

#define UPDATE_ALL_FROM_FRAME() do {            \
    frame_ip = frame->ip;                       \
  } while (0)

#define RECOVER_CONTEXT(new_frame) do {                             \
    frame = (new_frame);                                            \
    cur_func = frame->function;                                     \
    prev_frame = frame->prev_frame;                                 \
    frame_ip = frame->ip;                                           \
    frame_lp = frame->lp;                                           \
  } while (0)

#if WASM_ENABLE_LABELS_AS_VALUES != 0
#define GET_OPCODE() opcode = *(frame_ip++);
#else
#define GET_OPCODE() (void)0
#endif

#define DEF_OP_EQZ(ctype, src_op_type) do {                         \
    SET_OPERAND(int32, 2, (GET_OPERAND(ctype, 0) == 0));            \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_CMP(src_type, src_op_type, cond) do {                \
    SET_OPERAND(uint32, 4, GET_OPERAND(src_type, 2) cond            \
        GET_OPERAND(src_type, 0));                                  \
    frame_ip += 6;                                                  \
  } while (0)

#define DEF_OP_BIT_COUNT(src_type, src_op_type, operation) do {     \
    SET_OPERAND(src_type, 2,                                        \
        (src_type)operation(GET_OPERAND(src_type, 0)));             \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_NUMERIC(src_type1, src_type2, src_op_type, operation) do {   \
    SET_OPERAND(src_type1, 4, (GET_OPERAND(src_type1, 2)                    \
        operation GET_OPERAND(src_type2, 0)));                              \
    frame_ip += 6;                                                          \
  } while (0)

#if defined(BUILD_TARGET_X86_32)
#define DEF_OP_REINTERPRET(src_type) do {                                   \
    void *src = frame_lp + GET_OFFSET();                                    \
    void *dst = frame_lp + GET_OFFSET();                                    \
    bh_memcpy_s(dst, sizeof(src_type), src, sizeof(src_type));              \
  } while (0)
#else
#define DEF_OP_REINTERPRET(src_type) do {                                   \
    SET_OPERAND(src_type, 2, GET_OPERAND(src_type, 0));                     \
    frame_ip += 4;                                                          \
  } while (0)
#endif

#if WASM_CPU_SUPPORTS_UNALIGNED_64BIT_ACCESS != 0
#define DEF_OP_NUMERIC_64 DEF_OP_NUMERIC
#else
#define DEF_OP_NUMERIC_64(src_type1, src_type2, src_op_type, operation) do {          \
    src_type1 val1;                                                                   \
    src_type2 val2;                                                                   \
    val1 =                                                                            \
      (src_type1)GET_##src_op_type##_FROM_ADDR(frame_lp + (*(int16*)(frame_ip + 2))); \
    val2 =                                                                            \
      (src_type2)GET_##src_op_type##_FROM_ADDR(frame_lp + (*(int16*)(frame_ip)));     \
    val1 operation##= val2;                                                           \
    PUT_##src_op_type##_TO_ADDR(frame_lp + (*(int16*)(frame_ip + 4)), val1);          \
    frame_ip += 6;                                                                    \
  } while (0)
#endif

#define DEF_OP_NUMERIC2(src_type1, src_type2, src_op_type, operation) do {  \
    SET_OPERAND(src_type1, 4, (GET_OPERAND(src_type1, 2)                    \
        operation (GET_OPERAND(src_type2, 0) % 32)));                       \
    frame_ip += 6;                                                          \
  } while (0)

#define DEF_OP_NUMERIC2_64(src_type1, src_type2, src_op_type, operation) do { \
    SET_OPERAND(src_type1, 4, (GET_OPERAND(src_type1, 2)                      \
        operation (GET_OPERAND(src_type2, 0) % 64)));                         \
    frame_ip += 6;                                                            \
  } while (0)

#define DEF_ATOMIC_RMW_OPCODE(OP_NAME, op)                          \
  case WASM_OP_ATOMIC_RMW_I32_##OP_NAME:                            \
  case WASM_OP_ATOMIC_RMW_I32_##OP_NAME##8_U:                       \
  case WASM_OP_ATOMIC_RMW_I32_##OP_NAME##16_U:                      \
  {                                                                 \
    uint32 readv, sval;                                             \
                                                                    \
    sval = POP_I32();                                               \
    addr = POP_I32();                                               \
                                                                    \
    if (opcode == WASM_OP_ATOMIC_RMW_I32_##OP_NAME##8_U) {          \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(1);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint32)(*(uint8*)maddr);                             \
      *(uint8*)maddr = (uint8)(readv op sval);                      \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    else if (opcode == WASM_OP_ATOMIC_RMW_I32_##OP_NAME##16_U) {    \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(2);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint32)LOAD_U16(maddr);                              \
      STORE_U16(maddr, (uint16)(readv op sval));                    \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    else {                                                          \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(4);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = LOAD_I32(maddr);                                      \
      STORE_U32(maddr, readv op sval);                              \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    PUSH_I32(readv);                                                \
    break;                                                          \
  }                                                                 \
  case WASM_OP_ATOMIC_RMW_I64_##OP_NAME:                            \
  case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##8_U:                       \
  case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##16_U:                      \
  case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##32_U:                      \
  {                                                                 \
    uint64 readv, sval;                                             \
                                                                    \
    sval = (uint64)POP_I64();                                       \
    addr = POP_I32();                                               \
                                                                    \
    if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##8_U) {          \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(1);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint64)(*(uint8*)maddr);                             \
      *(uint8*)maddr = (uint8)(readv op sval);                      \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    else if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##16_U) {    \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(2);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint64)LOAD_U16(maddr);                              \
      STORE_U16(maddr, (uint16)(readv op sval));                    \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    else if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##32_U) {    \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(4);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint64)LOAD_U32(maddr);                              \
      STORE_U32(maddr, (uint32)(readv op sval));                    \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    else {                                                          \
      uint64 op_result;                                             \
      CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);          \
      CHECK_ATOMIC_MEMORY_ACCESS(8);                                \
                                                                    \
      os_mutex_lock(&memory->mem_lock);                             \
      readv = (uint64)LOAD_I64(maddr);                              \
      op_result = readv op sval;                                    \
      STORE_I64(maddr, op_result);                                  \
      os_mutex_unlock(&memory->mem_lock);                           \
    }                                                               \
    PUSH_I64(readv);                                                \
    break;                                                          \
  }

#define DEF_OP_MATH(src_type, src_op_type, method) do {             \
    SET_OPERAND(src_type, 2, method(GET_OPERAND(src_type, 0)));     \
      frame_ip += 4;                                                \
  } while (0)

#define TRUNC_FUNCTION(func_name, src_type, dst_type, signed_type)  \
static dst_type                                                     \
func_name(src_type src_value, src_type src_min, src_type src_max,   \
          dst_type dst_min, dst_type dst_max, bool is_sign)         \
{                                                                   \
  dst_type dst_value = 0;                                           \
  if (!isnan(src_value)) {                                          \
      if (src_value <= src_min)                                     \
          dst_value = dst_min;                                      \
      else if (src_value >= src_max)                                \
          dst_value = dst_max;                                      \
      else {                                                        \
          if (is_sign)                                              \
              dst_value = (dst_type)(signed_type)src_value;         \
          else                                                      \
              dst_value = (dst_type)src_value;                      \
      }                                                             \
  }                                                                 \
  return dst_value;                                                 \
}

TRUNC_FUNCTION(trunc_f32_to_i32, float32, uint32, int32)
TRUNC_FUNCTION(trunc_f32_to_i64, float32, uint64, int64)
TRUNC_FUNCTION(trunc_f64_to_i32, float64, uint32, int32)
TRUNC_FUNCTION(trunc_f64_to_i64, float64, uint64, int64)

static bool
trunc_f32_to_int(WASMModuleInstance *module,
                 uint8 *frame_ip, uint32 *frame_lp,
                 float32 src_min, float32 src_max,
                 bool saturating, bool is_i32, bool is_sign)
{
    float32 src_value = GET_OPERAND(float32, 0);
    uint64 dst_value_i64;
    uint32 dst_value_i32;

    if (!saturating) {
        if (isnan(src_value)) {
            wasm_set_exception(module, "invalid conversion to integer");
            return true;
        }
        else if (src_value <= src_min || src_value >= src_max) {
            wasm_set_exception(module, "integer overflow");
            return true;
        }
    }

    if (is_i32) {
        uint32 dst_min = is_sign ? INT32_MIN : 0;
        uint32 dst_max = is_sign ? INT32_MAX : UINT32_MAX;
        dst_value_i32 = trunc_f32_to_i32(src_value, src_min, src_max,
                                         dst_min, dst_max, is_sign);
        SET_OPERAND(uint32, 2, dst_value_i32);
    }
    else {
        uint64 dst_min = is_sign ? INT64_MIN : 0;
        uint64 dst_max = is_sign ? INT64_MAX : UINT64_MAX;
        dst_value_i64 = trunc_f32_to_i64(src_value, src_min, src_max,
                                         dst_min, dst_max, is_sign);
        SET_OPERAND(uint64, 2, dst_value_i64);
    }
    return false;
}

static bool
trunc_f64_to_int(WASMModuleInstance *module,
                 uint8 *frame_ip, uint32 *frame_lp,
                 float64 src_min, float64 src_max,
                 bool saturating, bool is_i32, bool is_sign)
{
    float64 src_value = GET_OPERAND(float64, 0);
    uint64 dst_value_i64;
    uint32 dst_value_i32;

    if (!saturating) {
        if (isnan(src_value)) {
            wasm_set_exception(module, "invalid conversion to integer");
            return true;
        }
        else if (src_value <= src_min || src_value >= src_max) {
            wasm_set_exception(module, "integer overflow");
            return true;
        }
    }

    if (is_i32) {
        uint32 dst_min = is_sign ? INT32_MIN : 0;
        uint32 dst_max = is_sign ? INT32_MAX : UINT32_MAX;
        dst_value_i32 = trunc_f64_to_i32(src_value, src_min, src_max,
                                         dst_min, dst_max, is_sign);
        SET_OPERAND(uint32, 2, dst_value_i32);
    }
    else {
        uint64 dst_min = is_sign ? INT64_MIN : 0;
        uint64 dst_max = is_sign ? INT64_MAX : UINT64_MAX;
        dst_value_i64 = trunc_f64_to_i64(src_value, src_min, src_max,
                                         dst_min, dst_max, is_sign);
        SET_OPERAND(uint64, 2, dst_value_i64);
    }
    return false;
}

#define DEF_OP_TRUNC_F32(min, max, is_i32, is_sign) do {            \
    if (trunc_f32_to_int(module, frame_ip, frame_lp, min, max,      \
                         false, is_i32, is_sign))                   \
        goto got_exception;                                         \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_TRUNC_F64(min, max, is_i32, is_sign) do {            \
    if (trunc_f64_to_int(module, frame_ip, frame_lp, min, max,      \
                         false, is_i32, is_sign))                   \
        goto got_exception;                                         \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_TRUNC_SAT_F32(min, max, is_i32, is_sign) do {        \
    (void)trunc_f32_to_int(module, frame_ip, frame_lp, min, max,    \
                           true, is_i32, is_sign);                  \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_TRUNC_SAT_F64(min, max, is_i32, is_sign) do {        \
    (void)trunc_f64_to_int(module, frame_ip, frame_lp, min, max,    \
                           true, is_i32, is_sign);                  \
    frame_ip += 4;                                                  \
  } while (0)

#define DEF_OP_CONVERT(dst_type, dst_op_type,                       \
                       src_type, src_op_type) do {                  \
    dst_type value = (dst_type)(src_type)POP_##src_op_type();       \
    PUSH_##dst_op_type(value);                                      \
  } while (0)

static bool
copy_stack_values(WASMModuleInstance *module,
                  uint32 *frame_lp,
                  uint32 arity,
                  uint32 total_cell_num,
                  const uint8 *cells,
                  const int16 *src_offsets,
                  const uint16 *dst_offsets)
{
  /* To avoid the overlap issue between src offsets and dst offset,
   * we use 2 steps to do the copy. First step, copy the src values
   * to a tmp buf. Second step, copy the values from tmp buf to dst.
   */
  uint32 buf[16] = {0}, i;
  uint32 *tmp_buf = buf;
  uint8 cell;
  int16 src, buf_index = 0;
  uint16 dst;

  /* Allocate memory if the buf is not large enough */
  if (total_cell_num > sizeof(buf)/sizeof(uint32)) {
    uint64 total_size = sizeof(uint32) * (uint64)total_cell_num;
    if (total_size >= UINT32_MAX
        || !(tmp_buf = wasm_runtime_malloc((uint32)total_size))) {
      wasm_set_exception(module, "allocate memory failed");
      return false;
    }
  }

  /* 1) Copy values from src to tmp buf */
  for (i = 0; i < arity; i++) {
    cell = cells[i];
    src = src_offsets[i];
    if (cell == 1)
      tmp_buf[buf_index] = frame_lp[src];
    else
      *(uint64*)(tmp_buf + buf_index) = *(uint64*)(frame_lp + src);
    buf_index += cell;
  }

  /* 2) Copy values from tmp buf to dest */
  buf_index = 0;
  for (i = 0; i < arity; i++) {
    cell = cells[i];
    dst = dst_offsets[i];
    if (cell == 1)
      frame_lp[dst] = tmp_buf[buf_index];
    else
      *(uint64*)(frame_lp + dst) = *(uint64*)(tmp_buf + buf_index);
    buf_index += cell;
  }

  if (tmp_buf !=  buf) {
    wasm_runtime_free(tmp_buf);
  }

    return true;
}

#define RECOVER_BR_INFO() do {                                \
    uint32 arity;                                             \
    /* read arity */                                          \
    arity = *(uint32*)frame_ip;                               \
    frame_ip += sizeof(arity);                                \
    if (arity) {                                              \
        uint32 total_cell;                                    \
        uint16 *dst_offsets = NULL;                           \
        uint8 *cells;                                         \
        int16 *src_offsets = NULL;                            \
        /* read total cell num */                             \
        total_cell = *(uint32*)frame_ip;                      \
        frame_ip += sizeof(total_cell);                       \
        /* cells */                                           \
        cells = (uint8 *)frame_ip;                            \
        frame_ip += arity * sizeof(uint8);                    \
        /* src offsets */                                     \
        src_offsets = (int16 *)frame_ip;                      \
        frame_ip += arity * sizeof(int16);                    \
        /* dst offsets */                                     \
        dst_offsets = (uint16*)frame_ip;                      \
        frame_ip += arity * sizeof(uint16);                   \
        if (arity == 1) {                                     \
            if (cells[0] == 1)                                \
                frame_lp[dst_offsets[0]] =                    \
                    frame_lp[src_offsets[0]];                 \
            else if (cells[0] == 2) {                         \
                *(int64*)(frame_lp + dst_offsets[0]) =        \
                    *(int64*)(frame_lp + src_offsets[0]);     \
            }                                                 \
        }                                                     \
        else {                                                \
            if (!copy_stack_values(module, frame_lp,          \
                                   arity, total_cell,         \
                                   cells, src_offsets,        \
                                   dst_offsets))              \
                goto got_exception;                           \
        }                                                     \
    }                                                         \
    frame_ip = *(uint8**)frame_ip;                            \
  } while (0)

#define SKIP_BR_INFO() do {                                                 \
    uint32 arity;                                                           \
    /* read and skip arity */                                               \
    arity = *(uint32*)frame_ip;                                             \
    frame_ip += sizeof(arity);                                              \
    if (arity) {                                                            \
        /* skip total cell num */                                           \
        frame_ip += sizeof(uint32);                                         \
        /* skip cells, src offsets and dst offsets */                       \
        frame_ip += (sizeof(uint8) + sizeof(int16) + sizeof(uint16)) * arity; \
    }                                                                       \
    /* skip target address */                                               \
    frame_ip += sizeof(uint8*);                                             \
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
                           "stack overflow");
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
    bool ret;

    if (!(frame = ALLOC_FRAME(exec_env,
                              wasm_interp_interp_frame_size(local_cell_num),
                              prev_frame)))
        return;

    frame->function = cur_func;
    frame->ip = NULL;
    frame->lp = frame->operand;

    wasm_exec_env_set_cur_frame(exec_env, frame);

    if (!func_import->func_ptr_linked) {
        char buf[128];
        snprintf(buf,
                 sizeof(buf), "fail to call unlinked import function (%s, %s)",
                 func_import->module_name, func_import->field_name);
        wasm_set_exception((WASMModuleInstance*)module_inst, buf);
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
        prev_frame->lp[prev_frame->ret_offset] = argv_ret[0];
    }
    else if (cur_func->ret_cell_num == 2) {
        prev_frame->lp[prev_frame->ret_offset] = argv_ret[0];
        prev_frame->lp[prev_frame->ret_offset + 1] = argv_ret[1];
    }

    FREE_FRAME(exec_env, frame);
    wasm_exec_env_set_cur_frame(exec_env, prev_frame);
}

#if WASM_ENABLE_MULTI_MODULE != 0
static void
wasm_interp_call_func_bytecode(WASMModuleInstance *module,
                               WASMExecEnv *exec_env,
                               WASMFunctionInstance *cur_func,
                               WASMInterpFrame *prev_frame);

static void
wasm_interp_call_func_import(WASMModuleInstance *module_inst,
                             WASMExecEnv *exec_env,
                             WASMFunctionInstance *cur_func,
                             WASMInterpFrame *prev_frame)
{
   WASMModuleInstance *sub_module_inst = cur_func->import_module_inst;
    WASMFunctionInstance *sub_func_inst = cur_func->import_func_inst;
    WASMFunctionImport *func_import = cur_func->u.func_import;
    uint8 *ip = prev_frame->ip;
    char buf[128];

    if (!sub_func_inst) {
        snprintf(buf, sizeof(buf),
                 "fail to call unlinked import function (%s, %s)",
                 func_import->module_name, func_import->field_name);
        wasm_set_exception(module_inst, buf);
        return;
    }

    /* set ip NULL to make call_func_bytecode return after executing
       this function */
    prev_frame->ip = NULL;

    /* replace exec_env's module_inst with sub_module_inst so we can
       call it */
    exec_env->module_inst = (WASMModuleInstanceCommon *)sub_module_inst;

    /* call function of sub-module*/
    wasm_interp_call_func_bytecode(sub_module_inst, exec_env,
                                   sub_func_inst, prev_frame);

    /* restore ip and module_inst */
    prev_frame->ip = ip;
    exec_env->module_inst = (WASMModuleInstanceCommon *)module_inst;

    /* transfer exception if it is thrown */
    if (wasm_get_exception(sub_module_inst)) {
        bh_memcpy_s(module_inst->cur_exception,
                    sizeof(module_inst->cur_exception),
                    sub_module_inst->cur_exception,
                    sizeof(sub_module_inst->cur_exception));
    }
}
#endif

#if WASM_ENABLE_THREAD_MGR != 0
#define CHECK_SUSPEND_FLAGS() do {                      \
    if (exec_env->suspend_flags.flags != 0) {           \
        if (exec_env->suspend_flags.flags & 0x01) {     \
            /* terminate current thread */              \
            return;                                     \
        }                                               \
        /* TODO: support suspend and breakpoint */      \
    }                                                   \
  } while (0)
#endif

#if WASM_ENABLE_OPCODE_COUNTER != 0
typedef struct OpcodeInfo {
    char *name;
    uint64 count;
} OpcodeInfo;

#define HANDLE_OPCODE(op) { #op, 0 }
DEFINE_GOTO_TABLE (OpcodeInfo, opcode_table);
#undef HANDLE_OPCODE

static void
wasm_interp_dump_op_count()
{
    uint32 i;
    uint64 total_count = 0;
    for (i = 0; i < WASM_OP_IMPDEP; i++)
        total_count += opcode_table[i].count;

    printf("total opcode count: %ld\n", total_count);
    for (i = 0; i < WASM_OP_IMPDEP; i++)
        if (opcode_table[i].count > 0)
            printf("\t\t%s count:\t\t%ld,\t\t%.2f%%\n",
                   opcode_table[i].name, opcode_table[i].count,
                   opcode_table[i].count * 100.0f / total_count);
}
#endif


#if WASM_ENABLE_LABELS_AS_VALUES != 0

/* #define HANDLE_OP(opcode) HANDLE_##opcode:printf(#opcode"\n");h_##opcode */
#if WASM_ENABLE_OPCODE_COUNTER != 0
#define HANDLE_OP(opcode) HANDLE_##opcode:opcode_table[opcode].count++;h_##opcode
#else
#define HANDLE_OP(opcode) HANDLE_##opcode
#endif
#if WASM_ENABLE_FAST_INTERP == 0
#define FETCH_OPCODE_AND_DISPATCH() goto *handle_table[*frame_ip++]
#else
#if WASM_ENABLE_ABS_LABEL_ADDR != 0
#define FETCH_OPCODE_AND_DISPATCH() do {                    \
    const void *p_label_addr = *(void**)frame_ip;           \
    frame_ip += sizeof(void*);                              \
    goto *p_label_addr;                                     \
  } while (0)
#else
#define FETCH_OPCODE_AND_DISPATCH() do {                    \
    const void *p_label_addr = label_base                   \
                               + *(int16*)frame_ip;         \
    frame_ip += sizeof(int16);                              \
    goto *p_label_addr;                                     \
  } while (0)
#endif
#endif
#define HANDLE_OP_END() FETCH_OPCODE_AND_DISPATCH()

#else   /* else of WASM_ENABLE_LABELS_AS_VALUES */

#define HANDLE_OP(opcode) case opcode
#define HANDLE_OP_END() continue

#endif  /* end of WASM_ENABLE_LABELS_AS_VALUES */

#if WASM_ENABLE_FAST_INTERP != 0
static void **global_handle_table;
#endif

static void
wasm_interp_call_func_bytecode(WASMModuleInstance *module,
                               WASMExecEnv *exec_env,
                               WASMFunctionInstance *cur_func,
                               WASMInterpFrame *prev_frame)
{
  WASMMemoryInstance *memory = module->default_memory;
  uint32 num_bytes_per_page = memory ? memory->num_bytes_per_page : 0;
  uint8 *global_data = module->global_data;
  uint32 linear_mem_size = memory ? num_bytes_per_page * memory->cur_page_count : 0;
  WASMTableInstance *table = module->default_table;
  WASMGlobalInstance *globals = module->globals, *global;
  uint8 opcode_IMPDEP = WASM_OP_IMPDEP;
  WASMInterpFrame *frame = NULL;
  /* Points to this special opcode so as to jump to the call_method_from_entry.  */
  register uint8  *frame_ip = &opcode_IMPDEP; /* cache of frame->ip */
  register uint32 *frame_lp = NULL;  /* cache of frame->lp */
#if WASM_ENABLE_ABS_LABEL_ADDR == 0
  register uint8 *label_base = &&HANDLE_WASM_OP_UNREACHABLE;  /* cache of label base addr */
#endif
  uint8 *frame_ip_end;
  uint32 cond, count, fidx, tidx, frame_size = 0;
  uint64 all_cell_num = 0;
  int16 addr1, addr2, addr_ret = 0;
  int32 didx, val;
  uint8 *maddr = NULL;
  uint32 local_idx, local_offset, global_idx;
  uint8 opcode, local_type, *global_addr;

#if WASM_ENABLE_LABELS_AS_VALUES != 0
  #define HANDLE_OPCODE(op) &&HANDLE_##op
  DEFINE_GOTO_TABLE (const void*, handle_table);
  #undef HANDLE_OPCODE
#if WASM_ENABLE_FAST_INTERP != 0
  if (exec_env == NULL) {
      global_handle_table = (void **)handle_table;
      return;
  }
#endif
#endif

#if WASM_ENABLE_LABELS_AS_VALUES == 0
  while (frame_ip < frame_ip_end) {
    opcode = *frame_ip++;
    switch (opcode) {
#else
      goto *handle_table[WASM_OP_IMPDEP];
#endif
      /* control instructions */
      HANDLE_OP (WASM_OP_UNREACHABLE):
        wasm_set_exception(module, "unreachable");
        goto got_exception;

      HANDLE_OP (WASM_OP_IF):
        cond = (uint32)POP_I32();

        if (cond == 0) {
          if (*(uint8**)frame_ip == NULL) {
            frame_ip = *(uint8**)(frame_ip + sizeof(uint8*));
          }
          else {
            frame_ip = *(uint8**)(frame_ip);
          }
        }
        else {
            frame_ip += sizeof(uint8*) * 2;
        }
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_ELSE):
        frame_ip = *(uint8**)(frame_ip);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR):
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
recover_br_info:
        RECOVER_BR_INFO();
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR_IF):
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
        cond = frame_lp[GET_OFFSET()];

        if (cond)
          goto recover_br_info;
        else
          SKIP_BR_INFO();

        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_BR_TABLE):
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
        count = read_uint32(frame_ip);
        didx = GET_OPERAND(uint32, 0);
        frame_ip += 2;

        if (!(didx >= 0 && (uint32)didx < count))
            didx = count;

        while (didx--)
          SKIP_BR_INFO();

        goto recover_br_info;

      HANDLE_OP (WASM_OP_RETURN):
        {
          uint32 ret_idx;
          WASMType *func_type;
          uint32 off, ret_offset;
          uint8 *ret_types;
          if (cur_func->is_import_func
#if WASM_ENABLE_MULTI_MODULE != 0
              && !cur_func->import_func_inst
#endif
          )
            func_type = cur_func->u.func_import->func_type;
          else
            func_type = cur_func->u.func->func_type;

          /* types of each return value */
          ret_types = func_type->types + func_type->param_count;
          ret_offset = prev_frame->ret_offset;

          for (ret_idx = 0, off = sizeof(int16) * (func_type->result_count - 1);
               ret_idx < func_type->result_count;
               ret_idx++, off -= sizeof(int16)) {
            if (ret_types[ret_idx] == VALUE_TYPE_I64
                || ret_types[ret_idx] == VALUE_TYPE_F64) {
              *((uint64 *)(prev_frame->lp + ret_offset)) =
                                           GET_OPERAND(uint64, off);
              ret_offset += 2;
            }
            else {
              prev_frame->lp[ret_offset] = GET_OPERAND(int32, off);
              ret_offset++;
            }
          }
        }
        goto return_func;

      HANDLE_OP (WASM_OP_CALL_INDIRECT):
#if WASM_ENABLE_TAIL_CALL != 0
      HANDLE_OP (WASM_OP_RETURN_CALL_INDIRECT):
#endif
        {
          WASMType *cur_type, *cur_func_type;
          WASMTableInstance *cur_table_inst;

#if WASM_ENABLE_TAIL_CALL != 0
          GET_OPCODE();
#endif
#if WASM_ENABLE_THREAD_MGR != 0
          CHECK_SUSPEND_FLAGS();
#endif

          tidx = read_uint32(frame_ip);
          val = GET_OPERAND(int32, 0);
          frame_ip += 2;

          if (tidx >= module->module->type_count) {
            wasm_set_exception(module, "type index is overflow");
            goto got_exception;
          }
          cur_type = module->module->types[tidx];

          /* careful, it might be a table in another module */
          cur_table_inst = table;
#if WASM_ENABLE_MULTI_MODULE != 0
          if (table->table_inst_linked) {
              cur_table_inst = table->table_inst_linked;
          }
#endif

          if (val < 0 || val >= (int32)cur_table_inst->cur_size) {
            wasm_set_exception(module, "undefined element");
            goto got_exception;
          }

          fidx = ((uint32*)cur_table_inst->base_addr)[val];
          if (fidx == (uint32)-1) {
            wasm_set_exception(module, "uninitialized element");
            goto got_exception;
          }

#if WASM_ENABLE_MULTI_MODULE != 0
          if (fidx >= module->function_count) {
            wasm_set_exception(module, "unknown function");
            goto got_exception;
          }
#endif

          /* always call module own functions */
          cur_func = module->functions + fidx;

          if (cur_func->is_import_func
#if WASM_ENABLE_MULTI_MODULE != 0
              && !cur_func->import_func_inst
#endif
          )
            cur_func_type = cur_func->u.func_import->func_type;
          else
            cur_func_type = cur_func->u.func->func_type;
          if (!wasm_type_equal(cur_type, cur_func_type)) {
            wasm_set_exception(module, "indirect call type mismatch");
            goto got_exception;
          }
#if WASM_ENABLE_TAIL_CALL != 0
          if (opcode == WASM_OP_RETURN_CALL_INDIRECT)
              goto call_func_from_return_call;
#endif
          goto call_func_from_interp;
        }

      /* parametric instructions */
      HANDLE_OP (WASM_OP_SELECT):
        {
          cond = frame_lp[GET_OFFSET()];
          addr1 = GET_OFFSET();
          addr2 = GET_OFFSET();
          addr_ret = GET_OFFSET();

          if (!cond) {
#if defined(BUILD_TARGET_X86_32)
            if (addr_ret != addr1)
              bh_memcpy_s(frame_lp + addr_ret, sizeof(int32),
                          frame_lp + addr1, sizeof(int32));
#else
            frame_lp[addr_ret] = frame_lp[addr1];
#endif
          }
          else {
#if defined(BUILD_TARGET_X86_32)
            if (addr_ret != addr2)
              bh_memcpy_s(frame_lp + addr_ret, sizeof(int32),
                          frame_lp + addr2, sizeof(int32));
#else
            frame_lp[addr_ret] = frame_lp[addr2];
#endif
          }
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SELECT_64):
        {
          cond = frame_lp[GET_OFFSET()];
          addr1 = GET_OFFSET();
          addr2 = GET_OFFSET();
          addr_ret = GET_OFFSET();

          if (!cond) {
#if defined(BUILD_TARGET_X86_32)
            if (addr_ret != addr1)
              bh_memcpy_s(frame_lp + addr_ret, sizeof(int64),
                          frame_lp + addr1, sizeof(int64));
#else
            *(int64*)(frame_lp + addr_ret) = *(int64*)(frame_lp + addr1);
#endif
          }
          else {
#if defined(BUILD_TARGET_X86_32)
            if (addr_ret != addr2)
              bh_memcpy_s(frame_lp + addr_ret, sizeof(int64),
                          frame_lp + addr2, sizeof(int64));
#else
            *(int64*)(frame_lp + addr_ret) = *(int64*)(frame_lp + addr2);
#endif
          }
          HANDLE_OP_END ();
        }

      /* variable instructions */
      HANDLE_OP (EXT_OP_SET_LOCAL_FAST):
      HANDLE_OP (EXT_OP_TEE_LOCAL_FAST):
        {
          local_offset = *frame_ip++;
          *(int32*)(frame_lp + local_offset) = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          HANDLE_OP_END ();
        }

      HANDLE_OP (EXT_OP_SET_LOCAL_FAST_I64):
      HANDLE_OP (EXT_OP_TEE_LOCAL_FAST_I64):
        {
          local_offset = *frame_ip++;
          PUT_I64_TO_ADDR((uint32*)(frame_lp + local_offset), GET_OPERAND(uint64, 0));
          frame_ip += 2;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_GET_GLOBAL):
        {
          global_idx = read_uint32(frame_ip);
          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
#if WASM_ENABLE_MULTI_MODULE == 0
          global_addr = global_data + global->data_offset;
#else
          global_addr = global->import_global_inst
                        ? global->import_module_inst->global_data
                          + global->import_global_inst->data_offset
                        : global_data + global->data_offset;
#endif
          addr_ret = GET_OFFSET();
          frame_lp[addr_ret] = *(uint32*)global_addr;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_GET_GLOBAL_64):
        {
          global_idx = read_uint32(frame_ip);
          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
#if WASM_ENABLE_MULTI_MODULE == 0
          global_addr = global_data + global->data_offset;
#else
          global_addr = global->import_global_inst
                        ? global->import_module_inst->global_data
                          + global->import_global_inst->data_offset
                        : global_data + global->data_offset;
#endif
          addr_ret = GET_OFFSET();
          *(uint64 *)(frame_lp + addr_ret) = GET_I64_FROM_ADDR((uint32*)global_addr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SET_GLOBAL):
        {
          global_idx = read_uint32(frame_ip);
          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
#if WASM_ENABLE_MULTI_MODULE == 0
          global_addr = global_data + global->data_offset;
#else
          global_addr = global->import_global_inst
                        ? global->import_module_inst->global_data
                          + global->import_global_inst->data_offset
                        : global_data + global->data_offset;
#endif
          addr1 = GET_OFFSET();
          *(int32*)global_addr = frame_lp[addr1];
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SET_GLOBAL_AUX_STACK):
        {
          global_idx = read_uint32(frame_ip);
          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
#if WASM_ENABLE_MULTI_MODULE == 0
          global_addr = global_data + global->data_offset;
#else
          global_addr = global->import_global_inst
                        ? global->import_module_inst->global_data
                          + global->import_global_inst->data_offset
                        : global_data + global->data_offset;
#endif
          addr1 = GET_OFFSET();
          if (frame_lp[addr1] < exec_env->aux_stack_boundary)
              goto out_of_bounds;
          *(int32*)global_addr = frame_lp[addr1];
#if WASM_ENABLE_MEMORY_PROFILING != 0
          if (module->module->aux_stack_top_global_index != (uint32)-1) {
              uint32 aux_stack_used =
                  module->module->aux_stack_bottom - *(uint32*)global_addr;
              if (aux_stack_used > module->max_aux_stack_used)
                  module->max_aux_stack_used = aux_stack_used;
          }
#endif
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_SET_GLOBAL_64):
        {
          global_idx = read_uint32(frame_ip);
          bh_assert(global_idx < module->global_count);
          global = globals + global_idx;
#if WASM_ENABLE_MULTI_MODULE == 0
          global_addr = global_data + global->data_offset;
#else
          global_addr = global->import_global_inst
                        ? global->import_module_inst->global_data
                          + global->import_global_inst->data_offset
                        : global_data + global->data_offset;
#endif
          addr1 = GET_OFFSET();
          PUT_I64_TO_ADDR((uint32*)global_addr, *(int64 *)(frame_lp + addr1));
          HANDLE_OP_END ();
        }

      /* memory load instructions */
      HANDLE_OP (WASM_OP_I32_LOAD):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(4);
          frame_lp[addr_ret] = LOAD_I32(maddr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(8);
          PUT_I64_TO_ADDR(frame_lp + addr_ret, LOAD_I64(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_LOAD8_S):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(1);
          frame_lp[addr_ret] = sign_ext_8_32(*(int8*)maddr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_LOAD8_U):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(1);
          frame_lp[addr_ret] = (uint32)(*(uint8*)maddr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_LOAD16_S):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(2);
          frame_lp[addr_ret] = sign_ext_16_32(LOAD_I16(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_LOAD16_U):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(2);
          frame_lp[addr_ret] = (uint32)(LOAD_U16(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD8_S):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(1);
          *(int64 *)(frame_lp + addr_ret) = sign_ext_8_64(*(int8*)maddr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD8_U):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(1);
          *(int64 *)(frame_lp + addr_ret) = (uint64)(*(uint8*)maddr);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD16_S):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(2);
          *(int64 *)(frame_lp + addr_ret) = sign_ext_16_64(LOAD_I16(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD16_U):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(2);
          *(int64 *)(frame_lp + addr_ret) = (uint64)(LOAD_U16(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD32_S):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(4);
          *(int64 *)(frame_lp + addr_ret) = sign_ext_32_64(LOAD_I32(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_LOAD32_U):
        {
          uint32 offset, addr;
          offset = read_uint32(frame_ip);
          addr = GET_OPERAND(uint32, 0);
          frame_ip += 2;
          addr_ret = GET_OFFSET();
          CHECK_MEMORY_OVERFLOW(4);
          *(int64 *)(frame_lp + addr_ret) = (uint64)(LOAD_U32(maddr));
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_STORE):
        {
          uint32 offset, addr;
          uint32 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint32, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(4);
          STORE_U32(maddr, sval);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_STORE8):
        {
          uint32 offset, addr;
          uint32 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint32, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(1);
          *(uint8*)maddr = (uint8)sval;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_STORE16):
        {
          uint32 offset, addr;
          uint32 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint32, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(2);
          STORE_U16(maddr, (uint16)sval);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_STORE):
        {
          uint32 offset, addr;
          uint64 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint64, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(8);
          STORE_I64(maddr, sval);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_STORE8):
        {
          uint32 offset, addr;
          uint64 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint64, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(1);
          *(uint8*)maddr = (uint8)sval;
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_STORE16):
        {
          uint32 offset, addr;
          uint64 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint64, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(2);
          STORE_U16(maddr, (uint16)sval);
          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I64_STORE32):
        {
          uint32 offset, addr;
          uint64 sval;
          offset = read_uint32(frame_ip);
          sval = GET_OPERAND(uint64, 0);
          addr = GET_OPERAND(uint32, 2);
          frame_ip += 4;
          CHECK_MEMORY_OVERFLOW(4);
          STORE_U32(maddr, (uint32)sval);
          HANDLE_OP_END ();
        }

      /* memory size and memory grow instructions */
      HANDLE_OP (WASM_OP_MEMORY_SIZE):
      {
        uint32 reserved;
        addr_ret = GET_OFFSET();
        frame_lp[addr_ret] = memory->cur_page_count;
        (void)reserved;
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_MEMORY_GROW):
      {
        uint32 reserved, delta, prev_page_count = memory->cur_page_count;

        addr1 = GET_OFFSET();
        addr_ret = GET_OFFSET();
        delta = (uint32)frame_lp[addr1];

        if (!wasm_enlarge_memory(module, delta)) {
          /* fail to memory.grow, return -1 */
          frame_lp[addr_ret] = -1;
        }
        else {
          /* success, return previous page count */
          frame_lp[addr_ret] = prev_page_count;
          /* update the memory instance ptr */
          memory = module->default_memory;
          linear_mem_size = num_bytes_per_page * memory->cur_page_count;
        }

        (void)reserved;
        HANDLE_OP_END ();
      }

      /* comparison instructions of i32 */
      HANDLE_OP (WASM_OP_I32_EQZ):
        DEF_OP_EQZ(int32, I32);
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
        DEF_OP_EQZ(int64, I64);
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

        b = frame_lp[GET_OFFSET()];
        a = frame_lp[GET_OFFSET()];
        addr_ret = GET_OFFSET();
        if (a == (int32)0x80000000 && b == -1) {
          wasm_set_exception(module, "integer overflow");
          goto got_exception;
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        frame_lp[addr_ret] = (a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_DIV_U):
      {
        uint32 a, b;

        addr1 = GET_OFFSET();
        addr2 = GET_OFFSET();
        addr_ret = GET_OFFSET();

        b = (uint32)frame_lp[addr1];
        a = (uint32)frame_lp[addr2];
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        frame_lp[addr_ret] = (a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_REM_S):
      {
        int32 a, b;

        addr1 = GET_OFFSET();
        addr2 = GET_OFFSET();
        addr_ret = GET_OFFSET();

        b = frame_lp[addr1];
        a = frame_lp[addr2];
        if (a == (int32)0x80000000 && b == -1) {
          frame_lp[addr_ret] = 0;
          HANDLE_OP_END ();
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        frame_lp[addr_ret] = (a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_REM_U):
      {
        uint32 a, b;

        addr1 = GET_OFFSET();
        addr2 = GET_OFFSET();
        addr_ret = GET_OFFSET();

        b = (uint32)frame_lp[addr1];
        a = (uint32)frame_lp[addr2];
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        frame_lp[addr_ret] = (a % b);
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

        b = (uint32)frame_lp[GET_OFFSET()];
        a = (uint32)frame_lp[GET_OFFSET()];
        frame_lp[GET_OFFSET()] = rotl32(a, b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I32_ROTR):
      {
        uint32 a, b;

        b = (uint32)frame_lp[GET_OFFSET()];
        a = (uint32)frame_lp[GET_OFFSET()];
        frame_lp[GET_OFFSET()] = rotr32(a, b);
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

        b = *(int64*)(frame_lp + GET_OFFSET());
        a = *(int64*)(frame_lp + GET_OFFSET());
        if (a == (int64)0x8000000000000000LL && b == -1) {
          wasm_set_exception(module, "integer overflow");
          goto got_exception;
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        *(int64*)(frame_lp + GET_OFFSET()) = (a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_DIV_U):
      {
        uint64 a, b;

        b = *(uint64*)(frame_lp + GET_OFFSET());
        a = *(uint64*)(frame_lp + GET_OFFSET());
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        *(uint64*)(frame_lp + GET_OFFSET()) = (a / b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_REM_S):
      {
        int64 a, b;

        b = *(int64*)(frame_lp + GET_OFFSET());
        a = *(int64*)(frame_lp + GET_OFFSET());
        if (a == (int64)0x8000000000000000LL && b == -1) {
          *(int64*)(frame_lp + GET_OFFSET()) = 0;
          HANDLE_OP_END ();
        }
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        *(int64*)(frame_lp + GET_OFFSET()) = (a % b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_REM_U):
      {
        uint64 a, b;

        b = *(uint64*)(frame_lp + GET_OFFSET());
        a = *(uint64*)(frame_lp + GET_OFFSET());
        if (b == 0) {
          wasm_set_exception(module, "integer divide by zero");
          goto got_exception;
        }
        *(uint64*)(frame_lp + GET_OFFSET()) = (a % b);
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

        b = *(int64*)(frame_lp + GET_OFFSET());
        a = *(int64*)(frame_lp + GET_OFFSET());
        *(int64*)(frame_lp + GET_OFFSET()) = rotl64(a, b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_I64_ROTR):
      {
        uint64 a, b;

        b = *(uint64*)(frame_lp + GET_OFFSET());
        a = *(uint64*)(frame_lp + GET_OFFSET());
        *(uint64*)(frame_lp + GET_OFFSET()) = rotr64(a, b);
        HANDLE_OP_END ();
      }

      /* numberic instructions of f32 */
      HANDLE_OP (WASM_OP_F32_ABS):
        DEF_OP_MATH(float32, F32, fabs);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F32_NEG):
      {
          int32 i32 = (int32)frame_lp[GET_OFFSET()];
          addr_ret = GET_OFFSET();
          int32 sign_bit = i32 & (1 << 31);
          if (sign_bit)
              frame_lp[addr_ret] = i32 & ~(1 << 31);
          else
              frame_lp[addr_ret] = (uint32)(i32 | (1 << 31));
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

        b = *(float32*)(frame_lp + GET_OFFSET());
        a = *(float32*)(frame_lp + GET_OFFSET());

        if (isnan(a))
            *(float32*)(frame_lp + GET_OFFSET()) = a;
        else if (isnan(b))
            *(float32*)(frame_lp + GET_OFFSET()) = b;
        else
            *(float32*)(frame_lp + GET_OFFSET()) = wa_fmin(a, b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F32_MAX):
      {
        float32 a, b;

        b = *(float32*)(frame_lp + GET_OFFSET());
        a = *(float32*)(frame_lp + GET_OFFSET());

        if (isnan(a))
            *(float32*)(frame_lp + GET_OFFSET()) = a;
        else if (isnan(b))
            *(float32*)(frame_lp + GET_OFFSET()) = b;
        else
            *(float32*)(frame_lp + GET_OFFSET()) = wa_fmax(a, b);
        HANDLE_OP_END ();
      }

      HANDLE_OP (WASM_OP_F32_COPYSIGN):
      {
        float32 a, b;

        b = *(float32*)(frame_lp + GET_OFFSET());
        a = *(float32*)(frame_lp + GET_OFFSET());
        *(float32*)(frame_lp + GET_OFFSET()) = (signbit(b) ? -fabs(a) : fabs(a));
        HANDLE_OP_END ();
      }

      /* numberic instructions of f64 */
      HANDLE_OP (WASM_OP_F64_ABS):
        DEF_OP_MATH(float64, F64, fabs);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_F64_NEG):
      {
          int64 i64 = *(int64*)(frame_lp + GET_OFFSET());
          int64 sign_bit = i64 & (((int64)1) << 63);
          if (sign_bit)
              *(int64*)(frame_lp + GET_OFFSET()) = (uint64)i64 & ~(((uint64)1) << 63);
          else
              *(int64*)(frame_lp + GET_OFFSET()) = (uint64)i64 | (((uint64)1) << 63);
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
        DEF_OP_TRUNC_F32(-2147483904.0f, 2147483648.0f, true, true);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_U_F32):
        DEF_OP_TRUNC_F32(-1.0f, 4294967296.0f, true, false);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_S_F64):
        DEF_OP_TRUNC_F64(-2147483649.0, 2147483648.0, true, true);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_TRUNC_U_F64):
        DEF_OP_TRUNC_F64(-1.0, 4294967296.0, true, false);
        HANDLE_OP_END ();

      /* conversions of i64 */
      HANDLE_OP (WASM_OP_I64_EXTEND_S_I32):
        DEF_OP_CONVERT(int64, I64, int32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EXTEND_U_I32):
        DEF_OP_CONVERT(int64, I64, uint32, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_S_F32):
        DEF_OP_TRUNC_F32(-9223373136366403584.0f,
                         9223372036854775808.0f, false, true);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_U_F32):
        DEF_OP_TRUNC_F32(-1.0f, 18446744073709551616.0f,
                         false, false);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_S_F64):
        DEF_OP_TRUNC_F64(-9223372036854777856.0,
                         9223372036854775808.0, false, true);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_TRUNC_U_F64):
        DEF_OP_TRUNC_F64(-1.0, 18446744073709551616.0,
                         false, false);
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
        DEF_OP_REINTERPRET(float32);
        HANDLE_OP_END ();
      HANDLE_OP (WASM_OP_I64_REINTERPRET_F64):
        DEF_OP_REINTERPRET(float64);
        HANDLE_OP_END ();
      HANDLE_OP (WASM_OP_F32_REINTERPRET_I32):
        DEF_OP_REINTERPRET(int32);
        HANDLE_OP_END ();
      HANDLE_OP (WASM_OP_F64_REINTERPRET_I64):
        DEF_OP_REINTERPRET(int64);
        HANDLE_OP_END ();

      HANDLE_OP (EXT_OP_COPY_STACK_TOP):
        addr1 = GET_OFFSET();
        addr2 = GET_OFFSET();
        frame_lp[addr2] = frame_lp[addr1];
        HANDLE_OP_END ();

      HANDLE_OP (EXT_OP_COPY_STACK_TOP_I64):
        addr1 = GET_OFFSET();
        addr2 = GET_OFFSET();
        *(uint64*)(frame_lp + addr2) = *(uint64*)(frame_lp + addr1);
        HANDLE_OP_END ();

      HANDLE_OP (EXT_OP_COPY_STACK_VALUES):
      {
        uint32 values_count, total_cell;
        uint8 *cells;
        int16 *src_offsets = NULL;
        uint16 *dst_offsets = NULL;

        /* read values_count */
        values_count = *(uint32*)frame_ip;
        frame_ip += sizeof(values_count);
        /* read total cell num */
        total_cell = *(uint32*)frame_ip;
        frame_ip += sizeof(total_cell);
        /* cells */
        cells = (uint8 *)frame_ip;
        frame_ip += values_count * sizeof(uint8);
        /* src offsets */
        src_offsets = (int16 *)frame_ip;
        frame_ip += values_count * sizeof(int16);
        /* dst offsets */
        dst_offsets = (uint16*)frame_ip;
        frame_ip += values_count * sizeof(uint16);

        if (!copy_stack_values(module, frame_lp,
                               values_count, total_cell,
                               cells, src_offsets,
                               dst_offsets))
          goto got_exception;

        HANDLE_OP_END ();
      }
      HANDLE_OP (WASM_OP_SET_LOCAL):
      HANDLE_OP (WASM_OP_TEE_LOCAL):
        {
          GET_LOCAL_INDEX_TYPE_AND_OFFSET();
          addr1 = GET_OFFSET();

          if (local_type == VALUE_TYPE_I32
              || local_type == VALUE_TYPE_F32) {
            *(int32*)(frame_lp + local_offset) = frame_lp[addr1];
          }
          else if (local_type == VALUE_TYPE_I64
              || local_type == VALUE_TYPE_F64) {
            PUT_I64_TO_ADDR((uint32*)(frame_lp + local_offset),
                  GET_I64_FROM_ADDR(frame_lp + addr1));
          }
          else {
            wasm_set_exception(module, "invalid local type");
            goto got_exception;
          }

          HANDLE_OP_END ();
        }

      HANDLE_OP (WASM_OP_I32_EXTEND8_S):
        DEF_OP_CONVERT(int32, I32, int8, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I32_EXTEND16_S):
        DEF_OP_CONVERT(int32, I32, int16, I32);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EXTEND8_S):
        DEF_OP_CONVERT(int64, I64, int8, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EXTEND16_S):
        DEF_OP_CONVERT(int64, I64, int16, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_I64_EXTEND32_S):
        DEF_OP_CONVERT(int64, I64, int32, I64);
        HANDLE_OP_END ();

      HANDLE_OP (WASM_OP_MISC_PREFIX):
      {
        GET_OPCODE();
        switch (opcode)
        {
        case WASM_OP_I32_TRUNC_SAT_S_F32:
          DEF_OP_TRUNC_SAT_F32(-2147483904.0f, 2147483648.0f,
                               true, true);
          break;
        case WASM_OP_I32_TRUNC_SAT_U_F32:
          DEF_OP_TRUNC_SAT_F32(-1.0f, 4294967296.0f,
                               true, false);
          break;
        case WASM_OP_I32_TRUNC_SAT_S_F64:
          DEF_OP_TRUNC_SAT_F64(-2147483649.0, 2147483648.0,
                               true, true);
          break;
        case WASM_OP_I32_TRUNC_SAT_U_F64:
          DEF_OP_TRUNC_SAT_F64(-1.0, 4294967296.0,
                               true, false);
          break;
        case WASM_OP_I64_TRUNC_SAT_S_F32:
          DEF_OP_TRUNC_SAT_F32(-9223373136366403584.0f, 9223372036854775808.0f,
                               false, true);
          break;
        case WASM_OP_I64_TRUNC_SAT_U_F32:
          DEF_OP_TRUNC_SAT_F32(-1.0f, 18446744073709551616.0f,
                               false, false);
          break;
        case WASM_OP_I64_TRUNC_SAT_S_F64:
          DEF_OP_TRUNC_SAT_F64(-9223372036854777856.0, 9223372036854775808.0,
                               false, true);
          break;
        case WASM_OP_I64_TRUNC_SAT_U_F64:
          DEF_OP_TRUNC_SAT_F64(-1.0, 18446744073709551616.0,
                               false, false);
          break;
#if WASM_ENABLE_BULK_MEMORY != 0
        case WASM_OP_MEMORY_INIT:
        {
          uint32 addr, segment;
          uint64 bytes, offset, seg_len;
          uint8* data;

          segment = read_uint32(frame_ip);

          bytes = (uint64)POP_I32();
          offset = (uint64)POP_I32();
          addr = POP_I32();

          CHECK_BULK_MEMORY_OVERFLOW(addr, bytes, maddr);

          seg_len = (uint64)module->module->data_segments[segment]->data_length;
          data = module->module->data_segments[segment]->data;
          if (offset + bytes > seg_len)
            goto out_of_bounds;

          bh_memcpy_s(maddr, linear_mem_size - addr,
                      data + offset, bytes);
          break;
        }
        case WASM_OP_DATA_DROP:
        {
          uint32 segment;

          segment = read_uint32(frame_ip);

          module->module->data_segments[segment]->data_length = 0;

          break;
        }
        case WASM_OP_MEMORY_COPY:
        {
          uint32 dst, src, len;
          uint8 *mdst, *msrc;

          len = POP_I32();
          src = POP_I32();
          dst = POP_I32();

          CHECK_BULK_MEMORY_OVERFLOW(src, len, msrc);
          CHECK_BULK_MEMORY_OVERFLOW(dst, len, mdst);

          /* allowing the destination and source to overlap */
          bh_memmove_s(mdst, linear_mem_size - dst,
                       msrc, len);

          break;
        }
        case WASM_OP_MEMORY_FILL:
        {
          uint32 dst, len;
          uint8 val, *mdst;

          len = POP_I32();
          val = POP_I32();
          dst = POP_I32();

          CHECK_BULK_MEMORY_OVERFLOW(dst, len, mdst);

          memset(mdst, val, len);

          break;
        }
#endif /* WASM_ENABLE_BULK_MEMORY */
        default:
          wasm_set_exception(module, "unsupported opcode");
            goto got_exception;
          break;
        }
        HANDLE_OP_END ();
      }

#if WASM_ENABLE_SHARED_MEMORY != 0
      HANDLE_OP (WASM_OP_ATOMIC_PREFIX):
      {
        uint32 offset, addr;

        GET_OPCODE();

        offset = read_uint32(frame_ip);

        switch (opcode) {
          case WASM_OP_ATOMIC_NOTIFY:
          {
            uint32 count, ret;

            count = POP_I32();
            addr = POP_I32();
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
            CHECK_ATOMIC_MEMORY_ACCESS(4);

            ret = wasm_runtime_atomic_notify((WASMModuleInstanceCommon*)module,
                                             maddr, count);
            bh_assert((int32)ret >= 0);

            PUSH_I32(ret);
            break;
          }
          case WASM_OP_ATOMIC_WAIT32:
          {
            uint64 timeout;
            uint32 expect, addr, ret;

            timeout = POP_I64();
            expect = POP_I32();
            addr = POP_I32();
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
            CHECK_ATOMIC_MEMORY_ACCESS(4);

            ret = wasm_runtime_atomic_wait((WASMModuleInstanceCommon*)module, maddr,
                                           (uint64)expect, timeout, false);
            if (ret == (uint32)-1)
              goto got_exception;

            PUSH_I32(ret);
            break;
          }
          case WASM_OP_ATOMIC_WAIT64:
          {
            uint64 timeout, expect;
            uint32 ret;

            timeout = POP_I64();
            expect = POP_I64();
            addr = POP_I32();
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
            CHECK_ATOMIC_MEMORY_ACCESS(8);

            ret = wasm_runtime_atomic_wait((WASMModuleInstanceCommon*)module,
                                           maddr, expect, timeout, true);
            if (ret == (uint32)-1)
              goto got_exception;

            PUSH_I32(ret);
            break;
          }

          case WASM_OP_ATOMIC_I32_LOAD:
          case WASM_OP_ATOMIC_I32_LOAD8_U:
          case WASM_OP_ATOMIC_I32_LOAD16_U:
          {
            uint32 readv;

            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_I32_LOAD8_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);
              os_mutex_lock(&memory->mem_lock);
              readv = (uint32)(*(uint8*)maddr);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_I32_LOAD16_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);
              os_mutex_lock(&memory->mem_lock);
              readv = (uint32)LOAD_U16(maddr);
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);
              os_mutex_lock(&memory->mem_lock);
              readv = LOAD_I32(maddr);
              os_mutex_unlock(&memory->mem_lock);
            }

            PUSH_I32(readv);
            break;
          }

          case WASM_OP_ATOMIC_I64_LOAD:
          case WASM_OP_ATOMIC_I64_LOAD8_U:
          case WASM_OP_ATOMIC_I64_LOAD16_U:
          case WASM_OP_ATOMIC_I64_LOAD32_U:
          {
            uint64 readv;

            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_I64_LOAD8_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);
              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)(*(uint8*)maddr);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_I64_LOAD16_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);
              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)LOAD_U16(maddr);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_I64_LOAD32_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);
              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)LOAD_U32(maddr);
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(8);
              os_mutex_lock(&memory->mem_lock);
              readv = LOAD_I64(maddr);
              os_mutex_unlock(&memory->mem_lock);
            }

            PUSH_I64(readv);
            break;
          }
          case WASM_OP_ATOMIC_I32_STORE:
          case WASM_OP_ATOMIC_I32_STORE8:
          case WASM_OP_ATOMIC_I32_STORE16:
          {
            uint32 sval;

            sval = (uint32)POP_I32();
            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_I32_STORE8) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);
              os_mutex_lock(&memory->mem_lock);
              *(uint8*)maddr = (uint8)sval;
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_I32_STORE16) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);
              os_mutex_lock(&memory->mem_lock);
              STORE_U16(maddr, (uint16)sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);
              os_mutex_lock(&memory->mem_lock);
              STORE_U32(maddr, sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            break;
          }

          case WASM_OP_ATOMIC_I64_STORE:
          case WASM_OP_ATOMIC_I64_STORE8:
          case WASM_OP_ATOMIC_I64_STORE16:
          case WASM_OP_ATOMIC_I64_STORE32:
          {
            uint64 sval;

            sval = (uint64)POP_I64();
            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_I64_STORE8) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);
              os_mutex_lock(&memory->mem_lock);
              *(uint8*)maddr = (uint8)sval;
              os_mutex_unlock(&memory->mem_lock);
            }
            else if(opcode == WASM_OP_ATOMIC_I64_STORE16) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);
              os_mutex_lock(&memory->mem_lock);
              STORE_U16(maddr, (uint16)sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_I64_STORE32) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);
              os_mutex_lock(&memory->mem_lock);
              STORE_U32(maddr, (uint32)sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(8);
              os_mutex_lock(&memory->mem_lock);
              STORE_I64(maddr, sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            break;
          }

          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG:
          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG8_U:
          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG16_U:
          {
            uint32 readv, sval, expect;

            sval = POP_I32();
            expect = POP_I32();
            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_RMW_I32_CMPXCHG8_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint32)(*(uint8*)maddr);
              if (readv == expect)
                *(uint8*)maddr = (uint8)(sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_RMW_I32_CMPXCHG16_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint32)LOAD_U16(maddr);
              if (readv == expect)
                STORE_U16(maddr, (uint16)(sval));
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);

              os_mutex_lock(&memory->mem_lock);
              readv = LOAD_I32(maddr);
              if (readv == expect)
                STORE_U32(maddr, sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            PUSH_I32(readv);
            break;
          }
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG:
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG8_U:
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG16_U:
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG32_U:
          {
            uint64 readv, sval, expect;

            sval = (uint64)POP_I64();
            expect = (uint64)POP_I64();
            addr = POP_I32();

            if (opcode == WASM_OP_ATOMIC_RMW_I64_CMPXCHG8_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(1);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)(*(uint8*)maddr);
              if (readv == expect)
                *(uint8*)maddr = (uint8)(sval);
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_RMW_I64_CMPXCHG16_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(2);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)LOAD_U16(maddr);
              if (readv == expect)
                STORE_U16(maddr, (uint16)(sval));
              os_mutex_unlock(&memory->mem_lock);
            }
            else if (opcode == WASM_OP_ATOMIC_RMW_I64_CMPXCHG32_U) {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(4);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)LOAD_U32(maddr);
              if (readv == expect)
                STORE_U32(maddr, (uint32)(sval));
              os_mutex_unlock(&memory->mem_lock);
            }
            else {
              CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
              CHECK_ATOMIC_MEMORY_ACCESS(8);

              os_mutex_lock(&memory->mem_lock);
              readv = (uint64)LOAD_I64(maddr);
              if (readv == expect) {
                STORE_I64(maddr, sval);
              }
              os_mutex_unlock(&memory->mem_lock);
            }
            PUSH_I64(readv);
            break;
          }

          DEF_ATOMIC_RMW_OPCODE(ADD, +);
          DEF_ATOMIC_RMW_OPCODE(SUB, -);
          DEF_ATOMIC_RMW_OPCODE(AND, &);
          DEF_ATOMIC_RMW_OPCODE(OR,  |);
          DEF_ATOMIC_RMW_OPCODE(XOR, ^);
          /* xchg, ignore the read value, and store the given value:
            readv * 0 + sval */
          DEF_ATOMIC_RMW_OPCODE(XCHG, *0 +);
        }

        HANDLE_OP_END ();
      }
#endif

      HANDLE_OP (WASM_OP_IMPDEP):
        frame = prev_frame;
        frame_ip = frame->ip;
        goto call_func_from_entry;

      HANDLE_OP (WASM_OP_CALL):
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
        fidx = read_uint32(frame_ip);
#if WASM_ENABLE_MULTI_MODULE != 0
        if (fidx >= module->function_count) {
          wasm_set_exception(module, "unknown function");
          goto got_exception;
        }
#endif
        cur_func = module->functions + fidx;
        goto call_func_from_interp;

#if WASM_ENABLE_TAIL_CALL != 0
    HANDLE_OP (WASM_OP_RETURN_CALL):
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
        fidx = read_uint32(frame_ip);
#if WASM_ENABLE_MULTI_MODULE != 0
        if (fidx >= module->function_count) {
          wasm_set_exception(module, "unknown function");
          goto got_exception;
        }
#endif
        cur_func = module->functions + fidx;
        goto call_func_from_return_call;
#endif /* WASM_ENABLE_TAIL_CALL */

#if WASM_ENABLE_LABELS_AS_VALUES == 0
      default:
        wasm_set_exception(module, "unsupported opcode");
        goto got_exception;
    }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES != 0
    HANDLE_OP (WASM_OP_UNUSED_0x06):
    HANDLE_OP (WASM_OP_UNUSED_0x07):
    HANDLE_OP (WASM_OP_UNUSED_0x08):
    HANDLE_OP (WASM_OP_UNUSED_0x09):
    HANDLE_OP (WASM_OP_UNUSED_0x0a):
#if WASM_ENABLE_TAIL_CALL == 0
    HANDLE_OP (WASM_OP_RETURN_CALL):
    HANDLE_OP (WASM_OP_RETURN_CALL_INDIRECT):
#endif
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
    /* optimized op code */
    HANDLE_OP (WASM_OP_F32_STORE):
    HANDLE_OP (WASM_OP_F64_STORE):
    HANDLE_OP (WASM_OP_F32_LOAD):
    HANDLE_OP (WASM_OP_F64_LOAD):
    HANDLE_OP (EXT_OP_GET_LOCAL_FAST):
    HANDLE_OP (WASM_OP_GET_LOCAL):
    HANDLE_OP (WASM_OP_F64_CONST):
    HANDLE_OP (WASM_OP_I64_CONST):
    HANDLE_OP (WASM_OP_F32_CONST):
    HANDLE_OP (WASM_OP_I32_CONST):
    HANDLE_OP (WASM_OP_DROP):
    HANDLE_OP (WASM_OP_DROP_64):
    HANDLE_OP (WASM_OP_BLOCK):
    HANDLE_OP (WASM_OP_LOOP):
    HANDLE_OP (WASM_OP_END):
    HANDLE_OP (WASM_OP_NOP):
    HANDLE_OP (EXT_OP_BLOCK):
    HANDLE_OP (EXT_OP_LOOP):
    HANDLE_OP (EXT_OP_IF):
    {
      wasm_set_exception(module, "unsupported opcode");
      goto got_exception;
    }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES == 0
    continue;
#else
    FETCH_OPCODE_AND_DISPATCH ();
#endif

#if WASM_ENABLE_TAIL_CALL !=0
  call_func_from_return_call:
  {
      uint32 *lp_base;
      uint32 *lp;
      int i;

      if (!(lp_base = lp = wasm_runtime_malloc(cur_func->param_cell_num * sizeof(uint32)))) {
          wasm_set_exception(module, "allocate memory failed");
          goto got_exception;
      }
      for (i = 0; i < cur_func->param_count; i++) {
          if (cur_func->param_types[i] == VALUE_TYPE_I64
              || cur_func->param_types[i] == VALUE_TYPE_F64) {
              *(int64*)(lp) =
                GET_OPERAND(int64, (2 * (cur_func->param_count - i - 1)));
              lp += 2;
          }
          else {
              *(lp) = GET_OPERAND(int32, (2 * (cur_func->param_count - i - 1)));
              lp ++;
          }
      }
      frame->lp = frame->operand + cur_func->const_cell_num;
      bh_memcpy_s(frame->lp, (lp - lp_base) * sizeof(uint32),
                  lp_base, (lp - lp_base) * sizeof(uint32));
      wasm_runtime_free(lp_base);
      FREE_FRAME(exec_env, frame);
      frame_ip += cur_func->param_count * sizeof(int16);
      wasm_exec_env_set_cur_frame(exec_env,
                                  (WASMRuntimeFrame *)prev_frame);
      goto call_func_from_entry;
  }
#endif /* WASM_ENABLE_TAIL_CALL */
  call_func_from_interp:
    /* Only do the copy when it's called from interpreter.  */
    {
      WASMInterpFrame *outs_area = wasm_exec_env_wasm_stack_top(exec_env);
      outs_area->lp = outs_area->operand + cur_func->const_cell_num;
      for (int i = 0; i < cur_func->param_count; i++) {
        if (cur_func->param_types[i] == VALUE_TYPE_I64
          || cur_func->param_types[i] == VALUE_TYPE_F64) {
            *(int64*)(outs_area->lp) =
              GET_OPERAND(int64, (2 * (cur_func->param_count - i - 1)));
            outs_area->lp += 2;
        } else {
          *(outs_area->lp) = GET_OPERAND(int32, (2 * (cur_func->param_count - i - 1)));
          outs_area->lp ++;
        }
      }
      frame_ip += cur_func->param_count * sizeof(int16);
      if (cur_func->ret_cell_num != 0) {
        /* Get the first return value's offset. Since loader emit all return
         * values' offset so we must skip remain return values' offsets.
         */
        WASMType *func_type;
        if (cur_func->is_import_func
#if WASM_ENABLE_MULTI_MODULE != 0
            && !cur_func->import_func_inst
#endif
        )
          func_type = cur_func->u.func_import->func_type;
        else
          func_type = cur_func->u.func->func_type;
        frame->ret_offset = GET_OFFSET();
        frame_ip += 2 * (func_type->result_count - 1);
      }
      SYNC_ALL_TO_FRAME();
      prev_frame = frame;
    }

  call_func_from_entry:
    {
      if (cur_func->is_import_func) {
#if WASM_ENABLE_MULTI_MODULE != 0
          if (cur_func->import_func_inst) {
              wasm_interp_call_func_import(module, exec_env, cur_func,
                                           prev_frame);
          }
          else
#endif
          {
              wasm_interp_call_func_native(module, exec_env, cur_func,
                                           prev_frame);
          }

          prev_frame = frame->prev_frame;
          cur_func = frame->function;
          UPDATE_ALL_FROM_FRAME();

          memory = module->default_memory;
          if (wasm_get_exception(module))
              goto got_exception;
      }
      else {
        WASMFunction *cur_wasm_func = cur_func->u.func;

        all_cell_num = (uint64)cur_func->param_cell_num
                       + (uint64)cur_func->local_cell_num
                       + (uint64)cur_func->const_cell_num
                       + (uint64)cur_wasm_func->max_stack_cell_num;
        if (all_cell_num >= UINT32_MAX) {
            wasm_set_exception(module, "stack overflow");
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

        frame_lp = frame->lp = frame->operand + cur_wasm_func->const_cell_num;

        /* Initialize the consts */
        bh_memcpy_s(frame->operand, all_cell_num * 4,
                    cur_wasm_func->consts, cur_wasm_func->const_cell_num * 4);

        /* Initialize the local varialbes */
        memset(frame_lp + cur_func->param_cell_num, 0,
               (uint32)(cur_func->local_cell_num * 4));

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

  (void)frame_ip_end;

#if WASM_ENABLE_SHARED_MEMORY != 0
  unaligned_atomic:
    wasm_set_exception(module, "unaligned atomic");
    goto got_exception;
#endif

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

#if WASM_ENABLE_FAST_INTERP != 0
void **
wasm_interp_get_handle_table()
{
    WASMModuleInstance module;
    memset(&module, 0, sizeof(WASMModuleInstance));
    wasm_interp_call_func_bytecode(&module, NULL, NULL, NULL);
    return global_handle_table;
}
#endif

void
wasm_interp_call_wasm(WASMModuleInstance *module_inst,
                      WASMExecEnv *exec_env,
                      WASMFunctionInstance *function,
                      uint32 argc, uint32 argv[])
{
    WASMRuntimeFrame *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMInterpFrame *frame, *outs_area;

    /* Allocate sufficient cells for all kinds of return values.  */
    unsigned all_cell_num = function->ret_cell_num > 2 ?
                            function->ret_cell_num : 2, i;
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

    if ((uint8*)&prev_frame < exec_env->native_stack_boundary) {
        wasm_set_exception((WASMModuleInstance*)exec_env->module_inst,
                           "native stack overflow");
        return;
    }

    if (!(frame = ALLOC_FRAME(exec_env, frame_size, (WASMInterpFrame*)prev_frame)))
        return;

    outs_area = wasm_exec_env_wasm_stack_top(exec_env);
    frame->function = NULL;
    frame->ip = NULL;
    /* There is no local variable. */
    frame->lp = frame->operand + 0;
    frame->ret_offset = 0;

    if (argc > 0)
        word_copy(outs_area->operand + function->const_cell_num, argv, argc);

    wasm_exec_env_set_cur_frame(exec_env, frame);

    if (function->is_import_func) {
#if WASM_ENABLE_MULTI_MODULE != 0
        if (function->import_module_inst) {
            LOG_DEBUG("it is a function of a sub module");
            wasm_interp_call_func_import(module_inst, exec_env,
                                         function, frame);
        }
        else
#endif
        {
            LOG_DEBUG("it is an native function");
            wasm_interp_call_func_native(module_inst, exec_env,
                                         function, frame);
        }
    }
    else {
        wasm_interp_call_func_bytecode(module_inst, exec_env, function, frame);
    }

    /* Output the return value to the caller */
    if (!wasm_get_exception(module_inst)) {
        for (i = 0; i < function->ret_cell_num; i++)
            argv[i] = *(frame->lp + i);
    }
    else {
#if WASM_ENABLE_CUSTOM_NAME_SECTION != 0
        wasm_interp_dump_call_stack(exec_env);
#endif
    }

    wasm_exec_env_set_cur_frame(exec_env, prev_frame);
    FREE_FRAME(exec_env, frame);
#if WASM_ENABLE_OPCODE_COUNTER != 0
    wasm_interp_dump_op_count();
#endif
}
