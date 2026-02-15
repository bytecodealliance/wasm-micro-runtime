// Copyright (C) 2025 Intel Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <vector>

#include "aot_export.h"
#include "wasm_export.h"
#include "bh_read_file.h"

static void
handle_aot_recent_error(const char *tag)
{
    const char *error = aot_get_last_error();
    if (strlen(error) == 0) {
        error = "UNKNOWN ERROR";
    }

    std::cout << tag << " " << error << std::endl;
}

static bool
is_supported_val_kind(wasm_valkind_t kind)
{
    return kind == WASM_I32 || kind == WASM_I64 || kind == WASM_F32
           || kind == WASM_F64 || kind == WASM_EXTERNREF
           || kind == WASM_FUNCREF;
}

static wasm_val_t
pre_defined_val(wasm_valkind_t kind)
{
    if (kind == WASM_I32) {
        return wasm_val_t{ .kind = WASM_I32, .of = { .i32 = 2025 } };
    }
    else if (kind == WASM_I64) {
        return wasm_val_t{ .kind = WASM_I64, .of = { .i64 = 168 } };
    }
    else if (kind == WASM_F32) {
        return wasm_val_t{ .kind = WASM_F32, .of = { .f32 = 3.14159f } };
    }
    else if (kind == WASM_F64) {
        return wasm_val_t{ .kind = WASM_F64, .of = { .f64 = 2.71828 } };
    }
    else if (kind == WASM_EXTERNREF) {
        return wasm_val_t{ .kind = WASM_EXTERNREF,
                           .of = { .foreign = 0xabcddead } };
    }
    // because aft is_supported_val_kind() check, so we can safely return as
    // WASM_FUNCREF
    else {
        return wasm_val_t{ .kind = WASM_FUNCREF, .of = { .ref = nullptr } };
    }
}
void
print_execution_args(const wasm_export_t &export_type,
                     const std::vector<wasm_val_t> &args, unsigned param_count)
{
    std::cout << "[EXECUTION] " << export_type.name << "(";
    for (unsigned p_i = 0; p_i < param_count; p_i++) {
        if (p_i != 0) {
            std::cout << ", ";
        }

        switch (args[p_i].kind) {
            case WASM_I32:
                std::cout << "i32:" << args[p_i].of.i32;
                break;
            case WASM_I64:
                std::cout << "i64:" << args[p_i].of.i64;
                break;
            case WASM_F32:
                std::cout << "f32:" << args[p_i].of.f32;
                break;
            case WASM_F64:
                std::cout << "f64:" << args[p_i].of.f64;
                break;
            case WASM_EXTERNREF:
                std::cout << "externref:" << args[p_i].of.foreign;
                break;
            default:
                // because aft is_supported_val_kind() check, so we can safely
                // return as WASM_FUNCREF
                std::cout << "funcref:" << args[p_i].of.ref;
                break;
        }
    }
    std::cout << ")" << std::endl;
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

            if (!is_supported_val_kind(param_type)) {
                std::cout
                    << "Bypass execution because of unsupported value kind: "
                    << param_type << std::endl;
                return true;
            }

            wasm_val_t arg = pre_defined_val(param_type);
            args.push_back(arg);
        }

        /* build results storage */
        uint32_t result_count = wasm_func_type_get_result_count(func_type);
        std::vector<wasm_val_t> results = std::vector<wasm_val_t>(result_count);

        print_execution_args(export_type, args, param_count);

        /* execute the function */
        wasm_exec_env_t exec_env = wasm_runtime_get_exec_env_singleton(inst);
        if (!exec_env) {
            std::cout << "Failed to get exec env" << std::endl;
            return false;
        }

        bool ret =
            wasm_runtime_call_wasm_a(exec_env, func, result_count,
                                     results.data(), param_count, args.data());
        if (!ret) {
            const char *exception = wasm_runtime_get_exception(inst);
            if (!exception) {
                std::cout << "[EXECUTION] " << export_type.name
                          << "() failed. No exception info." << std::endl;
            }
            else {
                std::cout << "[EXECUTION] " << export_type.name << "() failed. "
                          << exception << std::endl;
            }
        }

        wasm_runtime_clear_exception(inst);
    }

    return true;
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    wasm_module_t module = NULL;
    char error_buf[128] = { 0 };
    AOTCompOption option = { 0 };
    aot_comp_data_t comp_data = NULL;
    aot_comp_context_t comp_ctx = NULL;
    uint8 *aot_file_buf = NULL;
    uint32 aot_file_size = 0;
    wasm_module_t aot_module = NULL;
    wasm_module_inst_t inst = NULL;

    /* libfuzzer don't allow to modify the given Data, so make a copy here */
    std::vector<uint8_t> myData(Data, Data + Size);

    if (Size >= 4
        && get_package_type(myData.data(), Size) != Wasm_Module_Bytecode) {
        printf("Invalid wasm file: magic header not detected\n");
        return 0;
    }

    wasm_runtime_init();

    module = wasm_runtime_load((uint8_t *)myData.data(), Size, error_buf, 120);
    if (!module) {
        std::cout << "[LOADING] " << error_buf << std::endl;
        goto DESTROY_RUNTIME;
    }

    // TODO: target_arch and other fields
    option.target_arch = "x86_64";
    option.target_abi = "gnu";
    option.enable_bulk_memory = true;
    option.enable_thread_mgr = true;
    option.enable_tail_call = true;
    option.enable_simd = true;
    option.enable_ref_types = true;
    option.enable_gc = true;
    option.aux_stack_frame_type = AOT_STACK_FRAME_TYPE_STANDARD;

    comp_data =
        aot_create_comp_data(module, option.target_arch, option.enable_gc);
    if (!comp_data) {
        handle_aot_recent_error("[CREATING comp_data]");
        goto UNLOAD_MODULE;
    }

    comp_ctx = aot_create_comp_context(comp_data, &option);
    if (!comp_ctx) {
        handle_aot_recent_error("[CREATING comp_context]");
        goto DESTROY_COMP_DATA;
    }

    if (!aot_compile_wasm(comp_ctx)) {
        handle_aot_recent_error("[COMPILING]");
        goto DESTROY_COMP_CTX;
    }

    aot_file_buf = aot_emit_aot_file_buf(comp_ctx, comp_data, &aot_file_size);
    if (!aot_file_buf) {
        handle_aot_recent_error("[EMITTING AOT FILE]");
        goto DESTROY_COMP_CTX;
    }

    aot_module =
        wasm_runtime_load(aot_file_buf, aot_file_size, error_buf, 128);
    if (!aot_module) {
        std::cout << "[LOADING AOT MODULE] " << error_buf << std::endl;
        goto RELEASE_AOT_FILE_BUF;
    }

    inst = wasm_runtime_instantiate(aot_module, 1024*8, 0, error_buf, 128);
    if (!inst) {
        std::cout << "[INSTANTIATING AOT MODULE] " << error_buf << std::endl;
        goto UNLOAD_AOT_MODULE;
    }

    execute_export_functions(module, inst);

DEINSTANTIZE_AOT_MODULE:
    wasm_runtime_deinstantiate(inst);
UNLOAD_AOT_MODULE:
    wasm_runtime_unload(aot_module);
RELEASE_AOT_FILE_BUF:
    wasm_runtime_free(aot_file_buf);
DESTROY_COMP_CTX:
    aot_destroy_comp_context(comp_ctx);
DESTROY_COMP_DATA:
    aot_destroy_comp_data(comp_data);
UNLOAD_MODULE:
    wasm_runtime_unload(module);
DESTROY_RUNTIME:
    wasm_runtime_destroy();

    /* Values other than 0 and -1 are reserved for future use. */
    return 0;
}
