/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! an exported wasm function.

use std::ffi::CString;
use wamr_sys::{
    wasm_exec_env_t, wasm_func_get_result_count, wasm_func_get_result_types, wasm_function_inst_t,
    wasm_runtime_call_wasm, wasm_runtime_get_exception, wasm_runtime_get_exec_env_singleton,
    wasm_runtime_lookup_function, wasm_valkind_enum_WASM_F32, wasm_valkind_enum_WASM_F64,
    wasm_valkind_enum_WASM_I32, wasm_valkind_enum_WASM_I64, wasm_valkind_t,
};

use crate::{helper::exception_to_string, instance::Instance, value::WasmValue, RuntimeError};

pub struct Function {
    function: wasm_function_inst_t,
}

impl Function {
    pub fn find_export_func(instance: &Instance, name: &str) -> Result<Function, RuntimeError> {
        let name = CString::new(name).expect("CString::new failed");
        let function = unsafe {
            wasm_runtime_lookup_function(
                instance.get_inner_instance(),
                name.as_ptr(),
                std::ptr::null(),
            )
        };
        match function.is_null() {
            true => Err(RuntimeError::FunctionNotFound),
            false => Ok(Function { function }),
        }
    }

    #[allow(non_upper_case_globals)]
    fn parse_result(
        &self,
        instance: &Instance,
        result: Vec<u32>,
    ) -> Result<WasmValue, RuntimeError> {
        let result_count =
            unsafe { wasm_func_get_result_count(self.function, instance.get_inner_instance()) };
        if result_count == 0 {
            return Ok(WasmValue::Void);
        }

        let mut result_type: wasm_valkind_t = 0;
        unsafe {
            wasm_func_get_result_types(
                self.function,
                instance.get_inner_instance(),
                &mut result_type,
            );
        }

        match result_type as u32 {
            wasm_valkind_enum_WASM_I32 => Ok(WasmValue::decode_to_i32(result)),
            wasm_valkind_enum_WASM_I64 => Ok(WasmValue::decode_to_i64(result)),
            wasm_valkind_enum_WASM_F32 => Ok(WasmValue::decode_to_f32(result)),
            wasm_valkind_enum_WASM_F64 => Ok(WasmValue::decode_to_f64(result)),
            _ => Err(RuntimeError::NotImplemented),
        }
    }

    pub fn call(
        &self,
        instance: &Instance,
        params: &Vec<WasmValue>,
    ) -> Result<WasmValue, RuntimeError> {
        // params -> Vec<u32>
        let mut argv = Vec::new();
        for p in params {
            argv.append(&mut p.encode());
        }

        let argc = params.len();
        let call_result: bool;
        unsafe {
            let exec_env: wasm_exec_env_t =
                wasm_runtime_get_exec_env_singleton(instance.get_inner_instance());

            call_result =
                wasm_runtime_call_wasm(exec_env, self.function, argc as u32, argv.as_mut_ptr());
        };

        if !call_result {
            unsafe {
                let exception_c = wasm_runtime_get_exception(instance.get_inner_instance());
                return Err(RuntimeError::ExecutionError(exception_to_string(
                    exception_c,
                )));
            }
        }

        self.parse_result(instance, argv)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{module::Module, runtime::Runtime};
    use std::path::PathBuf;

    #[test]
    fn test_func_in_wasm32_unknown() {
        let _ = Runtime::new().expect("runtime init failed");

        // (module
        //   (func (export "add") (param i32 i32) (result i32)
        //     (local.get 0)
        //     (local.get 1)
        //     (i32.add)
        //   )
        // )
        let binary = vec![
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60, 0x02, 0x7f,
            0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64,
            0x00, 0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b,
        ];
        let mut binary = binary.into_iter().map(|c| c as u8).collect::<Vec<u8>>();

        let module = Module::from_buf(&mut binary);
        assert_eq!(module.is_ok(), true);
        let module = module.unwrap();

        let instance = Instance::new(&module, 1024);
        assert_eq!(instance.is_ok(), true);
        let instance: &Instance = &instance.unwrap();

        let function = Function::find_export_func(instance, "add");
        assert_eq!(function.is_ok(), true);
        let function = function.unwrap();

        let mut params: Vec<WasmValue> = Vec::new();
        params.push(WasmValue::I32(3));
        params.push(WasmValue::I32(6));

        let call_result = function.call(instance, &params);
        assert_eq!(call_result.is_ok(), true);
        assert_eq!(call_result.unwrap(), WasmValue::I32(9));
    }

    #[test]
    fn test_func_in_wasm32_wasi() {
        let _ = Runtime::new().expect("runtime init failed");

        let mut d = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        d.push("resources/test");
        d.push("hello_wasm32-wasi.wasm");
        let module = Module::from_file(d.as_path());
        assert_eq!(module.is_ok(), true);
        let module = module.unwrap();

        module.set_wasi_arg_pre_open_path(vec![String::from(".")], vec![]);

        let instance = Instance::new(&module, 1024 * 64);
        assert_eq!(instance.is_ok(), true);
        let instance: &Instance = &instance.unwrap();

        let function = Function::find_export_func(instance, "_start");
        assert_eq!(function.is_ok(), true);
        let function = function.unwrap();

        let result = function.call(instance, &vec![]);
        println!("{:?}", result);
        assert_eq!(result.is_ok(), true);
    }
}
