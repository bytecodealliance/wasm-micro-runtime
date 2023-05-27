/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_interp.h"
#include "bh_log.h"
#include "wasm_runtime.h"
#include "wasm_opcode.h"
#include "wasm_loader.h"
#include "wasm_memory.h"
#include "../common/wasm_exec_env.h"
#if WASM_ENABLE_GC != 0
#include "../common/gc/gc_object.h"
#include "mem_alloc.h"
#endif
#if WASM_ENABLE_SHARED_MEMORY != 0
#include "../common/wasm_shared_memory.h"
#endif

typedef int32 CellType_I32;
typedef int64 CellType_I64;
typedef float32 CellType_F32;
typedef float64 CellType_F64;

#if WASM_ENABLE_THREAD_MGR == 0
#define get_linear_mem_size() linear_mem_size
#else
/**
 * Load memory data size in each time boundary check in
 * multi-threading mode since it may be changed by other
 * threads in memory.grow
 */
#define get_linear_mem_size() memory->memory_data_size
#endif

#if !defined(OS_ENABLE_HW_BOUND_CHECK) \
    || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0
#define CHECK_MEMORY_OVERFLOW(bytes)                             \
    do {                                                         \
        uint64 offset1 = (uint64)offset + (uint64)addr;          \
        if (offset1 + bytes <= (uint64)get_linear_mem_size())    \
            /* If offset1 is in valid range, maddr must also     \
                be in valid range, no need to check it again. */ \
            maddr = memory->memory_data + offset1;               \
        else                                                     \
            goto out_of_bounds;                                  \
    } while (0)

#define CHECK_BULK_MEMORY_OVERFLOW(start, bytes, maddr) \
    do {                                                \
        uint64 offset1 = (uint32)(start);               \
        if (offset1 + bytes <= get_linear_mem_size())   \
            /* App heap space is not valid space for    \
               bulk memory operation */                 \
            maddr = memory->memory_data + offset1;      \
        else                                            \
            goto out_of_bounds;                         \
    } while (0)
#else
#define CHECK_MEMORY_OVERFLOW(bytes)                    \
    do {                                                \
        uint64 offset1 = (uint64)offset + (uint64)addr; \
        maddr = memory->memory_data + offset1;          \
    } while (0)

#define CHECK_BULK_MEMORY_OVERFLOW(start, bytes, maddr) \
    do {                                                \
        maddr = memory->memory_data + (uint32)(start);  \
    } while (0)
#endif /* !defined(OS_ENABLE_HW_BOUND_CHECK) \
          || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0 */

#define CHECK_ATOMIC_MEMORY_ACCESS(align)          \
    do {                                           \
        if (((uintptr_t)maddr & (align - 1)) != 0) \
            goto unaligned_atomic;                 \
    } while (0)

static inline uint32
rotl32(uint32 n, uint32 c)
{
    const uint32 mask = (31);
    c = c % 32;
    c &= mask;
    return (n << c) | (n >> ((0 - c) & mask));
}

static inline uint32
rotr32(uint32 n, uint32 c)
{
    const uint32 mask = (31);
    c = c % 32;
    c &= mask;
    return (n >> c) | (n << ((0 - c) & mask));
}

static inline uint64
rotl64(uint64 n, uint64 c)
{
    const uint64 mask = (63);
    c = c % 64;
    c &= mask;
    return (n << c) | (n >> ((0 - c) & mask));
}

static inline uint64
rotr64(uint64 n, uint64 c)
{
    const uint64 mask = (63);
    c = c % 64;
    c &= mask;
    return (n >> c) | (n << ((0 - c) & mask));
}

static inline float32
f32_min(float32 a, float32 b)
{
    if (isnan(a) || isnan(b))
        return NAN;
    else if (a == 0 && a == b)
        return signbit(a) ? a : b;
    else
        return a > b ? b : a;
}

static inline float32
f32_max(float32 a, float32 b)
{
    if (isnan(a) || isnan(b))
        return NAN;
    else if (a == 0 && a == b)
        return signbit(a) ? b : a;
    else
        return a > b ? a : b;
}

static inline float64
f64_min(float64 a, float64 b)
{
    if (isnan(a) || isnan(b))
        return NAN;
    else if (a == 0 && a == b)
        return signbit(a) ? a : b;
    else
        return a > b ? b : a;
}

static inline float64
f64_max(float64 a, float64 b)
{
    if (isnan(a) || isnan(b))
        return NAN;
    else if (a == 0 && a == b)
        return signbit(a) ? b : a;
    else
        return a > b ? a : b;
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

static float
local_copysignf(float x, float y)
{
    union {
        float f;
        uint32_t i;
    } ux = { x }, uy = { y };
    ux.i &= 0x7fffffff;
    ux.i |= uy.i & 0x80000000;
    return ux.f;
}

static double
local_copysign(double x, double y)
{
    union {
        double f;
        uint64_t i;
    } ux = { x }, uy = { y };
    ux.i &= -1ULL / 2;
    ux.i |= uy.i & 1ULL << 63;
    return ux.f;
}

#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
#define LOAD_U32_WITH_2U16S(addr) (*(uint32 *)(addr))
#define LOAD_PTR(addr) (*(void **)(addr))
#else /* else of WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS */
static inline uint32
LOAD_U32_WITH_2U16S(void *addr)
{
    union {
        uint32 val;
        uint16 u16[2];
    } u;

    bh_assert(((uintptr_t)addr & 1) == 0);
    u.u16[0] = ((uint16 *)addr)[0];
    u.u16[1] = ((uint16 *)addr)[1];
    return u.val;
}
#if UINTPTR_MAX == UINT32_MAX
#define LOAD_PTR(addr) ((void *)LOAD_U32_WITH_2U16S(addr))
#elif UINTPTR_MAX == UINT64_MAX
static inline void *
LOAD_PTR(void *addr)
{
    uintptr_t addr1 = (uintptr_t)addr;
    union {
        void *val;
        uint32 u32[2];
        uint16 u16[4];
    } u;

    bh_assert(((uintptr_t)addr & 1) == 0);
    if ((addr1 & (uintptr_t)7) == 0)
        return *(void **)addr;

    if ((addr1 & (uintptr_t)3) == 0) {
        u.u32[0] = ((uint32 *)addr)[0];
        u.u32[1] = ((uint32 *)addr)[1];
    }
    else {
        u.u16[0] = ((uint16 *)addr)[0];
        u.u16[1] = ((uint16 *)addr)[1];
        u.u16[2] = ((uint16 *)addr)[2];
        u.u16[3] = ((uint16 *)addr)[3];
    }
    return u.val;
}
#endif /* end of UINTPTR_MAX */
#endif /* end of WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS */

#if WASM_ENABLE_GC != 0
static void
init_frame_refs(uint8 *frame_ref, uint32 cell_num, WASMFunctionInstance *func)
{
    uint32 i, j;

    memset(frame_ref, 0, cell_num);

    for (i = 0, j = 0; i < func->param_count; i++) {
        if (wasm_is_type_reftype(func->param_types[i])
            && !wasm_is_reftype_i31ref(func->param_types[i])) {
            frame_ref[j++] = 1;
#if UINTPTR_MAX == UINT64_MAX
            frame_ref[j++] = 1;
#endif
        }
        else if (func->param_types[i] == VALUE_TYPE_I32
                 || func->param_types[i] == VALUE_TYPE_F32)
            j++;
        else
            j += 2;
    }

    for (i = 0; i < func->local_count; i++) {
        if (wasm_is_type_reftype(func->local_types[i])
            && !wasm_is_reftype_i31ref(func->local_types[i])) {
            frame_ref[j++] = 1;
#if UINTPTR_MAX == UINT64_MAX
            frame_ref[j++] = 1;
#endif
        }
        else if (func->local_types[i] == VALUE_TYPE_I32
                 || func->local_types[i] == VALUE_TYPE_F32)
            j++;
        else
            j += 2;
    }
}

/* Return the corresponding ref slot of the given slot of local
   variable or stack pointer. */

#define COMPUTE_FRAME_REF(ref, off) (ref + (unsigned)(off))

#define FRAME_REF(off) COMPUTE_FRAME_REF(frame_ref, off)

#if UINTPTR_MAX == UINT64_MAX
#define SET_FRAME_REF(off) *FRAME_REF(off) = *FRAME_REF(off + 1) = 1
#define CLEAR_FRAME_REF(off)                          \
    (unsigned)off >= local_cell_num                   \
        ? (*FRAME_REF(off) = *FRAME_REF(off + 1) = 0) \
        : (void)0
#else
#define SET_FRAME_REF(off) *FRAME_REF(off) = 1
#define CLEAR_FRAME_REF(off) \
    off >= local_cell_num ? (*FRAME_REF(off) = 0) : (void)0
#endif

#define FRAME_REF_FOR(frame, p) \
    COMPUTE_FRAME_REF(frame->frame_ref, p - frame->lp)

#define CLEAR_FRAME_REF_FOR(p, n)               \
    do {                                        \
        int32 ref_i, ref_n = (int32)(n);        \
        uint8 *ref = FRAME_REF(p - frame_lp);   \
        for (ref_i = 0; ref_i < ref_n; ref_i++) \
            ref[ref_i] = 0;                     \
    } while (0)
#endif /* end of WASM_ENABLE_GC != 0 */

#define read_uint32(p) \
    (p += sizeof(uint32), LOAD_U32_WITH_2U16S(p - sizeof(uint32)))

#define GET_LOCAL_INDEX_TYPE_AND_OFFSET()                                \
    do {                                                                 \
        uint32 param_count = cur_func->param_count;                      \
        local_idx = read_uint32(frame_ip);                               \
        bh_assert(local_idx < param_count + cur_func->local_count);      \
        local_offset = cur_func->local_offsets[local_idx];               \
        if (local_idx < param_count)                                     \
            local_type = cur_func->param_types[local_idx];               \
        else                                                             \
            local_type = cur_func->local_types[local_idx - param_count]; \
    } while (0)

#define GET_OFFSET() (frame_ip += 2, *(int16 *)(frame_ip - 2))

#define SET_OPERAND_I32(off, value)                                 \
    do {                                                            \
        *(uint32 *)(frame_lp + *(int16 *)(frame_ip + off)) = value; \
    } while (0)
#define SET_OPERAND_F32(off, value)                                  \
    do {                                                             \
        *(float32 *)(frame_lp + *(int16 *)(frame_ip + off)) = value; \
    } while (0)
#define SET_OPERAND_I64(off, value)                               \
    do {                                                          \
        uint32 *addr_tmp = frame_lp + *(int16 *)(frame_ip + off); \
        PUT_I64_TO_ADDR(addr_tmp, value);                         \
    } while (0)
#define SET_OPERAND_F64(off, value)                               \
    do {                                                          \
        uint32 *addr_tmp = frame_lp + *(int16 *)(frame_ip + off); \
        PUT_F64_TO_ADDR(addr_tmp, value);                         \
    } while (0)
#define SET_OPERAND_REF(off, value)            \
    do {                                       \
        uint32 *addr_tmp;                      \
        opnd_off = *(int16 *)(frame_ip + off); \
        addr_tmp = frame_lp + opnd_off;        \
        PUT_REF_TO_ADDR(addr_tmp, value);      \
        SET_FRAME_REF(ond_off);                \
    } while (0)

#define SET_OPERAND(op_type, off, value) SET_OPERAND_##op_type(off, value)

#define GET_OPERAND_I32(type, off) \
    *(type *)(frame_lp + *(int16 *)(frame_ip + off))
#define GET_OPERAND_F32(type, off) \
    *(type *)(frame_lp + *(int16 *)(frame_ip + off))
#define GET_OPERAND_I64(type, off) \
    (type) GET_I64_FROM_ADDR(frame_lp + *(int16 *)(frame_ip + off))
#define GET_OPERAND_F64(type, off) \
    (type) GET_F64_FROM_ADDR(frame_lp + *(int16 *)(frame_ip + off))
#define GET_OPERAND_REF(type, off) \
    (type) GET_REF_FROM_ADDR(frame_lp + *(int16 *)(frame_ip + off))

#define GET_OPERAND(type, op_type, off) GET_OPERAND_##op_type(type, off)

#define PUSH_I32(value)                              \
    do {                                             \
        *(int32 *)(frame_lp + GET_OFFSET()) = value; \
    } while (0)

#define PUSH_F32(value)                                \
    do {                                               \
        *(float32 *)(frame_lp + GET_OFFSET()) = value; \
    } while (0)

#define PUSH_I64(value)                             \
    do {                                            \
        uint32 *addr_tmp = frame_lp + GET_OFFSET(); \
        PUT_I64_TO_ADDR(addr_tmp, value);           \
    } while (0)

#define PUSH_F64(value)                             \
    do {                                            \
        uint32 *addr_tmp = frame_lp + GET_OFFSET(); \
        PUT_F64_TO_ADDR(addr_tmp, value);           \
    } while (0)

#define PUSH_REF(value)                   \
    do {                                  \
        uint32 *addr_tmp;                 \
        opnd_off = GET_OFFSET();          \
        addr_tmp = frame_lp + opnd_off;   \
        PUT_REF_TO_ADDR(addr_tmp, value); \
        SET_FRAME_REF(opnd_off);          \
    } while (0)

#define PUSH_I31REF(value)                \
    do {                                  \
        uint32 *addr_tmp;                 \
        opnd_off = GET_OFFSET();          \
        addr_tmp = frame_lp + opnd_off;   \
        PUT_REF_TO_ADDR(addr_tmp, value); \
    } while (0)

#define POP_I32() (*(int32 *)(frame_lp + GET_OFFSET()))

#define POP_F32() (*(float32 *)(frame_lp + GET_OFFSET()))

#define POP_I64() (GET_I64_FROM_ADDR(frame_lp + GET_OFFSET()))

#define POP_F64() (GET_F64_FROM_ADDR(frame_lp + GET_OFFSET()))

#define POP_REF()                                                    \
    (opnd_off = GET_OFFSET(), CLEAR_FRAME_REF((unsigned)(opnd_off)), \
     GET_REF_FROM_ADDR(frame_lp + opnd_off))

#if WASM_ENABLE_GC != 0
#define SYNC_FRAME_REF() frame->frame_ref = frame_ref
#define UPDATE_FRAME_REF() frame_ref = frame->frame_ref
#else
#define SYNC_FRAME_REF() (void)0
#define UPDATE_FRAME_REF() (void)0
#endif

#define SYNC_ALL_TO_FRAME()   \
    do {                      \
        frame->ip = frame_ip; \
        SYNC_FRAME_REF();     \
    } while (0)

#define UPDATE_ALL_FROM_FRAME() \
    do {                        \
        frame_ip = frame->ip;   \
        UPDATE_FRAME_REF();     \
    } while (0)

#if WASM_ENABLE_LABELS_AS_VALUES != 0
#define UPDATE_FRAME_IP_END() (void)0
#else
#define UPDATE_FRAME_IP_END() frame_ip_end = wasm_get_func_code_end(cur_func)
#endif

#if WASM_ENABLE_GC != 0
#define RECOVER_FRAME_REF() frame_ref = frame->frame_ref
#else
#define RECOVER_FRAME_REF() (void)0
#endif

#define RECOVER_CONTEXT(new_frame)      \
    do {                                \
        frame = (new_frame);            \
        cur_func = frame->function;     \
        prev_frame = frame->prev_frame; \
        frame_ip = frame->ip;           \
        UPDATE_FRAME_IP_END();          \
        frame_lp = frame->lp;           \
        RECOVER_FRAME_REF();            \
    } while (0)

#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
#define GET_OPCODE() opcode = *frame_ip++;
#else
#define GET_OPCODE()    \
    opcode = *frame_ip; \
    frame_ip += 2;
#endif

#define DEF_OP_EQZ(ctype, src_op_type)                                  \
    do {                                                                \
        SET_OPERAND(I32, 2, (GET_OPERAND(ctype, src_op_type, 0) == 0)); \
        frame_ip += 4;                                                  \
    } while (0)

#define DEF_OP_CMP(src_type, src_op_type, cond)                      \
    do {                                                             \
        SET_OPERAND(I32, 4,                                          \
                    GET_OPERAND(src_type, src_op_type, 2)            \
                        cond GET_OPERAND(src_type, src_op_type, 0)); \
        frame_ip += 6;                                               \
    } while (0)

#define DEF_OP_BIT_COUNT(src_type, src_op_type, operation)               \
    do {                                                                 \
        SET_OPERAND(                                                     \
            src_op_type, 2,                                              \
            (src_type)operation(GET_OPERAND(src_type, src_op_type, 0))); \
        frame_ip += 4;                                                   \
    } while (0)

#define DEF_OP_NUMERIC(src_type1, src_type2, src_op_type, operation)       \
    do {                                                                   \
        SET_OPERAND(src_op_type, 4,                                        \
                    GET_OPERAND(src_type1, src_op_type, 2)                 \
                        operation GET_OPERAND(src_type2, src_op_type, 0)); \
        frame_ip += 6;                                                     \
    } while (0)

#define DEF_OP_REINTERPRET(src_type, src_op_type)                           \
    do {                                                                    \
        SET_OPERAND(src_op_type, 2, GET_OPERAND(src_type, src_op_type, 0)); \
        frame_ip += 4;                                                      \
    } while (0)

#define DEF_OP_NUMERIC_64 DEF_OP_NUMERIC

#define DEF_OP_NUMERIC2(src_type1, src_type2, src_op_type, operation)  \
    do {                                                               \
        SET_OPERAND(src_op_type, 4,                                    \
                    GET_OPERAND(src_type1, src_op_type, 2) operation(  \
                        GET_OPERAND(src_type2, src_op_type, 0) % 32)); \
        frame_ip += 6;                                                 \
    } while (0)

#define DEF_OP_NUMERIC2_64(src_type1, src_type2, src_op_type, operation) \
    do {                                                                 \
        SET_OPERAND(src_op_type, 4,                                      \
                    GET_OPERAND(src_type1, src_op_type, 2) operation(    \
                        GET_OPERAND(src_type2, src_op_type, 0) % 64));   \
        frame_ip += 6;                                                   \
    } while (0)

#define DEF_ATOMIC_RMW_OPCODE(OP_NAME, op)                           \
    case WASM_OP_ATOMIC_RMW_I32_##OP_NAME:                           \
    case WASM_OP_ATOMIC_RMW_I32_##OP_NAME##8_U:                      \
    case WASM_OP_ATOMIC_RMW_I32_##OP_NAME##16_U:                     \
    {                                                                \
        uint32 readv, sval;                                          \
                                                                     \
        sval = POP_I32();                                            \
        addr = POP_I32();                                            \
                                                                     \
        if (opcode == WASM_OP_ATOMIC_RMW_I32_##OP_NAME##8_U) {       \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(1);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint32)(*(uint8 *)maddr);                       \
            *(uint8 *)maddr = (uint8)(readv op sval);                \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        else if (opcode == WASM_OP_ATOMIC_RMW_I32_##OP_NAME##16_U) { \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(2);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint32)LOAD_U16(maddr);                         \
            STORE_U16(maddr, (uint16)(readv op sval));               \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        else {                                                       \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(4);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = LOAD_I32(maddr);                                 \
            STORE_U32(maddr, readv op sval);                         \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        PUSH_I32(readv);                                             \
        break;                                                       \
    }                                                                \
    case WASM_OP_ATOMIC_RMW_I64_##OP_NAME:                           \
    case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##8_U:                      \
    case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##16_U:                     \
    case WASM_OP_ATOMIC_RMW_I64_##OP_NAME##32_U:                     \
    {                                                                \
        uint64 readv, sval;                                          \
                                                                     \
        sval = (uint64)POP_I64();                                    \
        addr = POP_I32();                                            \
                                                                     \
        if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##8_U) {       \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 1, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(1);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint64)(*(uint8 *)maddr);                       \
            *(uint8 *)maddr = (uint8)(readv op sval);                \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        else if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##16_U) { \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(2);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint64)LOAD_U16(maddr);                         \
            STORE_U16(maddr, (uint16)(readv op sval));               \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        else if (opcode == WASM_OP_ATOMIC_RMW_I64_##OP_NAME##32_U) { \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(4);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint64)LOAD_U32(maddr);                         \
            STORE_U32(maddr, (uint32)(readv op sval));               \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        else {                                                       \
            uint64 op_result;                                        \
            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);     \
            CHECK_ATOMIC_MEMORY_ACCESS(8);                           \
                                                                     \
            os_mutex_lock(&node->shared_mem_lock);                   \
            readv = (uint64)LOAD_I64(maddr);                         \
            op_result = readv op sval;                               \
            STORE_I64(maddr, op_result);                             \
            os_mutex_unlock(&node->shared_mem_lock);                 \
        }                                                            \
        PUSH_I64(readv);                                             \
        break;                                                       \
    }

#define DEF_OP_MATH(src_type, src_op_type, method)                            \
    do {                                                                      \
        SET_OPERAND(src_op_type, 2,                                           \
                    (src_type)method(GET_OPERAND(src_type, src_op_type, 0))); \
        frame_ip += 4;                                                        \
    } while (0)

#define TRUNC_FUNCTION(func_name, src_type, dst_type, signed_type)  \
    static dst_type func_name(src_type src_value, src_type src_min, \
                              src_type src_max, dst_type dst_min,   \
                              dst_type dst_max, bool is_sign)       \
    {                                                               \
        dst_type dst_value = 0;                                     \
        if (!isnan(src_value)) {                                    \
            if (src_value <= src_min)                               \
                dst_value = dst_min;                                \
            else if (src_value >= src_max)                          \
                dst_value = dst_max;                                \
            else {                                                  \
                if (is_sign)                                        \
                    dst_value = (dst_type)(signed_type)src_value;   \
                else                                                \
                    dst_value = (dst_type)src_value;                \
            }                                                       \
        }                                                           \
        return dst_value;                                           \
    }

TRUNC_FUNCTION(trunc_f32_to_i32, float32, uint32, int32)
TRUNC_FUNCTION(trunc_f32_to_i64, float32, uint64, int64)
TRUNC_FUNCTION(trunc_f64_to_i32, float64, uint32, int32)
TRUNC_FUNCTION(trunc_f64_to_i64, float64, uint64, int64)

static bool
trunc_f32_to_int(WASMModuleInstance *module, uint8 *frame_ip, uint32 *frame_lp,
                 float32 src_min, float32 src_max, bool saturating, bool is_i32,
                 bool is_sign)
{
    float32 src_value = GET_OPERAND(float32, F32, 0);
    uint64 dst_value_i64;
    uint32 dst_value_i32;

    if (!saturating) {
        if (isnan(src_value)) {
            wasm_set_exception(module, "invalid conversion to integer");
            return false;
        }
        else if (src_value <= src_min || src_value >= src_max) {
            wasm_set_exception(module, "integer overflow");
            return false;
        }
    }

    if (is_i32) {
        uint32 dst_min = is_sign ? INT32_MIN : 0;
        uint32 dst_max = is_sign ? INT32_MAX : UINT32_MAX;
        dst_value_i32 = trunc_f32_to_i32(src_value, src_min, src_max, dst_min,
                                         dst_max, is_sign);
        SET_OPERAND(I32, 2, dst_value_i32);
    }
    else {
        uint64 dst_min = is_sign ? INT64_MIN : 0;
        uint64 dst_max = is_sign ? INT64_MAX : UINT64_MAX;
        dst_value_i64 = trunc_f32_to_i64(src_value, src_min, src_max, dst_min,
                                         dst_max, is_sign);
        SET_OPERAND(I64, 2, dst_value_i64);
    }
    return true;
}

static bool
trunc_f64_to_int(WASMModuleInstance *module, uint8 *frame_ip, uint32 *frame_lp,
                 float64 src_min, float64 src_max, bool saturating, bool is_i32,
                 bool is_sign)
{
    float64 src_value = GET_OPERAND(float64, F64, 0);
    uint64 dst_value_i64;
    uint32 dst_value_i32;

    if (!saturating) {
        if (isnan(src_value)) {
            wasm_set_exception(module, "invalid conversion to integer");
            return false;
        }
        else if (src_value <= src_min || src_value >= src_max) {
            wasm_set_exception(module, "integer overflow");
            return false;
        }
    }

    if (is_i32) {
        uint32 dst_min = is_sign ? INT32_MIN : 0;
        uint32 dst_max = is_sign ? INT32_MAX : UINT32_MAX;
        dst_value_i32 = trunc_f64_to_i32(src_value, src_min, src_max, dst_min,
                                         dst_max, is_sign);
        SET_OPERAND(I32, 2, dst_value_i32);
    }
    else {
        uint64 dst_min = is_sign ? INT64_MIN : 0;
        uint64 dst_max = is_sign ? INT64_MAX : UINT64_MAX;
        dst_value_i64 = trunc_f64_to_i64(src_value, src_min, src_max, dst_min,
                                         dst_max, is_sign);
        SET_OPERAND(I64, 2, dst_value_i64);
    }
    return true;
}

#define DEF_OP_TRUNC_F32(min, max, is_i32, is_sign)                        \
    do {                                                                   \
        if (!trunc_f32_to_int(module, frame_ip, frame_lp, min, max, false, \
                              is_i32, is_sign))                            \
            goto got_exception;                                            \
        frame_ip += 4;                                                     \
    } while (0)

#define DEF_OP_TRUNC_F64(min, max, is_i32, is_sign)                        \
    do {                                                                   \
        if (!trunc_f64_to_int(module, frame_ip, frame_lp, min, max, false, \
                              is_i32, is_sign))                            \
            goto got_exception;                                            \
        frame_ip += 4;                                                     \
    } while (0)

#define DEF_OP_TRUNC_SAT_F32(min, max, is_i32, is_sign)                    \
    do {                                                                   \
        (void)trunc_f32_to_int(module, frame_ip, frame_lp, min, max, true, \
                               is_i32, is_sign);                           \
        frame_ip += 4;                                                     \
    } while (0)

#define DEF_OP_TRUNC_SAT_F64(min, max, is_i32, is_sign)                    \
    do {                                                                   \
        (void)trunc_f64_to_int(module, frame_ip, frame_lp, min, max, true, \
                               is_i32, is_sign);                           \
        frame_ip += 4;                                                     \
    } while (0)

#define DEF_OP_CONVERT(dst_type, dst_op_type, src_type, src_op_type) \
    do {                                                             \
        dst_type value = (dst_type)(src_type)POP_##src_op_type();    \
        PUSH_##dst_op_type(value);                                   \
    } while (0)

#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
#define CELL_SIZE sizeof(uint8)
#else
#define CELL_SIZE (sizeof(uint8) * 2)
#endif

static bool
copy_stack_values(WASMModuleInstance *module, uint32 *frame_lp, uint32 arity,
#if WASM_ENABLE_GC != 0
                  uint8 *frame_ref,
#endif
                  uint32 total_cell_num, const uint8 *cells,
                  const int16 *src_offsets, const uint16 *dst_offsets)
{
    /* To avoid the overlap issue between src offsets and dst offset,
     * we use 2 steps to do the copy. First step, copy the src values
     * to a tmp buf. Second step, copy the values from tmp buf to dst.
     */
    bool ret = false;
    uint32 buf[16] = { 0 }, i;
    uint32 *tmp_buf = buf;
    uint8 cell;
    int16 src, buf_index = 0;
    uint16 dst;
#if WASM_ENABLE_GC != 0
    uint8 ref_buf[4];
    uint8 *tmp_ref_buf = ref_buf;
#endif

    /* Allocate memory if the buf is not large enough */
    if (total_cell_num > sizeof(buf) / sizeof(uint32)) {
        uint64 total_size = sizeof(uint32) * (uint64)total_cell_num;
        if (total_size >= UINT32_MAX
            || !(tmp_buf = wasm_runtime_malloc((uint32)total_size))) {
            wasm_set_exception(module, "allocate memory failed");
            goto fail;
        }
    }

#if WASM_ENABLE_GC != 0
    if (total_cell_num > sizeof(ref_buf) / sizeof(uint8)) {
        uint64 total_size = sizeof(uint8) * (uint64)total_cell_num;
        if (total_size >= UINT32_MAX
            || !(tmp_ref_buf = wasm_runtime_malloc((uint32)total_size))) {
            wasm_set_exception(module, "allocate memory failed");
            goto fail;
        }
    }
#endif

    /* 1) Copy values from src to tmp buf */
    for (i = 0; i < arity; i++) {
        cell = cells[i * CELL_SIZE];
        src = src_offsets[i];
        if (cell == 1) {
            tmp_buf[buf_index] = frame_lp[src];
#if WASM_ENABLE_GC != 0
            tmp_ref_buf[buf_index] = frame_ref[src];
#endif
        }
        else {
            tmp_buf[buf_index] = frame_lp[src];
            tmp_buf[buf_index + 1] = frame_lp[src + 1];
#if WASM_ENABLE_GC != 0
            tmp_ref_buf[buf_index] = frame_ref[src];
            tmp_ref_buf[buf_index + 1] = frame_ref[src + 1];
#endif
        }
        buf_index += cell;
    }

    /* 2) Copy values from tmp buf to dest */
    buf_index = 0;
    for (i = 0; i < arity; i++) {
        cell = cells[i * CELL_SIZE];
        dst = dst_offsets[i];
        if (cell == 1) {
            frame_lp[dst] = tmp_buf[buf_index];
#if WASM_ENABLE_GC != 0
            frame_ref[dst] = tmp_ref_buf[buf_index];
#endif
        }
        else {
            frame_lp[dst] = tmp_buf[buf_index];
            frame_lp[dst + 1] = tmp_buf[buf_index + 1];
#if WASM_ENABLE_GC != 0
            frame_ref[dst] = tmp_ref_buf[buf_index];
            frame_ref[dst + 1] = tmp_ref_buf[buf_index + 1];
#endif
        }
        buf_index += cell;
    }

    ret = true;

fail:
    if (tmp_buf != buf) {
        wasm_runtime_free(tmp_buf);
    }

#if WASM_ENABLE_GC != 0
    if (tmp_ref_buf != ref_buf) {
        wasm_runtime_free(tmp_ref_buf);
    }
#endif

    return ret;
}

#if WASM_ENABLE_GC != 0
#define RECOVER_BR_INFO()                                                  \
    do {                                                                   \
        uint32 arity;                                                      \
        /* read arity */                                                   \
        arity = read_uint32(frame_ip);                                     \
        if (arity) {                                                       \
            uint32 total_cell;                                             \
            uint16 *dst_offsets = NULL;                                    \
            uint8 *cells;                                                  \
            int16 *src_offsets = NULL;                                     \
            /* read total cell num */                                      \
            total_cell = read_uint32(frame_ip);                            \
            /* cells */                                                    \
            cells = (uint8 *)frame_ip;                                     \
            frame_ip += arity * CELL_SIZE;                                 \
            /* src offsets */                                              \
            src_offsets = (int16 *)frame_ip;                               \
            frame_ip += arity * sizeof(int16);                             \
            /* dst offsets */                                              \
            dst_offsets = (uint16 *)frame_ip;                              \
            frame_ip += arity * sizeof(uint16);                            \
            if (arity == 1) {                                              \
                if (cells[0] == 1) {                                       \
                    frame_lp[dst_offsets[0]] = frame_lp[src_offsets[0]];   \
                    CLEAR_FRAME_REF((unsigned)(src_offsets[0]));           \
                    SET_FRAME_REF(dst_offsets[0]);                         \
                }                                                          \
                else if (cells[0] == 2) {                                  \
                    frame_lp[dst_offsets[0]] = frame_lp[src_offsets[0]];   \
                    frame_lp[dst_offsets[0] + 1] =                         \
                        frame_lp[src_offsets[0] + 1];                      \
                    CLEAR_FRAME_REF((unsigned)src_offsets[0]);             \
                    CLEAR_FRAME_REF((unsigned)(src_offsets[0] + 1));       \
                    SET_FRAME_REF((unsigned)dst_offsets[0]);               \
                    SET_FRAME_REF((unsigned)(dst_offsets[0] + 1));         \
                }                                                          \
            }                                                              \
            else {                                                         \
                if (!copy_stack_values(module, frame_lp, arity, frame_ref, \
                                       total_cell, cells, src_offsets,     \
                                       dst_offsets))                       \
                    goto got_exception;                                    \
            }                                                              \
        }                                                                  \
        frame_ip = (uint8 *)LOAD_PTR(frame_ip);                            \
    } while (0)
#else
#define RECOVER_BR_INFO()                                                   \
    do {                                                                    \
        uint32 arity;                                                       \
        /* read arity */                                                    \
        arity = read_uint32(frame_ip);                                      \
        if (arity) {                                                        \
            uint32 total_cell;                                              \
            uint16 *dst_offsets = NULL;                                     \
            uint8 *cells;                                                   \
            int16 *src_offsets = NULL;                                      \
            /* read total cell num */                                       \
            total_cell = read_uint32(frame_ip);                             \
            /* cells */                                                     \
            cells = (uint8 *)frame_ip;                                      \
            frame_ip += arity * CELL_SIZE;                                  \
            /* src offsets */                                               \
            src_offsets = (int16 *)frame_ip;                                \
            frame_ip += arity * sizeof(int16);                              \
            /* dst offsets */                                               \
            dst_offsets = (uint16 *)frame_ip;                               \
            frame_ip += arity * sizeof(uint16);                             \
            if (arity == 1) {                                               \
                if (cells[0] == 1)                                          \
                    frame_lp[dst_offsets[0]] = frame_lp[src_offsets[0]];    \
                else if (cells[0] == 2) {                                   \
                    frame_lp[dst_offsets[0]] = frame_lp[src_offsets[0]];    \
                    frame_lp[dst_offsets[0] + 1] =                          \
                        frame_lp[src_offsets[0] + 1];                       \
                }                                                           \
            }                                                               \
            else {                                                          \
                if (!copy_stack_values(module, frame_lp, arity, total_cell, \
                                       cells, src_offsets, dst_offsets))    \
                    goto got_exception;                                     \
            }                                                               \
        }                                                                   \
        frame_ip = (uint8 *)LOAD_PTR(frame_ip);                             \
    } while (0)
#endif

#define SKIP_BR_INFO()                                                        \
    do {                                                                      \
        uint32 arity;                                                         \
        /* read and skip arity */                                             \
        arity = read_uint32(frame_ip);                                        \
        if (arity) {                                                          \
            /* skip total cell num */                                         \
            frame_ip += sizeof(uint32);                                       \
            /* skip cells, src offsets and dst offsets */                     \
            frame_ip += (CELL_SIZE + sizeof(int16) + sizeof(uint16)) * arity; \
        }                                                                     \
        /* skip target address */                                             \
        frame_ip += sizeof(uint8 *);                                          \
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
        return (int64)val | (int64)0xffffffffffffff00LL;
    return val;
}

static inline int64
sign_ext_16_64(int16 val)
{
    if (val & 0x8000)
        return (int64)val | (int64)0xffffffffffff0000LL;
    return val;
}

static inline int64
sign_ext_32_64(int32 val)
{
    if (val & (int32)0x80000000)
        return (int64)val | (int64)0xffffffff00000000LL;
    return val;
}

static inline void
word_copy(uint32 *dest, uint32 *src, unsigned num)
{
    bh_assert(dest != NULL);
    bh_assert(src != NULL);
    bh_assert(num > 0);
    if (dest != src) {
        /* No overlap buffer */
        bh_assert(!((src < dest) && (dest < src + num)));
        for (; num > 0; num--)
            *dest++ = *src++;
    }
}

static inline WASMInterpFrame *
ALLOC_FRAME(WASMExecEnv *exec_env, uint32 size, WASMInterpFrame *prev_frame)
{
    WASMInterpFrame *frame = wasm_exec_env_alloc_wasm_frame(exec_env, size);

    if (frame) {
        frame->prev_frame = prev_frame;
#if WASM_ENABLE_PERF_PROFILING != 0
        frame->time_started = os_time_get_boot_microsecond();
#endif
    }
    else {
        wasm_set_exception((WASMModuleInstance *)exec_env->module_inst,
                           "wasm operand stack overflow");
    }

    return frame;
}

static inline void
FREE_FRAME(WASMExecEnv *exec_env, WASMInterpFrame *frame)
{
#if WASM_ENABLE_PERF_PROFILING != 0
    if (frame->function) {
        frame->function->total_exec_time +=
            os_time_get_boot_microsecond() - frame->time_started;
        frame->function->total_exec_cnt++;
    }
#endif
    wasm_exec_env_free_wasm_frame(exec_env, frame);
}

static void
wasm_interp_call_func_native(WASMModuleInstance *module_inst,
                             WASMExecEnv *exec_env,
                             WASMFunctionInstance *cur_func,
                             WASMInterpFrame *prev_frame)
{
    WASMFunctionImport *func_import = cur_func->u.func_import;
    CApiFuncImport *c_api_func_import = NULL;
    unsigned local_cell_num =
        cur_func->param_cell_num > 2 ? cur_func->param_cell_num : 2;
    unsigned all_cell_num;
    WASMInterpFrame *frame;
    uint32 argv_ret[2], cur_func_index;
    void *native_func_pointer = NULL;
    bool ret;
#if WASM_ENABLE_GC != 0
    WASMFuncType *func_type;
    uint8 *frame_ref;
#endif

    all_cell_num = local_cell_num;
#if WASM_ENABLE_GC != 0
    all_cell_num += (local_cell_num + 3) / 4;
#endif

    if (!(frame =
              ALLOC_FRAME(exec_env, wasm_interp_interp_frame_size(all_cell_num),
                          prev_frame)))
        return;

    frame->function = cur_func;
    frame->ip = NULL;
    frame->lp = frame->operand;
#if WASM_ENABLE_GC != 0
    frame->frame_ref = (uint8 *)(frame->lp + local_cell_num);
    init_frame_refs(frame->frame_ref, local_cell_num, cur_func);
#endif

    wasm_exec_env_set_cur_frame(exec_env, frame);

    cur_func_index = (uint32)(cur_func - module_inst->e->functions);
    bh_assert(cur_func_index < module_inst->module->import_function_count);
    if (!func_import->call_conv_wasm_c_api) {
        native_func_pointer = module_inst->import_func_ptrs[cur_func_index];
    }
    else if (module_inst->e->c_api_func_imports) {
        c_api_func_import = module_inst->e->c_api_func_imports + cur_func_index;
        native_func_pointer = c_api_func_import->func_ptr_linked;
    }

    if (!native_func_pointer) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "failed to call unlinked import function (%s, %s)",
                 func_import->module_name, func_import->field_name);
        wasm_set_exception((WASMModuleInstance *)module_inst, buf);
        return;
    }

    if (func_import->call_conv_wasm_c_api) {
        ret = wasm_runtime_invoke_c_api_native(
            (WASMModuleInstanceCommon *)module_inst, native_func_pointer,
            func_import->func_type, cur_func->param_cell_num, frame->lp,
            c_api_func_import->with_env_arg, c_api_func_import->env_arg);
        if (ret) {
            argv_ret[0] = frame->lp[0];
            argv_ret[1] = frame->lp[1];
        }
    }
    else if (!func_import->call_conv_raw) {
        ret = wasm_runtime_invoke_native(
            exec_env, native_func_pointer, func_import->func_type,
            func_import->signature, func_import->attachment, frame->lp,
            cur_func->param_cell_num, argv_ret);
    }
    else {
        ret = wasm_runtime_invoke_native_raw(
            exec_env, native_func_pointer, func_import->func_type,
            func_import->signature, func_import->attachment, frame->lp,
            cur_func->param_cell_num, argv_ret);
    }

    if (!ret)
        return;

#if WASM_ENABLE_GC != 0
    func_type = cur_func->u.func_import->func_type;
    if (func_type->result_count
        && wasm_is_type_reftype(func_type->types[cur_func->param_count])
        && !wasm_is_reftype_i31ref(func_type->types[cur_func->param_count])) {
        frame_ref = prev_frame->frame_ref + prev_frame->ret_offset;
#if UINTPTR_MAX == UINT64_MAX
        *frame_ref = *(frame_ref + 1) = 1;
#else
        *frame_ref = 1;
#endif
    }
#endif

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
    WASMExecEnv *sub_module_exec_env = NULL;
    uint32 aux_stack_origin_boundary = 0;
    uint32 aux_stack_origin_bottom = 0;

    if (!sub_func_inst) {
        snprintf(buf, sizeof(buf),
                 "failed to call unlinked import function (%s, %s)",
                 func_import->module_name, func_import->field_name);
        wasm_set_exception(module_inst, buf);
        return;
    }

    /* Switch exec_env but keep using the same one by replacing necessary
     * variables */
    sub_module_exec_env = wasm_runtime_get_exec_env_singleton(
        (WASMModuleInstanceCommon *)sub_module_inst);
    if (!sub_module_exec_env) {
        wasm_set_exception(module_inst, "create singleton exec_env failed");
        return;
    }

    /* - module_inst */
    exec_env->module_inst = (WASMModuleInstanceCommon *)sub_module_inst;
    /* - aux_stack_boundary */
    aux_stack_origin_boundary = exec_env->aux_stack_boundary.boundary;
    exec_env->aux_stack_boundary.boundary =
        sub_module_exec_env->aux_stack_boundary.boundary;
    /* - aux_stack_bottom */
    aux_stack_origin_bottom = exec_env->aux_stack_bottom.bottom;
    exec_env->aux_stack_bottom.bottom =
        sub_module_exec_env->aux_stack_bottom.bottom;

    /* set ip NULL to make call_func_bytecode return after executing
       this function */
    prev_frame->ip = NULL;

    /* call function of sub-module*/
    wasm_interp_call_func_bytecode(sub_module_inst, exec_env, sub_func_inst,
                                   prev_frame);

    /* restore ip and other replaced */
    prev_frame->ip = ip;
    exec_env->aux_stack_boundary.boundary = aux_stack_origin_boundary;
    exec_env->aux_stack_bottom.bottom = aux_stack_origin_bottom;
    exec_env->module_inst = (WASMModuleInstanceCommon *)module_inst;

    /* transfer exception if it is thrown */
    if (wasm_copy_exception(sub_module_inst, NULL)) {
        bh_memcpy_s(module_inst->cur_exception,
                    sizeof(module_inst->cur_exception),
                    sub_module_inst->cur_exception,
                    sizeof(sub_module_inst->cur_exception));
    }
}
#endif

#if WASM_ENABLE_THREAD_MGR != 0
#define CHECK_SUSPEND_FLAGS()                           \
    do {                                                \
        os_mutex_lock(&exec_env->wait_lock);            \
        if (exec_env->suspend_flags.flags != 0) {       \
            if (exec_env->suspend_flags.flags & 0x01) { \
                /* terminate current thread */          \
                os_mutex_unlock(&exec_env->wait_lock);  \
                return;                                 \
            }                                           \
            /* TODO: support suspend and breakpoint */  \
        }                                               \
        os_mutex_unlock(&exec_env->wait_lock);          \
    } while (0)
#endif

#if WASM_ENABLE_OPCODE_COUNTER != 0
typedef struct OpcodeInfo {
    char *name;
    uint64 count;
} OpcodeInfo;

/* clang-format off */
#define HANDLE_OPCODE(op) \
    {                     \
        #op, 0            \
    }
DEFINE_GOTO_TABLE(OpcodeInfo, opcode_table);
#undef HANDLE_OPCODE
/* clang-format on */

static void
wasm_interp_dump_op_count()
{
    uint32 i;
    uint64 total_count = 0;
    for (i = 0; i < WASM_OP_IMPDEP; i++)
        total_count += opcode_table[i].count;

    os_printf("total opcode count: %ld\n", total_count);
    for (i = 0; i < WASM_OP_IMPDEP; i++)
        if (opcode_table[i].count > 0)
            os_printf("\t\t%s count:\t\t%ld,\t\t%.2f%%\n", opcode_table[i].name,
                      opcode_table[i].count,
                      opcode_table[i].count * 100.0f / total_count);
}
#endif

#if WASM_ENABLE_LABELS_AS_VALUES != 0

/* #define HANDLE_OP(opcode) HANDLE_##opcode:printf(#opcode"\n"); */
#if WASM_ENABLE_OPCODE_COUNTER != 0
#define HANDLE_OP(opcode) HANDLE_##opcode : opcode_table[opcode].count++;
#else
#define HANDLE_OP(opcode) HANDLE_##opcode:
#endif
#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
#define FETCH_OPCODE_AND_DISPATCH()                    \
    do {                                               \
        const void *p_label_addr = *(void **)frame_ip; \
        frame_ip += sizeof(void *);                    \
        goto *p_label_addr;                            \
    } while (0)
#else
#define FETCH_OPCODE_AND_DISPATCH()                                 \
    do {                                                            \
        const void *p_label_addr = label_base + *(int16 *)frame_ip; \
        frame_ip += sizeof(int16);                                  \
        goto *p_label_addr;                                         \
    } while (0)
#endif /* end of WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS */
#define HANDLE_OP_END() FETCH_OPCODE_AND_DISPATCH()

#else /* else of WASM_ENABLE_LABELS_AS_VALUES */

#define HANDLE_OP(opcode) case opcode:
#define HANDLE_OP_END() continue

#endif /* end of WASM_ENABLE_LABELS_AS_VALUES */

#if WASM_ENABLE_LABELS_AS_VALUES != 0
static void **global_handle_table;
#endif

static inline uint8 *
get_global_addr(uint8 *global_data, WASMGlobalInstance *global)
{
#if WASM_ENABLE_MULTI_MODULE == 0
    return global_data + global->data_offset;
#else
    return global->import_global_inst
               ? global->import_module_inst->global_data
                     + global->import_global_inst->data_offset
               : global_data + global->data_offset;
#endif
}

static void
wasm_interp_call_func_bytecode(WASMModuleInstance *module,
                               WASMExecEnv *exec_env,
                               WASMFunctionInstance *cur_func,
                               WASMInterpFrame *prev_frame)
{
#if WASM_ENABLE_SHARED_MEMORY != 0
    WASMSharedMemNode *node =
        wasm_module_get_shared_memory((WASMModuleCommon *)module->module);
#endif
    WASMMemoryInstance *memory = wasm_get_default_memory(module);
#if !defined(OS_ENABLE_HW_BOUND_CHECK)              \
    || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0 \
    || WASM_ENABLE_BULK_MEMORY != 0
    uint32 linear_mem_size = memory ? memory->memory_data_size : 0;
#endif
    WASMGlobalInstance *globals = module->e ? module->e->globals : NULL;
    WASMGlobalInstance *global;
    uint8 *global_data = module->global_data;
    uint8 opcode_IMPDEP = WASM_OP_IMPDEP;
    WASMInterpFrame *frame = NULL;
    /* Points to this special opcode so as to jump to the
     * call_method_from_entry.  */
    register uint8 *frame_ip = &opcode_IMPDEP; /* cache of frame->ip */
    register uint32 *frame_lp = NULL;          /* cache of frame->lp */
#if WASM_ENABLE_LABELS_AS_VALUES != 0
#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0
    /* cache of label base addr */
    register uint8 *label_base = &&HANDLE_WASM_OP_UNREACHABLE;
#endif
#endif
#if WASM_ENABLE_GC != 0
    register uint8 *frame_ref = NULL; /* cache of frame->ref */
    uint32 local_cell_num = 0;
    int16 opnd_off;
#endif
    uint8 *frame_ip_end = frame_ip + 1;
    uint32 cond, count, fidx, tidx, frame_size = 0;
    uint32 all_cell_num = 0;
    int16 addr1, addr2, addr_ret = 0;
    int32 didx, val;
    uint8 *maddr = NULL;
    uint32 local_idx, local_offset, global_idx;
    uint8 opcode = 0, local_type, *global_addr;
#if WASM_ENABLE_GC != 0
    WASMObjectRef gc_obj;
    WASMStructObjectRef struct_obj;
    WASMArrayObjectRef array_obj;
    WASMFuncObjectRef func_obj;
    WASMI31ObjectRef i31_obj;
    WASMExternrefObjectRef externref_obj;
    uint32 type_idx;
#endif

#if WASM_ENABLE_LABELS_AS_VALUES != 0
#define HANDLE_OPCODE(op) &&HANDLE_##op
    DEFINE_GOTO_TABLE(const void *, handle_table);
#undef HANDLE_OPCODE
    if (exec_env == NULL) {
        global_handle_table = (void **)handle_table;
        return;
    }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES == 0
    while (frame_ip < frame_ip_end) {
        opcode = *frame_ip++;
#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0
        frame_ip++;
#endif
        switch (opcode) {
#else
    goto *handle_table[WASM_OP_IMPDEP];
#endif
            /* control instructions */
            HANDLE_OP(WASM_OP_UNREACHABLE)
            {
                wasm_set_exception(module, "unreachable");
                goto got_exception;
            }

            HANDLE_OP(WASM_OP_IF)
            {
                cond = (uint32)POP_I32();

                if (cond == 0) {
                    uint8 *else_addr = (uint8 *)LOAD_PTR(frame_ip);
                    if (else_addr == NULL) {
                        frame_ip =
                            (uint8 *)LOAD_PTR(frame_ip + sizeof(uint8 *));
                    }
                    else {
                        frame_ip = else_addr;
                    }
                }
                else {
                    frame_ip += sizeof(uint8 *) * 2;
                }
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_ELSE)
            {
                frame_ip = (uint8 *)LOAD_PTR(frame_ip);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_BR)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
            recover_br_info:
                RECOVER_BR_INFO();
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_BR_IF)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                cond = frame_lp[GET_OFFSET()];

                if (cond)
                    goto recover_br_info;
                else
                    SKIP_BR_INFO();

                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_BR_TABLE)
            {
                uint32 arity, br_item_size;

#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                count = read_uint32(frame_ip);
                didx = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;

                if (!(didx >= 0 && (uint32)didx < count))
                    didx = count;

                /* all br items must have the same arity and item size,
                   so we only calculate the first item size */
                arity = LOAD_U32_WITH_2U16S(frame_ip);
                br_item_size = sizeof(uint32); /* arity */
                if (arity) {
                    /* total cell num */
                    br_item_size += sizeof(uint32);
                    /* cells, src offsets and dst offsets */
                    br_item_size +=
                        (CELL_SIZE + sizeof(int16) + sizeof(uint16)) * arity;
                }
                /* target address */
                br_item_size += sizeof(uint8 *);

                frame_ip += br_item_size * didx;
                goto recover_br_info;
            }

            HANDLE_OP(WASM_OP_RETURN)
            {
                uint32 ret_idx;
                WASMFuncType *func_type;
                uint32 off, ret_offset;
                uint8 *ret_types;
                if (cur_func->is_import_func)
                    func_type = cur_func->u.func_import->func_type;
                else
                    func_type = cur_func->u.func->func_type;

                /* types of each return value */
                ret_types = func_type->types + func_type->param_count;
                ret_offset = prev_frame->ret_offset;

                for (ret_idx = 0,
                    off = sizeof(int16) * (func_type->result_count - 1);
                     ret_idx < func_type->result_count;
                     ret_idx++, off -= sizeof(int16)) {
                    if (ret_types[ret_idx] == VALUE_TYPE_I64
                        || ret_types[ret_idx] == VALUE_TYPE_F64) {
                        PUT_I64_TO_ADDR(prev_frame->lp + ret_offset,
                                        GET_OPERAND(uint64, I64, off));
                        ret_offset += 2;
                    }
#if WASM_ENABLE_GC != 0
                    else if (wasm_is_type_reftype(ret_types[ret_idx])) {
                        PUT_REF_TO_ADDR(prev_frame->lp + ret_offset,
                                        GET_OPERAND(void *, REF, off));
                        if (!wasm_is_reftype_i31ref(ret_types[ret_idx])) {
                            *(prev_frame->frame_ref + ret_offset) = 1;
#if UINTPTR_MAX == UINT64_MAX
                            *(prev_frame->frame_ref + ret_offset + 1) = 1;
#endif
                        }
                        ret_offset += REF_CELL_NUM;
                    }
#endif
                    else {
                        prev_frame->lp[ret_offset] =
                            GET_OPERAND(uint32, I32, off);
                        ret_offset++;
                    }
                }
                goto return_func;
            }

            HANDLE_OP(WASM_OP_CALL_INDIRECT)
#if WASM_ENABLE_TAIL_CALL != 0
            HANDLE_OP(WASM_OP_RETURN_CALL_INDIRECT)
#endif
            {
                WASMFuncType *cur_type, *cur_func_type;
                WASMTableInstance *tbl_inst;
                uint32 tbl_idx;

#if WASM_ENABLE_TAIL_CALL != 0
                GET_OPCODE();
#endif
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif

                tidx = read_uint32(frame_ip);
                cur_type = (WASMFuncType *)module->module->types[tidx];

                tbl_idx = read_uint32(frame_ip);
                bh_assert(tbl_idx < module->table_count);

                tbl_inst = wasm_get_table_inst(module, tbl_idx);

                val = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;

                if ((uint32)val >= tbl_inst->cur_size) {
                    wasm_set_exception(module, "undefined element");
                    goto got_exception;
                }

                /* clang-format off */
#if WASM_ENABLE_GC == 0
                fidx = tbl_inst->elems[val];
                if (fidx == (uint32)-1) {
                    wasm_set_exception(module, "uninitialized element");
                    goto got_exception;
                }
#else
                func_obj = (WASMFuncObjectRef)tbl_inst->elems[val];
                if (!func_obj) {
                    wasm_set_exception(module, "uninitialized element");
                    goto got_exception;
                }
                fidx = wasm_func_obj_get_func_idx_bound(func_obj);
#endif
                /* clang-format on */

                /*
                 * we might be using a table injected by host or
                 * another module. in that case, we don't validate
                 * the elem value while loading
                 */
                if (fidx >= module->e->function_count) {
                    wasm_set_exception(module, "unknown function");
                    goto got_exception;
                }

                /* always call module own functions */
                cur_func = module->e->functions + fidx;

                if (cur_func->is_import_func)
                    cur_func_type = cur_func->u.func_import->func_type;
                else
                    cur_func_type = cur_func->u.func->func_type;

                    /* clang-format off */
#if WASM_ENABLE_GC == 0
                if (cur_type != cur_func_type) {
                    wasm_set_exception(module, "indirect call type mismatch");
                    goto got_exception;
                }
#else
                if (cur_type->min_type_idx_normalized
                        != cur_func_type->min_type_idx_normalized) {
                    wasm_set_exception(module, "indirect call type mismatch");
                    goto got_exception;
                }
#endif
                /* clang-format on */

#if WASM_ENABLE_TAIL_CALL != 0
                if (opcode == WASM_OP_RETURN_CALL_INDIRECT)
                    goto call_func_from_return_call;
#endif
                goto call_func_from_interp;
            }

            /* parametric instructions */
            HANDLE_OP(WASM_OP_SELECT)
            {
                cond = frame_lp[GET_OFFSET()];
                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                addr_ret = GET_OFFSET();

                if (!cond) {
                    if (addr_ret != addr1)
                        frame_lp[addr_ret] = frame_lp[addr1];
                }
                else {
                    if (addr_ret != addr2)
                        frame_lp[addr_ret] = frame_lp[addr2];
                }
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_SELECT_64)
            {
                cond = frame_lp[GET_OFFSET()];
                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                addr_ret = GET_OFFSET();

                if (!cond) {
                    if (addr_ret != addr1)
                        PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                        GET_I64_FROM_ADDR(frame_lp + addr1));
                }
                else {
                    if (addr_ret != addr2)
                        PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                        GET_I64_FROM_ADDR(frame_lp + addr2));
                }
                HANDLE_OP_END();
            }

#if WASM_ENABLE_GC != 0
            HANDLE_OP(WASM_OP_SELECT_T)
            {
                cond = frame_lp[GET_OFFSET()];
                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                addr_ret = GET_OFFSET();

                if (!cond) {
                    if (addr_ret != addr1)
                        PUT_REF_TO_ADDR(frame_lp + addr_ret,
                                        GET_REF_FROM_ADDR(frame_lp + addr1));
                }
                else {
                    if (addr_ret != addr2)
                        PUT_REF_TO_ADDR(frame_lp + addr_ret,
                                        GET_REF_FROM_ADDR(frame_lp + addr2));
                }
                {
                    uint8 orig_ref = *FRAME_REF(addr1);
                    CLEAR_FRAME_REF(addr1);
                    CLEAR_FRAME_REF(addr2);

                    if (orig_ref) {
                        /* If original ref is i31ref, should not set frameref
                         * for target cell */
                        SET_FRAME_REF(addr_ret);
                    }
                }

                HANDLE_OP_END();
            }
#endif

#if WASM_ENABLE_REF_TYPES != 0 || WASM_ENABLE_GC != 0
            HANDLE_OP(WASM_OP_TABLE_GET)
            {
                uint32 tbl_idx, elem_idx;
                WASMTableInstance *tbl_inst;

                tbl_idx = read_uint32(frame_ip);
                bh_assert(tbl_idx < module->table_count);

                tbl_inst = wasm_get_table_inst(module, tbl_idx);

                elem_idx = POP_I32();
                if (elem_idx >= tbl_inst->cur_size) {
                    wasm_set_exception(module, "out of bounds table access");
                    goto got_exception;
                }

#if WASM_ENABLE_GC == 0
                PUSH_I32(tbl_inst->elems[elem_idx]);
#else
                PUSH_REF(tbl_inst->elems[elem_idx]);
#endif
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_TABLE_SET)
            {
                uint32 tbl_idx, elem_idx;
                WASMTableInstance *tbl_inst;
                table_elem_type_t elem_val;

                tbl_idx = read_uint32(frame_ip);
                bh_assert(tbl_idx < module->table_count);

                tbl_inst = wasm_get_table_inst(module, tbl_idx);

#if WASM_ENABLE_GC == 0
                elem_val = POP_I32();
#else
                elem_val = POP_REF();
#endif
                elem_idx = POP_I32();
                if (elem_idx >= tbl_inst->cur_size) {
                    wasm_set_exception(module, "out of bounds table access");
                    goto got_exception;
                }

                tbl_inst->elems[elem_idx] = elem_val;
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_REF_NULL)
            {
#if WASM_ENABLE_GC == 0
                PUSH_I32(NULL_REF);
#else
                PUSH_REF(NULL_REF);
#endif
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_REF_IS_NULL)
            {
#if WASM_ENABLE_GC == 0
                uint32 ref_val;
                ref_val = POP_I32();
#else
                void *ref_val;
                ref_val = POP_REF();
#endif
                PUSH_I32(ref_val == NULL_REF ? 1 : 0);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_REF_FUNC)
            {
                uint32 func_idx = read_uint32(frame_ip);

#if WASM_ENABLE_GC == 0
                PUSH_I32(func_idx);
#else
                SYNC_ALL_TO_FRAME();
                if (!(gc_obj = wasm_create_func_obj(module, func_idx, true,
                                                    NULL, 0))) {
                    goto got_exception;
                }
                PUSH_REF(gc_obj);
#endif
                HANDLE_OP_END();
            }
#endif /* end of WASM_ENABLE_REF_TYPES != 0 || WASM_ENABLE_GC != 0 */

#if WASM_ENABLE_GC != 0
            HANDLE_OP(WASM_OP_CALL_REF)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                func_obj = POP_REF();
                if (!func_obj) {
                    wasm_set_exception(module, "null function object");
                    goto got_exception;
                }

                fidx = wasm_func_obj_get_func_idx_bound(func_obj);
                cur_func = module->e->functions + fidx;
                goto call_func_from_interp;
            }
            HANDLE_OP(WASM_OP_RETURN_CALL_REF)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                func_obj = POP_REF();
                if (!func_obj) {
                    wasm_set_exception(module, "null function object");
                    goto got_exception;
                }

                fidx = wasm_func_obj_get_func_idx_bound(func_obj);
                cur_func = module->e->functions + fidx;
                goto call_func_from_return_call;
            }
            HANDLE_OP(WASM_OP_REF_AS_NON_NULL)
            {
                opnd_off = GET_OFFSET();
                gc_obj = GET_REF_FROM_ADDR(frame_lp + opnd_off);
                if (gc_obj == NULL_REF) {
                    wasm_set_exception(module, "null reference");
                    goto got_exception;
                }
                HANDLE_OP_END();
            }
            HANDLE_OP(WASM_OP_REF_EQ)
            {
                WASMObjectRef gc_obj1, gc_obj2;
                gc_obj2 = POP_REF();
                gc_obj1 = POP_REF();
                val = wasm_obj_equal(gc_obj1, gc_obj2);
                PUSH_I32(val);
                HANDLE_OP_END();
            }
            HANDLE_OP(WASM_OP_BR_ON_NULL)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                opnd_off = GET_OFFSET();
                gc_obj = GET_REF_FROM_ADDR(frame_lp + opnd_off);
                if (gc_obj == NULL_REF) {
                    CLEAR_FRAME_REF(opnd_off);
                    goto recover_br_info;
                }
                else {
                    SKIP_BR_INFO();
                }
                HANDLE_OP_END();
            }
            HANDLE_OP(WASM_OP_BR_ON_NON_NULL)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                opnd_off = GET_OFFSET();
                gc_obj = GET_REF_FROM_ADDR(frame_lp + opnd_off);
                if (gc_obj != NULL_REF) {
                    goto recover_br_info;
                }
                else {
                    CLEAR_FRAME_REF(opnd_off);
                    SKIP_BR_INFO();
                }
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_GC_PREFIX)
            {
                GET_OPCODE();

                switch (opcode) {
                    case WASM_OP_STRUCT_NEW_CANON:
                    case WASM_OP_STRUCT_NEW_CANON_DEFAULT:
                    {
                        WASMModule *wasm_module = module->module;
                        WASMStructType *struct_type;
                        WASMRttType *rtt_type;
                        WASMValue field_value = { 0 };

                        type_idx = read_uint32(frame_ip);
                        struct_type =
                            (WASMStructType *)module->module->types[type_idx];

                        if (!(rtt_type = wasm_rtt_type_new(
                                  (WASMType *)struct_type, type_idx,
                                  wasm_module->rtt_types,
                                  wasm_module->type_count,
                                  &wasm_module->rtt_type_lock))) {
                            wasm_set_exception(module,
                                               "create rtt type failed");
                            goto got_exception;
                        }

                        SYNC_ALL_TO_FRAME();
                        struct_obj = wasm_struct_obj_new(exec_env, rtt_type);
                        if (!struct_obj) {
                            wasm_set_exception(module,
                                               "create struct object failed");
                            goto got_exception;
                        }

                        if (opcode == WASM_OP_STRUCT_NEW_CANON) {
                            WASMStructFieldType *fields = struct_type->fields;
                            int32 field_count = (int32)struct_type->field_count;
                            int32 field_idx;
                            uint8 field_type;

                            for (field_idx = field_count - 1; field_idx >= 0;
                                 field_idx--) {
                                field_type = fields[field_idx].field_type;
                                if (wasm_is_type_reftype(field_type)) {
                                    field_value.gc_obj = POP_REF();
                                }
                                else if (field_type == VALUE_TYPE_I32
                                         || field_type == VALUE_TYPE_F32
                                         || field_type == PACKED_TYPE_I8
                                         || field_type == PACKED_TYPE_I16) {
                                    field_value.i32 = POP_I32();
                                }
                                else {
                                    field_value.i64 = POP_I64();
                                }
                                wasm_struct_obj_set_field(struct_obj, field_idx,
                                                          &field_value);
                            }
                        }
                        PUSH_REF(struct_obj);
                        HANDLE_OP_END();
                    }
                    case WASM_OP_STRUCT_GET:
                    case WASM_OP_STRUCT_GET_S:
                    case WASM_OP_STRUCT_GET_U:
                    {
                        WASMStructType *struct_type;
                        WASMValue field_value = { 0 };
                        uint32 field_idx;
                        uint8 field_type;

                        type_idx = read_uint32(frame_ip);
                        field_idx = read_uint32(frame_ip);

                        struct_type =
                            (WASMStructType *)module->module->types[type_idx];

                        struct_obj = POP_REF();

                        if (!struct_obj) {
                            wasm_set_exception(module, "null structure object");
                            goto got_exception;
                        }
                        if (field_idx >= struct_type->field_count) {
                            wasm_set_exception(
                                module, "struct field index out of bounds");
                            goto got_exception;
                        }

                        wasm_struct_obj_get_field(
                            struct_obj, field_idx,
                            opcode == WASM_OP_STRUCT_GET_S ? true : false,
                            &field_value);

                        field_type = struct_type->fields[field_idx].field_type;
                        if (wasm_is_reftype_i31ref(field_type)) {
                            PUSH_I31REF(field_value.gc_obj);
                        }
                        else if (wasm_is_type_reftype(field_type)) {
                            PUSH_REF(field_value.gc_obj);
                        }
                        else if (field_type == VALUE_TYPE_I32
                                 || field_type == VALUE_TYPE_F32
                                 || field_type == PACKED_TYPE_I8
                                 || field_type == PACKED_TYPE_I16) {
                            PUSH_I32(field_value.i32);
                        }
                        else {
                            PUSH_I64(field_value.i64);
                        }
                        HANDLE_OP_END();
                    }
                    case WASM_OP_STRUCT_SET:
                    {
                        WASMStructType *struct_type;
                        WASMValue field_value = { 0 };
                        uint32 field_idx;
                        uint8 field_type;

                        type_idx = read_uint32(frame_ip);
                        field_idx = read_uint32(frame_ip);

                        struct_type =
                            (WASMStructType *)module->module->types[type_idx];
                        field_type = struct_type->fields[field_idx].field_type;

                        if (wasm_is_type_reftype(field_type)) {
                            field_value.gc_obj = POP_REF();
                        }
                        else if (field_type == VALUE_TYPE_I32
                                 || field_type == VALUE_TYPE_F32
                                 || field_type == PACKED_TYPE_I8
                                 || field_type == PACKED_TYPE_I16) {
                            field_value.i32 = POP_I32();
                        }
                        else {
                            field_value.i64 = POP_I64();
                        }

                        struct_obj = POP_REF();
                        if (!struct_obj) {
                            wasm_set_exception(module, "null structure object");
                            goto got_exception;
                        }
                        if (field_idx >= struct_type->field_count) {
                            wasm_set_exception(
                                module, "struct field index out of bounds");
                            goto got_exception;
                        }

                        wasm_struct_obj_set_field(struct_obj, field_idx,
                                                  &field_value);
                        HANDLE_OP_END();
                    }
                    case WASM_OP_ARRAY_NEW_CANON:
                    case WASM_OP_ARRAY_NEW_CANON_DEFAULT:
                    case WASM_OP_ARRAY_NEW_CANON_FIXED:
                    {
                        WASMModule *wasm_module = module->module;
                        WASMArrayType *array_type;
                        WASMRttType *rtt_type;
                        WASMValue array_elem = { 0 };
                        uint32 array_len, i;

                        type_idx = read_uint32(frame_ip);
                        array_type =
                            (WASMArrayType *)wasm_module->types[type_idx];

                        if (!(rtt_type = wasm_rtt_type_new(
                                  (WASMType *)array_type, type_idx,
                                  wasm_module->rtt_types,
                                  wasm_module->type_count,
                                  &wasm_module->rtt_type_lock))) {
                            wasm_set_exception(module,
                                               "create rtt type failed");
                            goto got_exception;
                        }

                        if (opcode != WASM_OP_ARRAY_NEW_CANON_FIXED)
                            array_len = POP_I32();
                        else
                            array_len = read_uint32(frame_ip);

                        if (opcode == WASM_OP_ARRAY_NEW_CANON) {
                            if (wasm_is_type_reftype(array_type->elem_type)) {
                                array_elem.gc_obj = POP_REF();
                            }
                            else if (array_type->elem_type == VALUE_TYPE_I32
                                     || array_type->elem_type == VALUE_TYPE_F32
                                     || array_type->elem_type == PACKED_TYPE_I8
                                     || array_type->elem_type
                                            == PACKED_TYPE_I16) {
                                array_elem.i32 = POP_I32();
                            }
                            else {
                                array_elem.i64 = POP_I64();
                            }
                        }

                        SYNC_ALL_TO_FRAME();
                        array_obj = wasm_array_obj_new(exec_env, rtt_type,
                                                       array_len, &array_elem);
                        if (!array_obj) {
                            wasm_set_exception(module,
                                               "create array object failed");
                            goto got_exception;
                        }

                        if (opcode == WASM_OP_ARRAY_NEW_CANON_FIXED) {
                            for (i = 0; i < array_len; i++) {
                                if (wasm_is_type_reftype(
                                        array_type->elem_type)) {
                                    array_elem.gc_obj = POP_REF();
                                }
                                else if (array_type->elem_type == VALUE_TYPE_I32
                                         || array_type->elem_type
                                                == VALUE_TYPE_F32
                                         || array_type->elem_type
                                                == PACKED_TYPE_I8
                                         || array_type->elem_type
                                                == PACKED_TYPE_I16) {
                                    array_elem.i32 = POP_I32();
                                }
                                else {
                                    array_elem.i64 = POP_I64();
                                }
                                wasm_array_obj_set_elem(
                                    array_obj, array_len - 1 - i, &array_elem);
                            }
                        }

                        PUSH_REF(array_obj);
                        HANDLE_OP_END();
                    }
                    case WASM_OP_ARRAY_NEW_CANON_DATA:
                    {
                        WASMModule *wasm_module = module->module;
                        WASMArrayType *array_type;
                        WASMRttType *rtt_type;
                        WASMValue array_elem = { 0 };
                        WASMDataSeg *data_seg;
                        uint8 *array_elem_base;
                        uint32 array_len, data_seg_idx, data_seg_offset;
                        uint32 elem_size = 0;
                        uint64 total_size;

                        type_idx = read_uint32(frame_ip);
                        data_seg_idx = read_uint32(frame_ip);
                        data_seg = wasm_module->data_segments[data_seg_idx];

                        array_type =
                            (WASMArrayType *)wasm_module->types[type_idx];

                        if (!(rtt_type = wasm_rtt_type_new(
                                  (WASMType *)array_type, type_idx,
                                  wasm_module->rtt_types,
                                  wasm_module->type_count,
                                  &wasm_module->rtt_type_lock))) {
                            wasm_set_exception(module,
                                               "create rtt type failed");
                            goto got_exception;
                        }

                        array_len = POP_I32();
                        data_seg_offset = POP_I32();

                        switch (array_type->elem_type) {
                            case PACKED_TYPE_I8:
                                elem_size = 1;
                                break;
                            case PACKED_TYPE_I16:
                                elem_size = 2;
                                break;
                            case VALUE_TYPE_I32:
                            case VALUE_TYPE_F32:
                                elem_size = 4;
                                break;
                            case VALUE_TYPE_I64:
                            case VALUE_TYPE_F64:
                                elem_size = 8;
                                break;
                            default:
                                bh_assert(0);
                        }

                        total_size = (uint64)elem_size * array_len;
                        if (data_seg_offset >= data_seg->data_length
                            || total_size
                                   > data_seg->data_length - data_seg_offset) {
                            wasm_set_exception(module,
                                               "data segment out of bounds");
                            goto got_exception;
                        }

                        SYNC_ALL_TO_FRAME();
                        array_obj = wasm_array_obj_new(exec_env, rtt_type,
                                                       array_len, &array_elem);
                        if (!array_obj) {
                            wasm_set_exception(module,
                                               "create array object failed");
                            goto got_exception;
                        }

                        array_elem_base =
                            (uint8 *)wasm_array_obj_first_elem_addr(array_obj);
                        bh_memcpy_s(array_elem_base, (uint32)total_size,
                                    data_seg->data + data_seg_offset,
                                    (uint32)total_size);

                        PUSH_REF(array_obj);
                        HANDLE_OP_END();
                    }
                    case WASM_OP_ARRAY_NEW_CANON_ELEM:
                    {
                        /* TODO */
                        wasm_set_exception(module, "unsupported opcode");
                        goto got_exception;
                    }
                    case WASM_OP_ARRAY_GET:
                    case WASM_OP_ARRAY_GET_S:
                    case WASM_OP_ARRAY_GET_U:
                    {
                        WASMArrayType *array_type;
                        WASMValue array_elem = { 0 };
                        uint32 elem_idx, elem_size_log;

                        type_idx = read_uint32(frame_ip);
                        array_type =
                            (WASMArrayType *)module->module->types[type_idx];

                        elem_idx = POP_I32();
                        array_obj = POP_REF();

                        if (!array_obj) {
                            wasm_set_exception(module, "null array object");
                            goto got_exception;
                        }
                        if (elem_idx >= wasm_array_obj_length(array_obj)) {
                            wasm_set_exception(module,
                                               "array index out of bounds");
                            goto got_exception;
                        }

                        wasm_array_obj_get_elem(
                            array_obj, elem_idx,
                            opcode == WASM_OP_ARRAY_GET_S ? true : false,
                            &array_elem);
                        elem_size_log = wasm_array_obj_elem_size_log(array_obj);

                        if (wasm_is_reftype_i31ref(array_type->elem_type)) {
                            PUSH_I31REF(array_elem.gc_obj);
                        }
                        else if (wasm_is_type_reftype(array_type->elem_type)) {
                            PUSH_REF(array_elem.gc_obj);
                        }
                        else if (elem_size_log < 3) {
                            PUSH_I32(array_elem.i32);
                        }
                        else {
                            PUSH_I64(array_elem.i64);
                        }
                        HANDLE_OP_END();
                    }
                    case WASM_OP_ARRAY_SET:
                    {
                        WASMArrayType *array_type;
                        WASMValue array_elem = { 0 };
                        uint32 elem_idx;

                        type_idx = read_uint32(frame_ip);
                        array_type =
                            (WASMArrayType *)module->module->types[type_idx];
                        if (wasm_is_type_reftype(array_type->elem_type)) {
                            array_elem.gc_obj = POP_REF();
                        }
                        else if (array_type->elem_type == VALUE_TYPE_I32
                                 || array_type->elem_type == VALUE_TYPE_F32
                                 || array_type->elem_type == PACKED_TYPE_I8
                                 || array_type->elem_type == PACKED_TYPE_I16) {
                            array_elem.i32 = POP_I32();
                        }
                        else {
                            array_elem.i64 = POP_I64();
                        }

                        elem_idx = POP_I32();
                        array_obj = POP_REF();

                        if (!array_obj) {
                            wasm_set_exception(module, "null array object");
                            goto got_exception;
                        }
                        if (elem_idx >= wasm_array_obj_length(array_obj)) {
                            wasm_set_exception(module,
                                               "array index out of bounds");
                            goto got_exception;
                        }

                        wasm_array_obj_set_elem(array_obj, elem_idx,
                                                &array_elem);
                        HANDLE_OP_END();
                    }
#if WASM_ENABLE_GC_BINARYEN != 0
                    case WASM_OP_ARRAY_COPY:
                    {
                        uint32 dst_offset, src_offset, len, src_type_index;
                        WASMArrayObjectRef src_obj, dst_obj;

                        type_idx = read_uint32(frame_ip);
                        src_type_index = read_uint32(frame_ip);

                        len = POP_I32();
                        src_offset = POP_I32();
                        src_obj = POP_REF();
                        dst_offset = POP_I32();
                        dst_obj = POP_REF();

                        if (!src_obj || !dst_obj) {
                            wasm_set_exception(module, "null array object");
                            goto got_exception;
                        }

                        if (len > 0) {
                            if ((dst_offset > UINT32_MAX - len)
                                || (dst_offset + len
                                    > wasm_array_obj_length(dst_obj))
                                || (src_offset > UINT32_MAX - len)
                                || (src_offset + len
                                    > wasm_array_obj_length(src_obj))) {
                                wasm_set_exception(module,
                                                   "array index out of bounds");
                                goto got_exception;
                            }

                            wasm_array_obj_copy(dst_obj, dst_offset, src_obj,
                                                src_offset, len);
                        }

                        (void)src_type_index;
                        HANDLE_OP_END();
                    }
#endif
                    case WASM_OP_ARRAY_LEN:
                    {
                        uint32 array_len;
                        array_obj = POP_REF();
                        if (!array_obj) {
                            wasm_set_exception(module, "null array object");
                            goto got_exception;
                        }
                        array_len = wasm_array_obj_length(array_obj);
                        PUSH_I32(array_len);
                        HANDLE_OP_END();
                    }

                    case WASM_OP_I31_NEW:
                    {
                        uint32 i31_val;

                        i31_val = POP_I32();
                        i31_obj = wasm_i31_obj_new(i31_val);
                        PUSH_I31REF(i31_obj);
                        HANDLE_OP_END();
                    }
                    case WASM_OP_I31_GET_S:
                    case WASM_OP_I31_GET_U:
                    {
                        uint32 i31_val;

                        i31_obj = (WASMI31ObjectRef)POP_REF();
                        if (!i31_obj) {
                            wasm_set_exception(module, "null i31 reference");
                            goto got_exception;
                        }
                        i31_val = wasm_i31_obj_get_value(
                            i31_obj,
                            opcode == WASM_OP_I31_GET_S ? true : false);
                        PUSH_I32(i31_val);
                        HANDLE_OP_END();
                    }

                    case WASM_OP_REF_TEST:
                    case WASM_OP_REF_CAST:
                    case WASM_OP_REF_TEST_NULLABLE:
                    case WASM_OP_REF_CAST_NULLABLE:
                    {
                        int32 heap_type;

                        heap_type = (int32)read_uint32(frame_ip);

                        gc_obj = POP_REF();
                        if (!gc_obj) {
                            if (opcode == WASM_OP_REF_TEST
                                || opcode == WASM_OP_REF_TEST_NULLABLE) {
                                if (opcode == WASM_OP_REF_TEST)
                                    PUSH_I32(0);
                                else
                                    PUSH_I32(1);
                            }
                            else if (opcode == WASM_OP_REF_CAST) {
                                wasm_set_exception(module, "cast failure");
                                goto got_exception;
                            }
                            else {
                                PUSH_REF(gc_obj);
                            }
                        }
                        else {
                            bool castable = false;

                            if (heap_type >= 0) {
                                WASMModule *wasm_module = module->module;
                                castable = wasm_obj_is_instance_of(
                                    gc_obj, (uint32)heap_type,
                                    wasm_module->types,
                                    wasm_module->type_count);
                            }
                            else {
                                castable =
                                    wasm_obj_is_type_of(gc_obj, heap_type);
                            }

                            if (opcode == WASM_OP_REF_TEST
                                || opcode == WASM_OP_REF_TEST_NULLABLE) {
                                if (castable)
                                    PUSH_I32(1);
                                else
                                    PUSH_I32(0);
                            }
                            else if (!castable) {
                                wasm_set_exception(module, "cast failure");
                                goto got_exception;
                            }
                            else {
                                PUSH_REF(gc_obj);
                            }
                        }
                        HANDLE_OP_END();
                    }

                    case WASM_OP_BR_ON_CAST:
                    case WASM_OP_BR_ON_CAST_NULLABLE:
                    case WASM_OP_BR_ON_CAST_FAIL:
                    case WASM_OP_BR_ON_CAST_FAIL_NULLABLE:
                    {
                        int32 heap_type;
                        uint16 opnd_off_br;

#if WASM_ENABLE_THREAD_MGR != 0
                        CHECK_SUSPEND_FLAGS();
#endif
                        heap_type = (int32)read_uint32(frame_ip);

                        opnd_off = GET_OFFSET();
                        opnd_off_br = GET_OFFSET();
                        gc_obj = GET_REF_FROM_ADDR(frame_lp + opnd_off);
                        PUT_REF_TO_ADDR(frame_lp + opnd_off_br, gc_obj);

                        if (!gc_obj) {
                            if (opcode == WASM_OP_BR_ON_CAST_NULLABLE
                                || opcode == WASM_OP_BR_ON_CAST_FAIL) {
                                CLEAR_FRAME_REF(opnd_off);
                                if (!wasm_is_reftype_i31ref(heap_type)) {
                                    SET_FRAME_REF(opnd_off_br);
                                }
                                goto recover_br_info;
                            }
                        }
                        else {
                            bool castable = false;

                            if (heap_type >= 0) {
                                WASMModule *wasm_module = module->module;
                                castable = wasm_obj_is_instance_of(
                                    gc_obj, (uint32)heap_type,
                                    wasm_module->types,
                                    wasm_module->type_count);
                            }
                            else {
                                castable =
                                    wasm_obj_is_type_of(gc_obj, heap_type);
                            }

                            if ((castable
                                 && (opcode == WASM_OP_BR_ON_CAST
                                     || opcode == WASM_OP_BR_ON_CAST_NULLABLE))
                                || (!castable
                                    && (opcode == WASM_OP_BR_ON_CAST_FAIL
                                        || opcode
                                               == WASM_OP_BR_ON_CAST_FAIL_NULLABLE))) {
                                CLEAR_FRAME_REF(opnd_off);
                                if (!wasm_is_reftype_i31ref(heap_type)) {
                                    SET_FRAME_REF(opnd_off_br);
                                }
                                goto recover_br_info;
                            }
                        }
                        SKIP_BR_INFO();
                        HANDLE_OP_END();
                    }

                    case WASM_OP_EXTERN_INTERNALIZE:
                    {
                        externref_obj = POP_REF();
                        if (externref_obj == NULL_REF)
                            PUSH_REF(NULL_REF);
                        else {
                            gc_obj = wasm_externref_obj_to_internal_obj(
                                externref_obj);
                            PUSH_REF(gc_obj);
                        }
                        HANDLE_OP_END();
                    }
                    case WASM_OP_EXTERN_EXTERNALIZE:
                    {
                        gc_obj = POP_REF();
                        if (gc_obj == NULL_REF)
                            PUSH_REF(NULL_REF);
                        else {
                            if (!(externref_obj =
                                      wasm_internal_obj_to_externref_obj(
                                          exec_env, gc_obj))) {
                                wasm_set_exception(
                                    module, "create externref object failed");
                                goto got_exception;
                            }
                            PUSH_REF(externref_obj);
                        }
                        HANDLE_OP_END();
                    }

                    default:
                    {
                        wasm_set_exception(module, "unsupported opcode");
                        goto got_exception;
                    }
                }
            }
#endif /* end of WASM_ENABLE_GC != 0 */

            /* variable instructions */
            HANDLE_OP(EXT_OP_SET_LOCAL_FAST)
            HANDLE_OP(EXT_OP_TEE_LOCAL_FAST)
            {
                /* clang-format off */
#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
                    local_offset = *frame_ip++;
#else
                    local_offset = *frame_ip;
                    frame_ip += 2;
#endif
                /* clang-format on */
                *(uint32 *)(frame_lp + local_offset) =
                    GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                HANDLE_OP_END();
            }

            HANDLE_OP(EXT_OP_SET_LOCAL_FAST_I64)
            HANDLE_OP(EXT_OP_TEE_LOCAL_FAST_I64)
            {
                /* clang-format off */
#if WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS != 0
                    local_offset = *frame_ip++;
#else
                    local_offset = *frame_ip;
                    frame_ip += 2;
#endif
                /* clang-format on */
                PUT_I64_TO_ADDR((uint32 *)(frame_lp + local_offset),
                                GET_OPERAND(uint64, I64, 0));
                frame_ip += 2;
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_GET_GLOBAL)
            {
                global_idx = read_uint32(frame_ip);
                bh_assert(global_idx < module->e->global_count);
                global = globals + global_idx;
                global_addr = get_global_addr(global_data, global);
                addr_ret = GET_OFFSET();
                /* clang-format off */
#if WASM_ENABLE_GC == 0
                frame_lp[addr_ret] = *(uint32 *)global_addr;
#else
                if (!wasm_is_type_reftype(global->type))
                    frame_lp[addr_ret] = *(uint32 *)global_addr;
                else {
                    PUT_REF_TO_ADDR(frame_lp + addr_ret,
                                    GET_REF_FROM_ADDR((uint32 *)global_addr));
                    if (!wasm_is_reftype_i31ref(global->type)) {
                        SET_FRAME_REF(addr_ret);
                    }
                }
#endif
                /* clang-format on */
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_GET_GLOBAL_64)
            {
                global_idx = read_uint32(frame_ip);
                bh_assert(global_idx < module->e->global_count);
                global = globals + global_idx;
                global_addr = get_global_addr(global_data, global);
                addr_ret = GET_OFFSET();
                PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                GET_I64_FROM_ADDR((uint32 *)global_addr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_SET_GLOBAL)
            {
                global_idx = read_uint32(frame_ip);
                bh_assert(global_idx < module->e->global_count);
                global = globals + global_idx;
                global_addr = get_global_addr(global_data, global);
                addr1 = GET_OFFSET();
                /* clang-format off */
#if WASM_ENABLE_GC == 0
                *(int32 *)global_addr = frame_lp[addr1];
#else
                if (!wasm_is_type_reftype(global->type))
                    *(int32 *)global_addr = frame_lp[addr1];
                else {
                    PUT_REF_TO_ADDR((uint32 *)global_addr,
                                    GET_REF_FROM_ADDR(frame_lp + addr1));
                    CLEAR_FRAME_REF(addr1);
                }
#endif
                /* clang-format on */
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_SET_GLOBAL_AUX_STACK)
            {
                uint32 aux_stack_top;

                global_idx = read_uint32(frame_ip);
                bh_assert(global_idx < module->e->global_count);
                global = globals + global_idx;
                global_addr = get_global_addr(global_data, global);
                aux_stack_top = frame_lp[GET_OFFSET()];
                if (aux_stack_top <= exec_env->aux_stack_boundary.boundary) {
                    wasm_set_exception(module, "wasm auxiliary stack overflow");
                    goto got_exception;
                }
                if (aux_stack_top > exec_env->aux_stack_bottom.bottom) {
                    wasm_set_exception(module,
                                       "wasm auxiliary stack underflow");
                    goto got_exception;
                }
                *(int32 *)global_addr = aux_stack_top;
#if WASM_ENABLE_MEMORY_PROFILING != 0
                if (module->module->aux_stack_top_global_index != (uint32)-1) {
                    uint32 aux_stack_used = module->module->aux_stack_bottom
                                            - *(uint32 *)global_addr;
                    if (aux_stack_used > module->e->max_aux_stack_used)
                        module->e->max_aux_stack_used = aux_stack_used;
                }
#endif
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_SET_GLOBAL_64)
            {
                global_idx = read_uint32(frame_ip);
                bh_assert(global_idx < module->e->global_count);
                global = globals + global_idx;
                global_addr = get_global_addr(global_data, global);
                addr1 = GET_OFFSET();
                PUT_I64_TO_ADDR((uint32 *)global_addr,
                                GET_I64_FROM_ADDR(frame_lp + addr1));
                HANDLE_OP_END();
            }

            /* memory load instructions */
            HANDLE_OP(WASM_OP_I32_LOAD)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(4);
                frame_lp[addr_ret] = LOAD_I32(maddr);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(8);
                PUT_I64_TO_ADDR(frame_lp + addr_ret, LOAD_I64(maddr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LOAD8_S)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(1);
                frame_lp[addr_ret] = sign_ext_8_32(*(int8 *)maddr);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LOAD8_U)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(1);
                frame_lp[addr_ret] = (uint32)(*(uint8 *)maddr);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LOAD16_S)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(2);
                frame_lp[addr_ret] = sign_ext_16_32(LOAD_I16(maddr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LOAD16_U)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(2);
                frame_lp[addr_ret] = (uint32)(LOAD_U16(maddr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD8_S)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(1);
                PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                sign_ext_8_64(*(int8 *)maddr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD8_U)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(1);
                PUT_I64_TO_ADDR(frame_lp + addr_ret, (uint64)(*(uint8 *)maddr));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD16_S)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(2);
                PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                sign_ext_16_64(LOAD_I16(maddr)));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD16_U)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(2);
                PUT_I64_TO_ADDR(frame_lp + addr_ret, (uint64)(LOAD_U16(maddr)));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD32_S)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(4);
                PUT_I64_TO_ADDR(frame_lp + addr_ret,
                                sign_ext_32_64(LOAD_I32(maddr)));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LOAD32_U)
            {
                uint32 offset, addr;
                offset = read_uint32(frame_ip);
                addr = GET_OPERAND(uint32, I32, 0);
                frame_ip += 2;
                addr_ret = GET_OFFSET();
                CHECK_MEMORY_OVERFLOW(4);
                PUT_I64_TO_ADDR(frame_lp + addr_ret, (uint64)(LOAD_U32(maddr)));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_STORE)
            {
                uint32 offset, addr;
                uint32 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint32, I32, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(4);
                STORE_U32(maddr, sval);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_STORE8)
            {
                uint32 offset, addr;
                uint32 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint32, I32, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(1);
                *(uint8 *)maddr = (uint8)sval;
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_STORE16)
            {
                uint32 offset, addr;
                uint32 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint32, I32, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(2);
                STORE_U16(maddr, (uint16)sval);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_STORE)
            {
                uint32 offset, addr;
                uint64 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint64, I64, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(8);
                STORE_I64(maddr, sval);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_STORE8)
            {
                uint32 offset, addr;
                uint64 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint64, I64, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(1);
                *(uint8 *)maddr = (uint8)sval;
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_STORE16)
            {
                uint32 offset, addr;
                uint64 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint64, I64, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(2);
                STORE_U16(maddr, (uint16)sval);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_STORE32)
            {
                uint32 offset, addr;
                uint64 sval;
                offset = read_uint32(frame_ip);
                sval = GET_OPERAND(uint64, I64, 0);
                addr = GET_OPERAND(uint32, I32, 2);
                frame_ip += 4;
                CHECK_MEMORY_OVERFLOW(4);
                STORE_U32(maddr, (uint32)sval);
                HANDLE_OP_END();
            }

            /* memory size and memory grow instructions */
            HANDLE_OP(WASM_OP_MEMORY_SIZE)
            {
                uint32 reserved;
                addr_ret = GET_OFFSET();
                frame_lp[addr_ret] = memory->cur_page_count;
                (void)reserved;
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_MEMORY_GROW)
            {
                uint32 reserved, delta,
                    prev_page_count = memory->cur_page_count;

                addr1 = GET_OFFSET();
                addr_ret = GET_OFFSET();
                delta = (uint32)frame_lp[addr1];

                if (!wasm_enlarge_memory(module, delta)) {
                    /* failed to memory.grow, return -1 */
                    frame_lp[addr_ret] = -1;
                }
                else {
                    /* success, return previous page count */
                    frame_lp[addr_ret] = prev_page_count;
                    /* update memory size, no need to update memory ptr as
                       it isn't changed in wasm_enlarge_memory */
#if !defined(OS_ENABLE_HW_BOUND_CHECK)              \
    || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0 \
    || WASM_ENABLE_BULK_MEMORY != 0
                    linear_mem_size = memory->memory_data_size;
#endif
                }

                (void)reserved;
                HANDLE_OP_END();
            }

            /* constant instructions */
            HANDLE_OP(WASM_OP_F64_CONST)
            HANDLE_OP(WASM_OP_I64_CONST)
            {
                uint8 *orig_ip = frame_ip;

                frame_ip += sizeof(uint64);
                addr_ret = GET_OFFSET();

                bh_memcpy_s(frame_lp + addr_ret, sizeof(uint64), orig_ip,
                            sizeof(uint64));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_CONST)
            HANDLE_OP(WASM_OP_I32_CONST)
            {
                uint8 *orig_ip = frame_ip;

                frame_ip += sizeof(uint32);
                addr_ret = GET_OFFSET();

                bh_memcpy_s(frame_lp + addr_ret, sizeof(uint32), orig_ip,
                            sizeof(uint32));
                HANDLE_OP_END();
            }

            /* comparison instructions of i32 */
            HANDLE_OP(WASM_OP_I32_EQZ)
            {
                DEF_OP_EQZ(int32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_EQ)
            {
                DEF_OP_CMP(uint32, I32, ==);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_NE)
            {
                DEF_OP_CMP(uint32, I32, !=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LT_S)
            {
                DEF_OP_CMP(int32, I32, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LT_U)
            {
                DEF_OP_CMP(uint32, I32, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_GT_S)
            {
                DEF_OP_CMP(int32, I32, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_GT_U)
            {
                DEF_OP_CMP(uint32, I32, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LE_S)
            {
                DEF_OP_CMP(int32, I32, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_LE_U)
            {
                DEF_OP_CMP(uint32, I32, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_GE_S)
            {
                DEF_OP_CMP(int32, I32, >=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_GE_U)
            {
                DEF_OP_CMP(uint32, I32, >=);
                HANDLE_OP_END();
            }

            /* comparison instructions of i64 */
            HANDLE_OP(WASM_OP_I64_EQZ)
            {
                DEF_OP_EQZ(int64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_EQ)
            {
                DEF_OP_CMP(uint64, I64, ==);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_NE)
            {
                DEF_OP_CMP(uint64, I64, !=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LT_S)
            {
                DEF_OP_CMP(int64, I64, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LT_U)
            {
                DEF_OP_CMP(uint64, I64, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_GT_S)
            {
                DEF_OP_CMP(int64, I64, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_GT_U)
            {
                DEF_OP_CMP(uint64, I64, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LE_S)
            {
                DEF_OP_CMP(int64, I64, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_LE_U)
            {
                DEF_OP_CMP(uint64, I64, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_GE_S)
            {
                DEF_OP_CMP(int64, I64, >=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_GE_U)
            {
                DEF_OP_CMP(uint64, I64, >=);
                HANDLE_OP_END();
            }

            /* comparison instructions of f32 */
            HANDLE_OP(WASM_OP_F32_EQ)
            {
                DEF_OP_CMP(float32, F32, ==);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_NE)
            {
                DEF_OP_CMP(float32, F32, !=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_LT)
            {
                DEF_OP_CMP(float32, F32, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_GT)
            {
                DEF_OP_CMP(float32, F32, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_LE)
            {
                DEF_OP_CMP(float32, F32, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_GE)
            {
                DEF_OP_CMP(float32, F32, >=);
                HANDLE_OP_END();
            }

            /* comparison instructions of f64 */
            HANDLE_OP(WASM_OP_F64_EQ)
            {
                DEF_OP_CMP(float64, F64, ==);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_NE)
            {
                DEF_OP_CMP(float64, F64, !=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_LT)
            {
                DEF_OP_CMP(float64, F64, <);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_GT)
            {
                DEF_OP_CMP(float64, F64, >);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_LE)
            {
                DEF_OP_CMP(float64, F64, <=);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_GE)
            {
                DEF_OP_CMP(float64, F64, >=);
                HANDLE_OP_END();
            }

            /* numberic instructions of i32 */
            HANDLE_OP(WASM_OP_I32_CLZ)
            {
                DEF_OP_BIT_COUNT(uint32, I32, clz32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_CTZ)
            {
                DEF_OP_BIT_COUNT(uint32, I32, ctz32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_POPCNT)
            {
                DEF_OP_BIT_COUNT(uint32, I32, popcount32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_ADD)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, +);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_SUB)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, -);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_MUL)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, *);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_DIV_S)
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
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_DIV_U)
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
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_REM_S)
            {
                int32 a, b;

                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                addr_ret = GET_OFFSET();

                b = frame_lp[addr1];
                a = frame_lp[addr2];
                if (a == (int32)0x80000000 && b == -1) {
                    frame_lp[addr_ret] = 0;
                    HANDLE_OP_END();
                }
                if (b == 0) {
                    wasm_set_exception(module, "integer divide by zero");
                    goto got_exception;
                }
                frame_lp[addr_ret] = (a % b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_REM_U)
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
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_AND)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, &);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_OR)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, |);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_XOR)
            {
                DEF_OP_NUMERIC(uint32, uint32, I32, ^);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_SHL)
            {
                DEF_OP_NUMERIC2(uint32, uint32, I32, <<);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_SHR_S)
            {
                DEF_OP_NUMERIC2(int32, uint32, I32, >>);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_SHR_U)
            {
                DEF_OP_NUMERIC2(uint32, uint32, I32, >>);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_ROTL)
            {
                uint32 a, b;

                b = (uint32)frame_lp[GET_OFFSET()];
                a = (uint32)frame_lp[GET_OFFSET()];
                frame_lp[GET_OFFSET()] = rotl32(a, b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_ROTR)
            {
                uint32 a, b;

                b = (uint32)frame_lp[GET_OFFSET()];
                a = (uint32)frame_lp[GET_OFFSET()];
                frame_lp[GET_OFFSET()] = rotr32(a, b);
                HANDLE_OP_END();
            }

            /* numberic instructions of i64 */
            HANDLE_OP(WASM_OP_I64_CLZ)
            {
                DEF_OP_BIT_COUNT(uint64, I64, clz64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_CTZ)
            {
                DEF_OP_BIT_COUNT(uint64, I64, ctz64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_POPCNT)
            {
                DEF_OP_BIT_COUNT(uint64, I64, popcount64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_ADD)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, +);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_SUB)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, -);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_MUL)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, *);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_DIV_S)
            {
                int64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                if (a == (int64)0x8000000000000000LL && b == -1) {
                    wasm_set_exception(module, "integer overflow");
                    goto got_exception;
                }
                if (b == 0) {
                    wasm_set_exception(module, "integer divide by zero");
                    goto got_exception;
                }
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), a / b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_DIV_U)
            {
                uint64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                if (b == 0) {
                    wasm_set_exception(module, "integer divide by zero");
                    goto got_exception;
                }
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), a / b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_REM_S)
            {
                int64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                if (a == (int64)0x8000000000000000LL && b == -1) {
                    *(int64 *)(frame_lp + GET_OFFSET()) = 0;
                    HANDLE_OP_END();
                }
                if (b == 0) {
                    wasm_set_exception(module, "integer divide by zero");
                    goto got_exception;
                }
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), a % b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_REM_U)
            {
                uint64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                if (b == 0) {
                    wasm_set_exception(module, "integer divide by zero");
                    goto got_exception;
                }
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), a % b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_AND)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, &);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_OR)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, |);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_XOR)
            {
                DEF_OP_NUMERIC_64(uint64, uint64, I64, ^);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_SHL)
            {
                DEF_OP_NUMERIC2_64(uint64, uint64, I64, <<);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_SHR_S)
            {
                DEF_OP_NUMERIC2_64(int64, uint64, I64, >>);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_SHR_U)
            {
                DEF_OP_NUMERIC2_64(uint64, uint64, I64, >>);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_ROTL)
            {
                uint64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), rotl64(a, b));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_ROTR)
            {
                uint64 a, b;

                b = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                a = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(), rotr64(a, b));
                HANDLE_OP_END();
            }

            /* numberic instructions of f32 */
            HANDLE_OP(WASM_OP_F32_ABS)
            {
                DEF_OP_MATH(float32, F32, fabsf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_NEG)
            {
                uint32 u32 = frame_lp[GET_OFFSET()];
                uint32 sign_bit = u32 & ((uint32)1 << 31);
                addr_ret = GET_OFFSET();
                if (sign_bit)
                    frame_lp[addr_ret] = u32 & ~((uint32)1 << 31);
                else
                    frame_lp[addr_ret] = u32 | ((uint32)1 << 31);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_CEIL)
            {
                DEF_OP_MATH(float32, F32, ceilf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_FLOOR)
            {
                DEF_OP_MATH(float32, F32, floorf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_TRUNC)
            {
                DEF_OP_MATH(float32, F32, truncf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_NEAREST)
            {
                DEF_OP_MATH(float32, F32, rintf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_SQRT)
            {
                DEF_OP_MATH(float32, F32, sqrtf);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_ADD)
            {
                DEF_OP_NUMERIC(float32, float32, F32, +);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_SUB)
            {
                DEF_OP_NUMERIC(float32, float32, F32, -);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_MUL)
            {
                DEF_OP_NUMERIC(float32, float32, F32, *);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_DIV)
            {
                DEF_OP_NUMERIC(float32, float32, F32, /);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_MIN)
            {
                float32 a, b;

                b = *(float32 *)(frame_lp + GET_OFFSET());
                a = *(float32 *)(frame_lp + GET_OFFSET());

                *(float32 *)(frame_lp + GET_OFFSET()) = f32_min(a, b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_MAX)
            {
                float32 a, b;

                b = *(float32 *)(frame_lp + GET_OFFSET());
                a = *(float32 *)(frame_lp + GET_OFFSET());

                *(float32 *)(frame_lp + GET_OFFSET()) = f32_max(a, b);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_COPYSIGN)
            {
                float32 a, b;

                b = *(float32 *)(frame_lp + GET_OFFSET());
                a = *(float32 *)(frame_lp + GET_OFFSET());
                *(float32 *)(frame_lp + GET_OFFSET()) = local_copysignf(a, b);
                HANDLE_OP_END();
            }

            /* numberic instructions of f64 */
            HANDLE_OP(WASM_OP_F64_ABS)
            {
                DEF_OP_MATH(float64, F64, fabs);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_NEG)
            {
                uint64 u64 = GET_I64_FROM_ADDR(frame_lp + GET_OFFSET());
                uint64 sign_bit = u64 & (((uint64)1) << 63);
                if (sign_bit)
                    PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(),
                                    (u64 & ~(((uint64)1) << 63)));
                else
                    PUT_I64_TO_ADDR(frame_lp + GET_OFFSET(),
                                    (u64 | (((uint64)1) << 63)));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_CEIL)
            {
                DEF_OP_MATH(float64, F64, ceil);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_FLOOR)
            {
                DEF_OP_MATH(float64, F64, floor);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_TRUNC)
            {
                DEF_OP_MATH(float64, F64, trunc);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_NEAREST)
            {
                DEF_OP_MATH(float64, F64, rint);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_SQRT)
            {
                DEF_OP_MATH(float64, F64, sqrt);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_ADD)
            {
                DEF_OP_NUMERIC_64(float64, float64, F64, +);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_SUB)
            {
                DEF_OP_NUMERIC_64(float64, float64, F64, -);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_MUL)
            {
                DEF_OP_NUMERIC_64(float64, float64, F64, *);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_DIV)
            {
                DEF_OP_NUMERIC_64(float64, float64, F64, /);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_MIN)
            {
                float64 a, b;

                b = POP_F64();
                a = POP_F64();

                PUSH_F64(f64_min(a, b));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_MAX)
            {
                float64 a, b;

                b = POP_F64();
                a = POP_F64();

                PUSH_F64(f64_max(a, b));
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_COPYSIGN)
            {
                float64 a, b;

                b = POP_F64();
                a = POP_F64();
                PUSH_F64(local_copysign(a, b));
                HANDLE_OP_END();
            }

            /* conversions of i32 */
            HANDLE_OP(WASM_OP_I32_WRAP_I64)
            {
                int32 value = (int32)(POP_I64() & 0xFFFFFFFFLL);
                PUSH_I32(value);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_TRUNC_S_F32)
            {
                /* We don't use INT32_MIN/INT32_MAX/UINT32_MIN/UINT32_MAX,
                   since float/double values of ieee754 cannot precisely
                   represent all int32/uint32/int64/uint64 values, e.g.:
                   UINT32_MAX is 4294967295, but (float32)4294967295 is
                   4294967296.0f, but not 4294967295.0f. */
                DEF_OP_TRUNC_F32(-2147483904.0f, 2147483648.0f, true, true);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_TRUNC_U_F32)
            {
                DEF_OP_TRUNC_F32(-1.0f, 4294967296.0f, true, false);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_TRUNC_S_F64)
            {
                DEF_OP_TRUNC_F64(-2147483649.0, 2147483648.0, true, true);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_TRUNC_U_F64)
            {
                DEF_OP_TRUNC_F64(-1.0, 4294967296.0, true, false);
                HANDLE_OP_END();
            }

            /* conversions of i64 */
            HANDLE_OP(WASM_OP_I64_EXTEND_S_I32)
            {
                DEF_OP_CONVERT(int64, I64, int32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_EXTEND_U_I32)
            {
                DEF_OP_CONVERT(int64, I64, uint32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_TRUNC_S_F32)
            {
                DEF_OP_TRUNC_F32(-9223373136366403584.0f,
                                 9223372036854775808.0f, false, true);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_TRUNC_U_F32)
            {
                DEF_OP_TRUNC_F32(-1.0f, 18446744073709551616.0f, false, false);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_TRUNC_S_F64)
            {
                DEF_OP_TRUNC_F64(-9223372036854777856.0, 9223372036854775808.0,
                                 false, true);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_TRUNC_U_F64)
            {
                DEF_OP_TRUNC_F64(-1.0, 18446744073709551616.0, false, false);
                HANDLE_OP_END();
            }

            /* conversions of f32 */
            HANDLE_OP(WASM_OP_F32_CONVERT_S_I32)
            {
                DEF_OP_CONVERT(float32, F32, int32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_CONVERT_U_I32)
            {
                DEF_OP_CONVERT(float32, F32, uint32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_CONVERT_S_I64)
            {
                DEF_OP_CONVERT(float32, F32, int64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_CONVERT_U_I64)
            {
                DEF_OP_CONVERT(float32, F32, uint64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F32_DEMOTE_F64)
            {
                DEF_OP_CONVERT(float32, F32, float64, F64);
                HANDLE_OP_END();
            }

            /* conversions of f64 */
            HANDLE_OP(WASM_OP_F64_CONVERT_S_I32)
            {
                DEF_OP_CONVERT(float64, F64, int32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_CONVERT_U_I32)
            {
                DEF_OP_CONVERT(float64, F64, uint32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_CONVERT_S_I64)
            {
                DEF_OP_CONVERT(float64, F64, int64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_CONVERT_U_I64)
            {
                DEF_OP_CONVERT(float64, F64, uint64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_F64_PROMOTE_F32)
            {
                DEF_OP_CONVERT(float64, F64, float32, F32);
                HANDLE_OP_END();
            }

            /* reinterpretations */
            HANDLE_OP(WASM_OP_I32_REINTERPRET_F32)
            HANDLE_OP(WASM_OP_F32_REINTERPRET_I32)
            {
                DEF_OP_REINTERPRET(uint32, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_REINTERPRET_F64)
            HANDLE_OP(WASM_OP_F64_REINTERPRET_I64)
            {
                DEF_OP_REINTERPRET(int64, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(EXT_OP_COPY_STACK_TOP)
            {
                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                frame_lp[addr2] = frame_lp[addr1];

#if WASM_ENABLE_GC != 0
                if (*FRAME_REF(addr1)) {
                    SET_FRAME_REF(addr2);
                    CLEAR_FRAME_REF(addr1);
                }
#endif

                HANDLE_OP_END();
            }

            HANDLE_OP(EXT_OP_COPY_STACK_TOP_I64)
            {
                addr1 = GET_OFFSET();
                addr2 = GET_OFFSET();
                frame_lp[addr2] = frame_lp[addr1];
                frame_lp[addr2 + 1] = frame_lp[addr1 + 1];

#if WASM_ENABLE_GC != 0
                if (*FRAME_REF(addr1)) {
                    SET_FRAME_REF(addr2);
                    SET_FRAME_REF(addr2 + 1);
                    CLEAR_FRAME_REF(addr1);
                    CLEAR_FRAME_REF(addr1 + 1);
                }
#endif

                HANDLE_OP_END();
            }

            HANDLE_OP(EXT_OP_COPY_STACK_VALUES)
            {
                uint32 values_count, total_cell;
                uint8 *cells;
                int16 *src_offsets = NULL;
                uint16 *dst_offsets = NULL;

                /* read values_count */
                values_count = read_uint32(frame_ip);
                /* read total cell num */
                total_cell = read_uint32(frame_ip);
                /* cells */
                cells = (uint8 *)frame_ip;
                frame_ip += values_count * CELL_SIZE;
                /* src offsets */
                src_offsets = (int16 *)frame_ip;
                frame_ip += values_count * sizeof(int16);
                /* dst offsets */
                dst_offsets = (uint16 *)frame_ip;
                frame_ip += values_count * sizeof(uint16);

                if (!copy_stack_values(module, frame_lp, values_count,
#if WASM_ENABLE_GC != 0
                                       frame_ref,
#endif
                                       total_cell, cells, src_offsets,
                                       dst_offsets))
                    goto got_exception;

                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_SET_LOCAL)
            {
                opcode = WASM_OP_SET_LOCAL;
                goto handle_op_set_tee_local;
            }
            HANDLE_OP(WASM_OP_TEE_LOCAL)
            {
                opcode = WASM_OP_TEE_LOCAL;
            handle_op_set_tee_local:

                GET_LOCAL_INDEX_TYPE_AND_OFFSET();
                addr1 = GET_OFFSET();

                if (local_type == VALUE_TYPE_I32
                    || local_type == VALUE_TYPE_F32) {
                    *(int32 *)(frame_lp + local_offset) = frame_lp[addr1];
                }
                else if (local_type == VALUE_TYPE_I64
                         || local_type == VALUE_TYPE_F64) {
                    PUT_I64_TO_ADDR((uint32 *)(frame_lp + local_offset),
                                    GET_I64_FROM_ADDR(frame_lp + addr1));
                }
#if WASM_ENABLE_GC != 0
                else if (wasm_is_type_reftype(local_type)) {
                    PUT_REF_TO_ADDR((uint32 *)(frame_lp + local_offset),
                                    GET_REF_FROM_ADDR(frame_lp + addr1));
                    if (opcode == WASM_OP_SET_LOCAL) {
                        CLEAR_FRAME_REF(addr1);
                    }
                }
#endif
                else {
                    wasm_set_exception(module, "invalid local type");
                    goto got_exception;
                }

                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_EXTEND8_S)
            {
                DEF_OP_CONVERT(int32, I32, int8, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I32_EXTEND16_S)
            {
                DEF_OP_CONVERT(int32, I32, int16, I32);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_EXTEND8_S)
            {
                DEF_OP_CONVERT(int64, I64, int8, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_EXTEND16_S)
            {
                DEF_OP_CONVERT(int64, I64, int16, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_I64_EXTEND32_S)
            {
                DEF_OP_CONVERT(int64, I64, int32, I64);
                HANDLE_OP_END();
            }

            HANDLE_OP(WASM_OP_MISC_PREFIX)
            {
                GET_OPCODE();
                switch (opcode) {
                    case WASM_OP_I32_TRUNC_SAT_S_F32:
                        DEF_OP_TRUNC_SAT_F32(-2147483904.0f, 2147483648.0f,
                                             true, true);
                        break;
                    case WASM_OP_I32_TRUNC_SAT_U_F32:
                        DEF_OP_TRUNC_SAT_F32(-1.0f, 4294967296.0f, true, false);
                        break;
                    case WASM_OP_I32_TRUNC_SAT_S_F64:
                        DEF_OP_TRUNC_SAT_F64(-2147483649.0, 2147483648.0, true,
                                             true);
                        break;
                    case WASM_OP_I32_TRUNC_SAT_U_F64:
                        DEF_OP_TRUNC_SAT_F64(-1.0, 4294967296.0, true, false);
                        break;
                    case WASM_OP_I64_TRUNC_SAT_S_F32:
                        DEF_OP_TRUNC_SAT_F32(-9223373136366403584.0f,
                                             9223372036854775808.0f, false,
                                             true);
                        break;
                    case WASM_OP_I64_TRUNC_SAT_U_F32:
                        DEF_OP_TRUNC_SAT_F32(-1.0f, 18446744073709551616.0f,
                                             false, false);
                        break;
                    case WASM_OP_I64_TRUNC_SAT_S_F64:
                        DEF_OP_TRUNC_SAT_F64(-9223372036854777856.0,
                                             9223372036854775808.0, false,
                                             true);
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
                        uint8 *data;

                        segment = read_uint32(frame_ip);

                        bytes = (uint64)POP_I32();
                        offset = (uint64)POP_I32();
                        addr = POP_I32();

#if WASM_ENABLE_THREAD_MGR
                        linear_mem_size = memory->memory_data_size;
#endif

#ifndef OS_ENABLE_HW_BOUND_CHECK
                        CHECK_BULK_MEMORY_OVERFLOW(addr, bytes, maddr);
#else
                        if ((uint64)(uint32)addr + bytes
                            > (uint64)linear_mem_size)
                            goto out_of_bounds;
                        maddr = memory->memory_data + (uint32)addr;
#endif

                        seg_len = (uint64)module->module->data_segments[segment]
                                      ->data_length;
                        data = module->module->data_segments[segment]->data;
                        if (offset + bytes > seg_len)
                            goto out_of_bounds;

                        bh_memcpy_s(maddr, linear_mem_size - addr,
                                    data + offset, (uint32)bytes);
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

#if WASM_ENABLE_THREAD_MGR
                        linear_mem_size = memory->memory_data_size;
#endif

#ifndef OS_ENABLE_HW_BOUND_CHECK
                        CHECK_BULK_MEMORY_OVERFLOW(src, len, msrc);
                        CHECK_BULK_MEMORY_OVERFLOW(dst, len, mdst);
#else
                        if ((uint64)(uint32)src + len > (uint64)linear_mem_size)
                            goto out_of_bounds;
                        msrc = memory->memory_data + (uint32)src;

                        if ((uint64)(uint32)dst + len > (uint64)linear_mem_size)
                            goto out_of_bounds;
                        mdst = memory->memory_data + (uint32)dst;
#endif

                        /* allowing the destination and source to overlap */
                        bh_memmove_s(mdst, linear_mem_size - dst, msrc, len);
                        break;
                    }
                    case WASM_OP_MEMORY_FILL:
                    {
                        uint32 dst, len;
                        uint8 fill_val, *mdst;

                        len = POP_I32();
                        fill_val = POP_I32();
                        dst = POP_I32();

#if WASM_ENABLE_THREAD_MGR
                        linear_mem_size = memory->memory_data_size;
#endif

#ifndef OS_ENABLE_HW_BOUND_CHECK
                        CHECK_BULK_MEMORY_OVERFLOW(dst, len, mdst);
#else
                        if ((uint64)(uint32)dst + len > (uint64)linear_mem_size)
                            goto out_of_bounds;
                        mdst = memory->memory_data + (uint32)dst;
#endif

                        memset(mdst, fill_val, len);
                        break;
                    }
#endif /* WASM_ENABLE_BULK_MEMORY */
#if WASM_ENABLE_REF_TYPES != 0 || WASM_ENABLE_GC != 0
                    case WASM_OP_TABLE_INIT:
                    {
                        uint32 tbl_idx, elem_idx;
                        uint64 n, s, d;
                        WASMTableInstance *tbl_inst;
#if WASM_ENABLE_GC != 0
                        void **table_elems;
                        uintptr_t *func_indexes;
                        uint64 i;
#endif

                        elem_idx = read_uint32(frame_ip);
                        bh_assert(elem_idx < module->module->table_seg_count);

                        tbl_idx = read_uint32(frame_ip);
                        bh_assert(tbl_idx < module->module->table_count);

                        tbl_inst = wasm_get_table_inst(module, tbl_idx);

                        n = (uint32)POP_I32();
                        s = (uint32)POP_I32();
                        d = (uint32)POP_I32();

                        if (!n) {
                            break;
                        }

                        if (n + s > module->module->table_segments[elem_idx]
                                        .function_count
                            || d + n > tbl_inst->cur_size) {
                            wasm_set_exception(module,
                                               "out of bounds table access");
                            goto got_exception;
                        }

                        if (module->module->table_segments[elem_idx]
                                .is_dropped) {
                            wasm_set_exception(module,
                                               "out of bounds table access");
                            goto got_exception;
                        }

                        if (!wasm_elem_is_passive(
                                module->module->table_segments[elem_idx]
                                    .mode)) {
                            wasm_set_exception(module,
                                               "out of bounds table access");
                            goto got_exception;
                        }

#if WASM_ENABLE_GC == 0
                        bh_memcpy_s((uint8 *)tbl_inst
                                        + offsetof(WASMTableInstance, elems)
                                        + d * sizeof(table_elem_type_t),
                                    (uint32)((tbl_inst->cur_size - d)
                                             * sizeof(table_elem_type_t)),
                                    module->module->table_segments[elem_idx]
                                            .func_indexes
                                        + s,
                                    (uint32)(n * sizeof(table_elem_type_t)));
#else
                        SYNC_ALL_TO_FRAME();
                        table_elems = tbl_inst->elems + d;
                        func_indexes = module->module->table_segments[elem_idx]
                                           .func_indexes
                                       + s;
                        for (i = 0; i < n; i++) {
                            /* UINT32_MAX indicates that it is an null ref */
                            if (func_indexes[i] != UINT32_MAX) {
                                if (!(func_obj = wasm_create_func_obj(
                                          module, func_indexes[i], true, NULL,
                                          0))) {
                                    goto got_exception;
                                }
                                table_elems[i] = func_obj;
                            }
                            else {
                                table_elems[i] = NULL_REF;
                            }
                        }
#endif
                        break;
                    }
                    case WASM_OP_ELEM_DROP:
                    {
                        uint32 elem_idx = read_uint32(frame_ip);
                        bh_assert(elem_idx < module->module->table_seg_count);

                        module->module->table_segments[elem_idx].is_dropped =
                            true;
                        break;
                    }
                    case WASM_OP_TABLE_COPY:
                    {
                        uint32 src_tbl_idx, dst_tbl_idx;
                        uint64 n, s, d;
                        WASMTableInstance *src_tbl_inst, *dst_tbl_inst;

                        dst_tbl_idx = read_uint32(frame_ip);
                        bh_assert(dst_tbl_idx < module->table_count);

                        dst_tbl_inst = wasm_get_table_inst(module, dst_tbl_idx);

                        src_tbl_idx = read_uint32(frame_ip);
                        bh_assert(src_tbl_idx < module->table_count);

                        src_tbl_inst = wasm_get_table_inst(module, src_tbl_idx);

                        n = (uint32)POP_I32();
                        s = (uint32)POP_I32();
                        d = (uint32)POP_I32();

                        if (d + n > dst_tbl_inst->cur_size
                            || s + n > src_tbl_inst->cur_size) {
                            wasm_set_exception(module,
                                               "out of bounds table access");
                            goto got_exception;
                        }

                        /* if s >= d, copy from front to back */
                        /* if s < d, copy from back to front */
                        /* merge all together */
                        bh_memmove_s((uint8 *)dst_tbl_inst
                                         + offsetof(WASMTableInstance, elems)
                                         + d * sizeof(table_elem_type_t),
                                     (uint32)((dst_tbl_inst->cur_size - d)
                                              * sizeof(table_elem_type_t)),
                                     (uint8 *)src_tbl_inst
                                         + offsetof(WASMTableInstance, elems)
                                         + s * sizeof(table_elem_type_t),
                                     (uint32)(n * sizeof(table_elem_type_t)));
                        break;
                    }
                    case WASM_OP_TABLE_GROW:
                    {
                        uint32 tbl_idx, n, orig_tbl_sz;
                        WASMTableInstance *tbl_inst;
                        table_elem_type_t init_val;

                        tbl_idx = read_uint32(frame_ip);
                        bh_assert(tbl_idx < module->table_count);

                        tbl_inst = wasm_get_table_inst(module, tbl_idx);

                        orig_tbl_sz = tbl_inst->cur_size;

                        n = POP_I32();
#if WASM_ENABLE_GC == 0
                        init_val = POP_I32();
#else
                        init_val = POP_REF();
#endif

                        if (!wasm_enlarge_table(module, tbl_idx, n, init_val)) {
                            PUSH_I32(-1);
                        }
                        else {
                            PUSH_I32(orig_tbl_sz);
                        }

                        break;
                    }
                    case WASM_OP_TABLE_SIZE:
                    {
                        uint32 tbl_idx;
                        WASMTableInstance *tbl_inst;

                        tbl_idx = read_uint32(frame_ip);
                        bh_assert(tbl_idx < module->table_count);

                        tbl_inst = wasm_get_table_inst(module, tbl_idx);

                        PUSH_I32(tbl_inst->cur_size);
                        break;
                    }
                    case WASM_OP_TABLE_FILL:
                    {
                        uint32 tbl_idx, n, i;
                        WASMTableInstance *tbl_inst;
                        table_elem_type_t fill_val;

                        tbl_idx = read_uint32(frame_ip);
                        bh_assert(tbl_idx < module->table_count);

                        tbl_inst = wasm_get_table_inst(module, tbl_idx);

                        n = POP_I32();
#if WASM_ENABLE_GC == 0
                        fill_val = POP_I32();
#else
                        fill_val = POP_REF();
#endif
                        i = POP_I32();

                        if (i + n > tbl_inst->cur_size) {
                            wasm_set_exception(module,
                                               "out of bounds table access");
                            goto got_exception;
                        }

                        for (; n != 0; i++, n--) {
                            tbl_inst->elems[i] = fill_val;
                        }

                        break;
                    }
#endif /* WASM_ENABLE_REF_TYPES */
                    default:
                        wasm_set_exception(module, "unsupported opcode");
                        goto got_exception;
                }
                HANDLE_OP_END();
            }

#if WASM_ENABLE_SHARED_MEMORY != 0
            HANDLE_OP(WASM_OP_ATOMIC_PREFIX)
            {
                uint32 offset = 0, addr;

                GET_OPCODE();

                if (opcode != WASM_OP_ATOMIC_FENCE) {
                    offset = read_uint32(frame_ip);
                }

                switch (opcode) {
                    case WASM_OP_ATOMIC_NOTIFY:
                    {
                        uint32 notify_count, ret;

                        notify_count = POP_I32();
                        addr = POP_I32();
                        CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                        CHECK_ATOMIC_MEMORY_ACCESS(4);

                        ret = wasm_runtime_atomic_notify(
                            (WASMModuleInstanceCommon *)module, maddr,
                            notify_count);
                        if (ret == (uint32)-1)
                            goto got_exception;

                        PUSH_I32(ret);
                        break;
                    }
                    case WASM_OP_ATOMIC_WAIT32:
                    {
                        uint64 timeout;
                        uint32 expect, ret;

                        timeout = POP_I64();
                        expect = POP_I32();
                        addr = POP_I32();
                        CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                        CHECK_ATOMIC_MEMORY_ACCESS(4);

                        ret = wasm_runtime_atomic_wait(
                            (WASMModuleInstanceCommon *)module, maddr,
                            (uint64)expect, timeout, false);
                        if (ret == (uint32)-1)
                            goto got_exception;

#if WASM_ENABLE_THREAD_MGR != 0
                        CHECK_SUSPEND_FLAGS();
#endif

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

                        ret = wasm_runtime_atomic_wait(
                            (WASMModuleInstanceCommon *)module, maddr, expect,
                            timeout, true);
                        if (ret == (uint32)-1)
                            goto got_exception;

#if WASM_ENABLE_THREAD_MGR != 0
                        CHECK_SUSPEND_FLAGS();
#endif

                        PUSH_I32(ret);
                        break;
                    }
                    case WASM_OP_ATOMIC_FENCE:
                    {
                        os_atomic_thread_fence(os_memory_order_seq_cst);
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
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint32)(*(uint8 *)maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I32_LOAD16_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint32)LOAD_U16(maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = LOAD_I32(maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
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
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)(*(uint8 *)maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I64_LOAD16_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)LOAD_U16(maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I64_LOAD32_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)LOAD_U32(maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(8);
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = LOAD_I64(maddr);
                            os_mutex_unlock(&node->shared_mem_lock);
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
                            os_mutex_lock(&node->shared_mem_lock);
                            *(uint8 *)maddr = (uint8)sval;
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I32_STORE16) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);
                            os_mutex_lock(&node->shared_mem_lock);
                            STORE_U16(maddr, (uint16)sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);
                            os_mutex_lock(&node->shared_mem_lock);
                            STORE_U32(maddr, sval);
                            os_mutex_unlock(&node->shared_mem_lock);
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
                            os_mutex_lock(&node->shared_mem_lock);
                            *(uint8 *)maddr = (uint8)sval;
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I64_STORE16) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);
                            os_mutex_lock(&node->shared_mem_lock);
                            STORE_U16(maddr, (uint16)sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_I64_STORE32) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);
                            os_mutex_lock(&node->shared_mem_lock);
                            STORE_U32(maddr, (uint32)sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(8);
                            os_mutex_lock(&node->shared_mem_lock);
                            STORE_I64(maddr, sval);
                            os_mutex_unlock(&node->shared_mem_lock);
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

                            expect = (uint8)expect;
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint32)(*(uint8 *)maddr);
                            if (readv == expect)
                                *(uint8 *)maddr = (uint8)(sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_RMW_I32_CMPXCHG16_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);

                            expect = (uint16)expect;
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint32)LOAD_U16(maddr);
                            if (readv == expect)
                                STORE_U16(maddr, (uint16)(sval));
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);

                            os_mutex_lock(&node->shared_mem_lock);
                            readv = LOAD_I32(maddr);
                            if (readv == expect)
                                STORE_U32(maddr, sval);
                            os_mutex_unlock(&node->shared_mem_lock);
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

                            expect = (uint8)expect;
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)(*(uint8 *)maddr);
                            if (readv == expect)
                                *(uint8 *)maddr = (uint8)(sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_RMW_I64_CMPXCHG16_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 2, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(2);

                            expect = (uint16)expect;
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)LOAD_U16(maddr);
                            if (readv == expect)
                                STORE_U16(maddr, (uint16)(sval));
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else if (opcode == WASM_OP_ATOMIC_RMW_I64_CMPXCHG32_U) {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 4, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(4);

                            expect = (uint32)expect;
                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)LOAD_U32(maddr);
                            if (readv == expect)
                                STORE_U32(maddr, (uint32)(sval));
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        else {
                            CHECK_BULK_MEMORY_OVERFLOW(addr + offset, 8, maddr);
                            CHECK_ATOMIC_MEMORY_ACCESS(8);

                            os_mutex_lock(&node->shared_mem_lock);
                            readv = (uint64)LOAD_I64(maddr);
                            if (readv == expect)
                                STORE_I64(maddr, sval);
                            os_mutex_unlock(&node->shared_mem_lock);
                        }
                        PUSH_I64(readv);
                        break;
                    }

                        DEF_ATOMIC_RMW_OPCODE(ADD, +);
                        DEF_ATOMIC_RMW_OPCODE(SUB, -);
                        DEF_ATOMIC_RMW_OPCODE(AND, &);
                        DEF_ATOMIC_RMW_OPCODE(OR, |);
                        DEF_ATOMIC_RMW_OPCODE(XOR, ^);
                        /* xchg, ignore the read value, and store the given
                          value: readv * 0 + sval */
                        DEF_ATOMIC_RMW_OPCODE(XCHG, *0 +);
                }

                HANDLE_OP_END();
            }
#endif

            HANDLE_OP(WASM_OP_IMPDEP)
            {
                frame = prev_frame;
                frame_ip = frame->ip;
                goto call_func_from_entry;
            }

            HANDLE_OP(WASM_OP_CALL)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                fidx = read_uint32(frame_ip);
#if WASM_ENABLE_MULTI_MODULE != 0
                if (fidx >= module->e->function_count) {
                    wasm_set_exception(module, "unknown function");
                    goto got_exception;
                }
#endif
                cur_func = module->e->functions + fidx;
                goto call_func_from_interp;
            }

#if WASM_ENABLE_TAIL_CALL != 0
            HANDLE_OP(WASM_OP_RETURN_CALL)
            {
#if WASM_ENABLE_THREAD_MGR != 0
                CHECK_SUSPEND_FLAGS();
#endif
                fidx = read_uint32(frame_ip);
#if WASM_ENABLE_MULTI_MODULE != 0
                if (fidx >= module->e->function_count) {
                    wasm_set_exception(module, "unknown function");
                    goto got_exception;
                }
#endif
                cur_func = module->e->functions + fidx;
                goto call_func_from_return_call;
            }
#endif /* WASM_ENABLE_TAIL_CALL */

#if WASM_ENABLE_LABELS_AS_VALUES == 0
            default:
                wasm_set_exception(module, "unsupported opcode");
                goto got_exception;
        }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES != 0
        HANDLE_OP(WASM_OP_UNUSED_0x06)
        HANDLE_OP(WASM_OP_UNUSED_0x07)
        HANDLE_OP(WASM_OP_UNUSED_0x08)
        HANDLE_OP(WASM_OP_UNUSED_0x09)
        HANDLE_OP(WASM_OP_UNUSED_0x0a)
#if WASM_ENABLE_TAIL_CALL == 0
        HANDLE_OP(WASM_OP_RETURN_CALL)
        HANDLE_OP(WASM_OP_RETURN_CALL_INDIRECT)
#endif
#if WASM_ENABLE_SHARED_MEMORY == 0
        HANDLE_OP(WASM_OP_ATOMIC_PREFIX)
#endif
#if WASM_ENABLE_REF_TYPES == 0 && WASM_ENABLE_GC == 0
        HANDLE_OP(WASM_OP_TABLE_GET)
        HANDLE_OP(WASM_OP_TABLE_SET)
        HANDLE_OP(WASM_OP_REF_NULL)
        HANDLE_OP(WASM_OP_REF_IS_NULL)
        HANDLE_OP(WASM_OP_REF_FUNC)
#endif
#if WASM_ENABLE_GC == 0
        /* SELECT_T is converted to SELECT or SELECT_64 */
        HANDLE_OP(WASM_OP_SELECT_T)
#endif
#if WASM_ENABLE_GC == 0
        HANDLE_OP(WASM_OP_CALL_REF)
        HANDLE_OP(WASM_OP_RETURN_CALL_REF)
        HANDLE_OP(WASM_OP_REF_EQ)
        HANDLE_OP(WASM_OP_REF_AS_NON_NULL)
        HANDLE_OP(WASM_OP_BR_ON_NULL)
        HANDLE_OP(WASM_OP_BR_ON_NON_NULL)
        HANDLE_OP(WASM_OP_GC_PREFIX)
#endif
        HANDLE_OP(WASM_OP_UNUSED_0x16)
        HANDLE_OP(WASM_OP_UNUSED_0x17)
        HANDLE_OP(WASM_OP_UNUSED_0x18)
        HANDLE_OP(WASM_OP_UNUSED_0x19)
        HANDLE_OP(WASM_OP_UNUSED_0x27)
        /* optimized op code */
        HANDLE_OP(WASM_OP_F32_STORE)
        HANDLE_OP(WASM_OP_F64_STORE)
        HANDLE_OP(WASM_OP_F32_LOAD)
        HANDLE_OP(WASM_OP_F64_LOAD)
        HANDLE_OP(EXT_OP_GET_LOCAL_FAST)
        HANDLE_OP(WASM_OP_GET_LOCAL)
        HANDLE_OP(WASM_OP_DROP)
        HANDLE_OP(WASM_OP_DROP_64)
        HANDLE_OP(WASM_OP_BLOCK)
        HANDLE_OP(WASM_OP_LOOP)
        HANDLE_OP(WASM_OP_END)
        HANDLE_OP(WASM_OP_NOP)
        HANDLE_OP(EXT_OP_BLOCK)
        HANDLE_OP(EXT_OP_LOOP)
        HANDLE_OP(EXT_OP_IF)
        HANDLE_OP(EXT_OP_BR_TABLE_CACHE)
        {
            wasm_set_exception(module, "unsupported opcode");
            goto got_exception;
        }
#endif

#if WASM_ENABLE_LABELS_AS_VALUES == 0
        continue;
#else
    FETCH_OPCODE_AND_DISPATCH();
#endif

#if WASM_ENABLE_TAIL_CALL != 0 || WASM_ENABLE_GC != 0
    call_func_from_return_call:
    {
        uint32 *lp_base = NULL, *lp = NULL;
        int i;

        if (cur_func->param_cell_num > 0
            && !(lp_base = lp = wasm_runtime_malloc(cur_func->param_cell_num
                                                    * sizeof(uint32)))) {
            wasm_set_exception(module, "allocate memory failed");
            goto got_exception;
        }
        for (i = 0; i < cur_func->param_count; i++) {
            if (cur_func->param_types[i] == VALUE_TYPE_I64
                || cur_func->param_types[i] == VALUE_TYPE_F64) {
                PUT_I64_TO_ADDR(
                    lp, GET_OPERAND(uint64, I64,
                                    2 * (cur_func->param_count - i - 1)));
                lp += 2;
            }
            else {
                *lp = GET_OPERAND(uint32, I32,
                                  (2 * (cur_func->param_count - i - 1)));
                lp++;
            }
        }
        frame->lp = frame->operand + cur_func->const_cell_num;
        if (lp - lp_base > 0) {
            word_copy(frame->lp, lp_base, lp - lp_base);
        }
        if (lp_base)
            wasm_runtime_free(lp_base);
        FREE_FRAME(exec_env, frame);
        frame_ip += cur_func->param_count * sizeof(int16);
        wasm_exec_env_set_cur_frame(exec_env, (WASMRuntimeFrame *)prev_frame);
        goto call_func_from_entry;
    }
#endif /* WASM_ENABLE_TAIL_CALL != 0 || WASM_ENABLE_GC != 0 */

    call_func_from_interp:
    {
        /* Only do the copy when it's called from interpreter. */
        WASMInterpFrame *outs_area = wasm_exec_env_wasm_stack_top(exec_env);
        int i;

#if WASM_ENABLE_MULTI_MODULE != 0
        if (cur_func->is_import_func) {
            outs_area->lp = outs_area->operand
                            + (cur_func->import_func_inst
                                   ? cur_func->import_func_inst->const_cell_num
                                   : 0);
        }
        else
#endif
        {
            outs_area->lp = outs_area->operand + cur_func->const_cell_num;
        }

        if ((uint8 *)(outs_area->lp + cur_func->param_cell_num)
            > exec_env->wasm_stack.s.top_boundary) {
            wasm_set_exception(module, "wasm operand stack overflow");
            goto got_exception;
        }

        for (i = 0; i < cur_func->param_count; i++) {
            if (cur_func->param_types[i] == VALUE_TYPE_I64
                || cur_func->param_types[i] == VALUE_TYPE_F64) {
                PUT_I64_TO_ADDR(
                    outs_area->lp,
                    GET_OPERAND(uint64, I64,
                                2 * (cur_func->param_count - i - 1)));
                outs_area->lp += 2;
            }
#if WASM_ENABLE_GC != 0
            else if (wasm_is_type_reftype(cur_func->param_types[i])) {
                PUT_REF_TO_ADDR(
                    outs_area->lp,
                    GET_OPERAND(void *, REF,
                                2 * (cur_func->param_count - i - 1)));
                CLEAR_FRAME_REF(
                    *(uint16 *)(frame_ip
                                + (2 * (cur_func->param_count - i - 1))));
                outs_area->lp += REF_CELL_NUM;
            }
#endif
            else {
                *outs_area->lp = GET_OPERAND(
                    uint32, I32, (2 * (cur_func->param_count - i - 1)));
                outs_area->lp++;
            }
        }
        frame_ip += cur_func->param_count * sizeof(int16);
        if (cur_func->ret_cell_num != 0) {
            /* Get the first return value's offset. Since loader emit
             * all return values' offset so we must skip remain return
             * values' offsets.
             */
            WASMFuncType *func_type;
            if (cur_func->is_import_func)
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

            /* update memory size, no need to update memory ptr as
               it isn't changed in wasm_enlarge_memory */
#if !defined(OS_ENABLE_HW_BOUND_CHECK)              \
    || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0 \
    || WASM_ENABLE_BULK_MEMORY != 0
            if (memory)
                linear_mem_size = memory->memory_data_size;
#endif
            if (wasm_copy_exception(module, NULL))
                goto got_exception;
        }
        else {
            WASMFunction *cur_wasm_func = cur_func->u.func;
            uint32 cell_num_of_local_stack;

            cell_num_of_local_stack = cur_func->param_cell_num
                                      + cur_func->local_cell_num
                                      + cur_wasm_func->max_stack_cell_num;
            all_cell_num = cur_func->const_cell_num + cell_num_of_local_stack;
#if WASM_ENABLE_GC != 0
            /* area of frame_ref */
            all_cell_num += (cell_num_of_local_stack + 3) / 4;
            /* cells occupied by locals, POP_REF should not clear frame_ref for
             * these cells */
            local_cell_num =
                cur_func->param_cell_num + cur_func->local_cell_num;
#endif
            /* param_cell_num, local_cell_num, const_cell_num and
               max_stack_cell_num are all no larger than UINT16_MAX (checked
               in loader), all_cell_num must be smaller than 1MB */
            bh_assert(all_cell_num < 1 * BH_MB);

            frame_size = wasm_interp_interp_frame_size(all_cell_num);
            if (!(frame = ALLOC_FRAME(exec_env, frame_size, prev_frame))) {
                frame = prev_frame;
                goto got_exception;
            }

            /* Initialize the interpreter context. */
            frame->function = cur_func;
            frame_ip = wasm_get_func_code(cur_func);
            frame_ip_end = wasm_get_func_code_end(cur_func);

            frame_lp = frame->lp =
                frame->operand + cur_wasm_func->const_cell_num;

            /* Initialize the consts */
            if (cur_wasm_func->const_cell_num > 0) {
                word_copy(frame->operand, (uint32 *)cur_wasm_func->consts,
                          cur_wasm_func->const_cell_num);
            }

            /* Initialize the local variables */
            memset(frame_lp + cur_func->param_cell_num, 0,
                   (uint32)(cur_func->local_cell_num * 4));

#if WASM_ENABLE_GC != 0
            frame_ref = frame->frame_ref =
                (uint8 *)(frame->lp + (uint32)cell_num_of_local_stack);
            init_frame_refs(frame_ref, (uint32)cell_num_of_local_stack,
                            cur_func);
#endif

            wasm_exec_env_set_cur_frame(exec_env, (WASMRuntimeFrame *)frame);
        }
#if WASM_ENABLE_THREAD_MGR != 0
        CHECK_SUSPEND_FLAGS();
#endif
        HANDLE_OP_END();
    }

    return_func:
    {
        FREE_FRAME(exec_env, frame);
        wasm_exec_env_set_cur_frame(exec_env, (WASMRuntimeFrame *)prev_frame);

        if (!prev_frame->ip)
            /* Called from native. */
            return;

        RECOVER_CONTEXT(prev_frame);
#if WASM_ENABLE_GC != 0
        local_cell_num = cur_func->param_cell_num + cur_func->local_cell_num;
#endif
        HANDLE_OP_END();
    }

        (void)frame_ip_end;

#if WASM_ENABLE_SHARED_MEMORY != 0
    unaligned_atomic:
        wasm_set_exception(module, "unaligned atomic");
        goto got_exception;
#endif

#if !defined(OS_ENABLE_HW_BOUND_CHECK)              \
    || WASM_CPU_SUPPORTS_UNALIGNED_ADDR_ACCESS == 0 \
    || WASM_ENABLE_BULK_MEMORY != 0
    out_of_bounds:
        wasm_set_exception(module, "out of bounds memory access");
#endif

    got_exception:
        SYNC_ALL_TO_FRAME();
        return;

#if WASM_ENABLE_LABELS_AS_VALUES == 0
    }
#else
    FETCH_OPCODE_AND_DISPATCH();
#endif
}

#if WASM_ENABLE_LABELS_AS_VALUES != 0
void **
wasm_interp_get_handle_table()
{
    WASMModuleInstance module;
    memset(&module, 0, sizeof(WASMModuleInstance));
    wasm_interp_call_func_bytecode(&module, NULL, NULL, NULL);
    return global_handle_table;
}
#endif

#if WASM_ENABLE_GC != 0
bool
wasm_interp_traverse_gc_rootset(WASMExecEnv *exec_env, void *heap)
{
    WASMInterpFrame *frame;
    WASMObjectRef gc_obj;
    WASMFunctionInstance *cur_func;
    uint8 *frame_ref;
    uint32 local_cell_num, i;

    frame = wasm_exec_env_get_cur_frame(exec_env);
    for (; frame; frame = frame->prev_frame) {
        frame_ref = frame->frame_ref;
        cur_func = frame->function;

        if (!cur_func)
            continue;

        local_cell_num = cur_func->param_cell_num;
        if (frame->ip)
            local_cell_num +=
                cur_func->local_cell_num + cur_func->u.func->max_stack_cell_num;

        for (i = 0; i < local_cell_num; i++) {
            if (frame_ref[i]) {
                gc_obj = GET_REF_FROM_ADDR(frame->lp + i);
                if (wasm_obj_is_created_from_heap(gc_obj)) {
                    if (mem_allocator_add_root((mem_allocator_t)heap, gc_obj)) {
                        return false;
                    }
                }
#if UINTPTR_MAX == UINT64_MAX
                bh_assert(frame_ref[i + 1]);
                i++;
#endif
            }
        }
    }
    return true;
}
#endif

void
wasm_interp_call_wasm(WASMModuleInstance *module_inst, WASMExecEnv *exec_env,
                      WASMFunctionInstance *function, uint32 argc,
                      uint32 argv[])
{
    WASMRuntimeFrame *prev_frame = wasm_exec_env_get_cur_frame(exec_env);
    WASMInterpFrame *frame, *outs_area;

    /* Allocate sufficient cells for all kinds of return values.  */
    unsigned all_cell_num =
                 function->ret_cell_num > 2 ? function->ret_cell_num : 2,
             i;
    /* This frame won't be used by JITed code, so only allocate interp
       frame here.  */
    unsigned frame_size;
    char exception[EXCEPTION_BUF_LEN];

#if WASM_ENABLE_GC != 0
    all_cell_num += (all_cell_num + 3) / 4;
#endif

    frame_size = wasm_interp_interp_frame_size(all_cell_num);

    if (argc < function->param_cell_num) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "invalid argument count %" PRIu32
                 ", must be no smaller than %" PRIu32,
                 argc, (uint32)function->param_cell_num);
        wasm_set_exception(module_inst, buf);
        return;
    }
    argc = function->param_cell_num;

    RECORD_STACK_USAGE(exec_env, (uint8 *)&prev_frame);
#if !(defined(OS_ENABLE_HW_BOUND_CHECK) \
      && WASM_DISABLE_STACK_HW_BOUND_CHECK == 0)
    if ((uint8 *)&prev_frame < exec_env->native_stack_boundary) {
        wasm_set_exception((WASMModuleInstance *)exec_env->module_inst,
                           "native stack overflow");
        return;
    }
#endif

    if (!(frame =
              ALLOC_FRAME(exec_env, frame_size, (WASMInterpFrame *)prev_frame)))
        return;

    outs_area = wasm_exec_env_wasm_stack_top(exec_env);
    frame->function = NULL;
    frame->ip = NULL;
    /* There is no local variable. */
    frame->lp = frame->operand + 0;
#if WASM_ENABLE_GC != 0
    frame->frame_ref =
        (uint8 *)(frame->lp
                  + (function->ret_cell_num > 2 ? function->ret_cell_num : 2));
#endif
    frame->ret_offset = 0;

    if ((uint8 *)(outs_area->operand + function->const_cell_num + argc)
        > exec_env->wasm_stack.s.top_boundary) {
        wasm_set_exception((WASMModuleInstance *)exec_env->module_inst,
                           "wasm operand stack overflow");
        return;
    }

    if (argc > 0)
        word_copy(outs_area->operand + function->const_cell_num, argv, argc);

    wasm_exec_env_set_cur_frame(exec_env, frame);

    if (function->is_import_func) {
#if WASM_ENABLE_MULTI_MODULE != 0
        if (function->import_module_inst) {
            LOG_DEBUG("it is a function of a sub module");
            wasm_interp_call_func_import(module_inst, exec_env, function,
                                         frame);
        }
        else
#endif
        {
            LOG_DEBUG("it is an native function");
            wasm_interp_call_func_native(module_inst, exec_env, function,
                                         frame);
        }
    }
    else {
        wasm_interp_call_func_bytecode(module_inst, exec_env, function, frame);
    }

    /* Output the return value to the caller */
    if (!wasm_copy_exception(module_inst, NULL)) {
        for (i = 0; i < function->ret_cell_num; i++)
            argv[i] = *(frame->lp + i);
    }
    else {
#if WASM_ENABLE_DUMP_CALL_STACK != 0
        if (wasm_interp_create_call_stack(exec_env)) {
            wasm_interp_dump_call_stack(exec_env, true, NULL, 0);
        }
#endif
        wasm_copy_exception(module_inst, exception);
        LOG_DEBUG("meet an exception %s", exception);
    }

    wasm_exec_env_set_cur_frame(exec_env, prev_frame);
    FREE_FRAME(exec_env, frame);
#if WASM_ENABLE_OPCODE_COUNTER != 0
    wasm_interp_dump_op_count();
#endif
}
