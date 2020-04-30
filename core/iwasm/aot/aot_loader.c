/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_runtime.h"
#include "bh_common.h"
#include "bh_log.h"
#include "aot_reloc.h"
#include "../common/wasm_runtime_common.h"
#include "../common/wasm_native.h"
#include "../compilation/aot.h"
#if WASM_ENABLE_JIT != 0
#include "../compilation/aot_llvm.h"
#include "../interpreter/wasm_loader.h"
#endif


static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

#define exchange_uint8(p_data) (void)0

static void
exchange_uint16(uint8 *p_data)
{
    uint8 value = *p_data;
    *p_data = *(p_data + 1);
    *(p_data + 1) = value;
}

static void
exchange_uint32(uint8 *p_data)
{
    uint8 value = *p_data;
    *p_data = *(p_data + 3);
    *(p_data + 3) = value;

    value = *(p_data + 1);
    *(p_data + 1) = *(p_data + 2);
    *(p_data + 2) = value;
}

static void
exchange_uint64(uint8 *pData)
{
    exchange_uint32(pData);
    exchange_uint32(pData + 4);
}

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

#define CHECK_BUF(buf, buf_end, length) do {                \
    if (buf + length > buf_end) {                           \
      set_error_buf(error_buf, error_buf_size,              \
                    "Read data failed: unexpected end.");   \
      goto fail;                                            \
    }                                                       \
  } while (0)

static uint8*
align_ptr(const uint8 *p, uint32 b)
{
    uintptr_t v = (uintptr_t)p;
    uintptr_t m = b - 1;
    return (uint8*)((v + m) & ~m);
}

static inline uint64
GET_U64_FROM_ADDR(uint32 *addr)
{
    union { uint64 val; uint32 parts[2]; } u;
    u.parts[0] = addr[0];
    u.parts[1] = addr[1];
    return u.val;
}

#define TEMPLATE_READ(p, p_end, res, type) do {    \
    if (sizeof(type) != sizeof(uint64))            \
      p = (uint8*)align_ptr(p, sizeof(type));      \
    else                                           \
      /* align 4 bytes if type is uint64 */        \
      p = (uint8*)align_ptr(p, sizeof(uint32));    \
    CHECK_BUF(p, p_end, sizeof(type));             \
    if (sizeof(type) != sizeof(uint64))            \
      res = *(type*)p;                             \
    else                                           \
      res = (type)GET_U64_FROM_ADDR((uint32*)p);   \
    if (!is_little_endian())                       \
      exchange_##type((uint8*)&res);               \
    p += sizeof(type);                             \
  } while (0)

#define read_uint8(p, p_end, res) TEMPLATE_READ(p, p_end, res, uint8)
#define read_uint16(p, p_end, res) TEMPLATE_READ(p, p_end, res, uint16)
#define read_uint32(p, p_end, res) TEMPLATE_READ(p, p_end, res, uint32)
#define read_uint64(p, p_end, res) TEMPLATE_READ(p, p_end, res, uint64)

#define read_byte_array(p, p_end, addr, len) do { \
    CHECK_BUF(p, p_end, len);                     \
    memcpy(addr, p, len);                         \
    p += len;                                     \
  } while (0)

#define read_string(p, p_end, str) do {           \
    uint16 str_len;                               \
    read_uint16(p, p_end, str_len);               \
    CHECK_BUF(p, p_end, str_len);                 \
    if (!(str = const_str_set_insert              \
                  (p, str_len, module,            \
                   error_buf, error_buf_size))) { \
        goto fail;                                \
    }                                             \
    p += str_len;                                 \
  } while (0)

/* Legal values for bin_type */
#define BIN_TYPE_ELF32L 0           /* 32-bit little endian */
#define BIN_TYPE_ELF32B 1           /* 32-bit big endian */
#define BIN_TYPE_ELF64L 2           /* 64-bit little endian */
#define BIN_TYPE_ELF64B 3           /* 64-bit big endian */

/* Legal values for e_type (object file type). */
#define E_TYPE_NONE     0           /* No file type */
#define E_TYPE_REL      1           /* Relocatable file */
#define E_TYPE_EXEC     2           /* Executable file */
#define E_TYPE_DYN      3           /* Shared object file */

/* Legal values for e_machine (architecture).  */
#define E_MACHINE_386       3       /* Intel 80386 */
#define E_MACHINE_MIPS      8       /* MIPS R3000 big-endian */
#define E_MACHINE_MIPS_RS3_LE  10   /* MIPS R3000 little-endian */
#define E_MACHINE_ARM      40       /* ARM/Thumb */
#define E_MACHINE_AARCH64  183      /* AArch64 */
#define E_MACHINE_ARC      45       /* Argonaut RISC Core */
#define E_MACHINE_IA_64    50       /* Intel Merced */
#define E_MACHINE_MIPS_X   51       /* Stanford MIPS-X */
#define E_MACHINE_X86_64   62       /* AMD x86-64 architecture */
#define E_MACHINE_XTENSA   94       /* Tensilica Xtensa Architecture */

/* Legal values for e_version */
#define E_VERSION_CURRENT  1        /* Current version */

static char*
const_str_set_insert(const uint8 *str, int32 len, AOTModule *module,
                     char* error_buf, uint32 error_buf_size)
{
    HashMap *set = module->const_str_set;
    char *c_str = wasm_runtime_malloc((uint32)len + 1), *value;

    if (!c_str) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return NULL;
    }

    bh_memcpy_s(c_str, (uint32)(len + 1), str, (uint32)len);
    c_str[len] = '\0';

    if ((value = bh_hash_map_find(set, c_str))) {
        wasm_runtime_free(c_str);
        return value;
    }

    if (!bh_hash_map_insert(set, c_str, c_str)) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "insert string to hash map failed.");
        wasm_runtime_free(c_str);
        return NULL;
    }

    return c_str;
}

static bool
get_aot_file_target(AOTTargetInfo *target_info,
                    char *target_buf, uint32 target_buf_size,
                    char *error_buf, uint32 error_buf_size)
{
    char *machine_type = NULL;
    switch (target_info->e_machine) {
        case E_MACHINE_X86_64:
            machine_type = "x86_64";
            break;
        case E_MACHINE_386:
            machine_type = "i386";
            break;
        case E_MACHINE_ARM:
        case E_MACHINE_AARCH64:
            machine_type = target_info->arch;
            break;
        case E_MACHINE_MIPS:
            machine_type = "mips";
            break;
        case E_MACHINE_XTENSA:
            machine_type = "xtensa";
            break;
        default:
            if (error_buf)
                snprintf(error_buf, error_buf_size,
                         "AOT module load failed: unknown machine type %d.",
                         target_info->e_machine);
            return false;
    }
    if (strncmp(target_info->arch, machine_type, strlen(machine_type))) {
        if (error_buf)
            snprintf(error_buf, error_buf_size,
                     "AOT module load failed: "
                     "machine type (%s) isn't consistent with target type (%s).",
                     machine_type, target_info->arch);
        return false;
    }
    snprintf(target_buf, target_buf_size, "%s", target_info->arch);
    return true;
}

static bool
check_machine_info(AOTTargetInfo *target_info,
                   char *error_buf, uint32 error_buf_size)
{
    char target_expected[32], target_got[32];

    get_current_target(target_expected, sizeof(target_expected));

    if (!get_aot_file_target(target_info, target_got, sizeof(target_got),
                             error_buf, error_buf_size))
        return false;

    if (strcmp(target_expected, target_got)) {
        if (error_buf) {
          snprintf(error_buf, error_buf_size,
                  "AOT module load failed: invalid target type, "
                  "expected %s but got %s.",
                  target_expected, target_got);
        }
        return false;
    }

    return true;
}

static bool
load_target_info_section(const uint8 *buf, const uint8 *buf_end,
                         AOTModule *module,
                         char *error_buf, uint32 error_buf_size)
{
    AOTTargetInfo target_info;
    const uint8 *p = buf, *p_end = buf_end;
    bool is_target_little_endian, is_target_64_bit;

    read_uint16(p, p_end, target_info.bin_type);
    read_uint16(p, p_end, target_info.abi_type);
    read_uint16(p, p_end, target_info.e_type);
    read_uint16(p, p_end, target_info.e_machine);
    read_uint32(p, p_end, target_info.e_version);
    read_uint32(p, p_end, target_info.e_flags);
    read_uint32(p, p_end, target_info.reserved);
    read_byte_array(p, p_end,
                    target_info.arch, sizeof(target_info.arch));

    if (p != buf_end) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid section size.");
        return false;
    }

    /* Check target endian type */
    is_target_little_endian = target_info.bin_type & 1 ? false : true;
    if (is_little_endian() != is_target_little_endian) {
        if (error_buf)
          snprintf(error_buf, error_buf_size,
                   "AOT module load failed: "
                   "invalid target endian type, expected %s but got %s.",
                   is_little_endian() ? "little endian" : "big endian",
                   is_target_little_endian ? "little endian" : "big endian");
        return false;
    }

    /* Check target bit width */
    is_target_64_bit = target_info.bin_type & 2 ? true : false;
    if ((sizeof(void*) == 8 ? true : false) != is_target_64_bit) {
        if (error_buf)
          snprintf(error_buf, error_buf_size,
                   "AOT module load failed: "
                   "invalid target bit width, expected %s but got %s.",
                   sizeof(void*) == 8 ? "64-bit" : "32-bit",
                   is_target_64_bit ? "64-bit" : "32-bit");
        return false;
    }

    /* Check target elf file type */
    if (target_info.e_type != E_TYPE_REL) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid object file type, "
                      "expected relocatable file type but got others.");
        return false;
    }

    /* Check machine info */
    if (!check_machine_info(&target_info, error_buf, error_buf_size)) {
        return false;
    }

    if (target_info.e_version != E_VERSION_CURRENT) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid elf file version.");
        return false;
    }

    return true;
fail:
    return false;
}

static void
destroy_mem_init_data_list(AOTMemInitData **data_list, uint32 count,
                           bool is_jit_mode)
{
    if (!is_jit_mode) {
        uint32 i;
        for (i = 0; i < count; i++)
            if (data_list[i])
                wasm_runtime_free(data_list[i]);
        wasm_runtime_free(data_list);
    }
}

static bool
load_mem_init_data_list(const uint8 **p_buf, const uint8 *buf_end,
                        AOTModule *module,
                        char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTMemInitData **data_list;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTMemInitData *) * (uint64)module->mem_init_data_count;
    if (size >= UINT32_MAX
        || !(module->mem_init_data_list =
             data_list = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(data_list, 0, size);

    /* Create each memory data segment */
    for (i = 0; i < module->mem_init_data_count; i++) {
        uint32 init_expr_type, byte_count;
        uint64 init_expr_value;
        read_uint32(buf, buf_end, init_expr_type);
        read_uint64(buf, buf_end, init_expr_value);
        read_uint32(buf, buf_end, byte_count);
        size = offsetof(AOTMemInitData, bytes) + (uint64)byte_count;
        if (size >= UINT32_MAX
            || !(data_list[i] = wasm_runtime_malloc((uint32)size))) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "allocate memory failed.");
            return false;
        }

        data_list[i]->offset.init_expr_type = (uint8)init_expr_type;
        data_list[i]->offset.u.i64 = (int64)init_expr_value;
        data_list[i]->byte_count = byte_count;
        read_byte_array(buf, buf_end,
                        data_list[i]->bytes, data_list[i]->byte_count);
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_memory_info(const uint8 **p_buf, const uint8 *buf_end,
                 AOTModule *module,
                 char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->num_bytes_per_page);
    read_uint32(buf, buf_end, module->mem_init_page_count);
    read_uint32(buf, buf_end, module->mem_max_page_count);
    read_uint32(buf, buf_end, module->mem_init_data_count);

    /* load memory init data list */
    if (module->mem_init_data_count > 0
        && !load_mem_init_data_list(&buf, buf_end, module,
                                    error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_table_init_data_list(AOTTableInitData **data_list, uint32 count,
                             bool is_jit_mode)
{
    if (!is_jit_mode) {
        uint32 i;
        for (i = 0; i < count; i++)
            if (data_list[i])
                wasm_runtime_free(data_list[i]);
        wasm_runtime_free(data_list);
    }
}

static bool
load_table_init_data_list(const uint8 **p_buf, const uint8 *buf_end,
                          AOTModule *module,
                          char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTTableInitData **data_list;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTTableInitData *) * (uint64)module->table_init_data_count;
    if (size >= UINT32_MAX
        || !(module->table_init_data_list =
             data_list = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(data_list, 0, size);

    /* Create each table data segment */
    for (i = 0; i < module->table_init_data_count; i++) {
        uint32 init_expr_type, func_index_count;
        uint64 init_expr_value, size1;

        read_uint32(buf, buf_end, init_expr_type);
        read_uint64(buf, buf_end, init_expr_value);
        read_uint32(buf, buf_end, func_index_count);

        size1 = sizeof(uint32) * (uint64)func_index_count;
        size = offsetof(AOTTableInitData, func_indexes) + size1;
        if (size >= UINT32_MAX
            || !(data_list[i] = wasm_runtime_malloc((uint32)size))) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "allocate memory failed.");
            return false;
        }

        data_list[i]->offset.init_expr_type = (uint8)init_expr_type;
        data_list[i]->offset.u.i64 = (int64)init_expr_value;
        data_list[i]->func_index_count = func_index_count;
        read_byte_array(buf, buf_end, data_list[i]->func_indexes, size1);
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_table_info(const uint8 **p_buf, const uint8 *buf_end,
                AOTModule *module,
                char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->table_size);
    read_uint32(buf, buf_end, module->table_init_data_count);

    /* load table init data list */
    if (module->table_init_data_count > 0
        && !load_table_init_data_list(&buf, buf_end, module,
                                      error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_func_types(AOTFuncType **func_types, uint32 count, bool is_jit_mode)
{
    if (!is_jit_mode) {
        uint32 i;
        for (i = 0; i < count; i++)
            if (func_types[i])
                wasm_runtime_free(func_types[i]);
        wasm_runtime_free(func_types);
    }
}

static bool
load_func_types(const uint8 **p_buf, const uint8 *buf_end,
                AOTModule *module,
                char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTFuncType **func_types;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTFuncType *) * (uint64)module->func_type_count;
    if (size >= UINT32_MAX
        || !(module->func_types = func_types = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(func_types, 0, size);

    /* Create each function type */
    for (i = 0; i < module->func_type_count; i++) {
        uint32 param_count, result_count;
        uint64 size1;

        read_uint32(buf, buf_end, param_count);
        read_uint32(buf, buf_end, result_count);

        size1 = (uint64)param_count + (uint64)result_count;
        size = offsetof(AOTFuncType, types) + size1;
        if (size >= UINT32_MAX
            || !(func_types[i] = wasm_runtime_malloc((uint32)size))) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "allocate memory failed.");
            return false;
        }

        func_types[i]->param_count = param_count;
        func_types[i]->result_count = result_count;
        read_byte_array(buf, buf_end, func_types[i]->types, (uint32)size1);
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_func_type_info(const uint8 **p_buf, const uint8 *buf_end,
                    AOTModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->func_type_count);

    /* load function type */
    if (module->func_type_count > 0
        && !load_func_types(&buf, buf_end, module, error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_import_globals(AOTImportGlobal *import_globals, bool is_jit_mode)
{
    if (!is_jit_mode)
        wasm_runtime_free(import_globals);
}

static bool
load_import_globals(const uint8 **p_buf, const uint8 *buf_end,
                    AOTModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTImportGlobal *import_globals;
    uint64 size;
    uint32 i, data_offset = 0;

    /* Allocate memory */
    size = sizeof(AOTImportGlobal) * (uint64)module->import_global_count;
    if (size >= UINT32_MAX
        || !(module->import_globals =
             import_globals = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(import_globals, 0, size);

    /* Create each import global */
    for (i = 0; i < module->import_global_count; i++) {
        buf = (uint8*)align_ptr(buf, 2);
        read_uint8(buf, buf_end, import_globals[i].type);
        read_uint8(buf, buf_end, import_globals[i].is_mutable);
        read_string(buf, buf_end, import_globals[i].module_name);
        read_string(buf, buf_end, import_globals[i].global_name);

        import_globals[i].size = wasm_value_type_size(import_globals[i].type);
        import_globals[i].data_offset = data_offset;
        data_offset += import_globals[i].size;
        module->global_data_size += import_globals[i].size;
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_import_global_info(const uint8 **p_buf, const uint8 *buf_end,
                        AOTModule *module,
                        char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->import_global_count);

    /* load import globals */
    if (module->import_global_count > 0
        && !load_import_globals(&buf, buf_end, module,
                                error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_globals(AOTGlobal *globals, bool is_jit_mode)
{
    if (!is_jit_mode)
        wasm_runtime_free(globals);
}

static bool
load_globals(const uint8 **p_buf, const uint8 *buf_end,
             AOTModule *module,
             char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTGlobal *globals;
    uint64 size;
    uint32 i, data_offset = 0;
    AOTImportGlobal *last_import_global;

    /* Allocate memory */
    size = sizeof(AOTGlobal) * (uint64)module->global_count;
    if (size >= UINT32_MAX
        || !(module->globals = globals = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(globals, 0, size);

    if (module->import_global_count > 0) {
        last_import_global =
            &module->import_globals[module->import_global_count - 1];
        data_offset = last_import_global->data_offset
                      + last_import_global->size;
    }

    /* Create each global */
    for (i = 0; i < module->global_count; i++) {
        uint16 init_expr_type;
        uint64 init_expr_value;

        read_uint8(buf, buf_end, globals[i].type);
        read_uint8(buf, buf_end, globals[i].is_mutable);
        read_uint16(buf, buf_end, init_expr_type);
        read_uint64(buf, buf_end, init_expr_value);
        globals[i].init_expr.init_expr_type = (uint8)init_expr_type;
        globals[i].init_expr.u.i64 = (int64)init_expr_value;

        globals[i].size = wasm_value_type_size(globals[i].type);
        globals[i].data_offset = data_offset;
        data_offset += globals[i].size;
        module->global_data_size += globals[i].size;
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_global_info(const uint8 **p_buf, const uint8 *buf_end,
                 AOTModule *module,
                 char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->global_count);

    /* load globals */
    if (module->global_count > 0
        && !load_globals(&buf, buf_end, module, error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_import_funcs(AOTImportFunc *import_funcs,
                     bool is_jit_mode)
{
    if (!is_jit_mode)
        wasm_runtime_free(import_funcs);
}

static bool
load_import_funcs(const uint8 **p_buf, const uint8 *buf_end,
                  AOTModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    const char *module_name, *field_name;
    const uint8 *buf = *p_buf;
    AOTImportFunc *import_funcs;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTImportFunc) * (uint64)module->import_func_count;
    if (size >= UINT32_MAX
        || !(module->import_funcs =
             import_funcs = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(import_funcs, 0, size);

    /* Create each import func */
    for (i = 0; i < module->import_func_count; i++) {
        read_uint16(buf, buf_end, import_funcs[i].func_type_index);
        if (import_funcs[i].func_type_index >= module->func_type_count) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "invalid function type index.");
            return false;
        }
        import_funcs[i].func_type = module->func_types[import_funcs[i].func_type_index];
        read_string(buf, buf_end, import_funcs[i].module_name);
        read_string(buf, buf_end, import_funcs[i].func_name);

        module_name = import_funcs[i].module_name;
        field_name = import_funcs[i].func_name;
        if (!(import_funcs[i].func_ptr_linked =
                    wasm_native_resolve_symbol(module_name, field_name,
                                               import_funcs[i].func_type,
                                               &import_funcs[i].signature,
                                               &import_funcs[i].attachment,
                                               &import_funcs[i].call_conv_raw))) {
            LOG_WARNING("warning: fail to link import function (%s, %s)\n",
                        module_name, field_name);
        }

#if WASM_ENABLE_LIBC_WASI != 0
        if (!strcmp(import_funcs[i].module_name, "wasi_unstable")
            || !strcmp(import_funcs[i].module_name, "wasi_snapshot_preview1"))
            module->is_wasi_module = true;
#endif
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_import_func_info(const uint8 **p_buf, const uint8 *buf_end,
                      AOTModule *module,
                      char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->import_func_count);

    /* load import funcs */
    if (module->import_func_count > 0
        && !load_import_funcs(&buf, buf_end, module,
                                error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static void
destroy_object_data_sections(AOTObjectDataSection *data_sections,
                             uint32 data_section_count)
{
    uint32 i;
    AOTObjectDataSection *data_section = data_sections;
    for (i = 0; i < data_section_count; i++, data_section++)
        if (data_section->data)
            os_munmap(data_section->data, data_section->size);
    wasm_runtime_free(data_sections);
}

static bool
load_object_data_sections(const uint8 **p_buf, const uint8 *buf_end,
                          AOTModule *module,
                          char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTObjectDataSection *data_sections;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTObjectDataSection) * (uint64)module->data_section_count;
    if (size >= UINT32_MAX
        || !(module->data_sections =
             data_sections = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(data_sections, 0, size);

    /* Create each data section */
    for (i = 0; i < module->data_section_count; i++) {
        int map_prot = MMAP_PROT_READ | MMAP_PROT_WRITE;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* aot code and data in x86_64 must be in range 0 to 2G due to
           relocation for R_X86_64_32/32S/PC32 */
        int map_flags = MMAP_MAP_32BIT;
#else
        int map_flags = MMAP_MAP_NONE;
#endif

        read_string(buf, buf_end, data_sections[i].name);
        read_uint32(buf, buf_end, data_sections[i].size);

        /* Allocate memory for data */
        if (!(data_sections[i].data =
                    os_mmap(NULL, data_sections[i].size, map_prot, map_flags))) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "allocate memory failed.");
            return false;
        }
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* address must be in the first 2 Gigabytes of
           the process address space */
        bh_assert((uintptr_t)data_sections[i].data < INT32_MAX);
#endif

        read_byte_array(buf, buf_end,
                        data_sections[i].data, data_sections[i].size);
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_object_data_sections_info(const uint8 **p_buf, const uint8 *buf_end,
                               AOTModule *module,
                               char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;

    read_uint32(buf, buf_end, module->data_section_count);

    /* load object data sections */
    if (module->data_section_count > 0
        && !load_object_data_sections(&buf, buf_end, module,
                                      error_buf, error_buf_size))
        return false;

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_init_data_section(const uint8 *buf, const uint8 *buf_end,
                       AOTModule *module,
                       char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;

    if (!load_memory_info(&p, p_end, module, error_buf, error_buf_size)
        || !load_table_info(&p, p_end, module, error_buf, error_buf_size)
        || !load_func_type_info(&p, p_end, module, error_buf, error_buf_size)
        || !load_import_global_info(&p, p_end, module, error_buf, error_buf_size)
        || !load_global_info(&p, p_end, module, error_buf, error_buf_size)
        || !load_import_func_info(&p, p_end, module, error_buf, error_buf_size))
        return false;

    /* load function count and start function index */
    read_uint32(p, p_end, module->func_count);
    read_uint32(p, p_end, module->start_func_index);

    /* check start function index */
    if (module->start_func_index != (uint32)-1
        && (module->start_func_index >= module->import_func_count
                                           + module->func_count)) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "invalid start function index");
        return false;
    }

    read_uint32(p, p_end, module->llvm_aux_data_end);
    read_uint32(p, p_end, module->llvm_aux_stack_bottom);
    read_uint32(p, p_end, module->llvm_aux_stack_size);
    read_uint32(p, p_end, module->llvm_aux_stack_global_index);

    if (!load_object_data_sections_info(&p, p_end, module,
                                        error_buf, error_buf_size))
        return false;

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "invalid init data section size");
        return false;
    }

    return true;

fail:
    return false;
}

static bool
load_text_section(const uint8 *buf, const uint8 *buf_end,
                  AOTModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    uint8 *plt_base;

    if (module->func_count > 0 && buf_end == buf) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid code size.");
        return false;
    }

    read_uint32(buf, buf_end, module->literal_size);

    /* literal data is at begining of the text section */
    module->literal = (uint8*)buf;
    module->code = (void*)(buf + module->literal_size);
    module->code_size = (uint32)(buf_end - (uint8*)module->code);

    if (module->code_size > 0) {
        plt_base = (uint8*)buf_end - get_plt_table_size();
        init_plt_table(plt_base);
    }
    return true;

fail:
    return false;
}

static bool
load_function_section(const uint8 *buf, const uint8 *buf_end,
                      AOTModule *module,
                      char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;
    uint32 i;
    uint64 size, text_offset;

    size = sizeof(void*) * (uint64)module->func_count;
    if (size >= UINT32_MAX
        || !(module->func_ptrs = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: allocate memory failed.");
        return false;
    }

    for (i = 0; i < module->func_count; i++) {
        if (sizeof(void*) == 8) {
            read_uint64(p, p_end, text_offset);
        }
        else {
            uint32 text_offset32;
            read_uint32(p, p_end, text_offset32);
            text_offset = text_offset32;
        }
        if (text_offset >= module->code_size) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "invalid function code offset.");
            return false;
        }
        module->func_ptrs[i] = (uint8*)module->code + text_offset;
#if defined(BUILD_TARGET_THUMB) || defined(BUILD_TARGET_THUMB_VFP)
        /* bits[0] of thumb function address must be 1 */
        module->func_ptrs[i] = (void*)((uintptr_t)module->func_ptrs[i] | 1);
#endif
    }

    /* Set start function when function pointers are resolved */
    if (module->start_func_index != (uint32)-1) {
        if (module->start_func_index >= module->import_func_count)
            module->start_function =
                module->func_ptrs[module->start_func_index
                            - module->import_func_count];
        else
            /* TODO: fix start function can be import function issue */
            module->start_function = NULL;
    }
    else {
        module->start_function = NULL;
    }

    size = sizeof(uint32) * (uint64)module->func_count;
    if (size >= UINT32_MAX
        || !(module->func_type_indexes = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: allocate memory failed.");
        return false;
    }

    for (i = 0; i < module->func_count; i++) {
        read_uint32(p, p_end, module->func_type_indexes[i]);
        if (module->func_type_indexes[i] >= module->func_type_count) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "invalid function type index.");
            return false;
        }
    }

    if (p != buf_end) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "invalid function section size");
        return false;
    }

    return true;
fail:
    return false;
}

static void
destroy_export_funcs(AOTExportFunc *export_funcs, bool is_jit_mode)
{
    if (!is_jit_mode)
        wasm_runtime_free(export_funcs);
}

static bool
load_export_funcs(const uint8 **p_buf, const uint8 *buf_end,
                  AOTModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    AOTExportFunc *export_funcs;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTExportFunc) * (uint64)module->export_func_count;
    if (size >= UINT32_MAX
        || !(module->export_funcs =
             export_funcs = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return false;
    }

    memset(export_funcs, 0, size);

    /* Create each export func */
    for (i = 0; i < module->export_func_count; i++) {
        read_uint32(buf, buf_end, export_funcs[i].func_index);
        if (export_funcs[i].func_index >=
              module->func_count + module->import_func_count) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "function index is out of range.");
            return false;
        }
        read_string(buf, buf_end, export_funcs[i].func_name);
    }

    *p_buf = buf;
    return true;
fail:
    return false;
}

static bool
load_export_section(const uint8 *buf, const uint8 *buf_end,
                    AOTModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    const uint8 *p = buf, *p_end = buf_end;

    /* load export functions */
    read_uint32(p, p_end, module->export_func_count);
    if (module->export_func_count > 0
        && !load_export_funcs(&p, p_end, module, error_buf, error_buf_size))
        return false;

    if (p != p_end) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "invalid export section size");
        return false;
    }

    return true;

fail:
    return false;
}


static void *
get_data_section_addr(AOTModule *module, const char *section_name,
                      uint32 *p_data_size)
{
    uint32 i;
    AOTObjectDataSection *data_section = module->data_sections;

    for (i = 0; i < module->data_section_count; i++, data_section++)
        if (!strcmp(data_section->name, section_name)) {
            if (p_data_size)
                *p_data_size = data_section->size;
            return data_section->data;
        }

    return NULL;
}

static void *
resolve_target_sym(const char *symbol, int32 *p_index)
{
    uint32 i, num = 0;
    SymbolMap *target_sym_map;

   if (!(target_sym_map = get_target_symbol_map(&num)))
       return NULL;

    for (i = 0; i < num; i++)
        if (!strcmp(target_sym_map[i].symbol_name, symbol)) {
            *p_index = (int32)i;
            return target_sym_map[i].symbol_addr;
        }
    return NULL;
}

static bool
is_literal_relocation(const char *reloc_sec_name)
{
    return !strcmp(reloc_sec_name, ".rela.literal");
}

static bool
do_text_relocation(AOTModule *module,
                   AOTRelocationGroup *group,
                   char *error_buf, uint32 error_buf_size)
{
    bool is_literal = is_literal_relocation(group->section_name);
    uint8 *aot_text = is_literal ? module->literal : module->code;
    uint32 aot_text_size = is_literal ? module->literal_size : module->code_size;
    uint32 i, func_index, symbol_len;
    char symbol_buf[128]  = { 0 }, *symbol, *p;
    void *symbol_addr;
    AOTRelocation *relocation = group->relocations;

    if (group->relocation_count > 0 && !aot_text) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid text relocation count.");
        return false;
    }

    for (i = 0; i < group->relocation_count; i++, relocation++) {
        int32 symbol_index = -1;
        symbol_len = (uint32)strlen(relocation->symbol_name);
        if (symbol_len + 1 <= sizeof(symbol_buf))
            symbol = symbol_buf;
        else {
            if (!(symbol = wasm_runtime_malloc(symbol_len + 1))) {
                set_error_buf(error_buf, error_buf_size,
                              "AOT module load failed: "
                              "allocate memory failed.");
                return false;
            }
        }
        memcpy(symbol, relocation->symbol_name, symbol_len);
        symbol[symbol_len] = '\0';

        if (!strncmp(symbol, AOT_FUNC_PREFIX, strlen(AOT_FUNC_PREFIX))) {
            p = symbol + strlen(AOT_FUNC_PREFIX);
            if (*p == '\0'
                || (func_index = (uint32)atoi(p)) > module->func_count) {
                if (error_buf != NULL)
                    snprintf(error_buf, error_buf_size,
                             "AOT module load failed: "
                             "invalid import symbol %s.",
                             symbol);
                goto check_symbol_fail;
            }
            symbol_addr = module->func_ptrs[func_index];
        }
        else if (!strcmp(symbol, ".text")) {
            symbol_addr = module->code;
        }
        else if (!strcmp(symbol, ".data")
                 || !strcmp(symbol, ".rodata")
                 /* ".rodata.cst4/8/16/.." */
                 || !strncmp(symbol, ".rodata.cst", strlen(".rodata.cst"))) {
            symbol_addr = get_data_section_addr(module, symbol, NULL);
            if (!symbol_addr) {
                if (error_buf != NULL)
                    snprintf(error_buf, error_buf_size,
                             "AOT module load failed: "
                             "invalid data section (%s).",
                             symbol);
                goto check_symbol_fail;
            }
        }
        else if (!strcmp(symbol, ".literal")) {
            symbol_addr = module->literal;
        }
        else if (!(symbol_addr = resolve_target_sym(symbol, &symbol_index))) {
            if (error_buf != NULL)
                snprintf(error_buf, error_buf_size,
                         "AOT module load failed: "
                         "resolve symbol %s failed.",
                         symbol);
            goto check_symbol_fail;
        }

        if (symbol != symbol_buf)
            wasm_runtime_free(symbol);

        if (!apply_relocation(module,
                              aot_text, aot_text_size,
                              relocation->relocation_offset,
                              relocation->relocation_addend,
                              relocation->relocation_type,
                              symbol_addr, symbol_index,
                              error_buf, error_buf_size))
            return false;
    }

    return true;

check_symbol_fail:
    if (symbol != symbol_buf)
        wasm_runtime_free(symbol);
    return false;
}

static bool
do_data_relocation(AOTModule *module,
                   AOTRelocationGroup *group,
                   char *error_buf, uint32 error_buf_size)

{
    uint8 *data_addr;
    uint32 data_size = 0, i;
    AOTRelocation *relocation = group->relocations;
    void *symbol_addr;
    char *symbol, *data_section_name;

    if (!strncmp(group->section_name, ".rela.", 6)) {
        data_section_name = group->section_name + strlen(".rela");
    }
    else if (!strncmp(group->section_name, ".rel.", 5)) {
        data_section_name = group->section_name + strlen(".rel");
    }
    else {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "invalid data relocation section name.");
        return false;
    }

    data_addr = get_data_section_addr(module, data_section_name,
                                      &data_size);
    if (group->relocation_count > 0 && !data_addr) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: invalid data relocation count.");
        return false;
    }

    for (i = 0; i < group->relocation_count; i++, relocation++) {
        symbol = relocation->symbol_name;
        if (!strcmp(symbol, ".text")) {
            symbol_addr = module->code;
        }
        else {
            if (error_buf != NULL)
                snprintf(error_buf, error_buf_size,
                         "AOT module load failed: "
                         "invalid relocation symbol %s.",
                         symbol);
            return false;
        }

        if (!apply_relocation(module,
                              data_addr, data_size,
                              relocation->relocation_offset,
                              relocation->relocation_addend,
                              relocation->relocation_type,
                              symbol_addr, -1,
                              error_buf, error_buf_size))
            return false;
    }

    return true;
}

static bool
validate_symbol_table(uint8 *buf, uint8 *buf_end,
                      uint32 *offsets, uint32 count,
                      char *error_buf, uint32 error_buf_size)
{
    uint32 i, str_len_addr = 0;
    uint16 str_len;

    for (i = 0; i < count; i++) {
        if (offsets[i] != str_len_addr)
            return false;

        read_uint16(buf, buf_end, str_len);
        str_len_addr += (uint32)sizeof(uint16) + str_len;
        str_len_addr = align_uint(str_len_addr, 2);
        buf += str_len;
        buf = (uint8*)align_ptr(buf, 2);
    }

    if (buf == buf_end)
        return true;
fail:
    return false;
}

static bool
load_relocation_section(const uint8 *buf, const uint8 *buf_end,
                        AOTModule *module,
                        char *error_buf, uint32 error_buf_size)
{
    AOTRelocationGroup *groups = NULL, *group;
    uint32 symbol_count = 0;
    uint32 group_count = 0, i, j, func_index, func_type_index;
    uint64 size;
    uint32 *symbol_offsets, total_string_len;
    uint8 *symbol_buf, *symbol_buf_end;
    bool ret = false;
    AOTExportFunc *export_func;

    read_uint32(buf, buf_end, symbol_count);

    symbol_offsets = (uint32 *)buf;
    for (i = 0; i < symbol_count; i++) {
        CHECK_BUF(buf, buf_end, sizeof(uint32));
        buf += sizeof(uint32);
    }

    read_uint32(buf, buf_end, total_string_len);
    symbol_buf = (uint8 *)buf;
    symbol_buf_end = symbol_buf + total_string_len;

    if (!validate_symbol_table(symbol_buf, symbol_buf_end,
                               symbol_offsets, symbol_count,
                               error_buf, error_buf_size)) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "validate symbol table failed.");
        goto fail;
    }

    buf = symbol_buf_end;
    read_uint32(buf, buf_end, group_count);

    /* Allocate memory for relocation groups */
    size = sizeof(AOTRelocationGroup) * (uint64)group_count;
    if (size >= UINT32_MAX || !(groups = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        goto fail;
    }

    memset(groups, 0, size);

    /* Load each relocation group */
    for (i = 0, group = groups; i < group_count; i++, group++) {
        AOTRelocation *relocation;
        uint32 name_index;
        uint16 str_len;
        uint8 *name_addr;

        /* section name address is 4 bytes aligned. */
        buf = (uint8*)align_ptr(buf, sizeof(uint32));
        read_uint32(buf, buf_end, name_index);

        if (name_index >= symbol_count) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "symbol index out of range.");
            goto fail;
        }

        name_addr = symbol_buf + symbol_offsets[name_index];
        str_len = *(uint16 *)name_addr;

        if (!(group->section_name =
                    const_str_set_insert(name_addr + sizeof(uint16),
                                         (int32)str_len, module,
                                         error_buf, error_buf_size))) {
            goto fail;
        }

        read_uint32(buf, buf_end, group->relocation_count);

        /* Allocate memory for relocations */
        size = sizeof(AOTRelocation) * (uint64)group->relocation_count;
        if (size >= UINT32_MAX
            || !(group->relocations = relocation =
                 wasm_runtime_malloc((uint32)size))) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "allocate memory failed.");
            ret = false;
            goto fail;
        }

        memset(group->relocations, 0, size);

        /* Load each relocation */
        for (j = 0; j < group->relocation_count; j++, relocation++) {
            uint32 symbol_index;
            uint16 str_len;
            uint8 *symbol_addr;

            if (sizeof(void *) == 8) {
                read_uint64(buf, buf_end, relocation->relocation_offset);
                read_uint64(buf, buf_end, relocation->relocation_addend);
            }
            else {
                uint32 offset32, addend32;
                read_uint32(buf, buf_end, offset32);
                relocation->relocation_offset = (uint64)offset32;
                read_uint32(buf, buf_end, addend32);
                relocation->relocation_addend = (uint64)addend32;
            }
            read_uint32(buf, buf_end, relocation->relocation_type);
            read_uint32(buf, buf_end, symbol_index);

            if (symbol_index >= symbol_count) {
                set_error_buf(error_buf, error_buf_size,
                              "AOT module load failed: "
                              "symbol index out of range.");
                goto fail;
            }

            symbol_addr = symbol_buf + symbol_offsets[symbol_index];
            str_len = *(uint16 *)symbol_addr;

            if (!(relocation->symbol_name =
                    const_str_set_insert(symbol_addr + sizeof(uint16),
                                         (int32)str_len, module,
                                         error_buf, error_buf_size))) {
                goto fail;
            }
        }

        if (!strcmp(group->section_name, ".rel.text")
            || !strcmp(group->section_name, ".rela.text")
            || !strcmp(group->section_name, ".rela.literal")) {
            if (!do_text_relocation(module, group, error_buf, error_buf_size))
                return false;
        }
        else {
            if (!do_data_relocation(module, group, error_buf, error_buf_size))
                return false;
        }
    }

    export_func = module->export_funcs;
    for (i = 0; i < module->export_func_count; i++, export_func++) {
        func_index = export_func->func_index - module->import_func_count;
        if (func_index >= module->func_count) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: "
                          "invalid export function index.");
            ret = false;
            goto fail;
        }
        func_type_index = module->func_type_indexes[func_index];
        export_func->func_type = module->func_types[func_type_index];
        export_func->func_ptr = module->func_ptrs[func_index];
    }

    ret = true;

fail:
    if (groups) {
        for (i = 0, group = groups; i < group_count; i++, group++)
            if (group->relocations)
                wasm_runtime_free(group->relocations);
        wasm_runtime_free(groups);
    }

    return ret;
}

static bool
load_from_sections(AOTModule *module, AOTSection *sections,
                   char *error_buf, uint32 error_buf_size)
{
    AOTSection *section = sections;
    const uint8 *buf, *buf_end;
    uint32 last_section_type = (uint32)-1, section_type;

    while (section) {
        buf = section->section_body;
        buf_end = buf + section->section_body_size;
        /* Check sections */
        section_type = (uint32)section->section_type;
        if ((last_section_type == (uint32)-1
             && section_type != AOT_SECTION_TYPE_TARGET_INFO)
            || (last_section_type != (uint32)-1
                && section_type != last_section_type + 1)) {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: invalid section order.");
            return false;
        }
        last_section_type = section_type;
        switch (section_type) {
            case AOT_SECTION_TYPE_TARGET_INFO:
                if (!load_target_info_section(buf, buf_end, module,
                                              error_buf, error_buf_size))
                    return false;
                break;
            case AOT_SECTION_TYPE_INIT_DATA:
                if (!load_init_data_section(buf, buf_end, module,
                                            error_buf, error_buf_size))
                    return false;
                break;
            case AOT_SECTION_TYPE_TEXT:
                if (!load_text_section(buf, buf_end, module,
                                       error_buf, error_buf_size))
                    return false;
                break;
            case AOT_SECTION_TYPE_FUNCTION:
                if (!load_function_section(buf, buf_end, module,
                                           error_buf, error_buf_size))
                    return false;
                break;
            case AOT_SECTION_TYPE_EXPORT:
                if (!load_export_section(buf, buf_end, module,
                                         error_buf, error_buf_size))
                    return false;
                break;
            case AOT_SECTION_TYPE_RELOCATION:
                if (!load_relocation_section(buf, buf_end, module,
                                             error_buf, error_buf_size))
                    return false;
                break;
        }

        section = section->next;
    }

    if (last_section_type != AOT_SECTION_TYPE_RELOCATION) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: section missing.");
        return false;
    }

    /* Flush data cache before executing AOT code,
     * otherwise unpredictable behavior can occur. */
    os_dcache_flush();

    return true;
}

#if BH_ENABLE_MEMORY_PROFILING != 0
static void aot_free(void *ptr)
{
    wasm_runtime_free(ptr);
}
#else
#define aot_free wasm_runtime_free
#endif

static AOTModule*
create_module(char *error_buf, uint32 error_buf_size)
{
    AOTModule *module = wasm_runtime_malloc(sizeof(AOTModule));

    if (!module) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(module, 0, sizeof(AOTModule));

    module->module_type = Wasm_Module_AoT;

    if (!(module->const_str_set =
                bh_hash_map_create(32, false,
                                   (HashFunc)wasm_string_hash,
                                   (KeyEqualFunc)wasm_string_equal,
                                   NULL,
                                   aot_free))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: "
                      "create const string set failed.");
        wasm_runtime_free(module);
        return NULL;
    }

    return module;
}

AOTModule*
aot_load_from_sections(AOTSection *section_list,
                       char *error_buf, uint32 error_buf_size)
{
    AOTModule *module = create_module(error_buf, error_buf_size);

    if (!module)
        return NULL;

    if (!load_from_sections(module, section_list,
                            error_buf, error_buf_size)) {
        aot_unload(module);
        return NULL;
    }

    LOG_VERBOSE("Load module from sections success.\n");
    return module;
}

static void
destroy_sections(AOTSection *section_list, bool destroy_aot_text)
{
    AOTSection *section = section_list, *next;
    while (section) {
        next = section->next;
        if (destroy_aot_text
            && section->section_type == AOT_SECTION_TYPE_TEXT
            && section->section_body)
            os_munmap((uint8*)section->section_body, section->section_body_size);
        wasm_runtime_free(section);
        section = next;
    }
}

static bool
create_sections(const uint8 *buf, uint32 size,
                AOTSection **p_section_list,
                char *error_buf, uint32 error_buf_size)
{
    AOTSection *section_list = NULL, *section_list_end = NULL, *section;
    const uint8 *p = buf, *p_end = buf + size;
    uint32 section_type;
    uint32 section_size;
    uint64 total_size;
    uint8 *aot_text;

    p += 8;
    while (p < p_end) {
        read_uint32(p, p_end, section_type);
        if (section_type < AOT_SECTION_TYPE_SIGANATURE) {
            read_uint32(p, p_end, section_size);
            CHECK_BUF(p, p_end, section_size);

            if (!(section = wasm_runtime_malloc(sizeof(AOTSection)))) {
                set_error_buf(error_buf, error_buf_size,
                              "AOT module load failed: "
                              "allocate memory failed.");
                goto fail;
            }

            memset(section, 0, sizeof(AOTSection));
            section->section_type = (int32)section_type;
            section->section_body = (uint8*)p;
            section->section_body_size = section_size;

            if (section_type == AOT_SECTION_TYPE_TEXT) {
                if (section_size > 0) {
                    int map_prot = MMAP_PROT_READ | MMAP_PROT_WRITE
                                   | MMAP_PROT_EXEC;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
                    /* aot code and data in x86_64 must be in range 0 to 2G due to
                       relocation for R_X86_64_32/32S/PC32 */
                    int map_flags = MMAP_MAP_32BIT;
#else
                    int map_flags = MMAP_MAP_NONE;
#endif
                    total_size = (uint64)section_size + aot_get_plt_table_size();
                    total_size = (total_size + 3) & ~((uint64)3);
                    if (total_size >= UINT32_MAX
                        || !(aot_text = os_mmap(NULL, (uint32)total_size,
                                                map_prot, map_flags))) {
                        wasm_runtime_free(section);
                        set_error_buf(error_buf, error_buf_size,
                                      "AOT module load failed: "
                                      "mmap memory failed.");
                        goto fail;
                    }
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
                    /* address must be in the first 2 Gigabytes of
                       the process address space */
                    bh_assert((uintptr_t)aot_text < INT32_MAX);
#endif
                    bh_memcpy_s(aot_text, (uint32)total_size,
                                section->section_body, (uint32)section_size);
                    section->section_body = aot_text;

                    if ((uint32)total_size > section->section_body_size) {
                        memset(aot_text + (uint32)section_size,
                               0, (uint32)total_size - section_size);
                        section->section_body_size = (uint32)total_size;
                    }
                }
                else
                    section->section_body = NULL;
            }

            if (!section_list)
                section_list = section_list_end = section;
            else {
                section_list_end->next = section;
                section_list_end = section;
            }

            p += section_size;
        }
        else {
            set_error_buf(error_buf, error_buf_size,
                          "AOT module load failed: invalid section id.");
            goto fail;
        }
    }

    if (!section_list) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module load failed: create section list failed.");
        return false;
    }

    *p_section_list = section_list;
    return true;
fail:
    if (section_list)
        destroy_sections(section_list, true);
    return false;
}

static bool
load(const uint8 *buf, uint32 size, AOTModule *module,
     char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf_end = buf + size;
    const uint8 *p = buf, *p_end = buf_end;
    uint32 magic_number, version;
    AOTSection *section_list = NULL;
    bool ret;

    read_uint32(p, p_end, magic_number);
    if (magic_number != AOT_MAGIC_NUMBER) {
        set_error_buf(error_buf, error_buf_size, "magic header not detected");
        return false;
    }

    read_uint32(p, p_end, version);
    if (version != AOT_CURRENT_VERSION) {
        set_error_buf(error_buf, error_buf_size, "unknown binary version");
        return false;
    }

    if (!create_sections(buf, size, &section_list, error_buf, error_buf_size))
        return false;

    ret = load_from_sections(module, section_list, error_buf, error_buf_size);
    if (!ret) {
        /* If load_from_sections() fails, then aot text is destroyed
           in destroy_sections() */
        destroy_sections(section_list, true);
        /* aot_unload() won't destroy aot text again */
        module->code = NULL;
    }
    else {
        /* If load_from_sections() succeeds, then aot text is set to
           module->code and will be destroyed in aot_unload() */
        destroy_sections(section_list, false);
    }
    return ret;
fail:
    return false;
}

AOTModule*
aot_load_from_aot_file(const uint8 *buf, uint32 size,
                       char *error_buf, uint32 error_buf_size)
{
    AOTModule *module = create_module(error_buf, error_buf_size);

    if (!module)
        return NULL;

    if (!load(buf, size, module, error_buf, error_buf_size)) {
        aot_unload(module);
        return NULL;
    }

    LOG_VERBOSE("Load module success.\n");
    return module;
}

#if WASM_ENABLE_JIT != 0
static AOTModule*
aot_load_from_comp_data(AOTCompData *comp_data, AOTCompContext *comp_ctx,
                        char *error_buf, uint32 error_buf_size)
{
    uint32 i;
    uint64 size;
    char func_name[32];
    AOTModule *module;

    /* Allocate memory for module */
    if (!(module = wasm_runtime_malloc(sizeof(AOTModule)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Allocate memory for AOT module failed.");
        return NULL;
    }

    memset(module, 0, sizeof(AOTModule));

    module->module_type = Wasm_Module_AoT;
    module->num_bytes_per_page = comp_data->num_bytes_per_page;
    module->mem_init_page_count = comp_data->mem_init_page_count;
    module->mem_max_page_count = comp_data->mem_max_page_count;

    module->mem_init_data_list = comp_data->mem_init_data_list;
    module->mem_init_data_count = comp_data->mem_init_data_count;

    module->table_init_data_list = comp_data->table_init_data_list;
    module->table_init_data_count = comp_data->table_init_data_count;
    module->table_size = comp_data->table_size;

    module->func_type_count = comp_data->func_type_count;
    module->func_types = comp_data->func_types;

    module->import_global_count = comp_data->import_global_count;
    module->import_globals = comp_data->import_globals;

    module->global_count = comp_data->global_count;
    module->globals = comp_data->globals;

    module->global_count = comp_data->global_count;
    module->globals = comp_data->globals;

    module->global_data_size = comp_data->global_data_size;

    module->import_func_count = comp_data->import_func_count;
    module->import_funcs = comp_data->import_funcs;

    module->func_count = comp_data->func_count;

    /* Allocate memory for function pointers */
    size = (uint64)module->func_count * sizeof(void *);
    if (size >= UINT32_MAX
        || !(module->func_ptrs = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size, "Create func ptrs fail.");
        goto fail1;
    }

    /* Resolve function addresses */
    bh_assert(comp_ctx->exec_engine);
    memset(module->func_ptrs, 0, (uint32)size);
    for (i = 0; i < comp_data->func_count; i++) {
        snprintf(func_name, sizeof(func_name), "%s%d", AOT_FUNC_PREFIX, i);
        if (!(module->func_ptrs[i] =
                    (void *)LLVMGetFunctionAddress(comp_ctx->exec_engine,
                                                   func_name))) {
            set_error_buf(error_buf, error_buf_size,
                          "Get function address fail.");
            goto fail2;
        }
    }

    /* Allocation memory for function type indexes */
    size = (uint64)module->func_count * sizeof(uint32);
    if (size >= UINT32_MAX
        || !(module->func_type_indexes = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size, "Create func type indexes fail.");
        goto fail2;
    }
    memset(module->func_type_indexes, 0, (uint32)size);
    for (i = 0; i < comp_data->func_count; i++)
        module->func_type_indexes[i] = comp_data->funcs[i]->func_type_index;

    module->export_func_count = comp_data->export_func_count;
    module->export_funcs = comp_data->export_funcs;

    /* Set export function pointers */
    for (i = 0; i < module->export_func_count; i++) {
        module->export_funcs[i].func_ptr =
            module->func_ptrs[module->export_funcs[i].func_index
                              - module->import_func_count];
    }

    module->start_func_index = comp_data->start_func_index;
    if (comp_data->start_func_index != (uint32)-1) {
        bh_assert(comp_data->start_func_index >= module->import_func_count
                  && comp_data->start_func_index < module->import_func_count
                                                   + module->func_count);
        module->start_function =
            module->func_ptrs[comp_data->start_func_index
                              - module->import_func_count];
    }
    else {
        module->start_function = NULL;
    }

    module->llvm_aux_data_end = comp_data->llvm_aux_data_end;
    module->llvm_aux_stack_bottom = comp_data->llvm_aux_stack_bottom;
    module->llvm_aux_stack_size = comp_data->llvm_aux_stack_size;
    module->llvm_aux_stack_global_index = comp_data->llvm_aux_stack_global_index;

    module->code = NULL;
    module->code_size = 0;

    module->is_jit_mode = true;

    module->wasm_module = comp_data->wasm_module;
    module->comp_ctx = comp_ctx;
    module->comp_data = comp_data;

#if WASM_ENABLE_LIBC_WASI != 0
    module->is_wasi_module = comp_data->wasm_module->is_wasi_module;
#endif

    return module;

fail2:
    wasm_runtime_free(module->func_ptrs);
fail1:
    wasm_runtime_free(module);
    return NULL;
}

AOTModule*
aot_convert_wasm_module(WASMModule *wasm_module,
                        char *error_buf, uint32 error_buf_size)
{
    AOTCompData *comp_data;
    AOTCompContext *comp_ctx;
    AOTModule *aot_module;
    AOTCompOption option = { 0 };
    char *aot_last_error;

    comp_data = aot_create_comp_data(wasm_module);
    if (!comp_data) {
        aot_last_error = aot_get_last_error();
        bh_assert(aot_last_error != NULL);
        set_error_buf(error_buf, error_buf_size, aot_last_error);
        return NULL;
    }

    option.is_jit_mode = true;
    comp_ctx = aot_create_comp_context(comp_data, &option);
    if (!comp_ctx) {
        aot_last_error = aot_get_last_error();
        bh_assert(aot_last_error != NULL);
        set_error_buf(error_buf, error_buf_size, aot_last_error);
        goto fail1;
    }

    if (!aot_compile_wasm(comp_ctx)) {
        aot_last_error = aot_get_last_error();
        bh_assert(aot_last_error != NULL);
        set_error_buf(error_buf, error_buf_size, aot_last_error);
        goto fail2;
    }

    aot_module = aot_load_from_comp_data(comp_data, comp_ctx,
                                         error_buf, error_buf_size);
    if (!aot_module) {
        goto fail2;
    }

    return aot_module;

fail2:
    aot_destroy_comp_context(comp_ctx);
fail1:
    aot_destroy_comp_data(comp_data);
    return NULL;
}
#endif

void
aot_unload(AOTModule *module)
{
#if WASM_ENABLE_JIT != 0
    if (module->comp_data)
        aot_destroy_comp_data(module->comp_data);

    if (module->comp_ctx)
        aot_destroy_comp_context(module->comp_ctx);

    if (module->wasm_module)
        wasm_loader_unload(module->wasm_module);
#endif

    if (module->mem_init_data_list)
        destroy_mem_init_data_list(module->mem_init_data_list,
                                   module->mem_init_data_count,
                                   module->is_jit_mode);

    if (module->table_init_data_list)
        destroy_table_init_data_list(module->table_init_data_list,
                                     module->table_init_data_count,
                                     module->is_jit_mode);

    if (module->func_types)
        destroy_func_types(module->func_types,
                           module->func_type_count,
                           module->is_jit_mode);

    if (module->import_globals)
        destroy_import_globals(module->import_globals,
                               module->is_jit_mode);

    if (module->globals)
        destroy_globals(module->globals,
                        module->is_jit_mode);

    if (module->import_funcs)
        destroy_import_funcs(module->import_funcs,
                             module->is_jit_mode);

    if (module->export_funcs)
        destroy_export_funcs(module->export_funcs,
                             module->is_jit_mode);

    if (module->func_type_indexes)
        wasm_runtime_free(module->func_type_indexes);

    if (module->func_ptrs)
        wasm_runtime_free(module->func_ptrs);

    if (module->const_str_set)
        bh_hash_map_destroy(module->const_str_set);

    if (module->code) {
        uint8 *mmap_addr = module->literal - sizeof(module->literal_size);
        uint32 total_size = sizeof(module->literal_size) + module->literal_size + module->code_size;
        os_munmap(mmap_addr, total_size);
    }

    if (module->data_sections)
        destroy_object_data_sections(module->data_sections,
                                     module->data_section_count);

    wasm_runtime_free(module);
}

uint32
aot_get_plt_table_size()
{
    return get_plt_table_size();
}

