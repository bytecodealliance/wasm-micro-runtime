// Copyright (C) 2019 Intel Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "wasm_export.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "../common/fuzzer_common.h"

using namespace std;

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    /* wasm_runtime_load may modify the input buffer in-place, 
     * so we must work on a copy to  avoid overwriting libFuzzer's const input */
    std::vector<uint8_t> data_copy(Data, Data + Size);

    /* init runtime environment */
    wasm_runtime_init();

    char error_buf[ERROR_BUF_SIZE] = { 0 };
    wasm_module_t module = wasm_runtime_load(data_copy.data(), Size,
                                             error_buf, MAX_ERROR_BUF_SIZE);
    if (!module) {
        std::cout << "[LOADING] " << error_buf << std::endl;
        wasm_runtime_destroy();
        /* return SUCCESS because the failure has been handled */
        return 0;
    }

    wasm_module_inst_t inst =
        wasm_runtime_instantiate(module, 8 * 1024 * 1024, 16 * 1024 * 1024,
                                 error_buf, MAX_ERROR_BUF_SIZE);
    if (!inst) {
        std::cout << "[INSTANTIATE] " << error_buf << std::endl;
        wasm_runtime_unload(module);
        wasm_runtime_destroy();
        /* return SUCCESS because the failure has been handled */
        return 0;
    }

    execute_export_functions(module, inst);

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

        /* 3.read modified file using RAII container */
        FILE *fread_fp = fopen("./modified.wasm", "rb");
        if (NULL == fread_fp) {
            printf("Faild to open modified.wasm file!\n");
            exit(0);
        }

        fseek(fread_fp, 0, SEEK_END);    /* location to file end */
        long file_len = ftell(fread_fp); /* get file size */
        fseek(fread_fp, 0, SEEK_SET);    /* location to file start */

        std::vector<uint8_t> buf(file_len);
        size_t read_len = fread(buf.data(), 1, file_len, fread_fp);
        fclose(fread_fp);
        fread_fp = NULL;

        int res = 0;
        if (read_len == static_cast<size_t>(file_len) && read_len < MaxSize) {
            /* 4.fill Data buffer */
            memcpy(Data, buf.data(), read_len);
            res = static_cast<int>(read_len);
        }

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
