// Copyright (C) 2025 Intel Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "fuzzer_common.h"
#include <iostream>
#include <string.h>

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

bool
execute_export_functions(wasm_module_t module, wasm_module_inst_t inst)
{
    int32_t export_count = wasm_runtime_get_export_count(module);

    for (int e_i = 0; e_i < export_count; e_i++) {
        wasm_export_t export_type;

        memset(&export_type, 0, sizeof(export_type));
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

        /* build arguments with capacity reservation */
        std::vector<wasm_val_t> args;
        args.reserve(param_count); // Optimization: prevent reallocations
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
        std::vector<wasm_val_t> results(
            result_count); // Optimization: direct initialization

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

void
report_fuzzer_error(FuzzerErrorPhase phase, const char *message)
{
    const char *phase_name = "";
    switch (phase) {
        case FuzzerErrorPhase::LOADING:
            phase_name = "LOADING";
            break;
        case FuzzerErrorPhase::INSTANTIATING:
            phase_name = "INSTANTIATING";
            break;
        case FuzzerErrorPhase::COMPILING:
            phase_name = "COMPILING";
            break;
        case FuzzerErrorPhase::EXECUTION:
            phase_name = "EXECUTION";
            break;
        case FuzzerErrorPhase::CLEANUP:
            phase_name = "CLEANUP";
            break;
    }
    std::cout << "[" << phase_name << "] " << message << std::endl;
}