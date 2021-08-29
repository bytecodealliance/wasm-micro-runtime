/*
 * Copyright (C) 2021 XiaoMi Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_intrinsic.h"

typedef struct {
    const char *llvm_intrinsic;
    const char *native_intrinsic;
    uint64 flag;
} aot_intrinsic;

static const aot_intrinsic g_intrinsic_mapping[] = {
    { "llvm.experimental.constrained.fadd.f32", "aot_intrinsic_fadd_f32",
      AOT_INTRINSIC_FLAG_F32_FADD },
    { "llvm.experimental.constrained.fadd.f64", "aot_intrinsic_fadd_f64",
      AOT_INTRINSIC_FLAG_F64_FADD },
    { "llvm.experimental.constrained.fsub.f32", "aot_intrinsic_fsub_f32",
      AOT_INTRINSIC_FLAG_F32_FSUB },
    { "llvm.experimental.constrained.fsub.f64", "aot_intrinsic_fsub_f64",
      AOT_INTRINSIC_FLAG_F64_FSUB },
    { "llvm.experimental.constrained.fmul.f32", "aot_intrinsic_fmul_f32",
      AOT_INTRINSIC_FLAG_F32_FMUL },
    { "llvm.experimental.constrained.fmul.f64", "aot_intrinsic_fmul_f64",
      AOT_INTRINSIC_FLAG_F64_FMUL },
    { "llvm.experimental.constrained.fdiv.f32", "aot_intrinsic_fdiv_f32",
      AOT_INTRINSIC_FLAG_F32_FDIV },
    { "llvm.experimental.constrained.fdiv.f64", "aot_intrinsic_fdiv_f64",
      AOT_INTRINSIC_FLAG_F64_FDIV },
    { "llvm.fabs.f32", "aot_intrinsic_fabs_f32", AOT_INTRINSIC_FLAG_F32_FABS },
    { "llvm.fabs.f64", "aot_intrinsic_fabs_f64", AOT_INTRINSIC_FLAG_F64_FABS },
    { "llvm.ceil.f32", "aot_intrinsic_ceil_f32", AOT_INTRINSIC_FLAG_F32_CEIL },
    { "llvm.ceil.f64", "aot_intrinsic_ceil_f64", AOT_INTRINSIC_FLAG_F64_CEIL },
    { "llvm.floor.f32", "aot_intrinsic_floor_f32",
      AOT_INTRINSIC_FLAG_F32_FLOOR },
    { "llvm.floor.f64", "aot_intrinsic_floor_f64",
      AOT_INTRINSIC_FLAG_F64_FLOOR },
    { "llvm.trunc.f32", "aot_intrinsic_trunc_f32",
      AOT_INTRINSIC_FLAG_F32_TRUNC },
    { "llvm.trunc.f64", "aot_intrinsic_trunc_f64",
      AOT_INTRINSIC_FLAG_F64_TRUNC },
    { "llvm.rint.f32", "aot_intrinsic_rint_f32", AOT_INTRINSIC_FLAG_F32_RINT },
    { "llvm.rint.f64", "aot_intrinsic_rint_f64", AOT_INTRINSIC_FLAG_F64_RINT },
    { "llvm.sqrt.f32", "aot_intrinsic_sqrt_f32", AOT_INTRINSIC_FLAG_F32_SQRT },
    { "llvm.sqrt.f64", "aot_intrinsic_sqrt_f64", AOT_INTRINSIC_FLAG_F64_SQRT },
    { "llvm.copysign.f32", "aot_intrinsic_copysign_f32",
      AOT_INTRINSIC_FLAG_F32_COPYSIGN },
    { "llvm.copysign.f64", "aot_intrinsic_copysign_f64",
      AOT_INTRINSIC_FLAG_F64_COPYSIGN },
    { "llvm.minnum.f32", "aot_intrinsic_fmin_f32", AOT_INTRINSIC_FLAG_F32_MIN },
    { "llvm.minnum.f64", "aot_intrinsic_fmin_f64", AOT_INTRINSIC_FLAG_F64_MIN },
    { "llvm.maxnum.f32", "aot_intrinsic_fmax_f32", AOT_INTRINSIC_FLAG_F32_MAX },
    { "llvm.maxnum.f64", "aot_intrinsic_fmax_f64", AOT_INTRINSIC_FLAG_F64_MAX },
    { "llvm.ctlz.i32", "aot_intrinsic_clz_i32", AOT_INTRINSIC_FLAG_I32_CLZ },
    { "llvm.ctlz.i64", "aot_intrinsic_clz_i64", AOT_INTRINSIC_FLAG_I64_CLZ },
    { "llvm.cttz.i32", "aot_intrinsic_ctz_i32", AOT_INTRINSIC_FLAG_I32_CTZ },
    { "llvm.cttz.i64", "aot_intrinsic_ctz_i64", AOT_INTRINSIC_FLAG_I64_CTZ },
    { "llvm.ctpop.i32", "aot_intrinsic_popcnt_i32", AOT_INTRINSIC_FLAG_I32_POPCNT },
    { "llvm.ctpop.i64", "aot_intrinsic_popcnt_i64", AOT_INTRINSIC_FLAG_I64_POPCNT },
};

static const uint32 g_intrinsic_count =
  sizeof(g_intrinsic_mapping) / sizeof(aot_intrinsic);

float32
aot_intrinsic_fadd_f32(float32 a, float32 b)
{
    return a + b;
}

float64
aot_intrinsic_fadd_f64(float64 a, float64 b)
{
    return a + b;
}

float32
aot_intrinsic_fsub_f32(float32 a, float32 b)
{
    return a - b;
}

float64
aot_intrinsic_fsub_f64(float64 a, float64 b)
{
    return a - b;
}

float32
aot_intrinsic_fmul_f32(float32 a, float32 b)
{
    return a * b;
}

float64
aot_intrinsic_fmul_f64(float64 a, float64 b)
{
    return a * b;
}

float32
aot_intrinsic_fdiv_f32(float32 a, float32 b)
{
    return a / b;
}

float64
aot_intrinsic_fdiv_f64(float64 a, float64 b)
{
    return a / b;
}

float32
aot_intrinsic_fabs_f32(float32 a)
{
    return (float32)fabs(a);
}

float64
aot_intrinsic_fabs_f64(float64 a)
{
    return fabs(a);
}

float32
aot_intrinsic_ceil_f32(float32 a)
{
    return (float32)ceilf(a);
}

float64
aot_intrinsic_ceil_f64(float64 a)
{
    return ceil(a);
}

float32
aot_intrinsic_floor_f32(float32 a)
{
    return (float32)floorf(a);
}

float64
aot_intrinsic_floor_f64(float64 a)
{
    return floor(a);
}

float32
aot_intrinsic_trunc_f32(float32 a)
{
    return (float32)trunc(a);
}

float64
aot_intrinsic_trunc_f64(float64 a)
{
    return trunc(a);
}

float32
aot_intrinsic_rint_f32(float32 a)
{
    return (float32)rint(a);
}

float64
aot_intrinsic_rint_f64(float64 a)
{
    return rint(a);
}

float32
aot_intrinsic_sqrt_f32(float32 a)
{
    return (float32)sqrt(a);
}

float64
aot_intrinsic_sqrt_f64(float64 a)
{
    return sqrt(a);
}

float32
aot_intrinsic_copysign_f32(float32 a, float32 b)
{
    return signbit(b) ? (float32)-fabs(a) : (float32)fabs(a);
}

float64
aot_intrinsic_copysign_f64(float64 a, float64 b)
{
    return signbit(b) ? -fabs(a) : fabs(a);
}

float32
aot_intrinsic_fmin_f32(float32 a, float32 b)
{
    if (isnan(a))
        return a;
    else if (isnan(b))
        return b;
    else
        return (float32)fmin(a, b);
}

float64
aot_intrinsic_fmin_f64(float64 a, float64 b)
{
    float64 c = fmin(a, b);
    if (c==0 && a==b)
        return signbit(a) ? a : b;
    return c;
}

float32
aot_intrinsic_fmax_f32(float32 a, float32 b)
{
    if (isnan(a))
        return a;
    else if (isnan(b))
        return b;
    else
        return (float32)fmax(a, b);
}

float64
aot_intrinsic_fmax_f64(float64 a, float64 b)
{
    float64 c = fmax(a, b);
    if (c==0 && a==b)
        return signbit(a) ? b : a;
    return c;
}

uint32
aot_intrinsic_clz_i32(uint32 type)
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

uint32
aot_intrinsic_clz_i64(uint64 type)
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

uint32
aot_intrinsic_ctz_i32(uint32 type)
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

uint32
aot_intrinsic_ctz_i64(uint64 type)
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

uint32
aot_intrinsic_popcnt_i32(uint32 u)
{
    uint32 ret = 0;
    while (u) {
        u = (u & (u - 1));
        ret++;
    }
    return ret;
}

uint32
aot_intrinsic_popcnt_i64(uint64 u)
{
    uint32 ret = 0;
    while (u) {
        u = (u & (u - 1));
        ret++;
    }
    return ret;
}

const char *
aot_intrinsic_get_symbol(const char *llvm_intrinsic)
{
    uint32 cnt;
    for (cnt = 0; cnt < g_intrinsic_count; cnt++) {
        if (!strcmp(llvm_intrinsic, g_intrinsic_mapping[cnt].llvm_intrinsic)) {
            return g_intrinsic_mapping[cnt].native_intrinsic;
        }
    }
    return NULL;
}

#if WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0

static void
add_intrinsic_capability(AOTCompContext *comp_ctx, uint64 flag)
{
    uint64 group = AOT_INTRINSIC_GET_GROUP_FROM_FLAG(flag);
    if (group < sizeof(comp_ctx->flags) / sizeof(uint64)) {
        comp_ctx->flags[group] |= flag;
    }
    else {
        bh_log(BH_LOG_LEVEL_WARNING, __FILE__, __LINE__,
               "intrinsic exceeds max limit.");
    }
}

static void
add_f32_common_intrinsics_for_thumb2_fpu(AOTCompContext *comp_ctx)
{
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_FABS);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_FADD);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_FSUB);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_FMUL);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_FDIV);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F32_SQRT);
}

static void
add_f64_common_intrinsics_for_thumb2_fpu(AOTCompContext *comp_ctx)
{
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_FABS);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_FADD);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_FSUB);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_FMUL);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_FDIV);
    add_intrinsic_capability(comp_ctx, AOT_INTRINSIC_FLAG_F64_SQRT);
}

bool
aot_intrinsic_check_capability(const AOTCompContext *comp_ctx,
                               const char *llvm_intrinsic)
{
    uint32 cnt;
    uint64 flag;
    uint64 group;

    for (cnt = 0; cnt < g_intrinsic_count; cnt++) {
        if (!strcmp(llvm_intrinsic, g_intrinsic_mapping[cnt].llvm_intrinsic)) {
            flag = g_intrinsic_mapping[cnt].flag;
            group = AOT_INTRINSIC_GET_GROUP_FROM_FLAG(flag);
            flag &= AOT_INTRINSIC_FLAG_MASK;
            if (group < sizeof(comp_ctx->flags) / sizeof(uint64)) {
                if (comp_ctx->flags[group] & flag) {
                    return true;
                }
            }
            else {
                bh_log(BH_LOG_LEVEL_WARNING, __FILE__, __LINE__,
                       "intrinsic exceeds max limit.");
            }
        }
    }
    return false;
}

void
aot_intrinsic_fill_capability_flags(AOTCompContext *comp_ctx)
{
    memset(comp_ctx->flags, 0, sizeof(comp_ctx->flags));

    if (!comp_ctx->target_cpu)
        return;

    if (!strncmp(comp_ctx->target_arch, "thumb", 5)) {
        if (!strcmp(comp_ctx->target_cpu, "cortex-m7")) {}
        else if (!strcmp(comp_ctx->target_cpu, "cortex-m4")) {
            add_f64_common_intrinsics_for_thumb2_fpu(comp_ctx);
        }
        else {
            add_f32_common_intrinsics_for_thumb2_fpu(comp_ctx);
            add_f64_common_intrinsics_for_thumb2_fpu(comp_ctx);
        }
    }
}

#endif /* WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0 */
