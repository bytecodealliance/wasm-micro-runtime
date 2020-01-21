/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_log.h"
#include "wasm_export.h"
#if WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0
#include "../interpreter/wasm.h"
#endif

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

#define get_module_inst(exec_env) \
    wasm_runtime_get_module_inst(exec_env)

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define validate_app_str_addr(offset) \
    wasm_runtime_validate_app_str_addr(module_inst, offset)

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
    (((uint32)sizeof(n) +  3) & (uint32)~3)
#define _va_arg(ap, t)      \
    (*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))

#define CHECK_VA_ARG(ap, t) do {                        \
    if ((uint8*)ap + _INTSIZEOF(t) > native_end_addr)   \
        goto fail;                                      \
} while (0)

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
    int shift = sizeof(num) * 8;
    int found_largest_digit = 0;
    int remaining = 16; /* 16 digits max */
    int digits = 0;
    char nibble;

     while (shift >= 4) {
         shift -= 4;
         nibble = (num >> shift) & 0xf;

        if (nibble || found_largest_digit || shift == 0) {
            found_largest_digit = 1;
            nibble = (char)(nibble + (nibble > 9 ? 87 : 48));
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

static bool
_vprintf_wa(out_func_t out, void *ctx, const char *fmt, _va_list ap,
            wasm_module_inst_t module_inst)
{
    int might_format = 0; /* 1 if encountered a '%' */
    enum pad_type padding = PAD_NONE;
    int min_width = -1;
    int long_ctr = 0;
    uint8 *native_end_addr;

    if (!wasm_runtime_get_native_addr_range(module_inst, (uint8*)ap,
                                            NULL, &native_end_addr))
        goto fail;

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
                    CHECK_VA_ARG(ap, int32);
                    d = _va_arg(ap, int32);
                }
                else {
                    int64 lld;
                    CHECK_VA_ARG(ap, int64);
                    lld = _va_arg(ap, int64);
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
                _printf_dec_uint(out, ctx, (uint32)d, padding, min_width);
                break;
            }
            case 'u': {
                uint32 u;

                if (long_ctr < 2) {
                    CHECK_VA_ARG(ap, uint32);
                    u = _va_arg(ap, uint32);
                }
                else {
                    uint64 llu;
                    CHECK_VA_ARG(ap, uint64);
                    llu = _va_arg(ap, uint64);
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
                    CHECK_VA_ARG(ap, uint32);
                    x = _va_arg(ap, uint32);
                } else {
                    CHECK_VA_ARG(ap, uint64);
                    x = _va_arg(ap, uint64);
                }
                _printf_hex_uint(out, ctx, x, !is_ptr, padding, min_width);
                break;
            }

            case 's': {
                char *s;
                char *start;
                int32 s_offset;

                CHECK_VA_ARG(ap, int32);
                s_offset = _va_arg(ap, int32);

                if (!validate_app_str_addr(s_offset)) {
                    return false;
                }

                s = start = addr_app_to_native(s_offset);

                while (*s)
                    out((int) (*s++), ctx);

                if (padding == PAD_SPACE_AFTER) {
                    int remaining = min_width - (int32)(s - start);
                    while (remaining-- > 0) {
                        out(' ', ctx);
                    }
                }
                break;
            }

            case 'c': {
                int c;
                CHECK_VA_ARG(ap, int);
                c = _va_arg(ap, int);
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
    return true;

fail:
    wasm_runtime_set_exception(module_inst, "out of bounds memory access");
    return false;
}

struct str_context {
    char *str;
    uint32 max;
    uint32 count;
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
        ctx->str[ctx->count++] = (char)c;
    }

    return c;
}

static int
printf_out(int c, struct str_context *ctx)
{
    bh_printf("%c", c);
    ctx->count++;
    return c;
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

    if (!validate_app_str_addr(fmt_offset)
        || !validate_app_addr(va_list_offset, sizeof(int32)))
        return false;

    fmt = (const char*) addr_app_to_native(fmt_offset);
    u.u = (uintptr_t) addr_app_to_native(va_list_offset);

    *p_fmt = fmt;
    *p_va_args = u.v;
    return true;
}

static int
_printf_wrapper(wasm_exec_env_t exec_env,
                int32 fmt_offset, int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    struct str_context ctx = { NULL, 0, 0 };
    const char *fmt;
    _va_list va_args;

    if (!parse_printf_args(module_inst, fmt_offset, va_list_offset, &fmt, &va_args))
        return 0;

    if (!_vprintf_wa((out_func_t)printf_out, &ctx, fmt, va_args, module_inst))
        return 0;
    return (int)ctx.count;
}

static int
_sprintf_wrapper(wasm_exec_env_t exec_env,
                 int32 str_offset, int32 fmt_offset, int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    int32 app_end_offset;
    struct str_context ctx;
    char *str;
    const char *fmt;
    _va_list va_args;

    if (!wasm_runtime_get_app_addr_range(module_inst, str_offset,
                                         NULL, &app_end_offset)) {
        wasm_runtime_set_exception(module_inst, "out of bounds memory access");
        return false;
    }

    str = addr_app_to_native(str_offset);

    if (!parse_printf_args(module_inst, fmt_offset, va_list_offset, &fmt, &va_args))
        return 0;

    ctx.str = str;
    ctx.max = (uint32)(app_end_offset - str_offset);
    ctx.count = 0;

    if (!_vprintf_wa((out_func_t)sprintf_out, &ctx, fmt, va_args, module_inst))
        return 0;

    if (ctx.count < ctx.max) {
        str[ctx.count] = '\0';
    }

    return (int)ctx.count;
}

static int
_snprintf_wrapper(wasm_exec_env_t exec_env,
                  int32 str_offset, uint32 size, int32 fmt_offset,
                  int32 va_list_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
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

    if (!_vprintf_wa((out_func_t)sprintf_out, &ctx, fmt, va_args, module_inst))
        return 0;

    if (ctx.count < ctx.max) {
        str[ctx.count] = '\0';
    }

    return (int)ctx.count;
}

static int
_puts_wrapper(wasm_exec_env_t exec_env,
              int32 str_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    const char *str;

    if (!validate_app_str_addr(str_offset))
        return 0;

    str = addr_app_to_native(str_offset);
    return bh_printf("%s\n", str);
}

static int
_putchar_wrapper(wasm_exec_env_t exec_env, int c)
{
    bh_printf("%c", c);
    return 1;
}

static int32
_strdup_wrapper(wasm_exec_env_t exec_env,
                int32 str_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *str, *str_ret;
    uint32 len;
    int32 str_ret_offset = 0;

    if (!validate_app_str_addr(str_offset))
        return 0;

    str = addr_app_to_native(str_offset);

    if (str) {
        len = (uint32)strlen(str) + 1;

        str_ret_offset = module_malloc(len);
        if (str_ret_offset) {
            str_ret = addr_app_to_native(str_ret_offset);
            bh_memcpy_s(str_ret, len, str, len);
        }
    }

    return str_ret_offset;
}

static int32
__strdup_wrapper(wasm_exec_env_t exec_env,
                int32 str_offset)
{
    return _strdup_wrapper(exec_env, str_offset);
}

static int32
_memcmp_wrapper(wasm_exec_env_t exec_env,
                int32 s1_offset, int32 s2_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *s1, *s2;

    if (!validate_app_addr(s1_offset, size)
        || !validate_app_addr(s2_offset, size))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return memcmp(s1, s2, size);
}

static int32
_memcpy_wrapper(wasm_exec_env_t exec_env,
                int32 dst_offset, int32 src_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *dst, *src;

    if (size == 0)
        return dst_offset;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return dst_offset;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);
    bh_memcpy_s(dst, size, src, size);
    return dst_offset;
}

static int32
_memmove_wrapper(wasm_exec_env_t exec_env,
                 int32 dst_offset, int32 src_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
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
_memset_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset, int32 c, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *s;

    if (!validate_app_addr(s_offset, size))
        return s_offset;

    s = addr_app_to_native(s_offset);
    memset(s, c, size);
    return s_offset;
}

static int32
_strchr_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset, int32 c)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    const char *s;
    char *ret;

    if (!validate_app_str_addr(s_offset))
        return s_offset;

    s = addr_app_to_native(s_offset);
    ret = strchr(s, c);
    return ret ? addr_native_to_app(ret) : 0;
}

static int32
_strcmp_wrapper(wasm_exec_env_t exec_env,
                int32 s1_offset, int32 s2_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *s1, *s2;

    if (!validate_app_str_addr(s1_offset)
        || !validate_app_str_addr(s2_offset))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return strcmp(s1, s2);
}

static int32
_strncmp_wrapper(wasm_exec_env_t exec_env,
                 int32 s1_offset, int32 s2_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *s1, *s2;

    if (!validate_app_addr(s1_offset, size)
        || !validate_app_addr(s2_offset, size))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);
    return strncmp(s1, s2, size);
}

static int32
_strcpy_wrapper(wasm_exec_env_t exec_env,
                int32 dst_offset, int32 src_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *dst, *src;
    uint32 len;

    if (!validate_app_str_addr(src_offset))
        return 0;

    src = addr_app_to_native(src_offset);
    len = (uint32)strlen(src);

    if (!validate_app_addr(dst_offset, len + 1))
        return 0;

    dst = addr_app_to_native(dst_offset);
    strncpy(dst, src, len + 1);
    return dst_offset;
}

static int32
_strncpy_wrapper(wasm_exec_env_t exec_env,
                 int32 dst_offset, int32 src_offset, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
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
_strlen_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *s;

    if (!validate_app_str_addr(s_offset))
        return 0;

    s = addr_app_to_native(s_offset);
    return (uint32)strlen(s);
}

static int32
_malloc_wrapper(wasm_exec_env_t exec_env,
                uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    return module_malloc(size);
}

static int32
_calloc_wrapper(wasm_exec_env_t exec_env,
                uint32 nmemb, uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    uint64 total_size = (uint64) nmemb * (uint64) size;
    int32 ret_offset = 0;
    uint8 *ret_ptr;

    if (total_size >= UINT32_MAX)
        return 0;

    ret_offset = module_malloc((uint32)total_size);
    if (ret_offset) {
        ret_ptr = addr_app_to_native(ret_offset);
        memset(ret_ptr, 0, (uint32) total_size);
    }

    return ret_offset;
}

static void
_free_wrapper(wasm_exec_env_t exec_env,
              int32 ptr_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    if (!validate_app_addr(ptr_offset, 4))
        return;
    return module_free(ptr_offset);
}

static int32
_atoi_wrapper(wasm_exec_env_t exec_env,
              int32 s_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *str;

    if (!validate_app_str_addr(s_offset))
        return 0;

    str = addr_app_to_native(s_offset);

    return atoi(str);
}

static int32
_bsearch_wrapper(wasm_exec_env_t exec_env,
                 int32 key_offset,     /* const void * */
                 int32 array_offset,   /* const void * */
                 uint32 count,
                 uint32 size,
                 int32 cmp_index)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_runtime_set_exception(module_inst, "bsearch not implemented.");

    return 0;
}

static void
_exit_wrapper(wasm_exec_env_t exec_env,
              int32 status)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char buf[32];
    snprintf(buf, sizeof(buf), "env.exit(%i)", status);
    wasm_runtime_set_exception(module_inst, buf);
}

static int32
_strtol_wrapper(wasm_exec_env_t exec_env,
                int32 nptr_offset,      /* const char * */
                int32 endptr_offset,    /* char ** */
                int32 base)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *nptr, **endptr;
    int32 num = 0;

    if (!validate_app_str_addr(nptr_offset)
        || !validate_app_addr(endptr_offset, sizeof(int32)))
        return 0;

    nptr = addr_app_to_native(nptr_offset);
    endptr = addr_app_to_native(endptr_offset);

    num = (int32)strtol(nptr, endptr, base);
    *(int32 *)endptr = addr_native_to_app(*endptr);

    return num;
}

static uint32
_strtoul_wrapper(wasm_exec_env_t exec_env,
                 int32 nptr_offset,      /* const char * */
                 int32 endptr_offset,    /* char ** */
                 int32 base)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *nptr, **endptr;
    uint32 num = 0;

    if (!validate_app_str_addr(nptr_offset)
        || !validate_app_addr(endptr_offset, sizeof(int32)))
        return 0;

    nptr = addr_app_to_native(nptr_offset);
    endptr = addr_app_to_native(endptr_offset);

    num = (uint32)strtoul(nptr, endptr, base);
    *(int32 *)endptr = addr_native_to_app(*endptr);

    return num;
}

static int32
_memchr_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset,     /* const void * */
                int32 c,
                uint32 n)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *s, *res;

    if (!validate_app_addr(s_offset, n))
        return 0;

    s = (void*)addr_app_to_native(s_offset);

    res = memchr(s, c, n);

    return addr_native_to_app(res);
}

static int32
_strncasecmp_wrapper(wasm_exec_env_t exec_env,
                     int32 s1_offset,   /* const char * */
                     int32 s2_offset,   /* const char * */
                     uint32 n)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *s1, *s2;

    if (!validate_app_str_addr(s1_offset)
        || !validate_app_str_addr(s2_offset))
        return 0;

    s1 = addr_app_to_native(s1_offset);
    s2 = addr_app_to_native(s2_offset);

    return strncasecmp(s1, s2, n);
}

static uint32
_strspn_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset,         /* const char * */
                int32 accept_offset)    /* const char * */
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *s, *accept;

    if (!validate_app_str_addr(s_offset)
        || !validate_app_str_addr(accept_offset))
        return 0;

    s = addr_app_to_native(s_offset);
    accept = addr_app_to_native(accept_offset);

    return (uint32)strspn(s, accept);
}

static uint32
_strcspn_wrapper(wasm_exec_env_t exec_env,
                 int32 s_offset,        /* const char * */
                 int32 reject_offset)   /* const char * */
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *s, *reject;

    if (!validate_app_str_addr(s_offset)
        || !validate_app_str_addr(reject_offset))
        return 0;

    s = addr_app_to_native(s_offset);
    reject = addr_app_to_native(reject_offset);

    return (uint32)strcspn(s, reject);
}

static int32
_strstr_wrapper(wasm_exec_env_t exec_env,
                int32 s_offset,     /* const char * */
                int32 find_offset)  /* const char * */
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char *s, *find, *res;

    if (!validate_app_str_addr(s_offset)
        || !validate_app_str_addr(find_offset))
        return 0;

    s = addr_app_to_native(s_offset);
    find = addr_app_to_native(find_offset);

    res = strstr(s, find);

    return addr_native_to_app(res);
}

static int32
_isupper_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isupper(c);
}

static int32
_isalpha_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isalpha(c);
}

static int32
_isspace_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isspace(c);
}

static int32
_isgraph_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isgraph(c);
}

static int32
_isprint_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isprint(c);
}

static int32
_isdigit_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isdigit(c);
}

static int32
_isxdigit_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isxdigit(c);
}

static int32
_tolower_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return tolower(c);
}

static int32
_toupper_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return toupper(c);
}

static int32
_isalnum_wrapper(wasm_exec_env_t exec_env, int32 c)
{
    return isalnum(c);
}

static void
setTempRet0_wrapper(wasm_exec_env_t exec_env,
                    uint32 temp_ret)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_runtime_set_temp_ret(module_inst, temp_ret);
}

static uint32
getTempRet0_wrapper(wasm_exec_env_t exec_env)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    return wasm_runtime_get_temp_ret(module_inst);
}

static uint32
_llvm_bswap_i16_wrapper(wasm_exec_env_t exec_env,
                        uint32 data)
{
    return (data & 0xFFFF0000)
           | ((data & 0xFF) << 8)
           | ((data & 0xFF00) >> 8);
}

static uint32
_llvm_bswap_i32_wrapper(wasm_exec_env_t exec_env,
                        uint32 data)
{
    return ((data & 0xFF) << 24)
           | ((data & 0xFF00) << 8)
           | ((data & 0xFF0000) >> 8)
           | ((data & 0xFF000000) >> 24);
}

static uint32
_bitshift64Lshr_wrapper(wasm_exec_env_t exec_env,
                        uint32 uint64_part0, uint32 uint64_part1,
                        uint32 bits)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
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
_bitshift64Shl_wrapper(wasm_exec_env_t exec_env,
                       uint32 int64_part0, uint32 int64_part1,
                       uint32 bits)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
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
_llvm_stackrestore_wrapper(wasm_exec_env_t exec_env,
                           uint32 llvm_stack)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    bh_printf("_llvm_stackrestore called!\n");
    wasm_runtime_set_llvm_stack(module_inst, llvm_stack);
}

static uint32
_llvm_stacksave_wrapper(wasm_exec_env_t exec_env)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    bh_printf("_llvm_stacksave called!\n");
    return wasm_runtime_get_llvm_stack(module_inst);
}

static int32
_emscripten_memcpy_big_wrapper(wasm_exec_env_t exec_env,
                               int32 dst_offset, int32 src_offset,
                               uint32 size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    void *dst, *src;

    if (!validate_app_addr(dst_offset, size)
        || !validate_app_addr(src_offset, size))
        return dst_offset;

    dst = addr_app_to_native(dst_offset);
    src = addr_app_to_native(src_offset);

    bh_memcpy_s(dst, size, src, size);
    return dst_offset;
}

static void
abort_wrapper(wasm_exec_env_t exec_env,
              int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char buf[32];
    snprintf(buf, sizeof(buf), "env.abort(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static void
abortStackOverflow_wrapper(wasm_exec_env_t exec_env,
                           int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char buf[32];
    snprintf(buf, sizeof(buf), "env.abortStackOverflow(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static void
nullFunc_X_wrapper(wasm_exec_env_t exec_env,
                   int32 code)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char buf[32];
    snprintf(buf, sizeof(buf), "env.nullFunc_X(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static int32
__cxa_allocate_exception_wrapper(wasm_exec_env_t exec_env,
                                 uint32 thrown_size)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    int32 exception = module_malloc(thrown_size);
    if (!exception)
        return 0;

    return exception;
}

static void
__cxa_begin_catch_wrapper(wasm_exec_env_t exec_env,
                          int32 exception_object_offset)
{

}

static void
__cxa_throw_wrapper(wasm_exec_env_t exec_env,
                    int32 thrown_exception_offset,
                    int32 tinfo_offset,
                    uint32 table_elem_idx)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    char buf[32];

    snprintf(buf, sizeof(buf), "%s", "exception thrown by stdc++");
    wasm_runtime_set_exception(module_inst, buf);
}

#ifndef ENABLE_SPEC_TEST
#define ENABLE_SPEC_TEST 0
#endif

#if ENABLE_SPEC_TEST != 0
static void
print_i32_wrapper(wasm_exec_env_t exec_env, int32 i32)
{
    bh_printf("%d\n", i32);
}
#endif

/* TODO: add function parameter/result types check */
#define REG_NATIVE_FUNC(module_name, func_name)     \
    { #module_name, #func_name, func_name##_wrapper }

typedef struct WASMNativeFuncDef {
    const char *module_name;
    const char *func_name;
    void *func_ptr;
} WASMNativeFuncDef;

static WASMNativeFuncDef native_func_defs[] = {
#if ENABLE_SPEC_TEST != 0
    REG_NATIVE_FUNC(spectest, print_i32),
#endif
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
    /* clang may introduce __strdup */
    REG_NATIVE_FUNC(env, __strdup),
    REG_NATIVE_FUNC(env, _free),
    REG_NATIVE_FUNC(env, _atoi),
    REG_NATIVE_FUNC(env, _bsearch),
    REG_NATIVE_FUNC(env, _exit),
    REG_NATIVE_FUNC(env, _strtol),
    REG_NATIVE_FUNC(env, _strtoul),
    REG_NATIVE_FUNC(env, _memchr),
    REG_NATIVE_FUNC(env, _strncasecmp),
    REG_NATIVE_FUNC(env, _strspn),
    REG_NATIVE_FUNC(env, _strcspn),
    REG_NATIVE_FUNC(env, _strstr),
    REG_NATIVE_FUNC(env, _isupper),
    REG_NATIVE_FUNC(env, _isalpha),
    REG_NATIVE_FUNC(env, _isspace),
    REG_NATIVE_FUNC(env, _isgraph),
    REG_NATIVE_FUNC(env, _isprint),
    REG_NATIVE_FUNC(env, _isdigit),
    REG_NATIVE_FUNC(env, _isxdigit),
    REG_NATIVE_FUNC(env, _tolower),
    REG_NATIVE_FUNC(env, _toupper),
    REG_NATIVE_FUNC(env, _isalnum),
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
    REG_NATIVE_FUNC(env, nullFunc_X),
    REG_NATIVE_FUNC(env, __cxa_allocate_exception),
    REG_NATIVE_FUNC(env, __cxa_begin_catch),
    REG_NATIVE_FUNC(env, __cxa_throw)
};

void *
wasm_native_lookup_libc_builtin_func(const char *module_name,
                                     const char *func_name)
{
    uint32 size = sizeof(native_func_defs) / sizeof(WASMNativeFuncDef);
    WASMNativeFuncDef *func_def = native_func_defs;
    WASMNativeFuncDef *func_def_end = func_def + size;

    if (!module_name || !func_name)
        return NULL;

    while (func_def < func_def_end) {
        if (!strcmp(func_def->module_name, module_name)
            && (!strcmp(func_def->func_name, func_name)
                || (func_def->func_name[0] == '_'
                    && !strcmp(func_def->func_name + 1, func_name))))
            return (void*) (uintptr_t) func_def->func_ptr;
        func_def++;
    }

    return NULL;
}

#if WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0

/*************************************
 * Global Variables                  *
 *************************************/

typedef struct WASMNativeGlobalDef {
    const char *module_name;
    const char *global_name;
    WASMValue global_data;
} WASMNativeGlobalDef;

static WASMNativeGlobalDef native_global_defs[] = {
#if ENABLE_SPEC_TEST != 0
    { "spectest", "global_i32", .global_data.i32 = 666 },
    { "spectest", "global_f32", .global_data.f32 = 0 },
    { "spectest", "global_f64", .global_data.f64 = 0 },
    { "test", "global-i32", .global_data.i32 = 0 },
    { "test", "global-f32", .global_data.f32 = 0 },
#endif
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
wasm_native_lookup_libc_builtin_global(const char *module_name,
                                       const char *global_name,
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

    return false;
}

#endif /* end of WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0 */

