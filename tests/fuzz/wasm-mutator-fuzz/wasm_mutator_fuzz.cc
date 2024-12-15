// Copyright (C) 2019 Intel Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "bh_read_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <random>

using namespace std;

/* use std generation */
static wasm_val_t
random_gen_val(wasm_valkind_t kind)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    if (kind == WASM_I32) {
        std::uniform_int_distribution<int32_t> dis;
        return wasm_val_t{ .kind = WASM_I32, .of = { .i32 = dis(gen) } };
    }
    else if (kind == WASM_I64) {
        std::uniform_int_distribution<int64_t> dis;
        return wasm_val_t{ .kind = WASM_I64, .of = { .i64 = dis(gen) } };
    }
    else if (kind == WASM_F32) {
        std::uniform_real_distribution<float> dis;
        return wasm_val_t{ .kind = WASM_F32, .of = { .f32 = dis(gen) } };
    }
    else if (kind == WASM_F64) {
        std::uniform_real_distribution<double> dis;
        return wasm_val_t{ .kind = WASM_F64, .of = { .f64 = dis(gen) } };
    }
    else if (kind == WASM_EXTERNREF) {
        // TODO:
        return wasm_val_t{ .kind = WASM_EXTERNREF, .of = { .foreign = 0 } };
    }
    else if (kind == WASM_FUNCREF) {
        // TODO:
        return wasm_val_t{ .kind = WASM_FUNCREF, .of = { .ref = nullptr } };
    }
    else {
        assert(0);
    }
}

static bool
execute_export_functions(wasm_module_t module, wasm_module_inst_t inst)
{
    int32_t export_count = wasm_runtime_get_export_count(module);

    for (int e_i = 0; e_i < export_count; e_i++) {
        wasm_export_t export_type = { 0 };
        wasm_runtime_get_export_type(module, e_i, &export_type);

        if (export_type.kind != WASM_IMPORT_EXPORT_KIND_FUNC) {
            continue;
        }

        wasm_function_inst_t func =
            wasm_runtime_lookup_function(inst, export_type.name);
        if (!func) {
            std::cout << "Failed to lookup function: " << export_type.name
                      << std::endl;
            continue;
        }

        wasm_func_type_t func_type = export_type.u.func_type;
        uint32_t param_count = wasm_func_type_get_param_count(func_type);

        /* build arguments */
        std::vector<wasm_val_t> args;
        for (unsigned p_i = 0; p_i < param_count; p_i++) {
            wasm_valkind_t param_type =
                wasm_func_type_get_param_valkind(func_type, p_i);
            wasm_val_t arg = random_gen_val(param_type);
            args.push_back(arg);
        }

        /* build results storage */
        uint32_t result_count = wasm_func_type_get_result_count(func_type);
        std::vector<wasm_val_t> results = std::vector<wasm_val_t>(result_count);

        /* execute the function */
        wasm_exec_env_t exec_env = wasm_runtime_get_exec_env_singleton(inst);
        bool ret =
            wasm_runtime_call_wasm_a(exec_env, func, result_count,
                                     results.data(), param_count, args.data());
        if (!ret) {
            const char *exception = wasm_runtime_get_exception(inst);
            if (!exception) {
                std::cout << "Failed to execute function: " << export_type.name
                          << ". No exception info." << std::endl;
            }
            else {
                std::cout << "Failed to execute function: " << export_type.name
                          << ". " << exception << std::endl;
            }
        }

        wasm_runtime_clear_exception(inst);
    }

    return true;
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    /* libfuzzer don't allow us to modify the given Data, so we copy the data
     * here */
    std::vector<uint8_t> myData(Data, Data + Size);

    /* init runtime environment */
    wasm_runtime_init();

    char error_buf[128] = { 0 };
    wasm_module_t module =
        wasm_runtime_load((uint8_t *)myData.data(), Size, error_buf, 120);
    if (!module) {
        std::cout << "[LOADING] " << error_buf << std::endl;
        wasm_runtime_destroy();
        /* return SUCCESS because the failure has been handled */
        return 0;
    }

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 8 * 1024 * 1024, 16 * 1024 * 1024, error_buf, 120);
    if (!inst) {
        std::cout << "[INSTANTIATE] " << error_buf << std::endl;
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        /* return SUCCESS because the failure has been handled */
        return 0;
    }

    execute_export_functions(module, inst);

    std::cout << "PASS" << std::endl;

    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();
    return 0; /* Values other than 0 and -1 are reserved for future use. */
}

/* Forward-declare the libFuzzer's mutator callback. */
extern "C" size_t
LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

/* The custom mutator: */
#ifdef CUSTOM_MUTATOR
extern "C" size_t
LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize,
                        unsigned int Seed)
{
    if ((NULL != Data) && (Size > 10)) {
        int mutate_ret = -1;
        /* delete */
        if (access("./cur.wasm", 0) == 0) {
            remove("./cur.wasm");
        }

        /* 1.write data to cur.wasm */
        FILE *fwrite_fp = fopen("./cur.wasm", "wb");
        if (NULL == fwrite_fp) {
            printf("Faild to open cur.wasm file!\n");
            return 0;
        }
        fwrite(Data, sizeof(uint8_t), Size, fwrite_fp);
        fclose(fwrite_fp);
        fwrite_fp = NULL;

        /* 2.wasm-tools mutate modify cur.wasm */
        char cmd_tmp[150] = { 0 };

        /* clang-format off */
        const char *preserve_semantic = (Seed % 2) ? "--preserve-semantics" : "";
        sprintf(cmd_tmp, "wasm-tools mutate cur.wasm --seed %d -o modified.wasm %s > /dev/null 2>&1", Seed, preserve_semantic);
        /* clang-format on */
        mutate_ret = system(cmd_tmp);
        memset(cmd_tmp, 0, sizeof(cmd_tmp));

        if (mutate_ret != 0) {
            /* If source file not valid, use libfuzzer's own modifier */
            return LLVMFuzzerMutate(Data, Size, MaxSize);
        }

        /* 3.read modified file */
        int read_len = 0;
        int file_len = 0;
        int res = 0;
        uint8_t *buf = NULL;
        FILE *fread_fp = fopen("./modified.wasm", "rb");
        if (NULL == fread_fp) {
            printf("Faild to open modified.wasm file!\n");
            exit(0);
        }

        fseek(fread_fp, 0, SEEK_END); /* location to file end */
        file_len = ftell(fread_fp);   /* get file size */
        buf = (uint8_t *)malloc(file_len);

        if (NULL != buf) {
            fseek(fread_fp, 0, SEEK_SET); /* location to file start */
            read_len = fread(buf, 1, file_len, fread_fp);
            if ((read_len == file_len) && (read_len < MaxSize)) {
                /* 4.fill Data buffer */
                memcpy(Data, buf, read_len);
                res = read_len;
            }
            else {
                res = 0;
            }
        }
        else {
            res = 0;
        }

        memset(buf, 0, file_len);
        free(buf);
        fclose(fread_fp);
        fread_fp = NULL;

        return res;
    }
    else {
        if (access("./modified.wasm", 0) == 0) {
            remove("./modified.wasm");
        }
        memset(Data, 0, Size);
        Size = 0;
        return 0;
    }
}
#endif // CUSTOM_MUTATOR
