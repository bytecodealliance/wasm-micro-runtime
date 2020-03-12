/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_loader.h"
#include "bh_common.h"
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
skip_leb(const uint8  *buf, const uint8 *buf_end,
         uint32 *p_offset, uint32 maxbits,
         char* error_buf, uint32 error_buf_size)
{
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
        bcnt += 1;
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return true;
}

#define skip_leb_int64(p, p_end) do {               \
  uint32 off = 0;                                   \
  if (!skip_leb(p, p_end, &off, 64,                 \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
} while (0)

#define skip_leb_uint32(p, p_end) do {              \
  uint32 off = 0;                                   \
  if (!skip_leb(p, p_end, &off, 32,                 \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
} while (0)

#define skip_leb_int32(p, p_end) do {               \
  uint32 off = 0;                                   \
  if (!skip_leb(p, p_end, &off, 32,                 \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
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

#define read_leb_int64(p, p_end, res) do {          \
  if (p < p_end) {                                  \
    uint8 _val = *p;                                \
    if (!(_val & 0x80)) {                           \
      res = (int64)_val;                            \
      if (_val & 0x40)                              \
        /* sign extend */                           \
        res |= 0xFFFFFFFFFFFFFF80LL;                \
      p++;                                          \
      break;                                        \
    }                                               \
  }                                                 \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 64, true, &res64,   \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (int64)res64;                               \
} while (0)

#define read_leb_uint32(p, p_end, res) do {         \
  if (p < p_end) {                                  \
    uint8 _val = *p;                                \
    if (!(_val & 0x80)) {                           \
      res = _val;                                   \
      p++;                                          \
      break;                                        \
    }                                               \
  }                                                 \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 32, false, &res64,  \
                error_buf, error_buf_size))         \
    return false;                                   \
  p += off;                                         \
  res = (uint32)res64;                              \
} while (0)

#define read_leb_int32(p, p_end, res) do {          \
  if (p < p_end) {                                  \
    uint8 _val = *p;                                \
    if (!(_val & 0x80)) {                           \
      res = (int32)_val;                            \
      if (_val & 0x40)                              \
        /* sign extend */                           \
        res |= 0xFFFFFF80;                          \
      p++;                                          \
      break;                                        \
    }                                               \
  }                                                 \
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
const_str_list_insert(const uint8 *str, uint32 len, WASMModule *module,
                     char* error_buf, uint32 error_buf_size)
{
    StringNode *node, *node_next;

    if (!check_utf8_str(str, len)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "invalid UTF-8 encoding");
        return NULL;
    }

    /* Search const str list */
    node = module->const_str_list;
    while (node) {
        node_next = node->next;
        if (strlen(node->str) == len
            && !memcmp(node->str, str, len))
            break;
        node = node_next;
    }

    if (node)
        return node->str;

    if (!(node = wasm_runtime_malloc(sizeof(StringNode) + len + 1))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "allocate memory failed.");
        return NULL;
    }

    node->str = ((char*)node) + sizeof(StringNode);
    bh_memcpy_s(node->str, len + 1, str, len);
    node->str[len] = '\0';

    if (!module->const_str_list) {
        /* set as head */
        module->const_str_list = node;
        node->next = NULL;
    }
    else {
        /* insert it */
        node->next = module->const_str_list;
        module->const_str_list = node;
    }

    return node->str;
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
            || !(module->types = wasm_runtime_malloc((uint32)total_size))) {
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
                || !(type = module->types[i] =
                                wasm_runtime_malloc((uint32)total_size))) {
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

unsigned
wasm_runtime_memory_pool_size();

static bool
load_memory_import(const uint8 **p_buf, const uint8 *buf_end,
                   WASMMemoryImport *memory,
                   char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = *p_buf, *p_end = buf_end;
    uint32 pool_size = wasm_runtime_memory_pool_size();
    uint32 max_page_count = pool_size * APP_MEMORY_MAX_GLOBAL_HEAP_PERCENT
                            / DEFAULT_NUM_BYTES_PER_PAGE;

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

    memory->num_bytes_per_page = DEFAULT_NUM_BYTES_PER_PAGE;

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
    uint32 pool_size = wasm_runtime_memory_pool_size();
    uint32 max_page_count = pool_size * APP_MEMORY_MAX_GLOBAL_HEAP_PERCENT
                            / DEFAULT_NUM_BYTES_PER_PAGE;

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

    memory->num_bytes_per_page = DEFAULT_NUM_BYTES_PER_PAGE;

    *p_buf = p;
    return true;
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
            || !(module->imports = wasm_runtime_malloc((uint32)total_size))) {
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

        /* insert "env" and "wasi_unstable" to const str list */
        if (!const_str_list_insert((uint8*)"env", 3, module, error_buf, error_buf_size)
            || !const_str_list_insert((uint8*)"wasi_unstable", 13, module,
                                     error_buf, error_buf_size)) {
            return false;
        }

        /* Scan again to read the data */
        for (i = 0; i < import_count; i++) {
            /* load module name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            if (!(module_name = const_str_list_insert
                        (p, name_len, module, error_buf, error_buf_size))) {
                return false;
            }
            p += name_len;

            /* load field name */
            read_leb_uint32(p, p_end, name_len);
            CHECK_BUF(p, p_end, name_len);
            if (!(field_name = const_str_list_insert
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

                    if (!(import->u.function.func_ptr_linked =
                            wasm_native_resolve_symbol(module_name, field_name,
                                        import->u.function.func_type,
                                        &import->u.function.signature))) {
#if WASM_ENABLE_WAMR_COMPILER == 0 /* Output warning except running aot compiler */
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
                    if (module->import_memory_count > 1) {
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
init_function_local_offsets(WASMFunction *func,
                            char *error_buf, uint32 error_buf_size)
{
    WASMType *param_type = func->func_type;
    uint32 param_count = param_type->param_count;
    uint8 *param_types = param_type->types;
    uint32 local_count = func->local_count;
    uint8 *local_types = func->local_types;
    uint32 i, local_offset = 0;
    uint64 total_size = sizeof(uint16) * ((uint64)param_count + local_count);

    if (total_size >= UINT32_MAX
        || !(func->local_offsets = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Load function section failed: allocate memory failed.");
        return false;
    }

    for (i = 0; i < param_count; i++) {
        func->local_offsets[i] = (uint16)local_offset;
        local_offset += wasm_value_type_cell_num(param_types[i]);
    }

    for (i = 0; i < local_count; i++) {
        func->local_offsets[param_count + i] = (uint16)local_offset;
        local_offset += wasm_value_type_cell_num(local_types[i]);
    }

    bh_assert(local_offset == func->param_cell_num + func->local_cell_num);
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
            || !(module->functions = wasm_runtime_malloc((uint32)total_size))) {
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
                || !(func = module->functions[i] =
                            wasm_runtime_malloc((uint32)total_size))) {
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

            func->param_cell_num = wasm_type_param_cell_num(func->func_type);
            func->ret_cell_num = wasm_type_return_cell_num(func->func_type);
            func->local_cell_num =
                wasm_get_cell_num(func->local_types, func->local_count);

            if (!init_function_local_offsets(func, error_buf, error_buf_size))
                return false;

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
            || !(module->tables = wasm_runtime_malloc((uint32)total_size))) {
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
            || !(module->memories = wasm_runtime_malloc((uint32)total_size))) {
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
            || !(module->globals = wasm_runtime_malloc((uint32)total_size))) {
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
            || !(module->exports = wasm_runtime_malloc((uint32)total_size))) {
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
            if (!(export->name = const_str_list_insert(p, str_len, module,
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
            || !(module->table_segments = wasm_runtime_malloc((uint32)total_size))) {
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
                        wasm_runtime_malloc((uint32)total_size))) {
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
            || !(module->data_segments = wasm_runtime_malloc((uint32)total_size))) {
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
                        wasm_runtime_malloc((uint32)sizeof(WASMDataSeg)))) {
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
                             BlockAddr *block_addr_cache,
                             char *error_buf, uint32 error_buf_size);

#if WASM_ENABLE_FAST_INTERP != 0
void **
wasm_interp_get_handle_table();

static void **handle_table;
#endif

static bool
load_from_sections(WASMModule *module, WASMSection *sections,
                   char *error_buf, uint32 error_buf_size)
{
    WASMExport *export;
    WASMSection *section = sections;
    const uint8 *buf, *buf_end, *buf_code = NULL, *buf_code_end = NULL,
                *buf_func = NULL, *buf_func_end = NULL;
    WASMGlobal *llvm_data_end_global = NULL, *llvm_heap_base_global = NULL;
    WASMGlobal *llvm_stack_top_global = NULL, *global;
    uint32 llvm_data_end = UINT32_MAX, llvm_heap_base = UINT32_MAX;
    uint32 llvm_stack_top = UINT32_MAX, global_index, i;
    uint32 data_end_global_index = UINT32_MAX;
    uint32 heap_base_global_index = UINT32_MAX;
    uint32 stack_top_global_index = UINT32_MAX;
    BlockAddr *block_addr_cache;
    uint64 total_size;

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

#if WASM_ENABLE_FAST_INTERP != 0
    handle_table = wasm_interp_get_handle_table();
#endif

    total_size = sizeof(BlockAddr) * (uint64)BLOCK_ADDR_CACHE_SIZE * BLOCK_ADDR_CONFLICT_SIZE;
    if (total_size >= UINT32_MAX
        || !(block_addr_cache = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: allocate memory failed");
        return false;
    }

    for (i = 0; i < module->function_count; i++) {
        WASMFunction *func = module->functions[i];
        memset(block_addr_cache, 0, (uint32)total_size);
        if (!wasm_loader_prepare_bytecode(module, func, block_addr_cache, error_buf, error_buf_size))
            return false;
    }
    wasm_runtime_free(block_addr_cache);

    /* Resolve llvm auxiliary data/stack/heap info and reset memory info */
    if (!module->possible_memory_grow) {
        export = module->exports;
        for (i = 0; i < module->export_count; i++, export++) {
            if (export->kind == EXPORT_KIND_GLOBAL) {
                if (!strcmp(export->name, "__heap_base")) {
                    global_index = export->index - module->import_global_count;
                    global = module->globals + global_index;
                    if (global->type == VALUE_TYPE_I32
                        && !global->is_mutable
                        && global->init_expr.init_expr_type ==
                                INIT_EXPR_TYPE_I32_CONST) {
                        heap_base_global_index = global_index;
                        llvm_heap_base_global = global;
                        llvm_heap_base = global->init_expr.u.i32;
                        LOG_VERBOSE("found llvm __heap_base global, value: %d\n",
                                    llvm_heap_base);
                    }
                }
                else if (!strcmp(export->name, "__data_end")) {
                    global_index = export->index - module->import_global_count;
                    global = module->globals + global_index;
                    if (global->type == VALUE_TYPE_I32
                        && !global->is_mutable
                        && global->init_expr.init_expr_type ==
                                INIT_EXPR_TYPE_I32_CONST) {
                        data_end_global_index = global_index;
                        llvm_data_end_global = global;
                        llvm_data_end = global->init_expr.u.i32;
                        LOG_VERBOSE("found llvm __data_end global, value: %d\n",
                                    llvm_data_end);

                        llvm_data_end = align_uint(llvm_data_end, 16);
                    }
                }

                if (llvm_data_end_global && llvm_heap_base_global) {
                    if ((data_end_global_index == heap_base_global_index + 1
                         && data_end_global_index > 0)
                        || (heap_base_global_index == data_end_global_index + 1
                            && heap_base_global_index > 0)) {
                        global_index =
                            data_end_global_index < heap_base_global_index
                            ? data_end_global_index - 1 : heap_base_global_index - 1;
                        global = module->globals + global_index;
                        if (global->type == VALUE_TYPE_I32
                            && global->is_mutable
                            && global->init_expr.init_expr_type ==
                                        INIT_EXPR_TYPE_I32_CONST) {
                            llvm_stack_top_global = global;
                            llvm_stack_top = global->init_expr.u.i32;
                            stack_top_global_index = global_index;
                            LOG_VERBOSE("found llvm stack top global, "
                                        "value: %d, global index: %d\n",
                                        llvm_stack_top, global_index);
                        }
                    }
                    break;
                }
            }
        }

        if (llvm_data_end_global
            && llvm_heap_base_global
            && llvm_stack_top_global
            && llvm_stack_top <= llvm_heap_base) {
            WASMMemoryImport *memory_import;
            WASMMemory *memory;
            uint64 init_memory_size;
            uint32 shrunk_memory_size = llvm_heap_base > llvm_data_end
                                        ? llvm_heap_base : llvm_data_end;
            if (module->import_memory_count) {
                memory_import = &module->import_memories[0].u.memory;
                init_memory_size = (uint64)memory_import->num_bytes_per_page *
                                   memory_import->init_page_count;
                if (llvm_heap_base <= init_memory_size
                    && llvm_data_end <= init_memory_size) {
                    /* Reset memory info to decrease memory usage */
                    memory_import->num_bytes_per_page = shrunk_memory_size;
                    memory_import->init_page_count = 1;
                    LOG_VERBOSE("reset import memory size to %d\n",
                                shrunk_memory_size);
                }
            }
            if (module->memory_count) {
                memory = &module->memories[0];
                init_memory_size = (uint64)memory->num_bytes_per_page *
                             memory->init_page_count;
                if (llvm_heap_base <= init_memory_size
                    && llvm_data_end <= init_memory_size) {
                    /* Reset memory info to decrease memory usage */
                    memory->num_bytes_per_page = shrunk_memory_size;
                    memory->init_page_count = 1;
                    LOG_VERBOSE("reset memory size to %d\n", shrunk_memory_size);
                }
            }

            module->llvm_aux_data_end = llvm_data_end;
            module->llvm_aux_stack_bottom = llvm_stack_top;
            module->llvm_aux_stack_size = llvm_stack_top > llvm_data_end
                                          ? llvm_stack_top - llvm_data_end
                                          : llvm_stack_top;
            module->llvm_aux_stack_global_index = stack_top_global_index;
            LOG_VERBOSE("aux stack bottom: %d, size: %d\n",
                        module->llvm_aux_stack_bottom,
                        module->llvm_aux_stack_size);
        }
    }

    return true;
}

#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
static void wasm_loader_free(void *ptr)
{
    wasm_runtime_free(ptr);
}
#else
#define wasm_loader_free wasm_free
#endif

static WASMModule*
create_module(char *error_buf, uint32 error_buf_size)
{
    WASMModule *module = wasm_runtime_malloc(sizeof(WASMModule));

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

    return module;
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
        wasm_runtime_free(section);
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

            if (!(section = wasm_runtime_malloc(sizeof(WASMSection)))) {
                set_error_buf(error_buf, error_buf_size,
                              "WASM module load failed: "
                              "allocate memory failed.");
                return false;
            }

            memset(section, 0, sizeof(WASMSection));
            section->section_type = section_type;
            section->section_body = (uint8*)p;
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
    WASMModule *module = wasm_runtime_malloc(sizeof(WASMModule));

    if (!module) {
        set_error_buf(error_buf, error_buf_size,
                "WASM module load failed: allocate memory failed.");
        return NULL;
    }

    memset(module, 0, sizeof(WASMModule));

    module->module_type = Wasm_Module_Bytecode;

    /* Set start_function to -1, means no start function */
    module->start_function = (uint32)-1;

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
                wasm_runtime_free(module->types[i]);
        }
        wasm_runtime_free(module->types);
    }

    if (module->imports)
        wasm_runtime_free(module->imports);

    if (module->functions) {
        for (i = 0; i < module->function_count; i++) {
            if (module->functions[i]) {
                if (module->functions[i]->local_offsets)
                    wasm_runtime_free(module->functions[i]->local_offsets);
#if WASM_ENABLE_FAST_INTERP != 0
                if (module->functions[i]->code_compiled)
                    wasm_runtime_free(module->functions[i]->code_compiled);
                if (module->functions[i]->consts)
                    wasm_runtime_free(module->functions[i]->consts);
#endif
                wasm_runtime_free(module->functions[i]);
            }
        }
        wasm_runtime_free(module->functions);
    }

    if (module->tables)
        wasm_runtime_free(module->tables);

    if (module->memories)
        wasm_runtime_free(module->memories);

    if (module->globals)
        wasm_runtime_free(module->globals);

    if (module->exports)
        wasm_runtime_free(module->exports);

    if (module->table_segments) {
        for (i = 0; i < module->table_seg_count; i++) {
            if (module->table_segments[i].func_indexes)
                wasm_runtime_free(module->table_segments[i].func_indexes);
        }
        wasm_runtime_free(module->table_segments);
    }

    if (module->data_segments) {
        for (i = 0; i < module->data_seg_count; i++) {
            if (module->data_segments[i])
                wasm_runtime_free(module->data_segments[i]);
        }
        wasm_runtime_free(module->data_segments);
    }

    if (module->const_str_list) {
        StringNode *node = module->const_str_list, *node_next;
        while (node) {
            node_next = node->next;
            wasm_runtime_free(node);
            node = node_next;
        }
    }

    wasm_runtime_free(module);
}

bool
wasm_loader_find_block_addr(BlockAddr *block_addr_cache,
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
    uint32 block_nested_depth = 1, count, i;
    uint8 opcode, u8;

    BlockAddr block_stack[16] = { 0 }, *block;
    uint32 j, t;

    i = ((uintptr_t)start_addr) % BLOCK_ADDR_CACHE_SIZE;
    block = block_addr_cache + BLOCK_ADDR_CONFLICT_SIZE * i;

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
                            i = ((uintptr_t)start_addr) % BLOCK_ADDR_CACHE_SIZE;
                            block = block_addr_cache + BLOCK_ADDR_CONFLICT_SIZE * i;
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
                skip_leb_uint32(p, p_end); /* labelidx */
                break;

            case WASM_OP_BR_TABLE:
                read_leb_uint32(p, p_end, count); /* lable num */
                for (i = 0; i <= count; i++) /* lableidxs */
                    skip_leb_uint32(p, p_end);
                break;

            case WASM_OP_RETURN:
                break;

            case WASM_OP_CALL:
                skip_leb_uint32(p, p_end); /* funcidx */
                break;

            case WASM_OP_CALL_INDIRECT:
                skip_leb_uint32(p, p_end); /* typeidx */
                CHECK_BUF(p, p_end, 1);
                u8 = read_uint8(p); /* 0x00 */
                break;

            case WASM_OP_DROP:
            case WASM_OP_SELECT:
            case WASM_OP_DROP_64:
            case WASM_OP_SELECT_64:
                break;

            case WASM_OP_GET_LOCAL:
            case WASM_OP_SET_LOCAL:
            case WASM_OP_TEE_LOCAL:
            case WASM_OP_GET_GLOBAL:
            case WASM_OP_SET_GLOBAL:
                skip_leb_uint32(p, p_end); /* localidx */
                break;

            case EXT_OP_GET_LOCAL_FAST:
            case EXT_OP_SET_LOCAL_FAST:
            case EXT_OP_TEE_LOCAL_FAST:
                CHECK_BUF(p, p_end, 1);
                p++;
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
                skip_leb_uint32(p, p_end); /* align */
                skip_leb_uint32(p, p_end); /* offset */
                break;

            case WASM_OP_MEMORY_SIZE:
            case WASM_OP_MEMORY_GROW:
                skip_leb_uint32(p, p_end); /* 0x00 */
                break;

            case WASM_OP_I32_CONST:
                skip_leb_int32(p, p_end);
                break;
            case WASM_OP_I64_CONST:
                skip_leb_int64(p, p_end);
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

    (void)u8;
    return false;
}

#define REF_I32   VALUE_TYPE_I32
#define REF_F32   VALUE_TYPE_F32
#define REF_I64_1 VALUE_TYPE_I64
#define REF_I64_2 VALUE_TYPE_I64
#define REF_F64_1 VALUE_TYPE_F64
#define REF_F64_2 VALUE_TYPE_F64

#if WASM_ENABLE_FAST_INTERP != 0

#if WASM_DEBUG_PREPROCESSOR != 0
#define LOG_OP(...)       bh_printf(__VA_ARGS__)
#else
#define LOG_OP(...)
#endif

#define PATCH_ELSE 0
#define PATCH_END  1
typedef struct BranchBlockPatch {
    struct BranchBlockPatch *next;
    uint8 patch_type;
    uint8 *code_compiled;
} BranchBlockPatch;
#endif

typedef struct BranchBlock {
    uint8 block_type;
    uint8 return_type;
    bool is_block_reachable;
    uint8 *start_addr;
    uint8 *else_addr;
    uint8 *end_addr;
    uint32 stack_cell_num;
#if WASM_ENABLE_FAST_INTERP != 0
    uint16 dynamic_offset;
    uint8 *code_compiled;
    BranchBlockPatch *patch_list;
#endif
} BranchBlock;

typedef struct WASMLoaderContext {
    /* frame ref stack */
    uint8 *frame_ref;
    uint8 *frame_ref_bottom;
    uint8 *frame_ref_boundary;
    uint32 frame_ref_size;
    uint32 stack_cell_num;
    uint32 max_stack_cell_num;

    /* frame csp stack */
    BranchBlock *frame_csp;
    BranchBlock *frame_csp_bottom;
    BranchBlock *frame_csp_boundary;
    uint32 frame_csp_size;
    uint32 csp_num;
    uint32 max_csp_num;

#if WASM_ENABLE_FAST_INTERP != 0
    /* frame offset stack */
    int16 *frame_offset;
    int16 *frame_offset_bottom;
    int16 *frame_offset_boundary;
    uint32 frame_offset_size;
    int16 dynamic_offset;
    int16 start_dynamic_offset;
    int16 max_dynamic_offset;

    /* const buffer */
    uint8 *const_buf;
    uint16 num_const;
    uint16 const_buf_size;
    uint16 const_cell_num;

    /* processed code */
    uint8 *p_code_compiled;
    uint8 *p_code_compiled_end;
    uint32 code_compiled_size;
#endif
} WASMLoaderContext;

typedef struct Const {
    WASMValue value;
    uint16 slot_index;
    uint8 value_type;
} Const;

static void*
memory_realloc(void *mem_old, uint32 size_old, uint32 size_new,
               char *error_buf, uint32 error_buf_size)
{
    uint8 *mem_new;
    bh_assert(size_new > size_old);
    if ((mem_new = wasm_runtime_malloc(size_new))) {
        bh_memcpy_s(mem_new, size_new, mem_old, size_old);
        memset(mem_new + size_old, 0, size_new - size_old);
        wasm_runtime_free(mem_old);
    }
    else {
        set_error_buf(error_buf, error_buf_size,
                    "WASM loader prepare bytecode failed: "
                    "allocate memory failed.");
    }
    return mem_new;
}

#define MEM_REALLOC(mem, size_old, size_new) do {                \
    void *mem_new = memory_realloc(mem, size_old, size_new,      \
                                   error_buf, error_buf_size);   \
    if (!mem_new)                                                \
        goto fail;                                               \
    mem = mem_new;                                               \
  } while (0)

#define CHECK_CSP_PUSH() do {                                    \
    if (ctx->frame_csp >= ctx->frame_csp_boundary) {             \
      MEM_REALLOC(ctx->frame_csp_bottom, ctx->frame_csp_size,    \
                  (uint32)(ctx->frame_csp_size                   \
                           + 8 * sizeof(BranchBlock)));          \
      ctx->frame_csp_size += (uint32)(8 * sizeof(BranchBlock));  \
      ctx->frame_csp_boundary = ctx->frame_csp_bottom +          \
                    ctx->frame_csp_size / sizeof(BranchBlock);   \
      ctx->frame_csp = ctx->frame_csp_bottom + ctx->csp_num;     \
    }                                                            \
  } while (0)

#define CHECK_CSP_POP() do {                                     \
    if (ctx->csp_num < 1) {                                      \
      set_error_buf(error_buf, error_buf_size,                   \
                  "WASM module load failed: type mismatch: "     \
                  "expect data but block stack was empty");      \
      goto fail;                                                 \
    }                                                            \
  } while (0)

#if WASM_ENABLE_FAST_INTERP != 0
static bool
check_offset_push(WASMLoaderContext *ctx,
                  char *error_buf, uint32 error_buf_size)
{
    if (ctx->frame_offset >= ctx->frame_offset_boundary) {
        MEM_REALLOC(ctx->frame_offset_bottom, ctx->frame_offset_size,
                    ctx->frame_offset_size + 16);
        ctx->frame_offset_size += 16;
        ctx->frame_offset_boundary = ctx->frame_offset_bottom +
                    ctx->frame_offset_size / sizeof(int16);
        ctx->frame_offset = ctx->frame_offset_bottom + ctx->stack_cell_num;
    }
    return true;
fail:
    return false;
}

static void free_label_patch_list(BranchBlock *frame_csp)
{
    BranchBlockPatch *label_patch = frame_csp->patch_list;
    BranchBlockPatch *next;
    while (label_patch != NULL) {
        next = label_patch->next;
        wasm_runtime_free(label_patch);
        label_patch = next;
    }
    frame_csp->patch_list = NULL;
}

static void free_all_label_patch_lists(BranchBlock *frame_csp, uint32 csp_num)
{
    BranchBlock *tmp_csp = frame_csp;

    for (uint32 i = 0; i < csp_num; i++) {
        free_label_patch_list(tmp_csp);
        tmp_csp ++;
    }
}

#endif

static bool
check_stack_push(WASMLoaderContext *ctx,
                 char *error_buf, uint32 error_buf_size)
{
    if (ctx->frame_ref >= ctx->frame_ref_boundary) {
        MEM_REALLOC(ctx->frame_ref_bottom, ctx->frame_ref_size,
                    ctx->frame_ref_size + 16);
        ctx->frame_ref_size += 16;
        ctx->frame_ref_boundary = ctx->frame_ref_bottom + ctx->frame_ref_size;
        ctx->frame_ref = ctx->frame_ref_bottom + ctx->stack_cell_num;
    }
    return true;
fail:
    return false;
}

static bool
check_stack_pop(WASMLoaderContext *ctx, uint8 type,
                char *error_buf, uint32 error_buf_size,
                const char *type_str)
{
    if (((type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32)
         && ctx->stack_cell_num < 1)
        || ((type == VALUE_TYPE_I64 || type == VALUE_TYPE_F64)
            && ctx->stack_cell_num < 2)) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "type mismatch: expect data but stack was empty");
        return false;
    }

    if ((type == VALUE_TYPE_I32 && *(ctx->frame_ref - 1) != REF_I32)
        || (type == VALUE_TYPE_F32 && *(ctx->frame_ref - 1) != REF_F32)
        || (type == VALUE_TYPE_I64
            && (*(ctx->frame_ref - 2) != REF_I64_1
            || *(ctx->frame_ref - 1) != REF_I64_2))
        || (type == VALUE_TYPE_F64
            && (*(ctx->frame_ref - 2) != REF_F64_1
            || *(ctx->frame_ref - 1) != REF_F64_2))) {
        if (error_buf != NULL)
            snprintf(error_buf, error_buf_size, "%s%s%s",
                     "WASM module load failed: type mismatch: expect ",
                     type_str, " but got other");
        return false;
    }
    return true;
}

static void wasm_loader_ctx_destroy(WASMLoaderContext *ctx)
{
    if (ctx) {
        if (ctx->frame_ref_bottom)
            wasm_runtime_free(ctx->frame_ref_bottom);
        if (ctx->frame_csp_bottom) {
#if WASM_ENABLE_FAST_INTERP != 0
            free_all_label_patch_lists(ctx->frame_csp_bottom, ctx->csp_num);
#endif
            wasm_runtime_free(ctx->frame_csp_bottom);
        }
#if WASM_ENABLE_FAST_INTERP != 0
        if (ctx->frame_offset_bottom)
            wasm_runtime_free(ctx->frame_offset_bottom);
        if (ctx->const_buf)
            wasm_runtime_free(ctx->const_buf);
#endif
        wasm_runtime_free(ctx);
    }
}

static WASMLoaderContext*
wasm_loader_ctx_init(WASMFunction *func)
{
    WASMLoaderContext *loader_ctx =
        wasm_runtime_malloc(sizeof(WASMLoaderContext));
    if (!loader_ctx)
        return false;
    memset(loader_ctx, 0, sizeof(WASMLoaderContext));

    loader_ctx->frame_ref_size = 32;
    if (!(loader_ctx->frame_ref_bottom = loader_ctx->frame_ref =
            wasm_runtime_malloc(loader_ctx->frame_ref_size)))
        goto fail;
    memset(loader_ctx->frame_ref_bottom, 0, loader_ctx->frame_ref_size);
    loader_ctx->frame_ref_boundary = loader_ctx->frame_ref_bottom +
                                        loader_ctx->frame_ref_size;

    loader_ctx->frame_csp_size = sizeof(BranchBlock) * 8;
    if (!(loader_ctx->frame_csp_bottom = loader_ctx->frame_csp =
            wasm_runtime_malloc(loader_ctx->frame_csp_size)))
        goto fail;
    memset(loader_ctx->frame_csp_bottom, 0, loader_ctx->frame_csp_size);
    loader_ctx->frame_csp_boundary = loader_ctx->frame_csp_bottom + 8;

#if WASM_ENABLE_FAST_INTERP != 0
    loader_ctx->frame_offset_size = sizeof(int16) * 32;
    if (!(loader_ctx->frame_offset_bottom = loader_ctx->frame_offset =
            wasm_runtime_malloc(loader_ctx->frame_offset_size)))
        goto fail;
    memset(loader_ctx->frame_offset_bottom, 0,
           loader_ctx->frame_offset_size);
    loader_ctx->frame_offset_boundary = loader_ctx->frame_offset_bottom + 32;

    loader_ctx->num_const = 0;
    loader_ctx->const_buf_size = sizeof(Const) * 8;
    if (!(loader_ctx->const_buf = wasm_runtime_malloc(loader_ctx->const_buf_size)))
        goto fail;
    memset(loader_ctx->const_buf, 0, loader_ctx->const_buf_size);

    loader_ctx->start_dynamic_offset = loader_ctx->dynamic_offset =
        loader_ctx->max_dynamic_offset = func->param_cell_num +
                                            func->local_cell_num;
#endif
    return loader_ctx;

fail:
    wasm_loader_ctx_destroy(loader_ctx);
    return NULL;
}

static bool
wasm_loader_push_frame_ref(WASMLoaderContext *ctx, uint8 type,
                           char *error_buf, uint32 error_buf_size)
{
    if (type == VALUE_TYPE_VOID)
        return true;

    if (!check_stack_push(ctx, error_buf, error_buf_size))
        return false;

    *ctx->frame_ref++ = type;
    ctx->stack_cell_num++;
    if (ctx->stack_cell_num > ctx->max_stack_cell_num)
        ctx->max_stack_cell_num = ctx->stack_cell_num;

    if (type == VALUE_TYPE_I32 ||  type == VALUE_TYPE_F32)
        return true;

    if (!check_stack_push(ctx, error_buf, error_buf_size))
        return false;
    *ctx->frame_ref++ = type;
    ctx->stack_cell_num++;
    if (ctx->stack_cell_num > ctx->max_stack_cell_num)
        ctx->max_stack_cell_num = ctx->stack_cell_num;
    return true;
}

static bool
wasm_loader_pop_frame_ref(WASMLoaderContext *ctx, uint8 type,
                          char *error_buf, uint32 error_buf_size)
{
    char *type_str[] = { "f64", "f32", "i64", "i32" };
    if (type == VALUE_TYPE_VOID)
        return true;

    if (!check_stack_pop(ctx, type, error_buf, error_buf_size,
                         type_str[type - VALUE_TYPE_F64]))
        return false;

    ctx->frame_ref--;
    ctx->stack_cell_num--;

    if (type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32)
        return true;

    ctx->frame_ref--;
    ctx->stack_cell_num--;
    return true;
}

static bool
wasm_loader_push_frame_csp(WASMLoaderContext *ctx, uint8 type,
                           uint8 ret_type, uint8* start_addr,
                           char *error_buf, uint32 error_buf_size)
{
    CHECK_CSP_PUSH();
    memset(ctx->frame_csp, 0, sizeof(BranchBlock));
    ctx->frame_csp->block_type = type;
    ctx->frame_csp->return_type = ret_type;
    ctx->frame_csp->start_addr = start_addr;
    ctx->frame_csp->stack_cell_num = ctx->stack_cell_num;
#if WASM_ENABLE_FAST_INTERP != 0
    ctx->frame_csp->dynamic_offset = ctx->dynamic_offset;
    ctx->frame_csp->patch_list = NULL;
#endif
    ctx->frame_csp++;
    ctx->csp_num++;
    if (ctx->csp_num > ctx->max_csp_num)
        ctx->max_csp_num = ctx->csp_num;
    return true;
fail:
    return false;
}

static bool
wasm_loader_pop_frame_csp(WASMLoaderContext *ctx,
                          char *error_buf, uint32 error_buf_size)
{
    CHECK_CSP_POP();
    ctx->frame_csp--;
    ctx->csp_num--;
    return true;
fail:
    return false;
}

static bool
wasm_loader_check_br(WASMLoaderContext *ctx, uint32 depth,
                     char *error_buf, uint32 error_buf_size)
{
    if (ctx->csp_num < depth + 1) {
      set_error_buf(error_buf, error_buf_size,
                    "WASM module load failed: type mismatch: "
                    "unexpected end of section or function");
      return false;
    }
    if ((ctx->frame_csp - (depth + 1))->block_type != BLOCK_TYPE_LOOP) {
        uint8 tmp_ret_type = (ctx->frame_csp - (depth + 1))->return_type;
        if ((tmp_ret_type == VALUE_TYPE_I32
                && (ctx->stack_cell_num < 1 || *(ctx->frame_ref - 1) != REF_I32))
            || (tmp_ret_type == VALUE_TYPE_F32
                && (ctx->stack_cell_num < 1 || *(ctx->frame_ref - 1) != REF_F32))
            || (tmp_ret_type == VALUE_TYPE_I64
                && (ctx->stack_cell_num < 2
                    || *(ctx->frame_ref - 2) != REF_I64_1
                    || *(ctx->frame_ref - 1) != REF_I64_2))
            || (tmp_ret_type == VALUE_TYPE_F64
                && (ctx->stack_cell_num < 2
                    || *(ctx->frame_ref - 2) != REF_F64_1
                    || *(ctx->frame_ref - 1) != REF_F64_2))) {
            set_error_buf(error_buf, error_buf_size,
                    "WASM module load failed: type mismatch: "
                    "expect data but stack was empty or other type");
            return false;
        }
        (ctx->frame_csp - (depth + 1))->is_block_reachable = true;
    }
    return true;
}

#if WASM_ENABLE_FAST_INTERP != 0
static bool
wasm_loader_ctx_reinit(WASMLoaderContext *ctx)
{
    if (!(ctx->p_code_compiled = wasm_runtime_malloc(ctx->code_compiled_size)))
        return false;
    memset(ctx->p_code_compiled, 0, ctx->code_compiled_size);
    ctx->p_code_compiled_end = ctx->p_code_compiled +
                                    ctx->code_compiled_size;

    /* clean up frame ref */
    memset(ctx->frame_ref_bottom, 0, ctx->frame_ref_size);
    ctx->frame_ref = ctx->frame_ref_bottom;
    ctx->stack_cell_num = 0;

    /* clean up frame csp */
    memset(ctx->frame_csp_bottom, 0, ctx->frame_csp_size);
    ctx->frame_csp = ctx->frame_csp_bottom;
    ctx->csp_num = 0;
    ctx->max_csp_num = 0;

    /* clean up frame offset */
    memset(ctx->frame_offset_bottom, 0, ctx->frame_offset_size);
    ctx->frame_offset = ctx->frame_offset_bottom;
    ctx->dynamic_offset = ctx->start_dynamic_offset;

    /* const buf is reserved */
    return true;
}

static void
wasm_loader_emit_int16(WASMLoaderContext *ctx, int16 value)
{
    if (ctx->p_code_compiled) {
        *(int16*)(ctx->p_code_compiled) = value;
        ctx->p_code_compiled += sizeof(int16);
    }
    else
        ctx->code_compiled_size += sizeof(int16);
}

static void
wasm_loader_emit_uint8(WASMLoaderContext *ctx, uint8 value)
{
    if (ctx->p_code_compiled) {
        *(ctx->p_code_compiled) = value;
        ctx->p_code_compiled += sizeof(uint8);
    }
    else
        ctx->code_compiled_size += sizeof(uint8);
}

static void
wasm_loader_emit_ptr(WASMLoaderContext *ctx, void *value)
{
    if (ctx->p_code_compiled) {
        *(uint8**)(ctx->p_code_compiled) = value;
        ctx->p_code_compiled += sizeof(void *);
    }
    else
        ctx->code_compiled_size += sizeof(void *);
}

static void
wasm_loader_emit_backspace(WASMLoaderContext *ctx, uint32 size)
{
    if (ctx->p_code_compiled) {
        ctx->p_code_compiled -= size;
    }
    else
        ctx->code_compiled_size -= size;
}

static void
wasm_loader_emit_leb(WASMLoaderContext *ctx, uint8* start, uint8* end)
{
    if (ctx->p_code_compiled) {
        bh_memcpy_s(ctx->p_code_compiled,
                    ctx->p_code_compiled_end - ctx->p_code_compiled,
                    start, end - start);
        ctx->p_code_compiled += (end - start);
    }
    else {
        ctx->code_compiled_size += (end - start);
    }

}

static bool
add_label_patch_to_list(BranchBlock *frame_csp,
                        uint8 patch_type, uint8 *p_code_compiled,
                        char *error_buf, uint32 error_buf_size)
{
    BranchBlockPatch *patch = wasm_runtime_malloc(sizeof(BranchBlockPatch));
    if (!patch) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
        return false;
    }
    patch->patch_type = patch_type;
    patch->code_compiled = p_code_compiled;
    if (!frame_csp->patch_list) {
        frame_csp->patch_list = patch;
        patch->next = NULL;
    }
    else {
        patch->next = frame_csp->patch_list;
        frame_csp->patch_list = patch;
    }
    return true;
}

static void
apply_label_patch(WASMLoaderContext *ctx, uint8 depth,
                  uint8 patch_type, uint8 *frame_ip)
{
    BranchBlock *frame_csp = ctx->frame_csp - depth;
    BranchBlockPatch *node = frame_csp->patch_list;
    BranchBlockPatch *node_prev = NULL, *node_next;

    if (!ctx->p_code_compiled)
        return;

    while (node) {
        node_next = node->next;
        if (node->patch_type == patch_type) {
            *((uint8**)node->code_compiled) = ctx->p_code_compiled;
            if (node_prev == NULL) {
                frame_csp->patch_list = node_next;
            }
            else {
                node_prev->next = node_next;
            }
            wasm_runtime_free(node);
        }
        else {
            node_prev = node;
        }
        node = node_next;
    }
}

#define emit_operand(ctx, offset) do {                              \
    wasm_loader_emit_int16(ctx, offset);                            \
    LOG_OP("%d\t", offset);                                         \
  } while (0)

#define emit_byte(ctx, byte) do {                                   \
    wasm_loader_emit_uint8(ctx, byte);                               \
    LOG_OP("%d\t", byte);                                           \
  } while (0)

#define emit_leb() do {                                             \
    wasm_loader_emit_leb(loader_ctx, p_org, p);                     \
  } while (0)

#define emit_const(value) do {                                      \
    GET_CONST_OFFSET(VALUE_TYPE_I32, value);                        \
    emit_operand(loader_ctx, operand_offset);                       \
  } while (0)

static bool
wasm_loader_emit_br_info(WASMLoaderContext *ctx, BranchBlock *frame_csp,
                         char *error_buf, uint32 error_buf_size)
{
    emit_operand(ctx, frame_csp->dynamic_offset);
    if (frame_csp->return_type == VALUE_TYPE_I32
        || frame_csp->return_type == VALUE_TYPE_F32) {
        emit_byte(ctx, 1);
        emit_operand(ctx, *(int16*)(ctx->frame_offset - 1));
    }
    else if (frame_csp->return_type == VALUE_TYPE_I64
             || frame_csp->return_type == VALUE_TYPE_F64) {
        emit_byte(ctx, 2);
        emit_operand(ctx, *(int16*)(ctx->frame_offset - 2));
    }
    else {
        emit_byte(ctx, 0);
        emit_operand(ctx, 0);
    }
    if (frame_csp->block_type == BLOCK_TYPE_LOOP) {
        wasm_loader_emit_ptr(ctx, frame_csp->code_compiled);
    }
    else {
        if (!add_label_patch_to_list(frame_csp, PATCH_END,
                                     ctx->p_code_compiled,
                                     error_buf, error_buf_size))
            return false;
        /* label address, to be patched */
        wasm_loader_emit_ptr(ctx, NULL);
    }
    return true;
}

static bool
wasm_loader_push_frame_offset(WASMLoaderContext *ctx, uint8 type,
                              bool disable_emit, int16 operand_offset,
                              char *error_buf, uint32 error_buf_size)
{
    if (type == VALUE_TYPE_VOID)
        return true;

    // only check memory overflow in first traverse
    if (ctx->p_code_compiled == NULL) {
        if (!check_offset_push(ctx, error_buf, error_buf_size))
            return false;
    }

    if (disable_emit)
        *(ctx->frame_offset)++ = operand_offset;
    else {
        emit_operand(ctx, ctx->dynamic_offset);
        *(ctx->frame_offset)++ = ctx->dynamic_offset;
        ctx->dynamic_offset++;
        if (ctx->dynamic_offset > ctx->max_dynamic_offset)
            ctx->max_dynamic_offset = ctx->dynamic_offset;
    }

    if (type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32)
        return true;

    if (ctx->p_code_compiled == NULL) {
        if (!check_offset_push(ctx, error_buf, error_buf_size))
            return false;
    }

    ctx->frame_offset++;
    ctx->dynamic_offset++;
    if (ctx->dynamic_offset > ctx->max_dynamic_offset)
        ctx->max_dynamic_offset = ctx->dynamic_offset;
    return true;
}

/* The frame_offset stack should always keep the same depth with
    frame_ref, so we don't check pop of frame_offset */
static bool
wasm_loader_pop_frame_offset(WASMLoaderContext *ctx, uint8 type,
                             char *error_buf, uint32 error_buf_size)
{
    if (type == VALUE_TYPE_VOID)
        return true;

    if (type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32) {
        ctx->frame_offset -= 1;
        if (*(ctx->frame_offset) > ctx->start_dynamic_offset)
            ctx->dynamic_offset -= 1;
    }
    else {
        ctx->frame_offset -= 2;
        if (*(ctx->frame_offset) > ctx->start_dynamic_offset)
            ctx->dynamic_offset -= 2;
    }
    emit_operand(ctx, *(ctx->frame_offset));
    return true;
}

static bool
wasm_loader_push_frame_ref_offset(WASMLoaderContext *ctx, uint8 type,
                                  bool disable_emit, int16 operand_offset,
                                  char *error_buf, uint32 error_buf_size)
{
    if (!(wasm_loader_push_frame_ref(ctx, type, error_buf, error_buf_size)))
        return false;
    if (!(wasm_loader_push_frame_offset(ctx, type, disable_emit, operand_offset,
                                        error_buf, error_buf_size)))
        return false;

    return true;
}

static bool
wasm_loader_pop_frame_ref_offset(WASMLoaderContext *ctx, uint8 type,
                                 char *error_buf, uint32 error_buf_size)
{
    if (!wasm_loader_pop_frame_ref(ctx, type, error_buf, error_buf_size))
        return false;
    if (!wasm_loader_pop_frame_offset(ctx, type, error_buf, error_buf_size))
        return false;

    return true;
}


static bool
wasm_loader_get_const_offset(WASMLoaderContext *ctx, uint8 type,
                             int64 val_int, float32 val_f32,
                             float64 val_f64, int16 *offset,
                             char *error_buf, uint32 error_buf_size)
{
    int16 operand_offset = 0;
    Const *c;
    for (c = (Const *)ctx->const_buf;
         (uint8*)c < ctx->const_buf + ctx->num_const * sizeof(Const); c ++) {
        if ((type == c->value_type)
            && ((type == VALUE_TYPE_I64 && (int64)val_int == c->value.i64)
            || (type == VALUE_TYPE_I32 && (int32)val_int == c->value.i32)
            || (type == VALUE_TYPE_F64 && (float64)val_f64 == c->value.f64)
            || (type == VALUE_TYPE_F32 && (float32)val_f32 == c->value.f32))) {
            operand_offset = c->slot_index;
            break;
        }
        if (c->value_type == VALUE_TYPE_I64
            || c->value_type == VALUE_TYPE_F64)
            operand_offset += 2;
        else
            operand_offset += 1;
    }
    if ((uint8 *)c == ctx->const_buf + ctx->num_const * sizeof(Const)) {
        if ((uint8 *)c == ctx->const_buf + ctx->const_buf_size) {
            MEM_REALLOC(ctx->const_buf,
                        ctx->const_buf_size,
                        ctx->const_buf_size + 4 * sizeof(Const));
            ctx->const_buf_size += 4 * sizeof(Const);
            c = (Const *)(ctx->const_buf + ctx->num_const * sizeof(Const));
        }
        c->value_type = type;
        switch (type) {
        case VALUE_TYPE_F64:
            c->value.f64 = (float64)val_f64;
            ctx->const_cell_num += 2;
            /* The const buf will be reversed, we use the second cell */
            /* of the i64/f64 const so the finnal offset is corrent */
            operand_offset ++;
            break;
        case VALUE_TYPE_I64:
            c->value.i64 = (int64)val_int;
            ctx->const_cell_num += 2;
            operand_offset ++;
            break;
        case VALUE_TYPE_F32:
            c->value.f32 = (float32)val_f32;
            ctx->const_cell_num ++;
            break;
        case VALUE_TYPE_I32:
            c->value.i32 = (int32)val_int;
            ctx->const_cell_num ++;
            break;
        default:
            break;
        }
        c->slot_index = operand_offset;
        ctx->num_const ++;
        LOG_OP("#### new const [%d]: %ld\n",
            ctx->num_const, (int64)c->value.i64);
    }
    /* use negetive index for const */
    operand_offset = -(operand_offset + 1);
    *offset = operand_offset;
    return true;
fail:
    return false;
}

/*
    PUSH(POP)_XXX = push(pop) frame_ref + push(pop) frame_offset
    -- Mostly used for the binary / compare operation
    PUSH(POP)_OFFSET_TYPE only push(pop) the frame_offset stack
    -- Mostly used in block / control instructions

    The POP will always emit the offset on the top of the frame_offset stack
    PUSH can be used in two ways:
    1. directly PUSH:
            PUSH_XXX();
        will allocate a dynamic space and emit
    2. silent PUSH:
            operand_offset = xxx; disable_emit = true;
            PUSH_XXX();
        only push the frame_offset stack, no emit
*/
#define PUSH_I32() do {                                                 \
    if (!wasm_loader_push_frame_ref_offset(loader_ctx, VALUE_TYPE_I32,  \
                                           disable_emit, operand_offset,\
                                           error_buf, error_buf_size))  \
        goto fail;                                                      \
  } while (0)

#define PUSH_F32() do {                                                 \
    if (!wasm_loader_push_frame_ref_offset(loader_ctx, VALUE_TYPE_F32,  \
                                           disable_emit, operand_offset,\
                                           error_buf, error_buf_size))  \
        goto fail;                                                      \
  } while (0)

#define PUSH_I64() do {                                                 \
    if (!wasm_loader_push_frame_ref_offset(loader_ctx, VALUE_TYPE_I64,  \
                                           disable_emit, operand_offset,\
                                           error_buf, error_buf_size))  \
        goto fail;                                                      \
  } while (0)

#define PUSH_F64() do {                                                 \
    if (!wasm_loader_push_frame_ref_offset(loader_ctx, VALUE_TYPE_F64,  \
                                           disable_emit, operand_offset,\
                                           error_buf, error_buf_size))  \
        goto fail;                                                      \
  } while (0)

#define POP_I32() do {                                                  \
    if (!wasm_loader_pop_frame_ref_offset(loader_ctx, VALUE_TYPE_I32,   \
                                          error_buf, error_buf_size))   \
        goto fail;                                                      \
  } while (0)

#define POP_F32() do {                                                  \
    if (!wasm_loader_pop_frame_ref_offset(loader_ctx, VALUE_TYPE_F32,   \
                                          error_buf, error_buf_size))   \
        goto fail;                                                      \
  } while (0)

#define POP_I64() do {                                                  \
    if (!wasm_loader_pop_frame_ref_offset(loader_ctx, VALUE_TYPE_I64,   \
                                          error_buf, error_buf_size))   \
        goto fail;                                                      \
  } while (0)

#define POP_F64() do {                                                  \
    if (!wasm_loader_pop_frame_ref_offset(loader_ctx, VALUE_TYPE_F64,   \
                                          error_buf, error_buf_size))   \
        goto fail;                                                      \
  } while (0)

#define PUSH_OFFSET_TYPE(type) do {                                     \
    if (!(wasm_loader_push_frame_offset(loader_ctx, type,               \
                                        disable_emit, operand_offset,   \
                                        error_buf, error_buf_size)))    \
        goto fail;                                                      \
  } while (0)

#define POP_OFFSET_TYPE(type) do {                                      \
    if (!(wasm_loader_pop_frame_offset(loader_ctx, type,                \
                                       error_buf, error_buf_size)))     \
        goto fail;                                                      \
  } while (0)

#else /* WASM_ENABLE_FAST_INTERP */

#define PUSH_I32() do {                                             \
    if (!(wasm_loader_push_frame_ref(loader_ctx, VALUE_TYPE_I32,    \
                                     error_buf, error_buf_size)))   \
        goto fail;                                                  \
  } while (0)

#define PUSH_F32() do {                                             \
    if (!(wasm_loader_push_frame_ref(loader_ctx, VALUE_TYPE_F32,    \
                                     error_buf, error_buf_size)))   \
        goto fail;                                                  \
  } while (0)

#define PUSH_I64() do {                                             \
    if (!(wasm_loader_push_frame_ref(loader_ctx, VALUE_TYPE_I64,    \
                                     error_buf, error_buf_size)))   \
        goto fail;                                                  \
  } while (0)

#define PUSH_F64() do {                                             \
    if (!(wasm_loader_push_frame_ref(loader_ctx, VALUE_TYPE_F64,    \
                                     error_buf, error_buf_size)))   \
        goto fail;                                                  \
  } while (0)

#define POP_I32() do {                                              \
    if (!(wasm_loader_pop_frame_ref(loader_ctx, VALUE_TYPE_I32,     \
                                    error_buf, error_buf_size)))    \
        goto fail;                                                  \
  } while (0)

#define POP_F32() do {                                              \
    if (!(wasm_loader_pop_frame_ref(loader_ctx, VALUE_TYPE_F32,     \
                                    error_buf, error_buf_size)))    \
        goto fail;                                                  \
  } while (0)

#define POP_I64() do {                                              \
    if (!(wasm_loader_pop_frame_ref(loader_ctx, VALUE_TYPE_I64,     \
                                    error_buf, error_buf_size)))    \
        goto fail;                                                  \
  } while (0)

#define POP_F64() do {                                              \
    if (!(wasm_loader_pop_frame_ref(loader_ctx, VALUE_TYPE_F64,     \
                                    error_buf, error_buf_size)))    \
        goto fail;                                                  \
  } while (0)

#endif /* WASM_ENABLE_FAST_INTERP */

#define PUSH_TYPE(type) do {                                        \
    if (!(wasm_loader_push_frame_ref(loader_ctx, type,              \
                                     error_buf, error_buf_size)))   \
        goto fail;                                                  \
  } while (0)

#define POP_TYPE(type) do {                                         \
    if (!(wasm_loader_pop_frame_ref(loader_ctx, type,               \
                                    error_buf, error_buf_size)))    \
        goto fail;                                                  \
  } while (0)

#define PUSH_CSP(type, ret_type, _start_addr) do {              \
    if (!wasm_loader_push_frame_csp(loader_ctx, type, ret_type, \
                                    _start_addr, error_buf,     \
                                    error_buf_size))            \
        goto fail;                                              \
  } while (0)

#define POP_CSP() do {                                          \
    if (!wasm_loader_pop_frame_csp(loader_ctx,                  \
                                   error_buf, error_buf_size))  \
        goto fail;                                              \
  } while (0)


#define GET_LOCAL_INDEX_TYPE_AND_OFFSET() do {      \
    read_leb_uint32(p, p_end, local_idx);           \
    if (local_idx >= param_count + local_count) {   \
      set_error_buf(error_buf, error_buf_size,      \
                    "WASM module load failed: "     \
                    "local index out of range");    \
      goto fail;                                    \
    }                                               \
    local_type = local_idx < param_count            \
        ? param_types[local_idx]                    \
        : local_types[local_idx - param_count];     \
    local_offset = local_offsets[local_idx];        \
  } while (0)

#define CHECK_BR(depth) do {                                        \
    if (!wasm_loader_check_br(loader_ctx, depth,                    \
                              error_buf, error_buf_size))           \
        goto fail;                                                  \
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

#define CHECK_MEMORY() do {                                         \
    if (!check_memory(module, error_buf, error_buf_size))           \
      goto fail;                                                    \
  } while (0)

#if WASM_ENABLE_FAST_INTERP != 0
#if WASM_ENABLE_ABS_LABEL_ADDR != 0

#define emit_label(opcode) do {                                     \
    wasm_loader_emit_ptr(loader_ctx, handle_table[opcode]);         \
    LOG_OP("\nemit_op [%02x]\t", opcode);                           \
  } while (0)

#define skip_label() do {                                           \
    wasm_loader_emit_backspace(loader_ctx, sizeof(void *));         \
    LOG_OP("\ndelete last op\n");                                   \
  } while (0)

#else

#define emit_label(opcode) do {                                     \
    int32 offset = (int32)(handle_table[opcode] - handle_table[0]); \
    if (!(offset >= INT16_MIN && offset < INT16_MAX)) {             \
        set_error_buf(error_buf, error_buf_size,                    \
                      "WASM module load failed: "                   \
                      "pre-compiled label offset out of range");    \
        goto fail;                                                  \
    }                                                               \
    wasm_loader_emit_int16(loader_ctx, offset);                     \
    LOG_OP("\nemit_op [%02x]\t", opcode);                           \
  } while (0)

// drop local.get / const / block / loop / end
#define skip_label() do {                                           \
    wasm_loader_emit_backspace(loader_ctx, sizeof(int16));          \
    LOG_OP("\ndelete last op\n");                                   \
  } while (0)

#endif /* WASM_ENABLE_ABS_LABEL_ADDR */

#define emit_empty_label_addr_and_frame_ip(type) do {               \
    if (!add_label_patch_to_list(loader_ctx->frame_csp - 1, type,   \
                                 loader_ctx->p_code_compiled,       \
                                 error_buf, error_buf_size))        \
        goto fail;                                                  \
    /* label address, to be patched */                              \
    wasm_loader_emit_ptr(loader_ctx, NULL);                         \
  } while (0)

#define emit_br_info(frame_csp) do {                                \
    if (!wasm_loader_emit_br_info(loader_ctx, frame_csp,            \
                                  error_buf, error_buf_size))       \
        goto fail;                                                  \
  } while (0)

#define LAST_OP_OUTPUT_I32() (last_op >= WASM_OP_I32_EQZ                \
                                && last_op <= WASM_OP_I32_ROTR)         \
                            || (last_op == WASM_OP_I32_LOAD             \
                                || last_op == WASM_OP_F32_LOAD)         \
                            || (last_op >= WASM_OP_I32_LOAD8_S          \
                                && last_op <= WASM_OP_I32_LOAD16_U)     \
                            || (last_op >= WASM_OP_F32_ABS              \
                                && last_op <= WASM_OP_F32_COPYSIGN)     \
                            || (last_op >= WASM_OP_I32_WRAP_I64         \
                                && last_op <= WASM_OP_I32_TRUNC_U_F64)  \
                            || (last_op >= WASM_OP_F32_CONVERT_S_I32    \
                                && last_op <= WASM_OP_F32_DEMOTE_F64)   \
                            || (last_op == WASM_OP_I32_REINTERPRET_F32) \
                            || (last_op == WASM_OP_F32_REINTERPRET_I32) \
                            || (last_op == EXT_OP_COPY_STACK_TOP)

#define LAST_OP_OUTPUT_I64() (last_op >= WASM_OP_I64_CLZ                \
                                && last_op <= WASM_OP_I64_ROTR)         \
                            || (last_op >= WASM_OP_F64_ABS              \
                                && last_op <= WASM_OP_F64_COPYSIGN)     \
                            || (last_op == WASM_OP_I64_LOAD             \
                                || last_op == WASM_OP_F64_LOAD)         \
                            || (last_op >= WASM_OP_I64_LOAD8_S          \
                                && last_op <= WASM_OP_I64_LOAD32_U)     \
                            || (last_op >= WASM_OP_I64_EXTEND_S_I32     \
                                && last_op <= WASM_OP_I64_TRUNC_U_F64)  \
                            || (last_op >= WASM_OP_F64_CONVERT_S_I32    \
                                && last_op <= WASM_OP_F64_PROMOTE_F32)  \
                            || (last_op == WASM_OP_I64_REINTERPRET_F64) \
                            || (last_op == WASM_OP_F64_REINTERPRET_I64) \
                            || (last_op == EXT_OP_COPY_STACK_TOP_I64)

#define GET_CONST_OFFSET(type, val) do {                                \
    if (!(wasm_loader_get_const_offset(loader_ctx, type,                \
                                       val, 0, 0, &operand_offset,      \
                                       error_buf, error_buf_size)))     \
        goto fail;                                                      \
  } while (0)

#define GET_CONST_F32_OFFSET(type, fval) do {                           \
    if (!(wasm_loader_get_const_offset(loader_ctx, type,                \
                                       0, fval, 0, &operand_offset,     \
                                       error_buf, error_buf_size)))     \
        goto fail;                                                      \
  } while (0)

#define GET_CONST_F64_OFFSET(type, fval) do {                           \
    if (!(wasm_loader_get_const_offset(loader_ctx, type,                \
                                       0, 0, fval, &operand_offset,     \
                                       error_buf, error_buf_size)))     \
        goto fail;                                                      \
  } while (0)

#endif /* WASM_ENABLE_FAST_INTERP */

static bool
wasm_loader_prepare_bytecode(WASMModule *module, WASMFunction *func,
                             BlockAddr *block_addr_cache,
                             char *error_buf, uint32 error_buf_size)
{
    uint8 *p = func->code, *p_end = func->code + func->code_size, *p_org;
    uint32 param_count, local_count, global_count;
    uint8 *param_types, ret_type, *local_types, local_type, global_type;
    uint16 *local_offsets, local_offset;
    uint32 count, i, local_idx, global_idx, depth, u32, align, mem_offset;
    uint32 cache_index, item_index;
    int32 i32, i32_const = 0;
    int64 i64;
    uint8 opcode, u8, block_return_type;
    bool return_value = false, is_i32_const = false;
    BlockAddr *cache_items;
    WASMLoaderContext *loader_ctx;
#if WASM_ENABLE_FAST_INTERP != 0
    uint8 *func_const_end, *func_const;
    int16 operand_offset;
    uint8 last_op = 0;
    bool disable_emit;
    float32 f32;
    float64 f64;

    LOG_OP("\nProcessing func | [%d] params | [%d] locals | [%d] return\n",
        func->param_cell_num,
        func->local_cell_num,
        func->ret_cell_num);
#endif

    global_count = module->import_global_count + module->global_count;

    param_count = func->func_type->param_count;
    param_types = func->func_type->types;
    ret_type = func->func_type->result_count
               ? param_types[param_count] : VALUE_TYPE_VOID;

    local_count = func->local_count;
    local_types = func->local_types;
    local_offsets = func->local_offsets;

    if (!(loader_ctx = wasm_loader_ctx_init(func))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
        goto fail;
    }

#if WASM_ENABLE_FAST_INTERP != 0
re_scan:
    if (loader_ctx->code_compiled_size > 0) {
        if (!wasm_loader_ctx_reinit(loader_ctx)) {
            set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
            goto fail;
        }
        p = func->code;
        func->code_compiled = loader_ctx->p_code_compiled;
    }
#endif

    PUSH_CSP(BLOCK_TYPE_FUNCTION, ret_type, p);
    (loader_ctx->frame_csp - 1)->is_block_reachable = true;

    while (p < p_end) {
        opcode = *p++;
#if WASM_ENABLE_FAST_INTERP != 0
        p_org = p;
        disable_emit = false;
        emit_label(opcode);
#endif

        switch (opcode) {
            case WASM_OP_UNREACHABLE:
                goto handle_next_reachable_block;

            case WASM_OP_NOP:
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
#endif
                break;

            case WASM_OP_BLOCK:
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_BLOCK, block_return_type, p);
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
#endif
                break;

            case WASM_OP_LOOP:
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_LOOP, block_return_type, p);
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                (loader_ctx->frame_csp - 1)->code_compiled =
                    loader_ctx->p_code_compiled;
#endif
                break;

            case WASM_OP_IF:
                POP_I32();
                /* 0x40/0x7F/0x7E/0x7D/0x7C */
                block_return_type = read_uint8(p);
                PUSH_CSP(BLOCK_TYPE_IF, block_return_type, p);
#if WASM_ENABLE_FAST_INTERP != 0
                emit_empty_label_addr_and_frame_ip(PATCH_ELSE);
                emit_empty_label_addr_and_frame_ip(PATCH_END);
#endif
                if (!is_i32_const)
                    (loader_ctx->frame_csp - 1)->is_block_reachable = true;
                else {
                    if (!i32_const) {
                        cache_index = ((uintptr_t)(loader_ctx->frame_csp - 1)->start_addr)
                                      & (uintptr_t)(BLOCK_ADDR_CACHE_SIZE - 1);
                        cache_items = block_addr_cache +
                                      BLOCK_ADDR_CONFLICT_SIZE * cache_index;
                        for (item_index = 0; item_index < BLOCK_ADDR_CONFLICT_SIZE;
                             item_index++) {
                            if (cache_items[item_index].start_addr ==
                                                (loader_ctx->frame_csp - 1)->start_addr) {
                                (loader_ctx->frame_csp - 1)->else_addr =
                                            cache_items[item_index].else_addr;
                                (loader_ctx->frame_csp - 1)->end_addr =
                                            cache_items[item_index].end_addr;
                                break;
                            }
                        }
                        if (item_index == BLOCK_ADDR_CONFLICT_SIZE
                            && !wasm_loader_find_block_addr(block_addr_cache,
                                                           (loader_ctx->frame_csp - 1)->start_addr,
                                                           p_end,
                                                           (loader_ctx->frame_csp - 1)->block_type,
                                                           &(loader_ctx->frame_csp - 1)->else_addr,
                                                           &(loader_ctx->frame_csp - 1)->end_addr,
                                                           error_buf, error_buf_size))
                            goto fail;

                        if ((loader_ctx->frame_csp - 1)->else_addr)
                            p = (loader_ctx->frame_csp - 1)->else_addr;
                        else
                            p = (loader_ctx->frame_csp - 1)->end_addr;

                        is_i32_const = false;
                        continue;
                    }
                }
                break;

            case WASM_OP_ELSE:
                if (loader_ctx->csp_num < 2
                    || (loader_ctx->frame_csp - 1)->block_type != BLOCK_TYPE_IF) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode else found without matched opcode if");
                    goto fail;
                }

                (loader_ctx->frame_csp - 1)->else_addr = p - 1;
                loader_ctx->stack_cell_num = (loader_ctx->frame_csp - 1)->stack_cell_num;
                loader_ctx->frame_ref = loader_ctx->frame_ref_bottom +
                                            loader_ctx->stack_cell_num;
#if WASM_ENABLE_FAST_INTERP != 0
                // if the result of if branch is in local or const area, add a copy op
                if ((loader_ctx->frame_csp - 1)->return_type != VALUE_TYPE_VOID) {
                    uint8 return_cells;
                    if ((loader_ctx->frame_csp - 1)->return_type == VALUE_TYPE_I32
                        || (loader_ctx->frame_csp - 1)->return_type == VALUE_TYPE_F32)
                        return_cells = 1;
                    else
                        return_cells = 2;
                    if ((loader_ctx->frame_csp - 1)->dynamic_offset !=
                            *(loader_ctx->frame_offset - return_cells)) {
                        skip_label();
                        if (return_cells == 1)
                            emit_label(EXT_OP_COPY_STACK_TOP);
                        else
                            emit_label(EXT_OP_COPY_STACK_TOP_I64);
                        emit_operand(loader_ctx, *(loader_ctx->frame_offset - return_cells));
                        emit_operand(loader_ctx, (loader_ctx->frame_csp - 1)->dynamic_offset);
                        *(loader_ctx->frame_offset - return_cells) =
                            loader_ctx->frame_csp->dynamic_offset;
                        emit_label(opcode);
                    }
                }
                loader_ctx->frame_offset = loader_ctx->frame_offset_bottom +
                                                loader_ctx->stack_cell_num;
                emit_empty_label_addr_and_frame_ip(PATCH_END);
                apply_label_patch(loader_ctx, 1, PATCH_ELSE, p);
#endif
                break;

            case WASM_OP_END:
            {
                POP_CSP();

                POP_TYPE(loader_ctx->frame_csp->return_type);
                PUSH_TYPE(loader_ctx->frame_csp->return_type);

#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                // copy the result to the block return address
                if (loader_ctx->frame_csp->return_type != VALUE_TYPE_VOID) {
                    uint8 return_cells;
                    if (loader_ctx->frame_csp->return_type == VALUE_TYPE_I32
                        || loader_ctx->frame_csp->return_type == VALUE_TYPE_F32)
                        return_cells = 1;
                    else
                        return_cells = 2;
                    if (loader_ctx->frame_csp->dynamic_offset !=
                            *(loader_ctx->frame_offset - return_cells)) {
                        if (return_cells == 1)
                            emit_label(EXT_OP_COPY_STACK_TOP);
                        else
                            emit_label(EXT_OP_COPY_STACK_TOP_I64);
                        emit_operand(loader_ctx, *(loader_ctx->frame_offset - return_cells));
                        emit_operand(loader_ctx, loader_ctx->frame_csp->dynamic_offset);
                    }
                    // the frame_offset stack top should be the return address of the block
                    loader_ctx->frame_offset -= return_cells;
                    loader_ctx->dynamic_offset = loader_ctx->frame_csp->dynamic_offset;
                    PUSH_OFFSET_TYPE(loader_ctx->frame_csp->return_type);
                    wasm_loader_emit_backspace(loader_ctx, sizeof(int16));
                }

                apply_label_patch(loader_ctx, 0, PATCH_END, p);
                free_label_patch_list(loader_ctx->frame_csp);
                if (loader_ctx->frame_csp->block_type == BLOCK_TYPE_FUNCTION) {
                    emit_label(WASM_OP_RETURN);
                    POP_OFFSET_TYPE(loader_ctx->frame_csp->return_type);
                }
#endif
                if (loader_ctx->csp_num > 0) {
                    loader_ctx->frame_csp->end_addr = p - 1;
                }
                else {
                    /* end of function block, function will return,
                       ignore the following bytecodes */
                    p = p_end;

                    is_i32_const = false;
                    continue;
                }
                break;
            }

            case WASM_OP_BR:
            {
#if WASM_ENABLE_FAST_INTERP != 0
                BranchBlock *frame_csp_tmp;
#endif
                read_leb_uint32(p, p_end, depth);
                CHECK_BR(depth);

#if WASM_ENABLE_FAST_INTERP != 0
                frame_csp_tmp = loader_ctx->frame_csp - depth - 1;
                emit_br_info(frame_csp_tmp);
#endif

handle_next_reachable_block:
                for (i = 1; i <= loader_ctx->csp_num; i++)
                    if ((loader_ctx->frame_csp - i)->is_block_reachable)
                        break;

                block_return_type = (loader_ctx->frame_csp - i)->return_type;

                cache_index = ((uintptr_t)(loader_ctx->frame_csp - i)->start_addr)
                              & (uintptr_t)(BLOCK_ADDR_CACHE_SIZE - 1);
                cache_items = block_addr_cache + BLOCK_ADDR_CONFLICT_SIZE * cache_index;
                for (item_index = 0; item_index < BLOCK_ADDR_CONFLICT_SIZE; item_index++) {
                    if (cache_items[item_index].start_addr == (loader_ctx->frame_csp - i)->start_addr) {
                        (loader_ctx->frame_csp - i)->else_addr = cache_items[item_index].else_addr;
                        (loader_ctx->frame_csp - i)->end_addr = cache_items[item_index].end_addr;
                        break;
                    }
                }
                if(item_index == BLOCK_ADDR_CONFLICT_SIZE
                   && !wasm_loader_find_block_addr(block_addr_cache,
                                                   (loader_ctx->frame_csp - i)->start_addr,
                                                   p_end,
                                                   (loader_ctx->frame_csp - i)->block_type,
                                                   &(loader_ctx->frame_csp - i)->else_addr,
                                                   &(loader_ctx->frame_csp - i)->end_addr,
                                                   error_buf, error_buf_size))
                    goto fail;

                loader_ctx->stack_cell_num = (loader_ctx->frame_csp - i)->stack_cell_num;
                loader_ctx->frame_ref = loader_ctx->frame_ref_bottom +
                                            loader_ctx->stack_cell_num;
                loader_ctx->csp_num -= i - 1;
                loader_ctx->frame_csp -= i - 1;

                if ((loader_ctx->frame_csp - 1)->block_type == BLOCK_TYPE_IF
                        && (loader_ctx->frame_csp - 1)->else_addr != NULL
                        && p <= (loader_ctx->frame_csp - 1)->else_addr)
                    p = (loader_ctx->frame_csp - 1)->else_addr;
                else {
                    p = (loader_ctx->frame_csp - 1)->end_addr;
                    PUSH_TYPE(block_return_type);
#if WASM_ENABLE_FAST_INTERP != 0
                    loader_ctx->frame_offset = loader_ctx->frame_offset_bottom +
                                                    loader_ctx->stack_cell_num;
#endif
                }

                is_i32_const = false;
                continue;
            }

            case WASM_OP_BR_IF:
            {
#if WASM_ENABLE_FAST_INTERP != 0
                BranchBlock *frame_csp_tmp;
#endif
                read_leb_uint32(p, p_end, depth);
                POP_I32();
                CHECK_BR(depth);
#if WASM_ENABLE_FAST_INTERP != 0
                frame_csp_tmp = loader_ctx->frame_csp - depth - 1;
                emit_br_info(frame_csp_tmp);
#endif
                if (!is_i32_const)
                    (loader_ctx->frame_csp - (depth + 1))->is_block_reachable = true;
                else {
                    if (i32_const)
                        goto handle_next_reachable_block;
                }
                break;
            }

            case WASM_OP_BR_TABLE:
            {
#if WASM_ENABLE_FAST_INTERP != 0
                BranchBlock *frame_csp_tmp;
#endif

                read_leb_uint32(p, p_end, count);
#if WASM_ENABLE_FAST_INTERP != 0
                emit_const(count);
#endif
                POP_I32();

                /* TODO: check the const */
                for (i = 0; i <= count; i++) {
                    read_leb_uint32(p, p_end, depth);
                    CHECK_BR(depth);
#if WASM_ENABLE_FAST_INTERP != 0
                    frame_csp_tmp = loader_ctx->frame_csp - depth - 1;
                    emit_br_info(frame_csp_tmp);
#endif
                }

                goto handle_next_reachable_block;
            }

            case WASM_OP_RETURN:
            {
                POP_TYPE(ret_type);
                PUSH_TYPE(ret_type);

                cache_index = ((uintptr_t)(loader_ctx->frame_csp - 1)->start_addr)
                              & (uintptr_t)(BLOCK_ADDR_CACHE_SIZE - 1);
                cache_items = block_addr_cache + BLOCK_ADDR_CONFLICT_SIZE * cache_index;
                for (item_index = 0; item_index < BLOCK_ADDR_CONFLICT_SIZE;
                     item_index++) {
                    if (cache_items[item_index].start_addr ==
                                                      (loader_ctx->frame_csp - 1)->start_addr) {
                        (loader_ctx->frame_csp - 1)->else_addr = cache_items[item_index].else_addr;
                        (loader_ctx->frame_csp - 1)->end_addr = cache_items[item_index].end_addr;
                      break;
                    }
                }
                if(item_index == BLOCK_ADDR_CONFLICT_SIZE
                   && !wasm_loader_find_block_addr(block_addr_cache,
                                                   (loader_ctx->frame_csp - 1)->start_addr,
                                                   p_end,
                                                   (loader_ctx->frame_csp - 1)->block_type,
                                                   &(loader_ctx->frame_csp - 1)->else_addr,
                                                   &(loader_ctx->frame_csp - 1)->end_addr,
                                                   error_buf, error_buf_size))
                    goto fail;

                loader_ctx->stack_cell_num = (loader_ctx->frame_csp - 1)->stack_cell_num;
                loader_ctx->frame_ref = loader_ctx->frame_ref_bottom + loader_ctx->stack_cell_num;

                if ((loader_ctx->frame_csp - 1)->block_type == BLOCK_TYPE_IF
                    && p <= (loader_ctx->frame_csp - 1)->else_addr) {
                    p = (loader_ctx->frame_csp - 1)->else_addr;
                }
                else {
                    p = (loader_ctx->frame_csp - 1)->end_addr;
                    PUSH_TYPE((loader_ctx->frame_csp - 1)->return_type);
                }

#if WASM_ENABLE_FAST_INTERP != 0
                // emit the offset after return opcode
                POP_OFFSET_TYPE(ret_type);
                loader_ctx->frame_offset = loader_ctx->frame_offset_bottom +
                                                loader_ctx->stack_cell_num;
#endif

                is_i32_const = false;
                continue;
            }

            case WASM_OP_CALL:
            {
                WASMType *func_type;
                uint32 func_idx;
                int32 idx;

                read_leb_uint32(p, p_end, func_idx);
#if WASM_ENABLE_FAST_INTERP != 0
                // we need to emit func_idx before arguments
                emit_const(func_idx);
#endif

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
                    for (idx = (int32)(func_type->param_count - 1); idx >= 0; idx--) {
                        POP_TYPE(func_type->types[idx]);
#if WASM_ENABLE_FAST_INTERP != 0
                        POP_OFFSET_TYPE(func_type->types[idx]);
#endif
                    }
                }

                if (func_type->result_count) {
                    PUSH_TYPE(func_type->types[func_type->param_count]);
#if WASM_ENABLE_FAST_INTERP != 0
                    PUSH_OFFSET_TYPE(func_type->types[func_type->param_count]);
#endif
                }

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
#if WASM_ENABLE_FAST_INTERP != 0
                // we need to emit func_idx before arguments
                emit_const(type_idx);
#endif

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
                    for (idx = (int32)(func_type->param_count - 1); idx >= 0; idx--) {
                        POP_TYPE(func_type->types[idx]);
#if WASM_ENABLE_FAST_INTERP != 0
                        POP_OFFSET_TYPE(func_type->types[idx]);
#endif
                    }
                }

                if (func_type->result_count > 0) {
                    PUSH_TYPE(func_type->types[func_type->param_count]);
#if WASM_ENABLE_FAST_INTERP != 0
                    PUSH_OFFSET_TYPE(func_type->types[func_type->param_count]);
#endif
                }

                func->has_op_func_call = true;
                break;
            }

            case WASM_OP_DROP:
            {
                if (loader_ctx->stack_cell_num <= 0) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode drop was found but stack was empty");
                    goto fail;
                }

                if (*(loader_ctx->frame_ref - 1) == REF_I32
                    || *(loader_ctx->frame_ref - 1) == REF_F32) {
                    loader_ctx->frame_ref--;
                    loader_ctx->stack_cell_num--;
#if WASM_ENABLE_FAST_INTERP != 0
                    skip_label();
                    loader_ctx->frame_offset--;
                    if (*(loader_ctx->frame_offset) >
                            loader_ctx->start_dynamic_offset)
                        loader_ctx->dynamic_offset --;
#endif
                }
                else {
                    if (loader_ctx->stack_cell_num <= 1) {
                        set_error_buf(error_buf, error_buf_size,
                                      "WASM loader prepare bytecode failed: "
                                      "opcode drop was found but stack was empty");
                        goto fail;
                    }
                    loader_ctx->frame_ref -= 2;
                    loader_ctx->stack_cell_num -= 2;
#if WASM_ENABLE_FAST_INTERP == 0
                    *(p - 1) = WASM_OP_DROP_64;
#endif
#if WASM_ENABLE_FAST_INTERP != 0
                    skip_label();
                    loader_ctx->frame_offset -= 2;
                    if (*(loader_ctx->frame_offset) >
                            loader_ctx->start_dynamic_offset)
                        loader_ctx->dynamic_offset -= 2;
#endif
                }
                break;
            }

            case WASM_OP_SELECT:
            {
                uint8 ref_type;

                POP_I32();

                if (loader_ctx->stack_cell_num <= 0) {
                    set_error_buf(error_buf, error_buf_size,
                                  "WASM loader prepare bytecode failed: "
                                  "opcode select was found but stack was empty");
                    goto fail;
                }

                switch (*(loader_ctx->frame_ref - 1)) {
                    case REF_I32:
                    case REF_F32:
                        break;
                    case REF_I64_2:
                    case REF_F64_2:
#if WASM_ENABLE_FAST_INTERP == 0
                        *(p - 1) = WASM_OP_SELECT_64;
#endif
#if WASM_ENABLE_FAST_INTERP != 0
                        if (loader_ctx->p_code_compiled) {
#if WASM_ENABLE_ABS_LABEL_ADDR != 0
                            *(void**)(loader_ctx->p_code_compiled - 10) =
                                handle_table[WASM_OP_SELECT_64];
#else
                            *((int16*)loader_ctx->p_code_compiled - 2) = (int16)
                                (handle_table[WASM_OP_SELECT_64] - handle_table[0]);
#endif
                        }
#endif
                        break;
                }

                ref_type = *(loader_ctx->frame_ref - 1);
                POP_TYPE(ref_type);
                POP_TYPE(ref_type);
                PUSH_TYPE(ref_type);
#if WASM_ENABLE_FAST_INTERP != 0
                POP_OFFSET_TYPE(ref_type);
                POP_OFFSET_TYPE(ref_type);
                PUSH_OFFSET_TYPE(ref_type);
#endif
                break;
            }

            case WASM_OP_GET_LOCAL:
            {
                p_org = p - 1;
                GET_LOCAL_INDEX_TYPE_AND_OFFSET();
                PUSH_TYPE(local_type);

#if WASM_ENABLE_FAST_INTERP != 0
                /* Get Local is optimized out */
                skip_label();
                disable_emit = true;
                operand_offset = local_offset;
                PUSH_OFFSET_TYPE(local_type);
#else
#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_JIT == 0)
                if (local_offset < 0x80) {
                    *p_org++ = EXT_OP_GET_LOCAL_FAST;
                    if (local_type == VALUE_TYPE_I32
                        || local_type == VALUE_TYPE_F32)
                        *p_org++ = (uint8)local_offset;
                    else
                        *p_org++ = (uint8)(local_offset | 0x80);
                    while (p_org < p)
                        *p_org++ = WASM_OP_NOP;
                }
#endif
#endif
                break;
            }

            case WASM_OP_SET_LOCAL:
            {
                p_org = p - 1;
                GET_LOCAL_INDEX_TYPE_AND_OFFSET();
                POP_TYPE(local_type);

#if WASM_ENABLE_FAST_INTERP != 0
                if (local_offset < 256) {
                    skip_label();
                    if (LAST_OP_OUTPUT_I32()) {
                        if (loader_ctx->p_code_compiled)
                            *(int16*)(loader_ctx->p_code_compiled - 2) = local_offset;
                        loader_ctx->frame_offset --;
                        loader_ctx->dynamic_offset --;
                    }
                    else if (LAST_OP_OUTPUT_I64()) {
                        if (loader_ctx->p_code_compiled)
                            *(int16*)(loader_ctx->p_code_compiled - 2) = local_offset;
                        loader_ctx->frame_offset -= 2;
                        loader_ctx->dynamic_offset -= 2;
                    }
                    else {
                        if (local_type == VALUE_TYPE_I32
                            || local_type == VALUE_TYPE_F32) {
                            emit_label(EXT_OP_SET_LOCAL_FAST);
                            emit_byte(loader_ctx, local_offset);
                        }
                        else {
                            emit_label(EXT_OP_SET_LOCAL_FAST_I64);
                            emit_byte(loader_ctx, local_offset);
                        }
                        POP_OFFSET_TYPE(local_type);
                    }
                }
                else {   /* local index larger than 255, reserve leb */
                    p_org ++;
                    emit_leb();
                    POP_OFFSET_TYPE(local_type);
                }
#else
#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_JIT == 0)
                if (local_offset < 0x80) {
                    *p_org++ = EXT_OP_SET_LOCAL_FAST;
                    if (local_type == VALUE_TYPE_I32
                        || local_type == VALUE_TYPE_F32)
                        *p_org++ = (uint8)local_offset;
                    else
                        *p_org++ = (uint8)(local_offset | 0x80);
                    while (p_org < p)
                        *p_org++ = WASM_OP_NOP;
                }
#endif
#endif
                break;
            }

            case WASM_OP_TEE_LOCAL:
            {
                p_org = p - 1;
                GET_LOCAL_INDEX_TYPE_AND_OFFSET();
                POP_TYPE(local_type);
                PUSH_TYPE(local_type);

#if WASM_ENABLE_FAST_INTERP != 0
                if (local_offset < 256) {
                    skip_label();
                    if (local_type == VALUE_TYPE_I32
                        || local_type == VALUE_TYPE_F32) {
                        emit_label(EXT_OP_TEE_LOCAL_FAST);
                        emit_byte(loader_ctx, local_offset);
                    }
                    else {
                        emit_label(EXT_OP_TEE_LOCAL_FAST_I64);
                        emit_byte(loader_ctx, local_offset);
                    }
                }
                else {  /* local index larger than 255, reserve leb */
                    p_org ++;
                    emit_leb();
                }
                emit_operand(loader_ctx, *(loader_ctx->frame_offset -
                        wasm_value_type_cell_num(local_type)));
#else
#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_JIT == 0)
                if (local_offset < 0x80) {
                    *p_org++ = EXT_OP_TEE_LOCAL_FAST;
                    if (local_type == VALUE_TYPE_I32
                        || local_type == VALUE_TYPE_F32)
                        *p_org++ = (uint8)local_offset;
                    else
                        *p_org++ = (uint8)(local_offset | 0x80);
                    while (p_org < p)
                        *p_org++ = WASM_OP_NOP;
                }
#endif
#endif
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
#if WASM_ENABLE_FAST_INTERP != 0
                emit_const(global_idx);
                PUSH_OFFSET_TYPE(global_type);
#endif
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
#if WASM_ENABLE_FAST_INTERP != 0
                emit_const(global_idx);
                POP_OFFSET_TYPE(global_type);
#endif
                break;
            }

            /* load */
            case WASM_OP_I32_LOAD:
            case WASM_OP_I32_LOAD8_S:
            case WASM_OP_I32_LOAD8_U:
            case WASM_OP_I32_LOAD16_S:
            case WASM_OP_I32_LOAD16_U:
            case WASM_OP_I64_LOAD:
            case WASM_OP_I64_LOAD8_S:
            case WASM_OP_I64_LOAD8_U:
            case WASM_OP_I64_LOAD16_S:
            case WASM_OP_I64_LOAD16_U:
            case WASM_OP_I64_LOAD32_S:
            case WASM_OP_I64_LOAD32_U:
            case WASM_OP_F32_LOAD:
            case WASM_OP_F64_LOAD:
            /* store */
            case WASM_OP_I32_STORE:
            case WASM_OP_I32_STORE8:
            case WASM_OP_I32_STORE16:
            case WASM_OP_I64_STORE:
            case WASM_OP_I64_STORE8:
            case WASM_OP_I64_STORE16:
            case WASM_OP_I64_STORE32:
            case WASM_OP_F32_STORE:
            case WASM_OP_F64_STORE:
            {
#if WASM_ENABLE_FAST_INTERP != 0
                /* change F32/F64 into I32/I64 */
                if (opcode == WASM_OP_F32_LOAD) {
                    skip_label();
                    emit_label(WASM_OP_I32_LOAD);
                }
                else if (opcode == WASM_OP_F64_LOAD) {
                    skip_label();
                    emit_label(WASM_OP_I64_LOAD);
                }
                else if (opcode == WASM_OP_F32_STORE) {
                    skip_label();
                    emit_label(WASM_OP_I32_STORE);
                }
                else if (opcode == WASM_OP_F64_STORE) {
                    skip_label();
                    emit_label(WASM_OP_I64_STORE);
                }
#endif
                CHECK_MEMORY();
                read_leb_uint32(p, p_end, align); /* align */
                read_leb_uint32(p, p_end, mem_offset); /* offset */
#if WASM_ENABLE_FAST_INTERP != 0
                emit_const(mem_offset);
#endif
                switch (opcode)
                {
                    /* load */
                    case WASM_OP_I32_LOAD:
                    case WASM_OP_I32_LOAD8_S:
                    case WASM_OP_I32_LOAD8_U:
                    case WASM_OP_I32_LOAD16_S:
                    case WASM_OP_I32_LOAD16_U:
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
                        POP_I32();
                        PUSH_I64();
                        break;
                    case WASM_OP_F32_LOAD:
                        POP_I32();
                        PUSH_F32();
                        break;
                    case WASM_OP_F64_LOAD:
                        POP_I32();
                        PUSH_F64();
                        break;
                    /* store */
                    case WASM_OP_I32_STORE:
                    case WASM_OP_I32_STORE8:
                    case WASM_OP_I32_STORE16:
                        POP_I32();
                        POP_I32();
                        break;
                    case WASM_OP_I64_STORE:
                    case WASM_OP_I64_STORE8:
                    case WASM_OP_I64_STORE16:
                    case WASM_OP_I64_STORE32:
                        POP_I64();
                        POP_I32();
                        break;
                    case WASM_OP_F32_STORE:
                        POP_F32();
                        POP_I32();
                        break;
                    case WASM_OP_F64_STORE:
                        POP_F64();
                        POP_I32();
                        break;
                    default:
                        break;
                }
                break;
            }

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
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                disable_emit = true;
                GET_CONST_OFFSET(VALUE_TYPE_I32, i32_const);
#endif
                PUSH_I32();
                break;

            case WASM_OP_I64_CONST:
                read_leb_int64(p, p_end, i64);
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                disable_emit = true;
                GET_CONST_OFFSET(VALUE_TYPE_I64, i64);
#endif
                PUSH_I64();
                break;

            case WASM_OP_F32_CONST:
                p += sizeof(float32);
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                disable_emit = true;
                f32 = *(float32 *)p_org;
                GET_CONST_F32_OFFSET(VALUE_TYPE_F32, f32);
#endif
                PUSH_F32();
                break;

            case WASM_OP_F64_CONST:
                p += sizeof(float64);
#if WASM_ENABLE_FAST_INTERP != 0
                skip_label();
                disable_emit = true;
                /* Some MCU may require 8-byte align */
                memcpy((uint8*)&f64, p_org, sizeof(float64));
                GET_CONST_F64_OFFSET(VALUE_TYPE_F64, f64);
#endif
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

#if WASM_ENABLE_FAST_INTERP != 0
        last_op = opcode;
#endif
    }

    if (loader_ctx->csp_num > 0) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "function body must end with END opcode.");
        goto fail;
    }

#if WASM_ENABLE_FAST_INTERP != 0
    if (loader_ctx->p_code_compiled == NULL)
        goto re_scan;

    func->const_cell_num = loader_ctx->const_cell_num;
    if (!(func->consts = func_const =
                wasm_runtime_malloc(func->const_cell_num * 4))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM loader prepare bytecode failed: "
                      "allocate memory failed");
        goto fail;
    }
    memset(func->consts, 0, func->const_cell_num * 4);
    func_const_end = func->consts + func->const_cell_num * 4;
    // reverse the const buf
    for (int i = loader_ctx->num_const - 1; i >= 0; i--) {
        Const *c = (Const*)(loader_ctx->const_buf + i * sizeof(Const));
        if (c->value_type == VALUE_TYPE_F64
            || c->value_type == VALUE_TYPE_I64) {
            bh_memcpy_s(func_const, func_const_end - func_const,
                        &c->value.f64, sizeof(int64));
            func_const += sizeof(int64);
        } else {
            *(uint32*)func_const = c->value.i32;
            func_const += sizeof(int32);
        }
    }

    func->max_stack_cell_num = loader_ctx->max_dynamic_offset -
                                    loader_ctx->start_dynamic_offset + 1;
#else
    func->max_stack_cell_num = loader_ctx->max_stack_cell_num;
#endif
    func->max_block_num = loader_ctx->max_csp_num;
    return_value = true;

fail:
    wasm_loader_ctx_destroy(loader_ctx);

    (void)u8;
    (void)u32;
    (void)i32;
    (void)i64;
    (void)local_offset;
    (void)p_org;
    (void)mem_offset;
    (void)align;
    return return_value;
}
