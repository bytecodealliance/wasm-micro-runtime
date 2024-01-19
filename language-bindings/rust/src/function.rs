//! a wasm function. only exported.

use std::ffi::CString;
use wamr_sys::{
    wasm_exec_env_t, wasm_func_get_result_types, wasm_function_inst_t, wasm_runtime_call_wasm,
    wasm_runtime_get_exception, wasm_runtime_get_exec_env_singleton, wasm_runtime_lookup_function,
    wasm_valkind_enum_WASM_F32, wasm_valkind_enum_WASM_F64, wasm_valkind_enum_WASM_I32,
    wasm_valkind_enum_WASM_I64, wasm_valkind_t,
};

use crate::{helper::exception_to_string, instance::Instance, value::WasmValue, RuntimeError};

pub struct Function {
    function: wasm_function_inst_t,
}

impl Function {
    pub fn look_up_func(instance: &Instance, name: &str) -> Result<Function, RuntimeError> {
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
            _ => Err(RuntimeError::InvalidResult),
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

            println!("🎇 b4 wasm_runtime_call_wasm");
            println!("{:?}", argc);
            println!("{:?}", argv);
            println!("{:?}", argv.as_mut_ptr());

            call_result =
                wasm_runtime_call_wasm(exec_env, self.function, argc as u32, argv.as_mut_ptr());
        };

        println!("🎇 {call_result:?}");

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

    #[test]
    fn test_look_up_func() {
        // (module
        //   (func (export "add") (param i32 i32) (result i32)
        //     (local.get 0)
        //     (local.get 1)
        //     (i32.add)
        //   )
        // )
        let binary = [
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60, 0x02, 0x7f,
            0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64,
            0x00, 0x00, 0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b,
        ];
        let mut binary = binary.map(|c| c as u8);

        let runtime = Runtime::builder().run_as_interpreter().build();
        assert_eq!(runtime.is_ok(), true);

        let runtime = runtime.unwrap();
        let module = Module::from_buf(&runtime, &mut binary);
        assert_eq!(module.is_ok(), true);

        let instance = Instance::new(&module.unwrap(), 1024);
        assert_eq!(instance.is_ok(), true);

        let instance: &Instance = &instance.unwrap();
        let function = Function::look_up_func(instance, "add");
        assert_eq!(function.is_ok(), true);

        let mut params: Vec<WasmValue> = Vec::new();
        params.push(WasmValue::I32(3));
        params.push(WasmValue::I32(6));

        let call_result = function.unwrap().call(instance, &params);
        println!("--> {call_result:?}");
        assert_eq!(call_result.is_ok(), true);
        assert_eq!(call_result.unwrap(), WasmValue::I32(9));
    }
}
