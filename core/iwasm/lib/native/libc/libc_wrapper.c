/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-native.h"
#include "wasm-export.h"
#include "wasm_log.h"
#include "wasm_platform_log.h"

void
wasm_runtime_set_exception(wasm_module_inst_t module, const char *exception);

uint32
wasm_runtime_get_temp_ret(wasm_module_inst_t module);

void
wasm_runtime_set_temp_ret(wasm_module_inst_t module, uint32 temp_ret);

uint32
wasm_runtime_get_llvm_stack(wasm_module_inst_t module);

void
wasm_runtime_set_llvm_stack(wasm_module_inst_t module, uint32 llvm_stack);

#define get_module_inst() \
    wasm_runtime_get_current_module_inst()

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

#define module_malloc(size) \
    wasm_runtime_module_malloc(module_inst, size)

#define module_free(offset) \
    wasm_runtime_module_free(module_inst, offset)

typedef int (*out_func_t)(int c, void *ctx);

enum pad_type {
    PAD_NONE,
    PAD_ZERO_BEFORE,
    PAD_SPACE_BEFORE,
    PAD_SPACE_AFTER,
};

typedef char *_va_list;
#define _INTSIZEOF(n)       \
    ((sizeof(n) +  3) & ~3)
#define _va_arg(ap,t)       \
    (*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))

/**
 * @brief Output an unsigned int in hex format
 *
 * Output an unsigned int on output installed by platform at init time. Should
 * be able to handle an unsigned int of any size, 32 or 64 bit.
 * @param num Number to output
 *
 * @return N/A
 */
static void
_printf_hex_uint(out_func_t out, void *ctx,
                 const uint64 num, bool is_u64,
                 enum pad_type padding,
                 int min_width)
{
    int size = sizeof(num) * (is_u64 ? 2 : 1);
    int found_largest_digit = 0;
    int remaining = 8; /* 8 digits max */
    int digits = 0;

    for (; size; size--) {
        char nibble = (num >> ((size - 1) << 2) & 0xf);

        if (nibble || found_largest_digit || size == 1) {
            found_largest_digit = 1;
            nibble += nibble > 9 ? 87 : 48;
            out((int) nibble, ctx);
            digits++;
            continue;
        }

        if (remaining-- <= min_width) {
            if (padding == PAD_ZERO_BEFORE) {
                out('0', ctx);
            } else if (padding == PAD_SPACE_BEFORE) {
                out(' ', ctx);
            }
        }
    }

    if (padding == PAD_SPACE_AFTER) {
        remaining = min_width * 2 - digits;
        while (remaining-- > 0) {
            out(' ', ctx);
        }
    }
}

/**
 * @brief Output an unsigned int in decimal format
 *
 * Output an unsigned int on output installed by platform at init time. Only
 * works with 32-bit values.
 * @param num Number to output
 *
 * @return N/A
 */
static void
_printf_dec_uint(out_func_t out, void *ctx,
                 const uint32 num,
                 enum pad_type padding,
                 int min_width)
{
    uint32 pos = 999999999;
    uint32 remainder = num;
    int found_largest_digit = 0;
    int remaining = 10; /* 10 digits max */
    int digits = 1;

    /* make sure we don't skip if value is zero */
    if (min_width <= 0) {
        min_width = 1;
    }

    while (pos >= 9) {
        if (found_largest_digit || remainder > pos) {
            found_largest_digit = 1;
            out((int) ((remainder / (pos + 1)) + 48), ctx);
            digits++;
        } else if (remaining <= min_width && padding < PAD_SPACE_AFTER) {
            out((int) (padding == PAD_ZERO_BEFORE ? '0' : ' '), ctx);
            digits++;
        }
        remaining--;
        remainder %= (pos + 1);
        pos /= 10;
    }
    out((int) (remainder + 48), ctx);

    if (padding == PAD_SPACE_AFTER) {
        remaining = min_width - digits;
        while (remaining-- > 0) {
            out(' ', ctx);
        }
    }
}

static void
print_err(out_func_t out, void *ctx)
{
    out('E', ctx);
    out('R', ctx);
    out('R', ctx);
}

static void
_vprintf(out_func_t out, void *ctx, const char *fmt, _va_list ap,
         wasm_module_inst_t module_inst)
{
    int might_format = 0; /* 1 if encountered a '%' */
    enum pad_type padding = PAD_NONE;
    int min_width = -1;
    int long_ctr = 0;

    /* fmt has already been adjusted if needed */

    while (*fmt) {
        if (!might_format) {
            if (*fmt != '%') {
                out((int) *fmt, ctx);
            }
            else {
                might_format = 1;
                min_width = -1;
                padding = PAD_NONE;
                long_ctr = 0;
            }
        }
        else {
            switch (*fmt) {
            case '-':
                padding = PAD_SPACE_AFTER;
                goto still_might_format;

            case '0':
                if (min_width < 0 && padding == PAD_NONE) {
                    padding = PAD_ZERO_BEFORE;
                    goto still_might_format;
                }
                /* Fall through */
            case '1' ... '9':
                if (min_width < 0) {
                    min_width = *fmt - '0';
                } else {
                    min_width = 10 * min_width + *fmt - '0';
                }

                if (padding == PAD_NONE) {
                    padding = PAD_SPACE_BEFORE;
                }
                goto still_might_format;

            case 'l':
                long_ctr++;
                /* Fall through */
            case 'z':
            case 'h':
                /* FIXME: do nothing for these modifiers */
                goto still_might_format;

            case 'd':
            case 'i': {
                int32 d;

                if (long_ctr < 2) {
                    d = _va_arg(ap, int32);
                }
                else {
                    int64 lld = _va_arg(ap, int64);
                    if (lld > INT32_MAX || lld < INT32_MIN) {
                        print_err(out, ctx);
                        break;
                    }
                    d = (int32)lld;
                }

                if (d < 0) {
                    out((int)'-', ctx);
                    d = -d;
                    min_width--;
                }
                _printf_dec_uint(out, ctx, d, padding, min_width);
                break;
            }
            case 'u': {
                uint32 u;

                if (long_ctr < 2) {
                    u = _va_arg(ap, uint32);
                }
                else {
                    uint64 llu = _va_arg(ap, uint64);
                    if (llu > INT32_MAX) {
                        print_err(out, ctx);
                        break;
                    }
                    u = (uint32)llu;
                }
                _printf_dec_uint(out, ctx, u, padding, min_width);
                break;
            }
            case 'p':
                out('0', ctx);
                out('x', ctx);
                /* left-pad pointers with zeros */
                padding = PAD_ZERO_BEFORE;
                min_width = 8;
                /* Fall through */
            case 'x':
            case 'X': {
                uint64 x;
                bool is_ptr = (*fmt == 'p') ? true : false;

                if (long_ctr < 2) {
                    x = _va_arg(ap, uint32);
                } else {
                    x = _va_arg(ap, uint64);
                }
                _printf_hex_uint(out, ctx, x, !is_ptr, padding, min_width);
                break;
            }

            case 's': {
                char *s;
                char *start;
                int32 s_offset = _va_arg(ap, uint32);

                if (!validate_app_addr(s_offset, 1)) {
                    wasm_runtime_set_exception(module_inst, "out of bounds memory access");
                    return;
                }

                s = start = addr_app_to_native(s_offset);

                while (*s)
                    out((int) (*s++), ctx);

                if (padding == PAD_SPACE_AFTER) {
                    int remaining = min_width - (s - start);
                    while (remaining-- > 0) {
                        out(' ', ctx);
                    }
                }
                break;
            }

            case 'c': {
                int c = _va_arg(ap, int);
                out(c, ctx);
                break;
            }

            case '%': {
                out((int) '%', ctx);
                break;
            }

            default:
                out((int) '%', ctx);
                out((int) *fmt, ctx);
                break;
            }

            might_format = 0;
        }

still_might_format:
        ++fmt;
    }
}

struct str_context {
    char *str;
    int max;
    int count;
};

static int
sprintf_out(int c, struct str_context *ctx)
{
    if (!ctx->str || ctx->count >= ctx->max) {
        ctx->count++;
        return c;
    }

    if (ctx->count == ctx->max - 1) {
        ctx->str[ctx->count++] = '\0';
    } else {
        ctx->str[ctx->count++] = c;
    }

    return c;
}

static int
printf_out(int c, struct str_context *ctx)
{
    printf("%c", c);
    ctx->count++;
    return c;
}

static inline _va_list
get_va_list(uint32 *args)
{
    union {
        uint32 u;
        _va_list v;
    } u;
    u.u = args[0];
    return u.v;
}

static bool
parse_printf_args(wasm_module_inst_t module_inst, int32 fmt_offset,
                  int32 va_list_offset, const char **p_fmt,
                  _va_list *p_va_args)
{
    const char *fmt;
    union {
        uintptr_t u;
        _va_list v;
    } u;

    if (!validate_app_addr(fmt_offset, 1)
        || !validate_app_addr(va_list_offset, sizeof(int32)))
        return false;

    fmt = (const char*) addr_app_to_native(fmt_offset);
    u.u = (uintptr_t) addr_app_to_native(va_list_offset);

    *p_fmt = fmt;
    *p_va_args = u.v;
    return true;
}

static int
_printf_wrapper(int32 fmt_offset, int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    struct str_context ctx = { NULL, 0, 0 };
    const char *fmt;
    _va_list va_args;

    if (!parse_printf_args(module_inst, fmt_offset, va_list_offset, &fmt, &va_args))
        return 0;

    _vprintf((out_func_t) printf_out, &ctx, fmt, va_args, module_inst);
    return ctx.count;
}

static int
_sprintf_wrapper(int32 str_offset, int32 fmt_offset, int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    struct str_context ctx;
    char *str;
    const char *fmt;
    _va_list va_args;

    if (!validate_app_addr(str_offset, 1))
        return 0;

    str = addr_app_to_native(str_offset);

    if (!parse_printf_args(module_inst, fmt_offset, va_list_offset, &fmt, &va_args))
        return 0;

    ctx.str = str;
    ctx.max = INT_MAX;
    ctx.count = 0;

    _vprintf((out_func_t) sprintf_out, &ctx, fmt, va_args, module_inst);

    if (ctx.count < ctx.max) {
        str[ctx.count] = '\0';
    }

    return ctx.count;
}

static int
_snprintf_wrapper(int32 str_offset, int32 size, int32 fmt_offset,
                  int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    struct str_context ctx;
    char *str;
    const char *fmt;
    _va_list va_args;

    if (!validate_app_addr(str_offset, size))
        return 0;

    str = addr_app_to_native(str_offset);

    if (!parse_printf_args(module_inst, fmt_offset, va_list_offset, &fmt, &va_args))
        return 0;

    ctx.str = str;
    ctx.max = size;
    ctx.count = 0;

    _vprintf((out_func_t) sprintf_out, &ctx, fmt, va_args, module_inst);

    if (ctx.count < ctx.max) {
        str[ctx.count] = '\0';
    }

    return ctx.count;
}

static int
_puts_wrapper(int32 str_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    const char *str;

    if (!validate_app_addr(str_offset, 1))
        return 0;

    str = addr_app_to_native(str_offset);
    return printf("%s\n", str);
}

static int
_putchar_wrapper(int c)
{
    printf("%c", c);
    return 1;
}

static int32
_strdup_wrapper(int32 str_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *str, *str_ret;
    uint32 len;
    int32 str_ret_offset = 0;

    if (!validate_app_addr(str_offset, 1))
        return 0;

    str = addr_app_to_native(str_offset);

    if (str) {
        len = strlen(str) + 1;

        str_ret_offset = module_malloc(len);
        if (str_ret_offset) {
            str_ret = addr_app_to_native(str_ret_offset);
            memcpy(str_ret, str, len);
        }
    }

    return str_ret_offset;
}

static int32
_memcmp_wrapper(int32 s1_offset, int32 s2_offset, int32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *s1, *s2;

    if (!validate_app_addr(s1_offset, size)
        || !validate_app_addr(s2_offset, size))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return memcmp(s1, s2, size);
}

static int32
_memcpy_wrapper(int32 dst_offset, int32 src_offset, int32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *dst, *src;

    if (size == 0)
        return dst_offset;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return dst_offset;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);
    memcpy(dst, src, size);
    return dst_offset;
}

static int32
_memmove_wrapper(int32 dst_offset, int32 src_offset, int32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *dst, *src;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return dst_offset;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);
    memmove(dst, src, size);
    return dst_offset;
}

static int32
_memset_wrapper(int32 s_offset, int32 c, int32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *s;

    if (!validate_app_addr(s_offset, size))
        return s_offset;

    s = addr_app_to_native(s_offset);
    memset(s, c, size);
    return s_offset;
}

static int32
_strchr_wrapper(int32 s_offset, int32 c)
{
    wasm_module_inst_t module_inst = get_module_inst();
    const char *s;
    char *ret;

    if (!validate_app_addr(s_offset, 1))
        return s_offset;

    s = addr_app_to_native(s_offset);
    ret = strchr(s, c);
    return ret ? addr_native_to_app(ret) : 0;
}

static int32
_strcmp_wrapper(int32 s1_offset, int32 s2_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *s1, *s2;

    if (!validate_app_addr(s1_offset, 1)
        || !validate_app_addr(s2_offset, 1))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return strcmp(s1, s2);
}

static int32
_strncmp_wrapper(int32 s1_offset, int32 s2_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *s1, *s2;

    if (!validate_app_addr(s1_offset, size)
        || !validate_app_addr(s2_offset, size))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return strncmp(s1, s2, size);
}

static int32
_strcpy_wrapper(int32 dst_offset, int32 src_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *dst, *src;

    if (!validate_app_addr(dst_offset, 1)
        || !validate_app_addr(src_offset, 1))
        return 0;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);
    strcpy(dst, src);
    return dst_offset;
}

static int32
_strncpy_wrapper(int32 dst_offset, int32 src_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *dst, *src;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return 0;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);
    strncpy(dst, src, size);
    return dst_offset;
}

static uint32
_strlen_wrapper(int32 s_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *s;

    if (!validate_app_addr(s_offset, 1))
        return 0;

    s = addr_app_to_native(s_offset);
    return strlen(s);
}

static int32
_malloc_wrapper(uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    return module_malloc(size);
}

static int32
_calloc_wrapper(uint32 nmemb, uint32 size)
{
    uint64 total_size = (uint64) nmemb * (uint64) size;
    wasm_module_inst_t module_inst = get_module_inst();
    uint32 ret_offset = 0;
    uint8 *ret_ptr;

    if (total_size > UINT32_MAX)
        total_size = UINT32_MAX;

    ret_offset = module_malloc((uint32 )total_size);
    if (ret_offset) {
        ret_ptr = addr_app_to_native(ret_offset);
        memset(ret_ptr, 0, (uint32) total_size);
    }

    return ret_offset;
}

static void
_free_wrapper(int32 ptr_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();

    if (!validate_app_addr(ptr_offset, 4))
        return;
    return module_free(ptr_offset);
}

static void
setTempRet0_wrapper(uint32 temp_ret)
{
    wasm_module_inst_t module_inst = get_module_inst();
    wasm_runtime_set_temp_ret(module_inst, temp_ret);
}

static uint32
getTempRet0_wrapper()
{
    wasm_module_inst_t module_inst = get_module_inst();
    return wasm_runtime_get_temp_ret(module_inst);
}

static uint32
_llvm_bswap_i16_wrapper(uint32 data)
{
    return (data & 0xFFFF0000)
           | ((data & 0xFF) << 8)
           | ((data & 0xFF00) >> 8);
}

static uint32
_llvm_bswap_i32_wrapper(uint32 data)
{
    return ((data & 0xFF) << 24)
           | ((data & 0xFF00) << 8)
           | ((data & 0xFF0000) >> 8)
           | ((data & 0xFF000000) >> 24);
}

static uint32
_bitshift64Lshr_wrapper(uint32 uint64_part0, uint32 uint64_part1,
                        uint32 bits)
{
    wasm_module_inst_t module_inst = get_module_inst();
    union {
        uint64 value;
        uint32 parts[2];
    } u;

    u.parts[0] = uint64_part0;
    u.parts[1] = uint64_part1;

    u.value >>= bits;
    /* return low 32bit and save high 32bit to temp ret */
    wasm_runtime_set_temp_ret(module_inst, (uint32) (u.value >> 32));
    return (uint32) u.value;
}

static uint32
_bitshift64Shl_wrapper(uint32 int64_part0, uint32 int64_part1,
                       uint32 bits)
{
    wasm_module_inst_t module_inst = get_module_inst();
    union {
        int64 value;
        uint32 parts[2];
    } u;

    u.parts[0] = int64_part0;
    u.parts[1] = int64_part1;

    u.value <<= bits;
    /* return low 32bit and save high 32bit to temp ret */
    wasm_runtime_set_temp_ret(module_inst, (uint32) (u.value >> 32));
    return (uint32) u.value;
}

static void
_llvm_stackrestore_wrapper(uint32 llvm_stack)
{
    wasm_module_inst_t module_inst = get_module_inst();
    printf("_llvm_stackrestore called!\n");
    wasm_runtime_set_llvm_stack(module_inst, llvm_stack);
}

static uint32
_llvm_stacksave_wrapper()
{
    wasm_module_inst_t module_inst = get_module_inst();
    printf("_llvm_stacksave called!\n");
    return wasm_runtime_get_llvm_stack(module_inst);
}

static int32
_emscripten_memcpy_big_wrapper(int32 dst_offset, int32 src_offset,
                               uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    void *dst, *src;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return dst_offset;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);

    memcpy(dst, src, size);
    return dst_offset;
}

static void
abort_wrapper(int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char buf[32];
    snprintf(buf, sizeof(buf), "env.abort(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static void
abortStackOverflow_wrapper(int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char buf[32];
    snprintf(buf, sizeof(buf), "env.abortStackOverflow(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static void
nullFunc_X_wrapper(int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char buf[32];
    snprintf(buf, sizeof(buf), "env.nullFunc_X(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

/* TODO: add function parameter/result types check */
#define REG_NATIVE_FUNC(module_name, func_name)     \
    { #module_name, #func_name, func_name##_wrapper }

typedef struct WASMNativeFuncDef {
    const char *module_name;
    const char *func_name;
    void *func_ptr;
} WASMNativeFuncDef;

static WASMNativeFuncDef native_func_defs[] = {
    REG_NATIVE_FUNC(env, _printf),
    REG_NATIVE_FUNC(env, _sprintf),
    REG_NATIVE_FUNC(env, _snprintf),
    REG_NATIVE_FUNC(env, _puts),
    REG_NATIVE_FUNC(env, _putchar),
    REG_NATIVE_FUNC(env, _memcmp),
    REG_NATIVE_FUNC(env, _memcpy),
    REG_NATIVE_FUNC(env, _memmove),
    REG_NATIVE_FUNC(env, _memset),
    REG_NATIVE_FUNC(env, _strchr),
    REG_NATIVE_FUNC(env, _strcmp),
    REG_NATIVE_FUNC(env, _strcpy),
    REG_NATIVE_FUNC(env, _strlen),
    REG_NATIVE_FUNC(env, _strncmp),
    REG_NATIVE_FUNC(env, _strncpy),
    REG_NATIVE_FUNC(env, _malloc),
    REG_NATIVE_FUNC(env, _calloc),
    REG_NATIVE_FUNC(env, _strdup),
    REG_NATIVE_FUNC(env, _free),
    REG_NATIVE_FUNC(env, setTempRet0),
    REG_NATIVE_FUNC(env, getTempRet0),
    REG_NATIVE_FUNC(env, _llvm_bswap_i16),
    REG_NATIVE_FUNC(env, _llvm_bswap_i32),
    REG_NATIVE_FUNC(env, _bitshift64Lshr),
    REG_NATIVE_FUNC(env, _bitshift64Shl),
    REG_NATIVE_FUNC(env, _llvm_stackrestore),
    REG_NATIVE_FUNC(env, _llvm_stacksave),
    REG_NATIVE_FUNC(env, _emscripten_memcpy_big),
    REG_NATIVE_FUNC(env, abort),
    REG_NATIVE_FUNC(env, abortStackOverflow),
    REG_NATIVE_FUNC(env, nullFunc_X)
};

void*
wasm_native_func_lookup(const char *module_name, const char *func_name)
{
    uint32 size = sizeof(native_func_defs) / sizeof(WASMNativeFuncDef);
    WASMNativeFuncDef *func_def = native_func_defs;
    WASMNativeFuncDef *func_def_end = func_def + size;
    void *ret;

    if (!module_name || !func_name)
        return NULL;

    while (func_def < func_def_end) {
        if (!strcmp(func_def->module_name, module_name)
            && !strcmp(func_def->func_name, func_name))
            return (void*) (uintptr_t) func_def->func_ptr;
        func_def++;
    }

    if ((ret = wasm_platform_native_func_lookup(module_name, func_name)))
        return ret;

    return NULL;
}

/*************************************
 * Global Variables                  *
 *************************************/

typedef struct WASMNativeGlobalDef {
    const char *module_name;
    const char *global_name;
    WASMValue global_data;
} WASMNativeGlobalDef;

static WASMNativeGlobalDef native_global_defs[] = {
    { "env", "STACKTOP", .global_data.u32 = 0 },
    { "env", "STACK_MAX", .global_data.u32 = 0 },
    { "env", "ABORT", .global_data.u32 = 0 },
    { "env", "memoryBase", .global_data.u32 = 0 },
    { "env", "__memory_base", .global_data.u32 = 0 },
    { "env", "tableBase", .global_data.u32 = 0 },
    { "env", "__table_base", .global_data.u32 = 0 },
    { "env", "DYNAMICTOP_PTR", .global_data.addr = 0 },
    { "env", "tempDoublePtr", .global_data.addr = 0 },
    { "global", "NaN", .global_data.u64 = 0x7FF8000000000000LL },
    { "global", "Infinity", .global_data.u64 = 0x7FF0000000000000LL }
};

bool
wasm_native_global_lookup(const char *module_name, const char *global_name,
                          WASMGlobalImport *global)
{
    uint32 size = sizeof(native_global_defs) / sizeof(WASMNativeGlobalDef);
    WASMNativeGlobalDef *global_def = native_global_defs;
    WASMNativeGlobalDef *global_def_end = global_def + size;

    if (!module_name || !global_name || !global)
        return false;

    /* Lookup constant globals which can be defined by table */
    while (global_def < global_def_end) {
        if (!strcmp(global_def->module_name, module_name)
            && !strcmp(global_def->global_name, global_name)) {
            global->global_data_linked = global_def->global_data;
            return true;
        }
        global_def++;
    }

    /* Lookup non-constant globals which cannot be defined by table */
    if (!strcmp(module_name, "env")) {
        if (!strcmp(global_name, "_stdin")) {
            global->global_data_linked.addr = (uintptr_t)stdin;
            global->is_addr = true;
            return true;
        } else if (!strcmp(global_name, "_stdout")) {
            global->global_data_linked.addr = (uintptr_t)stdout;
            global->is_addr = true;
            return true;
        } else if (!strcmp(global_name, "_stderr")) {
            global->global_data_linked.addr = (uintptr_t)stderr;
            global->is_addr = true;
            return true;
        }
    }

    return false;
}

bool
wasm_native_init()
{
    /* TODO: qsort the function defs and global defs. */
    return true;
}

