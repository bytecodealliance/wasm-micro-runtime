/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_loader.h"
#include "bh_common.h"
#include "bh_memory.h"
#include "bh_log.h"
#include "wasm.h"
#include "wasm_opcode.h"
#include "wasm_runtime.h"
#include "../common/wasm_native.h"

/* Read a value of given type from the address pointed to by the given
   pointer and increase the pointer to the position just after the
   value being read.  */
#define TEMPLATE_READ_VALUE(Type, p)                    \
    (p += sizeof(Type), *(Type *)(p - sizeof(Type)))

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

#define CHECK_BUF(buf, buf_end, length) do {                \
  if (buf + length > buf_end) {                             \
    set_error_buf(error_buf, error_buf_size,                \
                  "WASM module load failed: "               \
                  "unexpected end of section or function"); \
    return false;                                           \
  }                                                         \
} while (0)

#define CHECK_BUF1(buf, buf_end, length) do {               \
  if (buf + length > buf_end) {                             \
    set_error_buf(error_buf, error_buf_size,                \
                  "WASM module load failed: unexpected end");\
    return false;                                           \
  }                                                         \
} while (0)

static bool
read_leb(const uint8 *buf, const uint8 *buf_end,
         uint32 *p_offset, uint32 maxbits,
         bool sign, uint64 *p_result,
         char* error_buf, uint32 error_buf_size)
{
    uint64 result = 0;
    uint32 shift = 0;
    uint32 bcnt = 0;
    uint64 byte;

    while (true) {
        if (bcnt + 1 > (maxbits + 6) / 7) {
            set_error_buf(error_buf, error_buf_size,
                          "WASM module load failed: "
                          "integer representation too long");
            return false;
        }

        CHECK_BUF(buf, buf_end, *p_offset + 1);
        byte = buf[*p_offset];
        *p_offset += 1;
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        bcnt += 1;
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    if (!sign && maxbits == 32 && shift >= maxbits) {
        /* The top bits set represent values > 32 bits */
        if (((uint8)byte) & 0xf0)
            goto fail_integer_too_large;
    }
    else if (sign && maxbits == 32) {
        if (shift < maxbits) {
            /* Sign extend */
            result = (((int32)result) << (maxbits - shift))
                     >> (maxbits - shift);
        }
        else {
            /* The top bits should be a sign-extension of the sign bit */
            bool sign_bit_set = ((uint8)byte) & 0x8;
            int top_bits = ((uint8)byte) & 0xf0;
            if ((sign_bit_set && top_bits != 0x70)
                || (!sign_bit_set && top_bits != 0))
                goto fail_integer_too_large;
        }
    }
    else if (sign && maxbits == 64) {
        if (shift < maxbits) {
            /* Sign extend */
            result = (((int64)result) << (maxbits - shift))
                     >> (maxbits - shift);
        }
        else {
            /* The top bits should be a sign-extension of the sign bit */
            bool sign_bit_set = ((uint8)byte) & 0x1;
            int top_bits = ((uint8)byte) & 0xfe;

            if ((sign_bit_set && top_bits != 0x7e)
                || (!sign_bit_set && top_bits != 0))
                goto fail_integer_too_large;
        }
    }

    *p_result = result;
    return true;

fail_integer_too_large:
    set_error_buf(error_buf, error_buf_size,
                  "WASM module load failed: integer too large");
    return false;
}

#define read_uint8(p)  TEMPLATE_READ_VALUE(uint8, p)
#define read_uint32(p) TEMPLATE_READ_VALUE(uint32, p)
#define read_bool(p)   TEMPLATE_READ_VALUE(bool, p)

#define read_leb_uint64(p, p_end, res) do {         \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 64, false, &res64,  \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (uint64)res64;                              \
} while (0)

#define read_leb_int64(p, p_end, res) do {          \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 64, true, &res64,   \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (int64)res64;                               \
} while (0)

#define read_leb_uint32(p, p_end, res) do {         \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 32, false, &res64,  \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (uint32)res64;                              \
} while (0)

#define read_leb_int32(p, p_end, res) do {          \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 32, true, &res64,   \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (int32)res64;                               \
} while (0)

static bool
check_utf8_str(const uint8* str, uint32 len)
{
    const uint8 *p = str, *p_end = str + len, *p_end1;
    uint8 chr, n_bytes;

    while (p < p_end) {
        chr = *p++;
        if (chr >= 0x80) {
            /* Calculate the byte count: the first byte must be
               110XXXXX, 1110XXXX, 11110XXX, 111110XX, or 1111110X,
               the count of leading '1' denotes the total byte count */
            n_bytes = 0;
            while ((chr & 0x80) != 0) {
                chr = (uint8)(chr << 1);
                n_bytes++;
            }

            /* Check byte count */
            if (n_bytes < 2 || n_bytes > 6
                || p + n_bytes - 1 > p_end)
                return false;

            /* Check the following bytes, which must be 10XXXXXX */
            p_end1 = p + n_bytes - 1;
            while (p < p_end1) {
                if (!(*p & 0x80) || (*p | 0x40))
                    return false;
                p++;
            }
        }
    }
    return true;
}

static char*
const_str_set_insert(const uint8 *str, uint32 len, WASMModule *module,
                     char* error_buf, uint32 error_buf_size)
{
    HashMap *set = module->const_str_set;
    char *c_str, *value;

    if (!check_utf8_str(str, len)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "invalid UTF-8 encoding");
        return NULL;
    }

    if (!(c_str = wasm_malloc(len + 1))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "allocate memory failed.");
        return NULL;
    }

    bh_memcpy_s(c_str, len + 1, str, len);
    c_str[len] = '\0';

    if ((value = bh_hash_map_find(set, c_str))) {
        wasm_free(c_str);
        return value;
    }

    if (!bh_hash_map_insert(set, c_str, c_str)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "insert string to hash map failed.");
        wasm_free(c_str);
        return NULL;
    }

    return c_str;
}

static bool
load_init_expr(const uint8 **p_buf, const uint8 *buf_end,
               InitializerExpression *init_expr,
               char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;
    uint8 flag, end_byte, *p_float;
    uint32 i;

    CHECK_BUF(p, p_end, 1);
    init_expr->init_expr_type = read_uint8(p);
    flag = init_expr->init_expr_type;

    switch (flag) {
        /* i32.const */
        case INIT_EXPR_TYPE_I32_CONST:
            read_leb_int32(p, p_end, init_expr->u.i32);
            break;
        /* i64.const */
        case INIT_EXPR_TYPE_I64_CONST:
            read_leb_int64(p, p_end, init_expr->u.i64);
            break;
        /* f32.const */
        case INIT_EXPR_TYPE_F32_CONST:
            CHECK_BUF(p, p_end, 4);
            p_float = (uint8*)&init_expr->u.f32;
            for (i = 0; i < sizeof(float32); i++)
                *p_float++ = *p++;
            break;
        /* f64.const */
        case INIT_EXPR_TYPE_F64_CONST:
            CHECK_BUF(p, p_end, 8);
            p_float = (uint8*)&init_expr->u.f64;
            for (i = 0; i < sizeof(float64); i++)
                *p_float++ = *p++;
            break;
        /* get_global */
        case INIT_EXPR_TYPE_GET_GLOBAL:
            read_leb_uint32(p, p_end, init_expr->u.global_index);
            break;
        default:
            set_error_buf(error_buf, error_buf_size,
                          "WASM module load failed: type mismatch");
            return false;
    }
    CHECK_BUF(p, p_end, 1);
    end_byte = read_uint8(p);
    if (end_byte != 0x0b) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "unexpected end of section or function");
        return false;
    }
    *p_buf = p;

    return true;
}

static bool
load_type_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end, *p_org;
    uint32 type_count, param_count, result_count, i, j;
    uint64 total_size;
    uint8 flag;
    WASMType *type;

    read_leb_uint32(p, p_end, type_count);

    if (type_count) {
        module->type_count = type_count;
        total_size = sizeof(WASMType*) * (uint64)type_count;
        if (total_size >= UINT32_MAX
            || !(module->types = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load type section failed: allocate memory failed.");
            return false;
        }

        memset(module->types, 0, (uint32)total_size);

        for (i = 0; i < type_count; i++) {
            CHECK_BUF(p, p_end, 1);
            flag = read_uint8(p);
            if (flag != 0x60) {
                set_error_buf(error_buf, error_buf_size,
                              "Load type section failed: invalid type flag.");
                return false;
            }

            read_leb_uint32(p, p_end, param_count);

            /* Resolve param count and result count firstly */
            p_org = p;
            CHECK_BUF(p, p_end, param_count);
            p += param_count;
            read_leb_uint32(p, p_end, result_count);
            if (result_count > 1) {
                set_error_buf(error_buf, error_buf_size,
                              "Load type section failed: invalid result count.");
                return false;
            }
            CHECK_BUF(p, p_end, result_count);
            p = p_org;

            total_size = offsetof(WASMType, types) +
                         sizeof(uint8) * (uint64)(param_count + result_count);
            if (total_size >= UINT32_MAX
                || !(type = module->types[i] = wasm_malloc((uint32)total_size))) {
                set_error_buf(error_buf, error_buf_size,
                              "Load type section failed: allocate memory failed.");
                return false;
            }

            /* Resolve param types and result types */
            type->param_count = param_count;
            type->result_count = result_count;
            for (j = 0; j < param_count; j++) {
                CHECK_BUF(p, p_end, 1);
                type->types[j] = read_uint8(p);
            }
            read_leb_uint32(p, p_end, result_count);
            for (j = 0; j < result_count; j++) {
                CHECK_BUF(p, p_end, 1);
                type->types[param_count + j] = read_uint8(p);
            }
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load type section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load type section success.\n");
    return true;
}

static bool
load_table_import(const uint8 **p_buf, const uint8 *buf_end,
                  WASMTableImport *table,
                  char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;

    CHECK_BUF(p, p_end, 1);
    /* 0x70 */
    table->elem_type = read_uint8(p);
    bh_assert(table->elem_type == TABLE_ELEM_TYPE_ANY_FUNC);
    read_leb_uint32(p, p_end, table->flags);
    read_leb_uint32(p, p_end, table->init_size);
    if (table->flags & 1)
        read_leb_uint32(p, p_end, table->max_size);
    else
        table->max_size = 0x10000;

    *p_buf = p;
    return true;
}

static bool
load_memory_import(const uint8 **p_buf, const uint8 *buf_end,
                   WASMMemoryImport *memory,
                   char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;
    uint32 pool_size = bh_memory_pool_size();
    uint32 max_page_count = pool_size * APP_MEMORY_MAX_GLOBAL_HEAP_PERCENT
                            / NumBytesPerPage;

    read_leb_uint32(p, p_end, memory->flags);
    read_leb_uint32(p, p_end, memory->init_page_count);
    if (memory->flags & 1) {
        read_leb_uint32(p, p_end, memory->max_page_count);
        if (memory->max_page_count > max_page_count)
            memory->max_page_count = max_page_count;
    }
    else
        /* Limit the maximum memory size to max_page_count */
        memory->max_page_count = max_page_count;

    *p_buf = p;
    return true;
}

static bool
load_table(const uint8 **p_buf, const uint8 *buf_end, WASMTable *table,
           char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;

    CHECK_BUF(p, p_end, 1);
    /* 0x70 */
    table->elem_type = read_uint8(p);
    bh_assert(table->elem_type == TABLE_ELEM_TYPE_ANY_FUNC);
    read_leb_uint32(p, p_end, table->flags);
    read_leb_uint32(p, p_end, table->init_size);
    if (table->flags & 1)
        read_leb_uint32(p, p_end, table->max_size);
    else
        table->max_size = 0x10000;

    *p_buf = p;
    return true;
}

static bool
load_memory(const uint8 **p_buf, const uint8 *buf_end, WASMMemory *memory,
            char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;
    uint32 pool_size = bh_memory_pool_size();
    uint32 max_page_count = pool_size * APP_MEMORY_MAX_GLOBAL_HEAP_PERCENT
                            / NumBytesPerPage;

    read_leb_uint32(p, p_end, memory->flags);
    read_leb_uint32(p, p_end, memory->init_page_count);
    if (memory->flags & 1) {
        read_leb_uint32(p, p_end, memory->max_page_count);
        if (memory->max_page_count > max_page_count)
            memory->max_page_count = max_page_count;
    }
    else
        /* Limit the maximum memory size to max_page_count */
        memory->max_page_count = max_page_count;

    *p_buf = p;
    return true;
}

static void*
resolve_sym(const char *module_name, const char *field_name)
{
    void *sym;

#if WASM_ENABLE_LIBC_BUILTIN != 0
    if ((sym = wasm_native_lookup_libc_builtin_func(module_name,
                                                    field_name)))
        return sym;
#endif

#if WASM_ENABLE_LIBC_WASI != 0
    if ((sym = wasm_native_lookup_libc_wasi_func(module_name,
                                                 field_name)))
        return sym;
#endif

#if WASM_ENABLE_BASE_LIB != 0
    if ((sym = wasm_native_lookup_base_lib_func(module_name,
                                                field_name)))
        return sym;
#endif

    if ((sym = wasm_native_lookup_extension_lib_func(module_name,
                                                     field_name)))
        return sym;

    return NULL;
}

static bool
load_import_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end, *p_old;
    uint32 import_count, name_len, type_index, i, u32, flags;
    uint64 total_size;
    WASMImport *import;
    WASMImport *import_functions = NULL, *import_tables = NULL;
    WASMImport *import_memories = NULL, *import_globals = NULL;
    char *module_name, *field_name;
    uint8 mutable, u8, kind;

    read_leb_uint32(p, p_end, import_count);

    if (import_count) {
        module->import_count = import_count;
        total_size = sizeof(WASMImport) * (uint64)import_count;
        if (total_size >= UINT32_MAX
            || !(module->imports = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load import section failed: allocate memory failed.");
            return false;
        }

        memset(module->imports, 0, (uint32)total_size);

        p_old = p;

        /* Scan firstly to get import count of each type */
        for (i = 0; i < import_count; i++) {
            /* module name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            p += name_len;

            /* field name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            p += name_len;

            CHECK_BUF(p, p_end, 1);
            /* 0x00/0x01/0x02/0x03 */
            kind = read_uint8(p);

            switch (kind) {
                case IMPORT_KIND_FUNC: /* import function */
                    read_leb_uint32(p, p_end, type_index);
                    module->import_function_count++;
                    break;

                case IMPORT_KIND_TABLE: /* import table */
                    CHECK_BUF(p, p_end, 1);
                    /* 0x70 */
                    u8 = read_uint8(p);
                    read_leb_uint32(p, p_end, flags);
                    read_leb_uint32(p, p_end, u32);
                    if (flags & 1)
                        read_leb_uint32(p, p_end, u32);
                    module->import_table_count++;
                    if (module->import_table_count > 1) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load import section failed: multiple tables");
                        return false;
                    }
                    break;

                case IMPORT_KIND_MEMORY: /* import memory */
                    read_leb_uint32(p, p_end, flags);
                    read_leb_uint32(p, p_end, u32);
                    if (flags & 1)
                        read_leb_uint32(p, p_end, u32);
                    module->import_memory_count++;
                    if (module->import_memory_count > 1) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load import section failed: multiple memories");
                        return false;
                    }
                    break;

                case IMPORT_KIND_GLOBAL: /* import global */
                    CHECK_BUF(p, p_end, 2);
                    p += 2;
                    module->import_global_count++;
                    break;

                default:
                    set_error_buf(error_buf, error_buf_size,
                                  "Load import section failed: invalid import type.");
                    return false;
            }
        }

        if (module->import_function_count)
            import_functions = module->import_functions = module->imports;
        if (module->import_table_count)
            import_tables = module->import_tables =
                module->imports + module->import_function_count;
        if (module->import_memory_count)
            import_memories = module->import_memories =
                module->imports + module->import_function_count + module->import_table_count;
        if (module->import_global_count)
            import_globals = module->import_globals =
                module->imports + module->import_function_count + module->import_table_count
                + module->import_memory_count;

        p = p_old;

        /* Scan again to read the data */
        for (i = 0; i < import_count; i++) {
            /* load module name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            if (!(module_name = const_str_set_insert
                        (p, name_len, module, error_buf, error_buf_size))) {
                return false;
            }
            p += name_len;

            /* load field name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            if (!(field_name = const_str_set_insert
                        (p, name_len, module, error_buf, error_buf_size))) {
                return false;
            }
            p += name_len;

            CHECK_BUF(p, p_end, 1);
            /* 0x00/0x01/0x02/0x03 */
            kind = read_uint8(p);
            switch (kind) {
                case IMPORT_KIND_FUNC: /* import function */
                    bh_assert(import_functions);
                    import = import_functions++;
                    read_leb_uint32(p, p_end, type_index);
                    if (type_index >= module->type_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load import section failed: "
                                      "function type index out of range.");
                        return false;
                    }
                    import->u.function.func_type = module->types[type_index];

                    if (!module->possible_memory_grow
                        && !strcmp(module_name, "env")
                        && !(strcmp(field_name, "enlargeMemory")))
                        module->possible_memory_grow = true;

                    if (!(import->u.function.func_ptr_linked =
                                resolve_sym(module_name, field_name))) {
#ifndef BUILD_AOT_COMPILER /* Output warning except running aot compiler */
                        LOG_WARNING("warning: fail to link import function (%s, %s)\n",
                                    module_name, field_name);
#endif
                    }
                    break;

                case IMPORT_KIND_TABLE: /* import table */
                    bh_assert(import_tables);
                    import = import_tables++;
                    if (!load_table_import(&p, p_end, &import->u.table,
                                error_buf, error_buf_size))
                        return false;
                    if (module->import_table_count > 1) {
                        set_error_buf(error_buf, error_buf_size, "multiple tables");
                        return false;
                    }
                    break;

                case IMPORT_KIND_MEMORY: /* import memory */
                    bh_assert(import_memories);
                    import = import_memories++;
                    if (!load_memory_import(&p, p_end, &import->u.memory,
                                error_buf, error_buf_size))
                        return false;
                    if (module->import_table_count > 1) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load import section failed: multiple memories");
                        return false;
                    }
                    break;

                case IMPORT_KIND_GLOBAL: /* import global */
                    bh_assert(import_globals);
                    import = import_globals++;
                    CHECK_BUF(p, p_end, 2);
                    import->u.global.type = read_uint8(p);
                    mutable = read_uint8(p);
                    if (mutable >= 2) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load import section failed: "
                                      "invalid mutability");
                        return false;
                    }
                    import->u.global.is_mutable = mutable & 1 ? true : false;
#if WASM_ENABLE_LIBC_BUILTIN != 0
                    if (!(wasm_native_lookup_libc_builtin_global(
                                    module_name, field_name,
                                    &import->u.global))) {
                        if (error_buf != NULL)
                            snprintf(error_buf, error_buf_size,
                                     "Load import section failed: "
                                     "resolve import global (%s, %s) failed.",
                                     module_name, field_name);
                        return false;
                    }
#endif
                    break;

                default:
                    set_error_buf(error_buf, error_buf_size,
                                  "Load import section failed: "
                                  "invalid import type.");
                    return false;
            }
            import->kind = kind;
            import->u.names.module_name = module_name;
            import->u.names.field_name = field_name;
        }

#if WASM_ENABLE_LIBC_WASI != 0
        import = module->import_functions;
        for (i = 0; i < module->import_function_count; i++, import++) {
            if (!strcmp(import->u.names.module_name, "wasi_unstable")) {
                module->is_wasi_module = true;
                break;
            }
        }
#endif
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load import section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load import section success.\n");
    (void)u8;
    (void)u32;
    return true;
}

static bool
load_function_section(const uint8 *buf, const uint8 *buf_end,
                      const uint8 *buf_code, const uint8 *buf_code_end,
                      WASMModule *module,
                      char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    const uint8 *p_code = buf_code, *p_code_end, *p_code_save;
    uint32 func_count;
    uint64 total_size;
    uint32 code_count = 0, code_size, type_index, i, j, k, local_type_index;
    uint32 local_count, local_set_count, sub_local_count;
    uint8 type;
    WASMFunction *func;

    read_leb_uint32(p, p_end, func_count);

    if (buf_code)
        read_leb_uint32(p_code, buf_code_end, code_count);

    if (func_count != code_count) {
        set_error_buf(error_buf, error_buf_size,
                      "Load function section failed: "
                      "function and code section have inconsistent lengths");
        return false;
    }

    if (func_count) {
        module->function_count = func_count;
        total_size = sizeof(WASMFunction*) * (uint64)func_count;
        if (total_size >= UINT32_MAX
            || !(module->functions = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load function section failed: allocate memory failed.");
            return false;
        }

        memset(module->functions, 0, (uint32)total_size);

        for (i = 0; i < func_count; i++) {
            /* Resolve function type */
            read_leb_uint32(p, p_end, type_index);
            if (type_index >= module->type_count) {
                set_error_buf(error_buf, error_buf_size,
                              "Load function section failed: "
                              "function type index out of range.");
                return false;
            }

            read_leb_uint32(p_code, buf_code_end, code_size);
            if (code_size == 0
                || p_code + code_size > buf_code_end) {
                set_error_buf(error_buf, error_buf_size,
                              "Load function section failed: "
                              "invalid function code size.");
                return false;
            }

            /* Resolve local set count */
            p_code_end = p_code + code_size;
            local_count = 0;
            read_leb_uint32(p_code, buf_code_end, local_set_count);
            p_code_save = p_code;

            /* Calculate total local count */
            for (j = 0; j < local_set_count; j++) {
                read_leb_uint32(p_code, buf_code_end, sub_local_count);
                if (sub_local_count > UINT32_MAX - local_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "Load function section failed: "
                                  "too many locals");
                    return false;
                }
                CHECK_BUF(p_code, buf_code_end, 1);
                /* 0x7F/0x7E/0x7D/0x7C */
                type = read_uint8(p_code);
                local_count += sub_local_count;
            }

            /* Alloc memory, layout: function structure + local types */
            code_size = (uint32)(p_code_end - p_code);

            total_size = sizeof(WASMFunction) + (uint64)local_count;
            if (total_size >= UINT32_MAX
                || !(func = module->functions[i] = wasm_malloc((uint32)total_size))) {
                set_error_buf(error_buf, error_buf_size,
                              "Load function section failed: "
                              "allocate memory failed.");
                return false;
            }

            /* Set function type, local count, code size and code body */
            memset(func, 0, (uint32)total_size);
            func->func_type = module->types[type_index];
            func->local_count = local_count;
            if (local_count > 0)
                func->local_types = (uint8*)func + sizeof(WASMFunction);
            func->code_size = code_size;
            func->code = (uint8*)p_code;

            /* Load each local type */
            p_code = p_code_save;
            local_type_index = 0;
            for (j = 0; j < local_set_count; j++) {
                read_leb_uint32(p_code, buf_code_end, sub_local_count);
                if (local_type_index + sub_local_count <= local_type_index
                    || local_type_index + sub_local_count > local_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "Load function section failed: "
                                  "invalid local count.");
                    return false;
                }
                CHECK_BUF(p_code, buf_code_end, 1);
                /* 0x7F/0x7E/0x7D/0x7C */
                type = read_uint8(p_code);
                if (type < VALUE_TYPE_F64 || type > VALUE_TYPE_I32) {
                    set_error_buf(error_buf, error_buf_size,
                                  "Load function section failed: "
                                  "invalid local type.");
                    return false;
                }
                for (k = 0; k < sub_local_count; k++) {
                    func->local_types[local_type_index++] = type;
                }
            }
            p_code = p_code_end;
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load function section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load function section success.\n");
    return true;
}

static bool
load_table_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                   char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 table_count, i;
    uint64 total_size;
    WASMTable *table;

    read_leb_uint32(p, p_end, table_count);
    bh_assert(table_count == 1);

    if (table_count) {
        if (table_count > 1) {
            set_error_buf(error_buf, error_buf_size,
                          "Load table section failed: multiple memories");
            return false;
        }
        module->table_count = table_count;
        total_size = sizeof(WASMTable) * (uint64)table_count;
        if (total_size >= UINT32_MAX
            || !(module->tables = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load table section failed: allocate memory failed.");
            return false;
        }

        memset(module->tables, 0, (uint32)total_size);

        /* load each table */
        table = module->tables;
        for (i = 0; i < table_count; i++, table++)
            if (!load_table(&p, p_end, table, error_buf, error_buf_size))
                return false;
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load table section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load table section success.\n");
    return true;
}

static bool
load_memory_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 memory_count, i;
    uint64 total_size;
    WASMMemory *memory;

    read_leb_uint32(p, p_end, memory_count);
    bh_assert(memory_count == 1);

    if (memory_count) {
        if (memory_count > 1) {
            set_error_buf(error_buf, error_buf_size,
                          "Load memory section failed: multiple memories");
            return false;
        }
        module->memory_count = memory_count;
        total_size = sizeof(WASMMemory) * (uint64)memory_count;
        if (total_size >= UINT32_MAX
            || !(module->memories = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                         "Load memory section failed: allocate memory failed.");
            return false;
        }

        memset(module->memories, 0, (uint32)total_size);

        /* load each memory */
        memory = module->memories;
        for (i = 0; i < memory_count; i++, memory++)
            if (!load_memory(&p, p_end, memory, error_buf, error_buf_size))
                return false;
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load memory section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load memory section success.\n");
    return true;
}

static bool
load_global_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 global_count, i;
    uint64 total_size;
    WASMGlobal *global;
    uint8 mutable;

    read_leb_uint32(p, p_end, global_count);

    if (global_count) {
        module->global_count = global_count;
        total_size = sizeof(WASMGlobal) * (uint64)global_count;
        if (total_size >= UINT32_MAX
            || !(module->globals = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load global section failed: "
                          "allocate memory failed.");
            return false;
        }

        memset(module->globals, 0, (uint32)total_size);

        global = module->globals;

        for(i = 0; i < global_count; i++, global++) {
            CHECK_BUF(p, p_end, 2);
            global->type = read_uint8(p);
            mutable = read_uint8(p);
            if (mutable >= 2) {
                set_error_buf(error_buf, error_buf_size,
                              "Load import section failed: "
                              "invalid mutability");
                return false;
            }
            global->is_mutable = mutable ? true : false;

            /* initialize expression */
            if (!load_init_expr(&p, p_end, &(global->init_expr), error_buf, error_buf_size))
                return false;
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load global section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load global section success.\n");
    return true;
}

static bool
load_export_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 export_count, i, index;
    uint64 total_size;
    uint32 str_len;
    WASMExport *export;

    read_leb_uint32(p, p_end, export_count);

    if (export_count) {
        module->export_count = export_count;
        total_size = sizeof(WASMExport) * (uint64)export_count;
        if (total_size >= UINT32_MAX
            || !(module->exports = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load export section failed: "
                          "allocate memory failed.");
            return false;
        }

        memset(module->exports, 0, (uint32)total_size);

        export = module->exports;
        for (i = 0; i < export_count; i++, export++) {
            read_leb_uint32(p, p_end, str_len);
            CHECK_BUF(p, p_end, str_len);
            if (!(export->name = const_str_set_insert(p, str_len, module,
                            error_buf, error_buf_size))) {
                return false;
            }
            p += str_len;
            CHECK_BUF(p, p_end, 1);
            export->kind = read_uint8(p);
            read_leb_uint32(p, p_end, index);
            export->index = index;

            switch(export->kind) {
                /*function index*/
                case EXPORT_KIND_FUNC:
                    if (index >= module->function_count + module->import_function_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load export section failed: "
                                      "function index out of range.");
                        return false;
                    }
                    break;
                /*table index*/
                case EXPORT_KIND_TABLE:
                    if (index >= module->table_count + module->import_table_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load export section failed: "
                                      "table index out of range.");
                        return false;
                    }
                    break;
                /*memory index*/
                case EXPORT_KIND_MEMORY:
                    if (index >= module->memory_count + module->import_memory_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load export section failed: "
                                      "memory index out of range.");
                        return false;
                    }
                    break;
                /*global index*/
                case EXPORT_KIND_GLOBAL:
                    if (index >= module->global_count + module->import_global_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "Load export section failed: "
                                      "global index out of range.");
                        return false;
                    }
                    break;
                default:
                    set_error_buf(error_buf, error_buf_size,
                                  "Load export section failed: "
                                  "invalid export kind.");
                    return false;
            }
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load export section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load export section success.\n");
    return true;
}

static bool
load_table_segment_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                           char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 table_segment_count, i, j, table_index, function_count, function_index;
    uint64 total_size;
    WASMTableSeg *table_segment;

    read_leb_uint32(p, p_end, table_segment_count);

    if (table_segment_count) {
        module->table_seg_count = table_segment_count;
        total_size = sizeof(WASMTableSeg) * (uint64)table_segment_count;
        if (total_size >= UINT32_MAX
            || !(module->table_segments = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load table segment section failed: "
                          "allocate memory failed.");
            return false;
        }

        memset(module->table_segments, 0, (uint32)total_size);

        table_segment = module->table_segments;
        for (i = 0; i < table_segment_count; i++, table_segment++) {
            if (p >= p_end) {
                set_error_buf(error_buf, error_buf_size,
                              "Load table segment section failed: "
                              "invalid value type");
                return false;
            }
            read_leb_uint32(p, p_end, table_index);
            table_segment->table_index = table_index;

            /* initialize expression */
            if (!load_init_expr(&p, p_end, &(table_segment->base_offset),
                                error_buf, error_buf_size))
                return false;

            read_leb_uint32(p, p_end, function_count);
            table_segment->function_count = function_count;
            total_size = sizeof(uint32) * (uint64)function_count;
            if (total_size >= UINT32_MAX
                || !(table_segment->func_indexes = (uint32 *)
                        wasm_malloc((uint32)total_size))) {
                set_error_buf(error_buf, error_buf_size,
                              "Load table segment section failed: "
                              "allocate memory failed.");
                return false;
            }
            for (j = 0; j < function_count; j++) {
                read_leb_uint32(p, p_end, function_index);
                table_segment->func_indexes[j] = function_index;
            }
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                     "Load table segment section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load table segment section success.\n");
    return true;
}

static bool
load_data_segment_section(const uint8 *buf, const uint8 *buf_end,
                          WASMModule *module,
                          char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 data_seg_count, i, mem_index, data_seg_len;
    uint64 total_size;
    WASMDataSeg *dataseg;
    InitializerExpression init_expr;

    read_leb_uint32(p, p_end, data_seg_count);

    if (data_seg_count) {
        module->data_seg_count = data_seg_count;
        total_size = sizeof(WASMDataSeg*) * (uint64)data_seg_count;
        if (total_size >= UINT32_MAX
            || !(module->data_segments = wasm_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Load data segment section failed: "
                          "allocate memory failed.");
            return false;
        }

        memset(module->data_segments, 0, (uint32)total_size);

        for (i = 0; i < data_seg_count; i++) {
            read_leb_uint32(p, p_end, mem_index);

            if (!load_init_expr(&p, p_end, &init_expr, error_buf, error_buf_size))
                return false;

            read_leb_uint32(p, p_end, data_seg_len);

            if (!(dataseg = module->data_segments[i] =
                        wasm_malloc((uint32)sizeof(WASMDataSeg)))) {
                set_error_buf(error_buf, error_buf_size,
                              "Load data segment section failed: "
                              "allocate memory failed.");
                return false;
            }

            bh_memcpy_s(&dataseg->base_offset, sizeof(InitializerExpression),
                        &init_expr, sizeof(InitializerExpression));

            dataseg->memory_index = mem_index;
            dataseg->data_length = data_seg_len;
            CHECK_BUF(p, p_end, data_seg_len);
            dataseg->data = (uint8*)p;
            p += data_seg_len;
        }
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load data segment section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load data segment section success.\n");
    return true;
}

static bool
load_code_section(const uint8 *buf, const uint8 *buf_end,
                  const uint8 *buf_func,
                  const uint8 *buf_func_end,
                  WASMModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    const uint8 *p_func = buf_func;
    uint32 func_count = 0, code_count;

    /* code has been loaded in function section, so pass it here, just check
     * whether function and code section have inconsistent lengths */
    read_leb_uint32(p, p_end, code_count);

    if (buf_func)
        read_leb_uint32(p_func, buf_func_end, func_count);

    if (func_count != code_count) {
        set_error_buf(error_buf, error_buf_size,
                      "Load code section failed: "
                      "function and code section have inconsistent lengths");
        return false;
    }

    LOG_VERBOSE("Load code segment section success.\n");
    return true;
}

static bool
load_start_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                   char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 start_function;

    read_leb_uint32(p, p_end, start_function);

    if (start_function) {
        if (start_function >= module->function_count + module->import_function_count) {
            set_error_buf(error_buf, error_buf_size,
                          "Load start section failed: "
                          "function index out of range.");
            return false;
        }
        module->start_function = start_function;
    }

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load start section failed: section size mismatch");
        return false;
    }

    LOG_VERBOSE("Load start section success.\n");
    return true;
}

static bool
load_user_section(const uint8 *buf, const uint8 *buf_end, WASMModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 name_len;

    if (p >= p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load custom section failed: unexpected end");
        return false;
    }

    read_leb_uint32(p, p_end, name_len);

    if (name_len == 0
        || p + name_len > p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "Load custom section failed: unexpected end");
        return false;
    }

    if (!check_utf8_str(p, name_len)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "invalid UTF-8 encoding");
        return false;
    }

    LOG_VERBOSE("Load custom section success.\n");
    return true;
}


static bool
wasm_loader_prepare_bytecode(WASMModule *module, WASMFunction *func,
                             char *error_buf, uint32 error_buf_size);

static bool
load_from_sections(WASMModule *module, WASMSection *sections,
                   char *error_buf, uint32 error_buf_size)
{
    WASMSection *section = sections;
    const uint8 *buf, *buf_end, *buf_code = NULL, *buf_code_end = NULL,
                *buf_func = NULL, *buf_func_end = NULL;
    uint32 i;

    /* Find code and function sections if have */
    while (section) {
        if (section->section_type == SECTION_TYPE_CODE) {
            buf_code = section->section_body;
            buf_code_end = buf_code + section->section_body_size;
        }
        else if (section->section_type == SECTION_TYPE_FUNC) {
            buf_func = section->section_body;
            buf_func_end = buf_func + section->section_body_size;
        }
        section = section->next;
    }

    section = sections;
    while (section) {
        buf = section->section_body;
        buf_end = buf + section->section_body_size;
        switch (section->section_type) {
            case SECTION_TYPE_USER:
                /* unsupported user section, ignore it. */
                if (!load_user_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_TYPE:
                if (!load_type_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_IMPORT:
                if (!load_import_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_FUNC:
                if (!load_function_section(buf, buf_end, buf_code, buf_code_end,
                            module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_TABLE:
                if (!load_table_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_MEMORY:
                if (!load_memory_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_GLOBAL:
                if (!load_global_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_EXPORT:
                if (!load_export_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_START:
                if (!load_start_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_ELEM:
                if (!load_table_segment_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_CODE:
                if (!load_code_section(buf, buf_end, buf_func, buf_func_end,
                                       module, error_buf, error_buf_size))
                    return false;
                break;
            case SECTION_TYPE_DATA:
                if (!load_data_segment_section(buf, buf_end, module, error_buf, error_buf_size))
                    return false;
                break;
            default:
                set_error_buf(error_buf, error_buf_size,
                              "WASM module load failed: invalid section id");
                return false;
        }

        section = section->next;
    }

    for (i = 0; i < module->function_count; i++) {
        WASMFunction *func = module->functions[i];
        if (!wasm_loader_prepare_bytecode(module, func, error_buf, error_buf_size))
            return false;
    }

    return true;
}

#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
static void wasm_loader_free(void *ptr)
{
    wasm_free(ptr);
}
#else
#define wasm_loader_free wasm_free
#endif

static WASMModule*
create_module(char *error_buf, uint32 error_buf_size)
{
    WASMModule *module = wasm_malloc(sizeof(WASMModule));

    if (!module) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(module, 0, sizeof(WASMModule));

    module->module_type = Wasm_Module_Bytecode;

    /* Set start_function to -1, means no start function */
    module->start_function = (uint32)-1;

    if (!(module->const_str_set = bh_hash_map_create(32, false,
                    (HashFunc)wasm_string_hash,
                    (KeyEqualFunc)wasm_string_equal,
                    NULL,
                    wasm_loader_free))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "create const string set failed.");
        goto fail;
    }

    return module;

fail:
    wasm_loader_unload(module);
    return NULL;
}

WASMModule *
wasm_loader_load_from_sections(WASMSection *section_list,
                               char *error_buf, uint32 error_buf_size)
{
    WASMModule *module = create_module(error_buf, error_buf_size);
    if (!module)
        return NULL;

    if (!load_from_sections(module, section_list, error_buf, error_buf_size)) {
        wasm_loader_unload(module);
        return NULL;
    }

    LOG_VERBOSE("Load module from sections success.\n");
    return module;
}

static void
destroy_sections(WASMSection *section_list)
{
    WASMSection *section = section_list, *next;
    while (section) {
        next = section->next;
        wasm_free(section);
        section = next;
    }
}

static bool
create_sections(const uint8 *buf, uint32 size,
                WASMSection **p_section_list,
                char *error_buf, uint32 error_buf_size)
{
    WASMSection *section_list_end = NULL, *section;
    const uint8 *p = buf, *p_end = buf + size/*, *section_body*/;
    uint8 section_type, last_section_type = (uint8)-1;
    uint32 section_size;

    bh_assert(!*p_section_list);

    p += 8;
    while (p < p_end) {
        CHECK_BUF(p, p_end, 1);
        section_type = read_uint8(p);
        if (section_type <= SECTION_TYPE_DATA) {
            if (section_type != SECTION_TYPE_USER) {
                /* Custom sections may be inserted at any place,
                   while other sections must occur at most once
                   and in prescribed order. */
                if (last_section_type != (uint8)-1
                    && section_type <= last_section_type) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM module load failed: "
                                  "junk after last section");
                    return false;
                }
                last_section_type = section_type;
            }
            CHECK_BUF1(p, p_end, 1);
            read_leb_uint32(p, p_end, section_size);
            CHECK_BUF1(p, p_end, section_size);

            if (!(section = wasm_malloc(sizeof(WASMSection)))) {
                set_error_buf(error_buf, error_buf_size,
                              "WASM module load failed: "
                              "allocate memory failed.");
                return false;
            }

            memset(section, 0, sizeof(WASMSection));
            section->section_type = section_type;
            section->section_body = p;
            section->section_body_size = section_size;

            if (!*p_section_list)
                *p_section_list = section_list_end = section;
            else {
                section_list_end->next = section;
                section_list_end = section;
            }

            p += section_size;
        }
        else {
            set_error_buf(error_buf, error_buf_size,
                          "WASM module load failed: invalid section id");
            return false;
        }
    }

    return true;
}

static void
exchange32(uint8* p_data)
{
    uint8 value = *p_data;
    *p_data = *(p_data + 3);
    *(p_data + 3) = value;

    value = *(p_data + 1);
    *(p_data + 1) = *(p_data + 2);
    *(p_data + 2) = value;
}

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

static bool
load(const uint8 *buf, uint32 size, WASMModule *module,
     char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf_end = buf + size;
    const uint8 *p = buf, *p_end = buf_end;
    uint32 magic_number, version;
    WASMSection *section_list = NULL;

    CHECK_BUF1(p, p_end, sizeof(uint32));
    magic_number = read_uint32(p);
    if (!is_little_endian())
        exchange32((uint8*)&magic_number);

    if (magic_number != WASM_MAGIC_NUMBER) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: magic header not detected");
        return false;
    }

    CHECK_BUF1(p, p_end, sizeof(uint32));
    version = read_uint32(p);
    if (!is_little_endian())
        exchange32((uint8*)&version);

    if (version != WASM_CURRENT_VERSION) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: unknown binary version");
        return false;
    }

    if (!create_sections(buf, size, &section_list, error_buf, error_buf_size)
        || !load_from_sections(module, section_list, error_buf, error_buf_size)) {
        destroy_sections(section_list);
        return false;
    }

    destroy_sections(section_list);
    return true;
}

WASMModule*
wasm_loader_load(const uint8 *buf, uint32 size, char *error_buf, uint32 error_buf_size)
{
    WASMModule *module = wasm_malloc(sizeof(WASMModule));

    if (!module) {
        set_error_buf(error_buf, error_buf_size,
                "WASM module load failed: allocate memory failed.");
        return NULL;
    }

    memset(module, 0, sizeof(WASMModule));

    module->module_type = Wasm_Module_Bytecode;

    /* Set start_function to -1, means no start function */
    module->start_function = (uint32)-1;

    if (!(module->const_str_set =
                bh_hash_map_create(32, false,
                                   (HashFunc)wasm_string_hash,
                                   (KeyEqualFunc)wasm_string_equal,
                                   NULL,
                                   wasm_loader_free)))
        goto fail;

    if (!load(buf, size, module, error_buf, error_buf_size))
        goto fail;

    LOG_VERBOSE("Load module success.\n");
    return module;

fail:
    wasm_loader_unload(module);
    return NULL;
}

void
wasm_loader_unload(WASMModule *module)
{
    uint32 i;

    if (!module)
        return;

    if (module->types) {
        for (i = 0; i < module->type_count; i++) {
            if (module->types[i])
                wasm_free(module->types[i]);
        }
        wasm_free(module->types);
    }

    if (module->imports)
        wasm_free(module->imports);

    if (module->functions) {
        for (i = 0; i < module->function_count; i++) {
            if (module->functions[i])
                wasm_free(module->functions[i]);
        }
        wasm_free(module->functions);
    }

    if (module->tables)
        wasm_free(module->tables);

    if (module->memories)
        wasm_free(module->memories);

    if (module->globals)
        wasm_free(module->globals);

    if (module->exports)
        wasm_free(module->exports);

    if (module->table_segments) {
        for (i = 0; i < module->table_seg_count; i++) {
            if (module->table_segments[i].func_indexes)
                wasm_free(module->table_segments[i].func_indexes);
        }
        wasm_free(module->table_segments);
    }

    if (module->data_segments) {
        for (i = 0; i < module->data_seg_count; i++) {
            if (module->data_segments[i])
                wasm_free(module->data_segments[i]);
        }
        wasm_free(module->data_segments);
    }

    if (module->const_str_set)
        bh_hash_map_destroy(module->const_str_set);

    wasm_free(module);
}

bool
wasm_loader_find_block_addr(WASMModule *module,
                            const uint8 *start_addr,
                            const uint8 *code_end_addr,
                            uint8 block_type,
                            uint8 **p_else_addr,
                            uint8 **p_end_addr,
                            char *error_buf,
                            uint32 error_buf_size)
{
    const uint8 *p = start_addr, *p_end = code_end_addr;
    uint8 *else_addr = NULL;
    uint32 block_nested_depth = 1, count, i, u32;
    uint64 u64;
    uint8 opcode, u8;

    BlockAddr block_stack[16] = { 0 }, *block;
    uint32 j, t;

    i = (uint32)(((uintptr_t)start_addr) ^ ((uintptr_t)start_addr >> 16));
    i = i % BLOCK_ADDR_CACHE_SIZE;
    block = module->block_addr_cache[i];
    for (j = 0; j < BLOCK_ADDR_CONFLICT_SIZE; j++) {
        if (block[j].start_addr == start_addr) {
            /* Cache hit */
            *p_else_addr = block[j].else_addr;
            *p_end_addr = block[j].end_addr;
            return true;
        }
    }

    /* Cache unhit */
    block_stack[0].start_addr = start_addr;

    while (p < code_end_addr) {
        opcode = *p++;

        switch (opcode) {
            case WASM_OP_UNREACHABLE:
            case WASM_OP_NOP:
                break;

            case WASM_OP_BLOCK:
            case WASM_OP_LOOP:
            case WASM_OP_IF:
                CHECK_BUF(p, p_end, 1);
                /* block result type: 0x40/0x7F/0x7E/0x7D/0x7C */
                u8 = read_uint8(p);
                if (block_nested_depth < sizeof(block_stack)/sizeof(BlockAddr)) {
                    block_stack[block_nested_depth].start_addr = p;
                    block_stack[block_nested_depth].else_addr = NULL;
                }
                block_nested_depth++;
                break;

            case WASM_OP_ELSE:
                if (block_type == BLOCK_TYPE_IF && block_nested_depth == 1)
                    else_addr = (uint8*)(p - 1);
                if (block_nested_depth - 1 < sizeof(block_stack)/sizeof(BlockAddr))
                    block_stack[block_nested_depth - 1].else_addr = (uint8*)(p - 1);
                break;

            case WASM_OP_END:
                if (block_nested_depth == 1) {
                    if (block_type == BLOCK_TYPE_IF)
                        *p_else_addr = else_addr;
                    *p_end_addr = (uint8*)(p - 1);

                    block_stack[0].end_addr = (uint8*)(p - 1);
                    for (t = 0; t < sizeof(block_stack)/sizeof(BlockAddr); t++) {
                        start_addr = block_stack[t].start_addr;
                        if (start_addr) {
                            i = (uint32)(((uintptr_t)start_addr) ^ ((uintptr_t)start_addr >> 16));
                            i = i % BLOCK_ADDR_CACHE_SIZE;
                            block = module->block_addr_cache[i];
                            for (j = 0; j < BLOCK_ADDR_CONFLICT_SIZE; j++)
                                if (!block[j].start_addr)
                                    break;

                            if (j == BLOCK_ADDR_CONFLICT_SIZE) {
                                memmove(block + 1, block, (BLOCK_ADDR_CONFLICT_SIZE - 1) *
                                                          sizeof(BlockAddr));
                                j = 0;

                            }
                            block[j].start_addr = block_stack[t].start_addr;
                            block[j].else_addr = block_stack[t].else_addr;
                            block[j].end_addr = block_stack[t].end_addr;
                        }
                        else
                            break;
                    }
                    return true;
                }
                else {
                    block_nested_depth--;
                    if (block_nested_depth < sizeof(block_stack)/sizeof(BlockAddr))
                        block_stack[block_nested_depth].end_addr = (uint8*)(p - 1);
                }
                break;

            case WASM_OP_BR:
            case WASM_OP_BR_IF:
                read_leb_uint32(p, p_end, u32); /* labelidx */
                break;

            case WASM_OP_BR_TABLE:
                read_leb_uint32(p, p_end, count); /* lable num */
                for (i = 0; i <= count; i++) /* lableidxs */
                    read_leb_uint32(p, p_end, u32);
                break;

            case WASM_OP_RETURN:
                break;

            case WASM_OP_CALL:
                read_leb_uint32(p, p_end, u32); /* funcidx */
                break;

            case WASM_OP_CALL_INDIRECT:
                read_leb_uint32(p, p_end, u32); /* typeidx */
                CHECK_BUF(p, p_end, 1);
                u8 = read_uint8(p); /* 0x00 */
                break;

            case WASM_OP_DROP:
            case WASM_OP_SELECT:
            case WASM_OP_DROP_32:
            case WASM_OP_DROP_64:
            case WASM_OP_SELECT_32:
            case WASM_OP_SELECT_64:
                break;

            case WASM_OP_GET_LOCAL:
            case WASM_OP_SET_LOCAL:
            case WASM_OP_TEE_LOCAL:
            case WASM_OP_GET_GLOBAL:
            case WASM_OP_SET_GLOBAL:
                read_leb_uint32(p, p_end, u32); /* localidx */
                break;

            case WASM_OP_I32_LOAD:
            case WASM_OP_I64_LOAD:
            case WASM_OP_F32_LOAD:
            case WASM_OP_F64_LOAD:
            case WASM_OP_I32_LOAD8_S:
            case WASM_OP_I32_LOAD8_U:
            case WASM_OP_I32_LOAD16_S:
            case WASM_OP_I32_LOAD16_U:
            case WASM_OP_I64_LOAD8_S:
            case WASM_OP_I64_LOAD8_U:
            case WASM_OP_I64_LOAD16_S:
            case WASM_OP_I64_LOAD16_U:
            case WASM_OP_I64_LOAD32_S:
            case WASM_OP_I64_LOAD32_U:
            case WASM_OP_I32_STORE:
            case WASM_OP_I64_STORE:
            case WASM_OP_F32_STORE:
            case WASM_OP_F64_STORE:
            case WASM_OP_I32_STORE8:
            case WASM_OP_I32_STORE16:
            case WASM_OP_I64_STORE8:
            case WASM_OP_I64_STORE16:
            case WASM_OP_I64_STORE32:
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                break;

            case WASM_OP_MEMORY_SIZE:
            case WASM_OP_MEMORY_GROW:
                read_leb_uint32(p, p_end, u32); /* 0x00 */
                break;

            case WASM_OP_I32_CONST:
                read_leb_int32(p, p_end, u32);
                break;
            case WASM_OP_I64_CONST:
                read_leb_int64(p, p_end, u64);
                break;
            case WASM_OP_F32_CONST:
                p += sizeof(float32);
                break;
            case WASM_OP_F64_CONST:
                p += sizeof(float64);
                break;

            case WASM_OP_I32_EQZ:
            case WASM_OP_I32_EQ:
            case WASM_OP_I32_NE:
            case WASM_OP_I32_LT_S:
            case WASM_OP_I32_LT_U:
            case WASM_OP_I32_GT_S:
            case WASM_OP_I32_GT_U:
            case WASM_OP_I32_LE_S:
            case WASM_OP_I32_LE_U:
            case WASM_OP_I32_GE_S:
            case WASM_OP_I32_GE_U:
            case WASM_OP_I64_EQZ:
            case WASM_OP_I64_EQ:
            case WASM_OP_I64_NE:
            case WASM_OP_I64_LT_S:
            case WASM_OP_I64_LT_U:
            case WASM_OP_I64_GT_S:
            case WASM_OP_I64_GT_U:
            case WASM_OP_I64_LE_S:
            case WASM_OP_I64_LE_U:
            case WASM_OP_I64_GE_S:
            case WASM_OP_I64_GE_U:
            case WASM_OP_F32_EQ:
            case WASM_OP_F32_NE:
            case WASM_OP_F32_LT:
            case WASM_OP_F32_GT:
            case WASM_OP_F32_LE:
            case WASM_OP_F32_GE:
            case WASM_OP_F64_EQ:
            case WASM_OP_F64_NE:
            case WASM_OP_F64_LT:
            case WASM_OP_F64_GT:
            case WASM_OP_F64_LE:
            case WASM_OP_F64_GE:
            case WASM_OP_I32_CLZ:
            case WASM_OP_I32_CTZ:
            case WASM_OP_I32_POPCNT:
            case WASM_OP_I32_ADD:
            case WASM_OP_I32_SUB:
            case WASM_OP_I32_MUL:
            case WASM_OP_I32_DIV_S:
            case WASM_OP_I32_DIV_U:
            case WASM_OP_I32_REM_S:
            case WASM_OP_I32_REM_U:
            case WASM_OP_I32_AND:
            case WASM_OP_I32_OR:
            case WASM_OP_I32_XOR:
            case WASM_OP_I32_SHL:
            case WASM_OP_I32_SHR_S:
            case WASM_OP_I32_SHR_U:
            case WASM_OP_I32_ROTL:
            case WASM_OP_I32_ROTR:
            case WASM_OP_I64_CLZ:
            case WASM_OP_I64_CTZ:
            case WASM_OP_I64_POPCNT:
            case WASM_OP_I64_ADD:
            case WASM_OP_I64_SUB:
            case WASM_OP_I64_MUL:
            case WASM_OP_I64_DIV_S:
            case WASM_OP_I64_DIV_U:
            case WASM_OP_I64_REM_S:
            case WASM_OP_I64_REM_U:
            case WASM_OP_I64_AND:
            case WASM_OP_I64_OR:
            case WASM_OP_I64_XOR:
            case WASM_OP_I64_SHL:
            case WASM_OP_I64_SHR_S:
            case WASM_OP_I64_SHR_U:
            case WASM_OP_I64_ROTL:
            case WASM_OP_I64_ROTR:
            case WASM_OP_F32_ABS:
            case WASM_OP_F32_NEG:
            case WASM_OP_F32_CEIL:
            case WASM_OP_F32_FLOOR:
            case WASM_OP_F32_TRUNC:
            case WASM_OP_F32_NEAREST:
            case WASM_OP_F32_SQRT:
            case WASM_OP_F32_ADD:
            case WASM_OP_F32_SUB:
            case WASM_OP_F32_MUL:
            case WASM_OP_F32_DIV:
            case WASM_OP_F32_MIN:
            case WASM_OP_F32_MAX:
            case WASM_OP_F32_COPYSIGN:
            case WASM_OP_F64_ABS:
            case WASM_OP_F64_NEG:
            case WASM_OP_F64_CEIL:
            case WASM_OP_F64_FLOOR:
            case WASM_OP_F64_TRUNC:
            case WASM_OP_F64_NEAREST:
            case WASM_OP_F64_SQRT:
            case WASM_OP_F64_ADD:
            case WASM_OP_F64_SUB:
            case WASM_OP_F64_MUL:
            case WASM_OP_F64_DIV:
            case WASM_OP_F64_MIN:
            case WASM_OP_F64_MAX:
            case WASM_OP_F64_COPYSIGN:
            case WASM_OP_I32_WRAP_I64:
            case WASM_OP_I32_TRUNC_S_F32:
            case WASM_OP_I32_TRUNC_U_F32:
            case WASM_OP_I32_TRUNC_S_F64:
            case WASM_OP_I32_TRUNC_U_F64:
            case WASM_OP_I64_EXTEND_S_I32:
            case WASM_OP_I64_EXTEND_U_I32:
            case WASM_OP_I64_TRUNC_S_F32:
            case WASM_OP_I64_TRUNC_U_F32:
            case WASM_OP_I64_TRUNC_S_F64:
            case WASM_OP_I64_TRUNC_U_F64:
            case WASM_OP_F32_CONVERT_S_I32:
            case WASM_OP_F32_CONVERT_U_I32:
            case WASM_OP_F32_CONVERT_S_I64:
            case WASM_OP_F32_CONVERT_U_I64:
            case WASM_OP_F32_DEMOTE_F64:
            case WASM_OP_F64_CONVERT_S_I32:
            case WASM_OP_F64_CONVERT_U_I32:
            case WASM_OP_F64_CONVERT_S_I64:
            case WASM_OP_F64_CONVERT_U_I64:
            case WASM_OP_F64_PROMOTE_F32:
            case WASM_OP_I32_REINTERPRET_F32:
            case WASM_OP_I64_REINTERPRET_F64:
            case WASM_OP_F32_REINTERPRET_I32:
            case WASM_OP_F64_REINTERPRET_I64:
                break;

            default:
                if (error_buf)
                    snprintf(error_buf, error_buf_size,
                             "WASM loader find block addr failed: "
                             "invalid opcode %02x.", opcode);
                return false;
        }
    }

    (void)u32;
    (void)u64;
    (void)u8;
    return false;
}

#define REF_I32   VALUE_TYPE_I32
#define REF_F32   VALUE_TYPE_F32
#define REF_I64_1 VALUE_TYPE_I64
#define REF_I64_2 VALUE_TYPE_I64
#define REF_F64_1 VALUE_TYPE_F64
#define REF_F64_2 VALUE_TYPE_F64

typedef struct BranchBlock {
    uint8 block_type;
    uint8 return_type;
    bool is_block_reachable;
    uint8 *start_addr;
    uint8 *else_addr;
    uint8 *end_addr;
    uint32 stack_cell_num;
} BranchBlock;

static void*
memory_realloc(void *mem_old, uint32 size_old, uint32 size_new)
{
    uint8 *mem_new;
    bh_assert(size_new > size_old);
    if ((mem_new = wasm_malloc(size_new))) {
        bh_memcpy_s(mem_new, size_new, mem_old, size_old);
        memset(mem_new + size_old, 0, size_new - size_old);
        wasm_free(mem_old);
    }
    return mem_new;
}

#define MEM_REALLOC(mem, size_old, size_new) do {           \
    void *mem_new = memory_realloc(mem, size_old, size_new);\
    if (!mem_new) {                                         \
      set_error_buf(error_buf, error_buf_size,              \
                    "WASM loader prepare bytecode failed: " \
                    "allocate memory failed.");             \
      goto fail;                                            \
    }                                                       \
    mem = mem_new;                                          \
  } while (0)

static bool
check_stack_push(uint8 **p_frame_ref_bottom, uint8 **p_frame_ref_boundary,
                 uint8 **p_frame_ref, uint32 *p_frame_ref_size,
                 uint32 stack_cell_num,
                 char *error_buf, uint32 error_buf_size)
{
    if (*p_frame_ref >= *p_frame_ref_boundary) {
        MEM_REALLOC(*p_frame_ref_bottom, *p_frame_ref_size,
                *p_frame_ref_size + 16);
        *p_frame_ref_size += 16;
        *p_frame_ref_boundary = *p_frame_ref_bottom + *p_frame_ref_size;
        *p_frame_ref = *p_frame_ref_bottom + stack_cell_num;
    }
    return true;
fail:
    return false;
}

#define CHECK_STACK_PUSH() do {                                 \
    if (!check_stack_push(&frame_ref_bottom, &frame_ref_boundary,\
                          &frame_ref, &frame_ref_size,          \
                          stack_cell_num,                       \
                          error_buf, error_buf_size))           \
      goto fail;                                                \
  } while (0)

static bool
check_stack_pop(uint8 type, uint8 *frame_ref, uint32 stack_cell_num,
                char *error_buf, uint32 error_buf_size,
                const char *type_str)
{
    if (((type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32)
         && stack_cell_num < 1)
        || ((type == VALUE_TYPE_I64 || type == VALUE_TYPE_F64)
            && stack_cell_num < 2)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "type mismatch: expect data but stack was empty");
        return false;
    }

    if ((type == VALUE_TYPE_I32 && *(frame_ref - 1) != REF_I32)
        || (type == VALUE_TYPE_F32 && *(frame_ref - 1) != REF_F32)
        || (type == VALUE_TYPE_I64
            && (*(frame_ref - 2) != REF_I64_1 || *(frame_ref - 1) != REF_I64_2))
        || (type == VALUE_TYPE_F64
            && (*(frame_ref - 2) != REF_F64_1 || *(frame_ref - 1) != REF_F64_2))) {
        if (error_buf != NULL)
            snprintf(error_buf, error_buf_size, "%s%s%s",
                     "WASM module load failed: type mismatch: expect ",
                     type_str, " but got other");
        return false;
    }
    return true;
}

#define CHECK_STACK_POP(TYPE, type) do {                    \
    if (!check_stack_pop(VALUE_TYPE_##TYPE,                 \
                         frame_ref, stack_cell_num,         \
                         error_buf, error_buf_size, #type)) \
      goto fail;                                            \
  } while (0)

#define PUSH_I32() do {                         \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_I32;                     \
    stack_cell_num++;                           \
    if (stack_cell_num > max_stack_cell_num)    \
      max_stack_cell_num = stack_cell_num;      \
  } while (0)

#define PUSH_F32() do {                         \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_F32;                     \
    stack_cell_num++;                           \
    if (stack_cell_num > max_stack_cell_num)    \
      max_stack_cell_num = stack_cell_num;      \
  } while (0)

#define PUSH_I64() do {                         \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_I64_1;                   \
    stack_cell_num++;                           \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_I64_2;                   \
    stack_cell_num++;                           \
    if (stack_cell_num > max_stack_cell_num)    \
      max_stack_cell_num = stack_cell_num;      \
  } while (0)

#define PUSH_F64() do {                         \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_F64_1;                   \
    stack_cell_num++;                           \
    CHECK_STACK_PUSH();                         \
    *frame_ref++ = REF_F64_2;                   \
    stack_cell_num++;                           \
    if (stack_cell_num > max_stack_cell_num)    \
      max_stack_cell_num = stack_cell_num;      \
  } while (0)

#define POP_I32() do {                          \
    CHECK_STACK_POP(I32, i32);                  \
    stack_cell_num--;                           \
    frame_ref--;                                \
  } while (0)

#define POP_I64() do {                          \
    CHECK_STACK_POP(I64, i64);                  \
    stack_cell_num -= 2;                        \
    frame_ref -= 2;                             \
  } while (0)

#define POP_F32() do {                          \
    CHECK_STACK_POP(F32, f32);                  \
    stack_cell_num--;                           \
    frame_ref--;                                \
  } while (0)

#define POP_F64() do {                          \
    CHECK_STACK_POP(F64, f64);                  \
    stack_cell_num -= 2;                        \
    frame_ref -= 2;                             \
  } while (0)

static bool
push_type(uint8 type, uint8 **p_frame_ref_bottom,
          uint8 **p_frame_ref_boundary,
          uint8 **p_frame_ref, uint32 *p_frame_ref_size,
          uint32 *p_stack_cell_num, uint32 *p_max_stack_cell_num,
          char *error_buf, uint32 error_buf_size)
{
    uint8 *frame_ref = *p_frame_ref;
    uint32 frame_ref_size = *p_frame_ref_size;
    uint32 max_stack_cell_num = *p_max_stack_cell_num;
    uint32 stack_cell_num = *p_stack_cell_num;

    switch (type) {
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            if (!check_stack_push(p_frame_ref_bottom, p_frame_ref_boundary,
                                  &frame_ref, &frame_ref_size,
                                  stack_cell_num,
                                  error_buf, error_buf_size))
                goto fail;
            *frame_ref++ = type;
            stack_cell_num++;
            if (stack_cell_num > max_stack_cell_num)
                max_stack_cell_num = stack_cell_num;
            goto handle_i32_f32;
handle_i32_f32:
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            if (!check_stack_push(p_frame_ref_bottom, p_frame_ref_boundary,
                                  &frame_ref, &frame_ref_size,
                                  stack_cell_num,
                                  error_buf, error_buf_size))
                goto fail;
            *frame_ref++ = type;
            stack_cell_num++;
            if (stack_cell_num > max_stack_cell_num)
                max_stack_cell_num = stack_cell_num;
            break;
    }

    *p_frame_ref = frame_ref;
    *p_frame_ref_size = frame_ref_size;
    *p_max_stack_cell_num = max_stack_cell_num;
    *p_stack_cell_num = stack_cell_num;
    return true;
fail:
    return false;
}

#define PUSH_TYPE(type) do {                        \
    if (!push_type(type, &frame_ref_bottom,         \
            &frame_ref_boundary,                    \
            &frame_ref, &frame_ref_size,            \
            &stack_cell_num, &max_stack_cell_num,   \
            error_buf, error_buf_size))             \
        goto fail;                                  \
  } while (0)

static bool
pop_type(uint8 type, uint8 **p_frame_ref, uint32 *p_stack_cell_num,
         char *error_buf, uint32 error_buf_size)
{
    char *type_str[] = { "f64", "f32", "i64", "i32" };
    switch (type) {
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            if (!check_stack_pop(type, *p_frame_ref, *p_stack_cell_num,
                                 error_buf, error_buf_size,
                                 type_str[type - VALUE_TYPE_F64]))
                return false;
            *p_frame_ref -= 2;
            *p_stack_cell_num -= 2;
            break;
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            if (!check_stack_pop(type, *p_frame_ref, *p_stack_cell_num,
                                 error_buf, error_buf_size,
                                 type_str[type - VALUE_TYPE_F64]))
                return false;
            *p_frame_ref -= 1;
            *p_stack_cell_num -= 1;
            break;
    }
    return true;
}

#define POP_TYPE(type) do {                         \
    if (!pop_type(type, &frame_ref, &stack_cell_num,\
                  error_buf, error_buf_size))       \
      goto fail;                                    \
  } while (0)

#define CHECK_CSP_PUSH() do {                               \
    if (frame_csp >= frame_csp_boundary) {                  \
      MEM_REALLOC(frame_csp_bottom, frame_csp_size,         \
                  (uint32)(frame_csp_size                   \
                           + 8 * sizeof(BranchBlock)));     \
      frame_csp_size += (uint32)(8 * sizeof(BranchBlock));  \
      frame_csp_boundary = frame_csp_bottom +               \
                    frame_csp_size / sizeof(BranchBlock);   \
      frame_csp = frame_csp_bottom + csp_num;               \
    }                                                       \
  } while (0)

#define CHECK_CSP_POP() do {                                \
    if (csp_num < 1) {                                      \
      set_error_buf(error_buf, error_buf_size,              \
                  "WASM module load failed: type mismatch: "\
                  "expect data but block stack was empty"); \
      goto fail;                                            \
    }                                                       \
  } while (0)

#define PUSH_CSP(type, ret_type, _start_addr) do {  \
    CHECK_CSP_PUSH();                               \
    frame_csp->block_type = type;                   \
    frame_csp->is_block_reachable = false;          \
    frame_csp->return_type = ret_type;              \
    frame_csp->start_addr = _start_addr;            \
    frame_csp->else_addr = NULL;                    \
    frame_csp->end_addr = NULL;                     \
    frame_csp->stack_cell_num = stack_cell_num;     \
    frame_csp++;                                    \
    csp_num++;                                      \
    if (csp_num > max_csp_num)                      \
      max_csp_num = csp_num;                        \
  } while (0)

#define POP_CSP() do {                              \
    CHECK_CSP_POP();                                \
    frame_csp--;                                    \
    csp_num--;                                      \
  } while (0)

#define GET_LOCAL_INDEX_AND_TYPE() do {             \
    read_leb_uint32(p, p_end, local_idx);           \
    if (local_idx >= param_count + local_count) {   \
      set_error_buf(error_buf, error_buf_size,      \
                    "WASM module load failed: "     \
                    "local index out of range"); \
      goto fail;                                    \
    }                                               \
    local_type = local_idx < param_count            \
        ? param_types[local_idx]                    \
        : local_types[local_idx - param_count];     \
  } while (0)

#define CHECK_BR(depth) do {                                        \
    if (csp_num < depth + 1) {                                      \
      set_error_buf(error_buf, error_buf_size,                      \
                    "WASM module load failed: type mismatch: "      \
                    "unexpected end of section or function");       \
      goto fail;                                                    \
    }                                                               \
    if ((frame_csp - (depth + 1))->block_type != BLOCK_TYPE_LOOP) { \
      uint8 tmp_ret_type = (frame_csp - (depth + 1))->return_type;  \
      if ((tmp_ret_type == VALUE_TYPE_I32                           \
            && (stack_cell_num < 1 || *(frame_ref - 1) != REF_I32)) \
          || (tmp_ret_type == VALUE_TYPE_F32                        \
              && (stack_cell_num < 1 || *(frame_ref - 1) != REF_F32))\
          || (tmp_ret_type == VALUE_TYPE_I64                        \
              && (stack_cell_num < 2                                \
                  || *(frame_ref - 2) != REF_I64_1                  \
                  || *(frame_ref - 1) != REF_I64_2))                \
          || (tmp_ret_type == VALUE_TYPE_F64                        \
              && (stack_cell_num < 2                                \
                  || *(frame_ref - 2) != REF_F64_1                  \
                  || *(frame_ref - 1) != REF_F64_2))) {             \
        set_error_buf(error_buf, error_buf_size,                    \
                "WASM module load failed: type mismatch: "          \
                "expect data but stack was empty or other type");   \
        goto fail;                                                  \
      }                                                             \
      (frame_csp - (depth + 1))->is_block_reachable = true;         \
    }                                                               \
  } while (0)

static bool
check_memory(WASMModule *module,
             char *error_buf, uint32 error_buf_size)
{
    if (module->memory_count == 0
        && module->import_memory_count == 0) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "load or store in module without default memory");
        return false;
    }
    return true;
}

#define CHECK_MEMORY() do {                                 \
    if (!check_memory(module, error_buf, error_buf_size))   \
      goto fail;                                            \
  } while (0)

static bool
wasm_loader_prepare_bytecode(WASMModule *module, WASMFunction *func,
                             char *error_buf, uint32 error_buf_size)
{
    uint8 *p = func->code, *p_end = func->code + func->code_size;
    uint8 *frame_ref_bottom = NULL, *frame_ref_boundary, *frame_ref;
    BranchBlock *frame_csp_bottom = NULL, *frame_csp_boundary, *frame_csp;
    uint32 param_count, local_count, global_count;
    uint32 max_stack_cell_num = 0, max_csp_num = 0;
    uint32 stack_cell_num = 0, csp_num = 0;
    uint32 frame_ref_size, frame_csp_size;
    uint8 *param_types, ret_type, *local_types, local_type, global_type;
    uint32 count, i, local_idx, global_idx, depth, u32;
    int32 i32, i32_const = 0;
    int64 i64;
    uint8 opcode, u8, block_return_type;
    bool return_value = false, is_i32_const = false;

    global_count = module->import_global_count + module->global_count;

    param_count = func->func_type->param_count;
    param_types = func->func_type->types;
    ret_type = func->func_type->result_count
               ? param_types[param_count] : VALUE_TYPE_VOID;

    local_count = func->local_count;
    local_types = func->local_types;

    frame_ref_size = 32;
    if (!(frame_ref_bottom = frame_ref = wasm_malloc(frame_ref_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
        goto fail;
    }
    memset(frame_ref_bottom, 0, frame_ref_size);
    frame_ref_boundary = frame_ref_bottom + frame_ref_size;

    frame_csp_size = sizeof(BranchBlock) * 8;
    if (!(frame_csp_bottom = frame_csp = wasm_malloc(frame_csp_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
        goto fail;
    }

    memset(frame_csp_bottom, 0, frame_csp_size);
    frame_csp_boundary = frame_csp_bottom + 8;

    PUSH_CSP(BLOCK_TYPE_FUNCTION, ret_type, p);
    (frame_csp - 1)->is_block_reachable = true;

    while (p < p_end) {
        opcode = *p++;

        switch (opcode) {
            case WASM_OP_UNREACHABLE:
                goto handle_next_reachable_block;

            case WASM_OP_NOP:
                break;

            case WASM_OP_BLOCK:
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_BLOCK, block_return_type, p);
                break;

            case WASM_OP_LOOP:
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_LOOP, block_return_type, p);
                break;

            case WASM_OP_IF:
                POP_I32();
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_IF, block_return_type, p);
                if (!is_i32_const)
                    (frame_csp - 1)->is_block_reachable = true;
                else {
                    if (!i32_const) {
                        if(!wasm_loader_find_block_addr(module,
                                                        (frame_csp - 1)->start_addr,
                                                        p_end,
                                                        (frame_csp - 1)->block_type,
                                                        &(frame_csp - 1)->else_addr,
                                                        &(frame_csp - 1)->end_addr,
                                                        error_buf, error_buf_size))
                            goto fail;

                        if ((frame_csp - 1)->else_addr)
                            p = (frame_csp - 1)->else_addr;
                        else
                            p = (frame_csp - 1)->end_addr;
                    }
                }
                break;

            case WASM_OP_ELSE:
                if (csp_num < 2
                    || (frame_csp - 1)->block_type != BLOCK_TYPE_IF) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode else found without matched opcode if");
                    goto fail;
                }

                (frame_csp - 1)->else_addr = p - 1;
                stack_cell_num = (frame_csp - 1)->stack_cell_num;
                frame_ref = frame_ref_bottom + stack_cell_num;
                break;

            case WASM_OP_END:
            {
                POP_CSP();

                POP_TYPE(frame_csp->return_type);
                PUSH_TYPE(frame_csp->return_type);

                if (csp_num > 0) {
                    frame_csp->end_addr = p - 1;
                }
                else {
                    /* end of function block, function will return,
                       ignore the following bytecodes */
                    p = p_end;
                }
                break;
            }

            case WASM_OP_BR:
            {
                read_leb_uint32(p, p_end, depth);
                CHECK_BR(depth);

handle_next_reachable_block:
                for (i = 1; i <= csp_num; i++)
                    if ((frame_csp - i)->is_block_reachable)
                        break;

                block_return_type = (frame_csp - i)->return_type;

                if(!wasm_loader_find_block_addr(module,
                                                (frame_csp - i)->start_addr,
                                                p_end,
                                                (frame_csp - i)->block_type,
                                                &(frame_csp - i)->else_addr,
                                                &(frame_csp - i)->end_addr,
                                                error_buf, error_buf_size))
                    goto fail;

                stack_cell_num = (frame_csp - i)->stack_cell_num;
                frame_ref = frame_ref_bottom + stack_cell_num;
                csp_num -= i - 1;
                frame_csp -= i - 1;

                if ((frame_csp - 1)->block_type == BLOCK_TYPE_IF
                        && (frame_csp - 1)->else_addr != NULL
                        && p <= (frame_csp - 1)->else_addr)
                    p = (frame_csp - 1)->else_addr;
                else {
                    p = (frame_csp - 1)->end_addr;
                    PUSH_TYPE(block_return_type);
                }

                break;
            }

            case WASM_OP_BR_IF:
                read_leb_uint32(p, p_end, depth);
                POP_I32();
                CHECK_BR(depth);
                if (!is_i32_const)
                    (frame_csp - (depth + 1))->is_block_reachable = true;
                else {
                    if (i32_const)
                        goto handle_next_reachable_block;
                }
                break;

            case WASM_OP_BR_TABLE:
            {
                read_leb_uint32(p, p_end, count);
                POP_I32();

                /* TODO: check the const */
                for (i = 0; i <= count; i++) {
                    read_leb_uint32(p, p_end, depth);
                    CHECK_BR(depth);
                }
                goto handle_next_reachable_block;
            }

            case WASM_OP_RETURN:
            {
                POP_TYPE(ret_type);
                PUSH_TYPE(ret_type);

                if(!wasm_loader_find_block_addr(module,
                                                (frame_csp - 1)->start_addr,
                                                p_end,
                                                (frame_csp - 1)->block_type,
                                                &(frame_csp - 1)->else_addr,
                                                &(frame_csp - 1)->end_addr,
                                                error_buf, error_buf_size))
                    goto fail;

                stack_cell_num = (frame_csp - 1)->stack_cell_num;
                frame_ref = frame_ref_bottom + stack_cell_num;
                if ((frame_csp - 1)->block_type == BLOCK_TYPE_IF
                    && p <= (frame_csp - 1)->else_addr) {
                    p = (frame_csp - 1)->else_addr;
                }
                else {
                    p = (frame_csp - 1)->end_addr;
                    PUSH_TYPE((frame_csp - 1)->return_type);
                }
                break;
            }

            case WASM_OP_CALL:
            {
                WASMType *func_type;
                uint32 func_idx;
                int32 idx;

                read_leb_uint32(p, p_end, func_idx);

                if (func_idx >= module->import_function_count + module->function_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "function index out of range");
                    goto fail;
                }

                if (func_idx < module->import_function_count)
                    func_type = module->import_functions[func_idx].u.function.func_type;
                else
                    func_type =
                        module->functions[func_idx - module->import_function_count]->func_type;

                if (func_type->param_count > 0) {
                    for (idx = (int32)(func_type->param_count - 1); idx >= 0; idx--)
                        POP_TYPE(func_type->types[idx]);
                }

                if (func_type->result_count)
                    PUSH_TYPE(func_type->types[func_type->param_count]);

                func->has_op_func_call = true;
                break;
            }

            case WASM_OP_CALL_INDIRECT:
            {
                int32 idx;
                WASMType *func_type;
                uint32 type_idx;

                if (module->table_count == 0
                    && module->import_table_count == 0) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "call indirect without default table");
                    goto fail;
                }

                read_leb_uint32(p, p_end, type_idx);

                /* reserved byte 0x00 */
                if (*p++ != 0x00) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "zero flag expected");
                    goto fail;
                }

                POP_I32();

                if (type_idx >= module->type_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "function index out of range");
                    goto fail;
                }

                func_type = module->types[type_idx];

                if (func_type->param_count > 0) {
                    for (idx = (int32)(func_type->param_count - 1); idx >= 0; idx--)
                        POP_TYPE(func_type->types[idx]);
                }

                PUSH_TYPE(func_type->types[func_type->param_count]);

                func->has_op_func_call = true;
                break;
            }

            case WASM_OP_DROP:
            {
                if (stack_cell_num <= 0) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode drop was found but stack was empty");
                    goto fail;
                }

                if (*(frame_ref - 1) == REF_I32
                    || *(frame_ref - 1) == REF_F32) {
                    frame_ref--;
                    stack_cell_num--;
                    *(p - 1) = WASM_OP_DROP_32;
                }
                else {
                    if (stack_cell_num <= 1) {
                        set_error_buf(error_buf, error_buf_size,
                                      "WASM loader prepare bytecode failed: "
                                      "opcode drop was found but stack was empty");
                        goto fail;
                    }
                    frame_ref -= 2;
                    stack_cell_num -= 2;
                    *(p - 1) = WASM_OP_DROP_64;
                }
                break;
            }

            case WASM_OP_SELECT:
            {
                uint8 ref_type;

                POP_I32();

                if (stack_cell_num <= 0) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode select was found but stack was empty");
                    goto fail;
                }

                switch (*(frame_ref - 1)) {
                    case REF_I32:
                    case REF_F32:
                        *(p - 1) = WASM_OP_SELECT_32;
                        break;
                    case REF_I64_2:
                    case REF_F64_2:
                        *(p - 1) = WASM_OP_SELECT_64;
                        break;
                }

                ref_type = *(frame_ref - 1);
                POP_TYPE(ref_type);
                POP_TYPE(ref_type);
                PUSH_TYPE(ref_type);
                break;
            }

            case WASM_OP_GET_LOCAL:
            {
                GET_LOCAL_INDEX_AND_TYPE();
                PUSH_TYPE(local_type);
                break;
            }

            case WASM_OP_SET_LOCAL:
            {
                GET_LOCAL_INDEX_AND_TYPE();
                POP_TYPE(local_type);
                break;
            }

            case WASM_OP_TEE_LOCAL:
            {
                GET_LOCAL_INDEX_AND_TYPE();
                POP_TYPE(local_type);
                PUSH_TYPE(local_type);
                break;
            }

            case WASM_OP_GET_GLOBAL:
            {
                read_leb_uint32(p, p_end, global_idx);
                if (global_idx >= global_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "global index out of range");
                    goto fail;
                }

                global_type = global_idx < module->import_global_count
                              ? module->import_globals[global_idx].u.global.type
                              :module->globals[global_idx - module->import_global_count].type;

                PUSH_TYPE(global_type);
                break;
            }

            case WASM_OP_SET_GLOBAL:
            {
                read_leb_uint32(p, p_end, global_idx);
                if (global_idx >= global_count) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "global index out of range");
                    goto fail;
                }

                global_type = global_idx < module->import_global_count
                              ? module->import_globals[global_idx].u.global.type
                              : module->globals[global_idx - module->import_global_count].type;

                POP_TYPE(global_type);
                break;
            }

            case WASM_OP_I32_LOAD:
            case WASM_OP_I32_LOAD8_S:
            case WASM_OP_I32_LOAD8_U:
            case WASM_OP_I32_LOAD16_S:
            case WASM_OP_I32_LOAD16_U:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I32();
                PUSH_I32();
                break;

            case WASM_OP_I64_LOAD:
            case WASM_OP_I64_LOAD8_S:
            case WASM_OP_I64_LOAD8_U:
            case WASM_OP_I64_LOAD16_S:
            case WASM_OP_I64_LOAD16_U:
            case WASM_OP_I64_LOAD32_S:
            case WASM_OP_I64_LOAD32_U:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I32();
                PUSH_I64();
                break;

            case WASM_OP_F32_LOAD:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I32();
                PUSH_F32();
                break;

            case WASM_OP_F64_LOAD:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I32();
                PUSH_F64();
                break;

            case WASM_OP_I32_STORE:
            case WASM_OP_I32_STORE8:
            case WASM_OP_I32_STORE16:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I32();
                POP_I32();
                break;

            case WASM_OP_I64_STORE:
            case WASM_OP_I64_STORE8:
            case WASM_OP_I64_STORE16:
            case WASM_OP_I64_STORE32:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_I64();
                POP_I32();
                break;

            case WASM_OP_F32_STORE:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_F32();
                POP_I32();
                break;

            case WASM_OP_F64_STORE:
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, u32); /* align */
                read_leb_uint32(p, p_end, u32); /* offset */
                POP_F64();
                POP_I32();
                break;

            case WASM_OP_MEMORY_SIZE:
                CHECK_MEMORY();
                /* reserved byte 0x00 */
                if (*p++ != 0x00) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "zero flag expected");
                    goto fail;
                }
                PUSH_I32();
                break;

            case WASM_OP_MEMORY_GROW:
                CHECK_MEMORY();
                /* reserved byte 0x00 */
                if (*p++ != 0x00) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "zero flag expected");
                    goto fail;
                }
                POP_I32();
                PUSH_I32();

                func->has_op_memory_grow = true;
                module->possible_memory_grow = true;
                break;

            case WASM_OP_I32_CONST:
                read_leb_int32(p, p_end, i32_const);
                /* Currently we only track simple I32_CONST opcode. */
                is_i32_const = true;
                PUSH_I32();
                break;

            case WASM_OP_I64_CONST:
                read_leb_int64(p, p_end, i64);
                PUSH_I64();
                break;

            case WASM_OP_F32_CONST:
                p += sizeof(float32);
                PUSH_F32();
                break;

            case WASM_OP_F64_CONST:
                p += sizeof(float64);
                PUSH_F64();
                break;

            case WASM_OP_I32_EQZ:
                POP_I32();
                PUSH_I32();
                break;

            case WASM_OP_I32_EQ:
            case WASM_OP_I32_NE:
            case WASM_OP_I32_LT_S:
            case WASM_OP_I32_LT_U:
            case WASM_OP_I32_GT_S:
            case WASM_OP_I32_GT_U:
            case WASM_OP_I32_LE_S:
            case WASM_OP_I32_LE_U:
            case WASM_OP_I32_GE_S:
            case WASM_OP_I32_GE_U:
                POP_I32();
                POP_I32();
                PUSH_I32();
                break;

            case WASM_OP_I64_EQZ:
                POP_I64();
                PUSH_I32();
                break;

            case WASM_OP_I64_EQ:
            case WASM_OP_I64_NE:
            case WASM_OP_I64_LT_S:
            case WASM_OP_I64_LT_U:
            case WASM_OP_I64_GT_S:
            case WASM_OP_I64_GT_U:
            case WASM_OP_I64_LE_S:
            case WASM_OP_I64_LE_U:
            case WASM_OP_I64_GE_S:
            case WASM_OP_I64_GE_U:
                POP_I64();
                POP_I64();
                PUSH_I32();
                break;

            case WASM_OP_F32_EQ:
            case WASM_OP_F32_NE:
            case WASM_OP_F32_LT:
            case WASM_OP_F32_GT:
            case WASM_OP_F32_LE:
            case WASM_OP_F32_GE:
                POP_F32();
                POP_F32();
                PUSH_I32();
                break;

            case WASM_OP_F64_EQ:
            case WASM_OP_F64_NE:
            case WASM_OP_F64_LT:
            case WASM_OP_F64_GT:
            case WASM_OP_F64_LE:
            case WASM_OP_F64_GE:
                POP_F64();
                POP_F64();
                PUSH_I32();
                break;

                break;

            case WASM_OP_I32_CLZ:
            case WASM_OP_I32_CTZ:
            case WASM_OP_I32_POPCNT:
                POP_I32();
                PUSH_I32();
                break;

            case WASM_OP_I32_ADD:
            case WASM_OP_I32_SUB:
            case WASM_OP_I32_MUL:
            case WASM_OP_I32_DIV_S:
            case WASM_OP_I32_DIV_U:
            case WASM_OP_I32_REM_S:
            case WASM_OP_I32_REM_U:
            case WASM_OP_I32_AND:
            case WASM_OP_I32_OR:
            case WASM_OP_I32_XOR:
            case WASM_OP_I32_SHL:
            case WASM_OP_I32_SHR_S:
            case WASM_OP_I32_SHR_U:
            case WASM_OP_I32_ROTL:
            case WASM_OP_I32_ROTR:
                POP_I32();
                POP_I32();
                PUSH_I32();
                break;

            case WASM_OP_I64_CLZ:
            case WASM_OP_I64_CTZ:
            case WASM_OP_I64_POPCNT:
                POP_I64();
                PUSH_I64();
                break;

            case WASM_OP_I64_ADD:
            case WASM_OP_I64_SUB:
            case WASM_OP_I64_MUL:
            case WASM_OP_I64_DIV_S:
            case WASM_OP_I64_DIV_U:
            case WASM_OP_I64_REM_S:
            case WASM_OP_I64_REM_U:
            case WASM_OP_I64_AND:
            case WASM_OP_I64_OR:
            case WASM_OP_I64_XOR:
            case WASM_OP_I64_SHL:
            case WASM_OP_I64_SHR_S:
            case WASM_OP_I64_SHR_U:
            case WASM_OP_I64_ROTL:
            case WASM_OP_I64_ROTR:
                POP_I64();
                POP_I64();
                PUSH_I64();
                break;

            case WASM_OP_F32_ABS:
            case WASM_OP_F32_NEG:
            case WASM_OP_F32_CEIL:
            case WASM_OP_F32_FLOOR:
            case WASM_OP_F32_TRUNC:
            case WASM_OP_F32_NEAREST:
            case WASM_OP_F32_SQRT:
                POP_F32();
                PUSH_F32();
                break;

            case WASM_OP_F32_ADD:
            case WASM_OP_F32_SUB:
            case WASM_OP_F32_MUL:
            case WASM_OP_F32_DIV:
            case WASM_OP_F32_MIN:
            case WASM_OP_F32_MAX:
            case WASM_OP_F32_COPYSIGN:
                POP_F32();
                POP_F32();
                PUSH_F32();
                break;

            case WASM_OP_F64_ABS:
            case WASM_OP_F64_NEG:
            case WASM_OP_F64_CEIL:
            case WASM_OP_F64_FLOOR:
            case WASM_OP_F64_TRUNC:
            case WASM_OP_F64_NEAREST:
            case WASM_OP_F64_SQRT:
                POP_F64();
                PUSH_F64();
                break;

            case WASM_OP_F64_ADD:
            case WASM_OP_F64_SUB:
            case WASM_OP_F64_MUL:
            case WASM_OP_F64_DIV:
            case WASM_OP_F64_MIN:
            case WASM_OP_F64_MAX:
            case WASM_OP_F64_COPYSIGN:
                POP_F64();
                POP_F64();
                PUSH_F64();
                break;

            case WASM_OP_I32_WRAP_I64:
                POP_I64();
                PUSH_I32();
                break;

            case WASM_OP_I32_TRUNC_S_F32:
            case WASM_OP_I32_TRUNC_U_F32:
                POP_F32();
                PUSH_I32();
                break;

            case WASM_OP_I32_TRUNC_S_F64:
            case WASM_OP_I32_TRUNC_U_F64:
                POP_F64();
                PUSH_I32();
                break;

            case WASM_OP_I64_EXTEND_S_I32:
            case WASM_OP_I64_EXTEND_U_I32:
                POP_I32();
                PUSH_I64();
                break;

            case WASM_OP_I64_TRUNC_S_F32:
            case WASM_OP_I64_TRUNC_U_F32:
                POP_F32();
                PUSH_I64();
                break;

            case WASM_OP_I64_TRUNC_S_F64:
            case WASM_OP_I64_TRUNC_U_F64:
                POP_F64();
                PUSH_I64();
                break;

            case WASM_OP_F32_CONVERT_S_I32:
            case WASM_OP_F32_CONVERT_U_I32:
                POP_I32();
                PUSH_F32();
                break;

            case WASM_OP_F32_CONVERT_S_I64:
            case WASM_OP_F32_CONVERT_U_I64:
                POP_I64();
                PUSH_F32();
                break;

            case WASM_OP_F32_DEMOTE_F64:
                POP_F64();
                PUSH_F32();
                break;

            case WASM_OP_F64_CONVERT_S_I32:
            case WASM_OP_F64_CONVERT_U_I32:
                POP_I32();
                PUSH_F64();
                break;

            case WASM_OP_F64_CONVERT_S_I64:
            case WASM_OP_F64_CONVERT_U_I64:
                POP_I64();
                PUSH_F64();
                break;

            case WASM_OP_F64_PROMOTE_F32:
                POP_F32();
                PUSH_F64();
                break;

            case WASM_OP_I32_REINTERPRET_F32:
                POP_F32();
                PUSH_I32();
                break;

            case WASM_OP_I64_REINTERPRET_F64:
                POP_F64();
                PUSH_I64();
                break;

            case WASM_OP_F32_REINTERPRET_I32:
                POP_I32();
                PUSH_F32();
                break;

            case WASM_OP_F64_REINTERPRET_I64:
                POP_I64();
                PUSH_F64();
                break;

            default:
                if (error_buf != NULL)
                    snprintf(error_buf, error_buf_size,
                             "WASM module load failed: "
                             "invalid opcode %02x.", opcode);
                goto fail;
        }

        if (opcode != WASM_OP_I32_CONST)
            is_i32_const = false;
    }

    if (csp_num > 0) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "function body must end with END opcode.");
        goto fail;
    }

    func->max_stack_cell_num = max_stack_cell_num;
    func->max_block_num = max_csp_num;
    return_value = true;

fail:
    if (frame_ref_bottom)
        wasm_free(frame_ref_bottom);
    if (frame_csp_bottom)
        wasm_free(frame_csp_bottom);

    (void)u8;
    (void)u32;
    (void)i32;
    (void)i64;
    return return_value;
}
