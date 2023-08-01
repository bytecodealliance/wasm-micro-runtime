/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unordered_map>

#include "Enclave_t.h"
#include "wasm_export.h"
#include "bh_platform.h"

#if WASM_ENABLE_LIB_RATS != 0
#include <openssl/sha.h>
#endif

extern "C" {
typedef int (*os_print_function_t)(const char *message);
extern void
os_set_print_function(os_print_function_t pf);

int
enclave_print(const char *message)
{
    int bytes_written = 0;

    if (SGX_SUCCESS != ocall_print(&bytes_written, message))
        return 0;

    return bytes_written;
}
}

typedef struct EnclaveModule {
    wasm_module_t module;
    uint8 *wasm_file;
    uint32 wasm_file_size;
    char *wasi_arg_buf;
    char **wasi_dir_list;
    uint32 wasi_dir_list_size;
    char **wasi_env_list;
    uint32 wasi_env_list_size;
    char **wasi_addr_pool_list;
    uint32 wasi_addr_pool_list_size;
    char **wasi_argv;
    uint32 wasi_argc;
    bool is_xip_file;
    uint32 total_size_mapped;
#if WASM_ENABLE_LIB_RATS != 0
    char module_hash[SHA256_DIGEST_LENGTH];
    struct EnclaveModule *next;
#endif
} EnclaveModule;

#if WASM_ENABLE_LIB_RATS != 0
static EnclaveModule *enclave_module_list = NULL;
static korp_mutex enclave_module_list_lock = OS_THREAD_MUTEX_INITIALIZER;
#endif

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
static char global_heap_buf[WASM_GLOBAL_HEAP_SIZE] = { 0 };
#endif

class PointerManager
{
  public:
    bool add(void *ptr, uint16_t *idx_output)
    {
        bool success = false;
        pthread_rwlock_wrlock(&mRwlock);
        for (uint16_t i = mCurrentIdx; i < std::numeric_limits<uint16_t>::max();
             i++) {
            if (mMap.count(i) == 0) {
                mMap[i] = ptr;
                *idx_output = i;
                success = true;
                mCurrentIdx++;
                break;
            }
        }
        pthread_rwlock_unlock(&mRwlock);
        return success;
    }

    void *get(uint16_t idx)
    {
        void *ptr = nullptr;
        pthread_rwlock_rdlock(&mRwlock);
        if (mMap.count(idx)) {
            ptr = mMap[idx];
        }
        pthread_rwlock_unlock(&mRwlock);
        return ptr;
    }

    bool remove(uint16_t idx)
    {
        bool success = false;
        pthread_rwlock_wrlock(&mRwlock);
        if (mMap.erase(idx) == 1) {
            success = true;
            if (idx < mCurrentIdx) {
                mCurrentIdx = idx;
            }
        }
        pthread_rwlock_unlock(&mRwlock);
        return success;
    }

  private:
    std::unordered_map<uint16_t, void *> mMap;
    uint16_t mCurrentIdx = 0;
    pthread_rwlock_t mRwlock = PTHREAD_RWLOCK_INITIALIZER;
};
PointerManager gEnclaveModuleMgr, gWasmModuleInstMgr;

/// @brief Deep copy an array of char string
/// @param SrcCStrArray Source array of char string
/// @param DstCStrArray Dest. array of char string, if null, then
/// automatically malloced
/// @param Length Array length. If 0, cause fail
/// @return \p DstCStrArray. If failed, return null, char string elements will
/// be auto free-ed, if \p DstCStrArray is feed with null, it will be auto
/// free-ed before return, but if \p DstCStrArray is input with not null, it's
/// caller duty to free \p DstCStrArray. If success, user need to free all char
/// string elements before free this \p DstCStrArray.
char **
DeepCopyCStrArray(char **SrcCStrArray, char **DstCStrArray, size_t Length)
{
    if ((SrcCStrArray == nullptr) or (Length == 0)) {
        return nullptr;
    }
    bool DstIsNull = false;
    if (DstCStrArray == nullptr) {
        DstIsNull = true;
        size_t AllocSize = Length * sizeof(char *);
        if ((AllocSize <= Length /* Int Overflow */)
            or (AllocSize
                > std::numeric_limits<uint32_t>::max() /* Too large */)
            or (!(DstCStrArray = (char **)wasm_runtime_malloc(
                      AllocSize)) /* Malloc Fail */)) {
            return nullptr;
        }
    }

    for (size_t i = 0; i < Length; i++) {
        size_t AllocSize = (strlen(SrcCStrArray[i]) + 1) * sizeof(char);
        if ((AllocSize > std::numeric_limits<uint32_t>::max() /* Too large */)
            or (!(DstCStrArray[i] = (char *)wasm_runtime_malloc(
                      AllocSize)) /* Malloc Fail */)) {
            for (size_t j = 0; j < i; j++) {
                wasm_runtime_free(DstCStrArray[j]);
                DstCStrArray[j] = nullptr;
            }
            if (DstIsNull) {
                // DstCStrArray is allocated by myself, free it
                wasm_runtime_free(DstCStrArray);
                DstCStrArray = nullptr;
            }
            return nullptr;
        }

        strcpy(DstCStrArray[i], SrcCStrArray[i]);
    }

    return DstCStrArray;
}

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

bool
ecall_handle_cmd_init_runtime(uint32_t max_thread_num)
{
    bool ret = false;
    RuntimeInitArgs init_args;

    os_set_print_function(enclave_print);

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.max_thread_num = max_thread_num;

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
#else
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
#endif

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERROR("Init runtime environment failed.\n");
        ret = false;
        goto exit;
    }

    ret = true;
    LOG_VERBOSE("Init runtime environment success.\n");
exit:
    return ret;
}

void
ecall_handle_cmd_destroy_runtime()
{
    wasm_runtime_destroy();

    LOG_VERBOSE("Destroy runtime success.\n");
}

static uint8 *
align_ptr(const uint8 *p, uint32 b)
{
    uintptr_t v = (uintptr_t)p;
    uintptr_t m = b - 1;
    return (uint8 *)((v + m) & ~m);
}

#define AOT_SECTION_TYPE_TARGET_INFO 0
#define AOT_SECTION_TYPE_SIGANATURE 6
#define E_TYPE_XIP 4

#define CHECK_BUF(buf, buf_end, length)                      \
    do {                                                     \
        if ((uintptr_t)buf + length < (uintptr_t)buf         \
            || (uintptr_t)buf + length > (uintptr_t)buf_end) \
            return false;                                    \
    } while (0)

#define read_uint16(p, p_end, res)                 \
    do {                                           \
        p = (uint8 *)align_ptr(p, sizeof(uint16)); \
        CHECK_BUF(p, p_end, sizeof(uint16));       \
        res = *(uint16 *)p;                        \
        p += sizeof(uint16);                       \
    } while (0)

#define read_uint32(p, p_end, res)                 \
    do {                                           \
        p = (uint8 *)align_ptr(p, sizeof(uint32)); \
        CHECK_BUF(p, p_end, sizeof(uint32));       \
        res = *(uint32 *)p;                        \
        p += sizeof(uint32);                       \
    } while (0)

static bool
is_xip_file(const uint8 *buf, uint32 size)
{
    const uint8 *p = buf, *p_end = buf + size;
    uint32 section_type, section_size;
    uint16 e_type;

    if (get_package_type(buf, size) != Wasm_Module_AoT)
        return false;

    CHECK_BUF(p, p_end, 8);
    p += 8;
    while (p < p_end) {
        read_uint32(p, p_end, section_type);
        read_uint32(p, p_end, section_size);
        CHECK_BUF(p, p_end, section_size);

        if (section_type == AOT_SECTION_TYPE_TARGET_INFO) {
            p += 4;
            read_uint16(p, p_end, e_type);
            return (e_type == E_TYPE_XIP) ? true : false;
        }
        else if (section_type >= AOT_SECTION_TYPE_SIGANATURE) {
            return false;
        }
        p += section_size;
    }

    return false;
}

bool
ecall_handle_cmd_load_module(char *wasm_file, uint32_t wasm_file_size,
                             char *error_buf, uint32_t error_buf_size,
                             uint16_t *enclave_module_idx)
{
    bool ret = false;
    uint64 total_size = sizeof(EnclaveModule) + (uint64)wasm_file_size;
    EnclaveModule *enclave_module;

    if (wasm_file == nullptr or enclave_module_idx == nullptr) {
        ret = false;
        goto exit;
    }

    if (!is_xip_file((uint8 *)wasm_file, wasm_file_size)) {
        if (total_size >= UINT32_MAX
            || !(enclave_module = (EnclaveModule *)wasm_runtime_malloc(
                     (uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "WASM module load failed: "
                          "allocate memory failed.");
            ret = false;
            goto exit;
        }
        memset(enclave_module, 0, (uint32)total_size);
    }
    else {
        int map_prot = MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_EXEC;
        int map_flags = MMAP_MAP_NONE;

        if (total_size >= UINT32_MAX
            || !(enclave_module = (EnclaveModule *)os_mmap(
                     NULL, (uint32)total_size, map_prot, map_flags))) {
            set_error_buf(error_buf, error_buf_size,
                          "WASM module load failed: mmap memory failed.");
            ret = false;
            goto exit;
        }
        memset(enclave_module, 0, (uint32)total_size);
        enclave_module->is_xip_file = true;
        enclave_module->total_size_mapped = (uint32)total_size;
    }

    enclave_module->wasm_file = (uint8 *)enclave_module + sizeof(EnclaveModule);
    bh_memcpy_s(enclave_module->wasm_file, wasm_file_size, wasm_file,
                wasm_file_size);

    if (!(enclave_module->module =
              wasm_runtime_load(enclave_module->wasm_file, wasm_file_size,
                                error_buf, error_buf_size))) {
        if (!enclave_module->is_xip_file)
            wasm_runtime_free(enclave_module);
        else
            os_munmap(enclave_module, (uint32)total_size);
        ret = false;
        goto exit;
    }

    if (!gEnclaveModuleMgr.add(enclave_module, enclave_module_idx)) {
        wasm_runtime_unload(enclave_module->module);
        if (!enclave_module->is_xip_file)
            wasm_runtime_free(enclave_module);
        else
            os_munmap(enclave_module, (uint32)total_size);
        ret = false;
        goto exit;
    }

#if WASM_ENABLE_LIB_RATS != 0
    /* Calculate the module hash */
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, wasm_file, wasm_file_size);
    SHA256_Final((unsigned char *)enclave_module->module_hash, &sha256);

    /* Insert enclave module to enclave module list */
    os_mutex_lock(&enclave_module_list_lock);
    enclave_module->next = enclave_module_list;
    enclave_module_list = enclave_module;
    os_mutex_unlock(&enclave_module_list_lock);
#endif

    LOG_VERBOSE("Load module success.\n");
    ret = true;
exit:
    return ret;
}

bool
ecall_handle_cmd_unload_module(uint16_t enclave_module_idx)
{
    bool ret = false;
    EnclaveModule *enclave_module =
        (EnclaveModule *)gEnclaveModuleMgr.get(enclave_module_idx);

    if (enclave_module == nullptr) {
        ret = false;
        goto exit;
    }

#if WASM_ENABLE_LIB_RATS != 0
    /* Remove enclave module from enclave module list */
    os_mutex_lock(&enclave_module_list_lock);

    EnclaveModule *node_prev = NULL;
    EnclaveModule *node = enclave_module_list;

    while (node && node != enclave_module) {
        node_prev = node;
        node = node->next;
    }
    bh_assert(node == enclave_module);

    if (!node_prev)
        enclave_module_list = node->next;
    else
        node_prev->next = node->next;

    os_mutex_unlock(&enclave_module_list_lock);
#endif

    /* Destroy enclave module resources */
    if (enclave_module->wasi_arg_buf)
        wasm_runtime_free(enclave_module->wasi_arg_buf);

    wasm_runtime_unload(enclave_module->module);
    if (!enclave_module->is_xip_file)
        wasm_runtime_free(enclave_module);
    else
        os_munmap(enclave_module, enclave_module->total_size_mapped);

    if (!gEnclaveModuleMgr.remove(enclave_module_idx)) {
        ret = false;
        goto exit;
    }
    LOG_VERBOSE("Unload module success.\n");
    ret = true;
exit:
    return ret;
}

#if WASM_ENABLE_LIB_RATS != 0
char *
wasm_runtime_get_module_hash(wasm_module_t module)
{
    EnclaveModule *enclave_module;
    char *module_hash = NULL;

    os_mutex_lock(&enclave_module_list_lock);

    enclave_module = enclave_module_list;
    while (enclave_module) {
        if (enclave_module->module == module) {
            module_hash = enclave_module->module_hash;
            break;
        }
        enclave_module = enclave_module->next;
    }
    os_mutex_unlock(&enclave_module_list_lock);

    return module_hash;
}
#endif

bool
ecall_handle_cmd_instantiate_module(uint16_t enclave_module_idx,
                                    uint32_t stack_size, uint32_t heap_size,
                                    char *error_buf, uint32_t error_buf_size,
                                    uint16_t *wasm_module_inst_idx)
{
    bool ret = false;
    EnclaveModule *enclave_module = nullptr;
    wasm_module_inst_t module_inst;

    if ((wasm_module_inst_idx == nullptr)
        or !(enclave_module =
                 (EnclaveModule *)gEnclaveModuleMgr.get(enclave_module_idx))) {
        ret = false;
        goto exit;
    }

    if (!(module_inst =
              wasm_runtime_instantiate(enclave_module->module, stack_size,
                                       heap_size, error_buf, error_buf_size))) {
        ret = false;
        goto exit;
    }

    if (!gWasmModuleInstMgr.add(module_inst, wasm_module_inst_idx)) {
        ret = false;
        wasm_runtime_deinstantiate(module_inst);
        goto exit;
    }

    LOG_VERBOSE("Instantiate module success.\n");
    ret = true;
exit:
    return ret;
}

bool
ecall_handle_cmd_deinstantiate_module(uint16_t wasm_module_inst_idx)
{
    bool ret = false;
    wasm_module_inst_t module_inst =
        (wasm_module_inst_t)gWasmModuleInstMgr.get(wasm_module_inst_idx);

    if (module_inst == nullptr) {
        ret = false;
        goto exit;
    }

    wasm_runtime_deinstantiate(module_inst);

    if (!gWasmModuleInstMgr.remove(wasm_module_inst_idx)) {
        ret = false;
        goto exit;
    }
    LOG_VERBOSE("Deinstantiate module success.\n");
    ret = true;
exit:
    return ret;
}

bool
ecall_handle_cmd_get_exception(uint16_t wasm_module_inst_idx, char *exception,
                               uint32_t exception_size)
{
    bool ret = false;
    wasm_module_inst_t module_inst = nullptr;
    const char *exception1;

    if ((exception == nullptr)
        or (!(module_inst = (wasm_module_inst_t)gWasmModuleInstMgr.get(
                  wasm_module_inst_idx)))) {
        ret = false;
        goto exit;
    }

    if ((exception1 = wasm_runtime_get_exception(module_inst))) {
        snprintf(exception, exception_size, "%s", exception1);
        ret = true;
    }
    else {
        ret = false;
    }
exit:
    return ret;
}

bool
ecall_handle_cmd_exec_app_main(uint16_t wasm_module_inst_idx, char **u_app_argv,
                               uint32_t app_argc)
{
    bool ret = false;
    wasm_module_inst_t module_inst = nullptr;
    char **app_argv = NULL;
    uint64 total_size;
    int32 i;

    if ((u_app_argv == nullptr) or (app_argc == 0)
        or (!(module_inst = (wasm_module_inst_t)gWasmModuleInstMgr.get(
                  wasm_module_inst_idx)))) {
        ret = false;
        goto exit;
    }
    bh_assert(app_argc >= 1);

    total_size = sizeof(char *) * (app_argc > 2 ? (uint64)app_argc : 2);

    if (total_size >= UINT32_MAX
        || !(app_argv = (char **)wasm_runtime_malloc(total_size))) {
        wasm_runtime_set_exception(module_inst, "allocate memory failed.");
        ret = false;
        goto exit;
    }

    if (!DeepCopyCStrArray(u_app_argv, app_argv, app_argc)) {
        wasm_runtime_free(app_argv);
        app_argv = nullptr;
        ret = false;
        goto exit;
    }

    wasm_application_execute_main(module_inst, app_argc - 1, app_argv + 1);

    for (uint32 i = 0; i < app_argc; i++) {
        wasm_runtime_free(app_argv[i]);
        app_argv[i] = nullptr;
    }
    wasm_runtime_free(app_argv);
    app_argv = nullptr;
    ret = true;
exit:
    return ret;
}

bool
ecall_handle_cmd_exec_app_func(uint16_t wasm_module_inst_idx,
                               const char *func_name, char **u_app_argv,
                               uint32_t app_argc)
{
    bool ret = false;
    wasm_module_inst_t module_inst = nullptr;
    char **app_argv = NULL;
    uint64 total_size;
    int32 i, func_name_len;

    if ((func_name == nullptr) or ((u_app_argv == nullptr) xor (app_argc == 0))
        or (!(module_inst = (wasm_module_inst_t)gWasmModuleInstMgr.get(
                  wasm_module_inst_idx)))) {
        ret = false;
        goto exit;
    }
    func_name_len = strlen(func_name);

    total_size = sizeof(char *) * (app_argc > 2 ? (uint64)app_argc : 2);

    if (total_size >= UINT32_MAX
        || !(app_argv = (char **)wasm_runtime_malloc(total_size))) {
        wasm_runtime_set_exception(module_inst, "allocate memory failed.");
        ret = false;
        goto exit;
    }

    /* if app_argc is 0, no need deep copy */
    if (app_argc != 0 and !DeepCopyCStrArray(u_app_argv, app_argv, app_argc)) {
        wasm_runtime_free(app_argv);
        app_argv = nullptr;
        ret = false;
        goto exit;
    }

    wasm_application_execute_func(module_inst, func_name, app_argc, app_argv);

    /* if app_argc is 0, no deep copy need to free */
    for (int32 i = 0; i < app_argc; i++) {
        wasm_runtime_free(app_argv[i]);
        app_argv[i] = nullptr;
    }
    if (app_argv) {
        wasm_runtime_free(app_argv);
        app_argv = nullptr;
    }
    ret = true;
exit:
    return ret;
}

void
ecall_handle_cmd_set_log_level(int log_level)
{
#if WASM_ENABLE_LOG != 0
    LOG_VERBOSE("Set log verbose level to %d.\n", log_level);
    bh_log_set_verbose_level(log_level);
#endif
}

#ifndef SGX_DISABLE_WASI
bool
ecall_handle_cmd_set_wasi_args(uint16_t enclave_module_idx,
                               const char **dir_list, uint32_t dir_list_size,
                               const char **env_list, uint32_t env_list_size,
                               int stdinfd, int stdoutfd, int stderrfd,
                               char **wasi_argv, uint32_t wasi_argc,
                               const char **addr_pool_list,
                               uint32_t addr_pool_list_size)
{
    bool ret = false;
    EnclaveModule *enclave_module = nullptr;
    char *p, *p1;
    uint64 total_size = 0;
    int32 i, str_len;

    if ((!dir_list and dir_list_size != 0) or (!env_list and env_list_size != 0)
        or (!wasi_argv and wasi_argc != 0)
        or (!addr_pool_list and addr_pool_list_size != 0)
        or !(enclave_module =
                 (EnclaveModule *)gEnclaveModuleMgr.get(enclave_module_idx))) {
        ret = false;
        goto exit;
    }

    total_size += sizeof(char *) * (uint64)dir_list_size
                  + sizeof(char *) * (uint64)env_list_size
                  + sizeof(char *) * (uint64)addr_pool_list_size
                  + sizeof(char *) * (uint64)wasi_argc;

    for (i = 0; i < dir_list_size; i++) {
        total_size += strlen(dir_list[i]) + 1;
    }

    for (i = 0; i < env_list_size; i++) {
        total_size += strlen(env_list[i]) + 1;
    }

    for (i = 0; i < addr_pool_list_size; i++) {
        total_size += strlen(addr_pool_list[i]) + 1;
    }

    for (i = 0; i < wasi_argc; i++) {
        total_size += strlen(wasi_argv[i]) + 1;
    }

    if (total_size >= UINT32_MAX
        || !(enclave_module->wasi_arg_buf = p =
                 (char *)wasm_runtime_malloc((uint32)total_size))) {
        ret = false;
        goto exit;
    }

    p1 = p + sizeof(char *) * dir_list_size + sizeof(char *) * env_list_size
         + sizeof(char *) * addr_pool_list_size + sizeof(char *) * wasi_argc;

    if (dir_list_size > 0) {
        enclave_module->wasi_dir_list = (char **)p;
        enclave_module->wasi_dir_list_size = dir_list_size;
        for (i = 0; i < dir_list_size; i++) {
            enclave_module->wasi_dir_list[i] = p1;
            str_len = strlen(dir_list[i]);
            bh_memcpy_s(p1, str_len + 1, dir_list[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * dir_list_size;
    }

    if (env_list_size > 0) {
        enclave_module->wasi_env_list = (char **)p;
        enclave_module->wasi_env_list_size = env_list_size;
        for (i = 0; i < env_list_size; i++) {
            enclave_module->wasi_env_list[i] = p1;
            str_len = strlen(env_list[i]);
            bh_memcpy_s(p1, str_len + 1, env_list[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * env_list_size;
    }

    if (addr_pool_list_size > 0) {
        enclave_module->wasi_addr_pool_list = (char **)p;
        enclave_module->wasi_addr_pool_list_size = addr_pool_list_size;
        for (i = 0; i < addr_pool_list_size; i++) {
            enclave_module->wasi_addr_pool_list[i] = p1;
            str_len = strlen(addr_pool_list[i]);
            bh_memcpy_s(p1, str_len + 1, addr_pool_list[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * addr_pool_list_size;
    }

    if (wasi_argc > 0) {
        enclave_module->wasi_argv = (char **)p;
        enclave_module->wasi_argc = wasi_argc;
        for (i = 0; i < wasi_argc; i++) {
            enclave_module->wasi_argv[i] = p1;
            str_len = strlen(wasi_argv[i]);
            bh_memcpy_s(p1, str_len + 1, wasi_argv[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * wasi_argc;
    }

    wasm_runtime_set_wasi_args_ex(
        enclave_module->module, (const char **)enclave_module->wasi_dir_list,
        dir_list_size, NULL, 0, (const char **)enclave_module->wasi_env_list,
        env_list_size, enclave_module->wasi_argv, enclave_module->wasi_argc,
        (stdinfd != -1) ? stdinfd : 0, (stdoutfd != -1) ? stdoutfd : 1,
        (stderrfd != -1) ? stderrfd : 2);

    wasm_runtime_set_wasi_addr_pool(
        enclave_module->module,
        (const char **)enclave_module->wasi_addr_pool_list,
        addr_pool_list_size);

    ret = true;
exit:
    return ret;
}
#else
bool
ecall_handle_cmd_set_wasi_args(uint16_t enclave_module_idx,
                               const char **dir_list, uint32_t dir_list_size,
                               const char **env_list, uint32_t env_list_size,
                               int stdinfd, int stdoutfd, int stderrfd,
                               char **wasi_argv, uint32_t wasi_argc,
                               const char **addr_pool_list,
                               uint32_t addr_pool_list_size)
{
    return true;
}
#endif /* end of SGX_DISABLE_WASI */

void
ecall_handle_cmd_get_version(uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    if (major and minor and patch) {
        wasm_runtime_get_version(major, minor, patch);
    }
}

#if WASM_ENABLE_STATIC_PGO != 0
uint32_t
ecall_handle_cmd_get_pgo_prof_buf_size(uint16_t wasm_module_inst_idx)
{
    wasm_module_inst_t module_inst = nullptr;
    uint32 buf_len;

    if ((module_inst =
             (wasm_module_inst_t)gWasmModuleInstMgr.get(wasm_module_inst_idx))
        == nullptr) {
        return 0;
    }

    buf_len = wasm_runtime_get_pgo_prof_data_size(module_inst);
    return buf_len;
}

uint32_t
ecall_handle_cmd_get_pgo_prof_buf_data(uint16_t wasm_module_inst_idx, char *buf,
                                       uint32_t len)
{
    wasm_module_inst_t module_inst = nullptr;
    uint32 bytes_dumped;

    if ((buf == nullptr)
        or !(module_inst = (wasm_module_inst_t)gWasmModuleInstMgr.get(
                 wasm_module_inst_idx))) {
        return 0;
    }

    bytes_dumped =
        wasm_runtime_dump_pgo_prof_data_to_buf(module_inst, buf, len);
    return bytes_dumped;
}
#else
uint32_t
ecall_handle_cmd_get_pgo_prof_buf_size(uint16_t wasm_module_inst_idx)
{
    return 0;
}

uint32_t
ecall_handle_cmd_get_pgo_prof_buf_data(uint16_t wasm_module_inst_idx, char *buf,
                                       uint32_t len)
{
    return 0;
}
#endif

void
ecall_iwasm_main(uint8_t *wasm_file_buf, uint32_t wasm_file_size)
{
    if (wasm_file_buf == nullptr) {
        return;
    }

    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
    const char *exception;

    os_set_print_function(enclave_print);

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
#else
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
#endif

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        enclave_print("Init runtime environment failed.");
        enclave_print("\n");
        return;
    }

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        enclave_print(error_buf);
        enclave_print("\n");
        goto fail1;
    }

    /* instantiate the module */
    if (!(wasm_module_inst =
              wasm_runtime_instantiate(wasm_module, 16 * 1024, 16 * 1024,
                                       error_buf, sizeof(error_buf)))) {
        enclave_print(error_buf);
        enclave_print("\n");
        goto fail2;
    }

    /* execute the main function of wasm app */
    wasm_application_execute_main(wasm_module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
        enclave_print(exception);
        enclave_print("\n");
    }

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
}
