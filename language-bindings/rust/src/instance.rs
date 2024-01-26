/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! an instantiated module. The module is instantiated with the given imports.

#![allow(unused_variables)]

use ::core::ffi::c_char;

use wamr_sys::{wasm_module_inst_t, wasm_runtime_deinstantiate, wasm_runtime_instantiate};

use crate::{
    helper::error_buf_to_string, helper::DEFAULT_ERROR_BUF_SIZE, module::Module, value::WasmValue,
    RuntimeError,
};

#[derive(Debug)]
pub struct Instance {
    instance: wasm_module_inst_t,
}

impl Instance {
    /// instantiate a module
    ///
    /// # Error
    ///
    /// Return `RuntimeError::CompilationError` if failed.
    pub fn new(module: &Module, stack_size: u32) -> Result<Self, RuntimeError> {
        Self::new_with_args(module, stack_size, 0)
    }

    pub fn new_with_args(
        module: &Module,
        stack_size: u32,
        heap_size: u32,
    ) -> Result<Self, RuntimeError> {
        let mut error_buf = [0 as c_char; DEFAULT_ERROR_BUF_SIZE];
        let instance = unsafe {
            wasm_runtime_instantiate(
                module.get_inner_module(),
                stack_size,
                heap_size,
                error_buf.as_mut_ptr(),
                error_buf.len() as u32,
            )
        };

        if instance.is_null() {
            match error_buf.len() {
                0 => {
                    return Err(RuntimeError::CompilationError(String::from(
                        "instantiation failed",
                    )))
                }
                _ => {
                    return Err(RuntimeError::CompilationError(error_buf_to_string(
                        &error_buf,
                    )))
                }
            }
        }

        Ok(Instance { instance })
    }

    pub fn get_inner_instance(&self) -> wasm_module_inst_t {
        self.instance
    }

    pub fn execute_export_func(
        &self,
        name: &str,
        params: &Vec<WasmValue>,
    ) -> Result<WasmValue, RuntimeError> {
        unimplemented!()
    }
}

impl Drop for Instance {
    fn drop(&mut self) {
        unsafe {
            wasm_runtime_deinstantiate(self.instance);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::runtime::Runtime;

    #[test]
    fn test_instance_new() {
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

        let module = &module.unwrap();

        let instance = Instance::new_with_args(&module, 1024, 1024);
        assert_eq!(instance.is_ok(), true);

        let instance = Instance::new_with_args(&module, 1024, 0);
        assert_eq!(instance.is_ok(), true);
    }
}
