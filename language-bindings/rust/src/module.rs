/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! .wasm compiled, in-memory representation

use crate::{
    helper::error_buf_to_string, helper::DEFAULT_ERROR_BUF_SIZE, runtime::Runtime, RuntimeError,
};
use std::{fs, path::Path, string::String};
use wamr_sys::{wasm_module_t, wasm_runtime_load, wasm_runtime_unload};

#[derive(Debug)]
pub struct Module {
    module: wasm_module_t,
}

impl Module {
    /// compile a module with the given wasm file path
    pub fn from_file(_runtime: &Runtime, wasm_file: &Path) -> Result<Self, RuntimeError> {
        let mut contents = match fs::read_to_string(wasm_file) {
            Ok(c) => c,
            Err(e) => return Err(RuntimeError::WasmFileFSError(e)),
        };

        let binary = unsafe { contents.as_bytes_mut() };
        Self::from_buf(_runtime, binary)
    }

    pub fn from_buf(_runtime: &Runtime, buf: &mut [u8]) -> Result<Self, RuntimeError> {
        let mut error_buf = [0i8; DEFAULT_ERROR_BUF_SIZE];
        let module = unsafe {
            wasm_runtime_load(
                buf.as_mut_ptr(),
                buf.len() as u32,
                error_buf.as_mut_ptr(),
                error_buf.len() as u32,
            )
        };

        if module.is_null() {
            match error_buf.len() {
                0 => {
                    return Err(RuntimeError::CompilationError(String::from(
                        "load module failed",
                    )))
                }
                _ => {
                    return Err(RuntimeError::CompilationError(error_buf_to_string(
                        &error_buf,
                    )))
                }
            }
        }

        Ok(Module { module })
    }

    pub fn get_inner_module(&self) -> wasm_module_t {
        self.module
    }
}

impl Drop for Module {
    fn drop(&mut self) {
        unsafe {
            wasm_runtime_unload(self.module);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_module_not_exist() {
        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);

        let runtime = runtime.unwrap();
        let module = Module::from_file(&runtime, Path::new("not_exist"));
        assert_eq!(module.is_err(), true);
    }

    #[test]
    fn test_module_from_buf() {
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

        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);

        let runtime = runtime.unwrap();
        let module = Module::from_buf(&runtime, &mut binary);
        assert_eq!(module.is_ok(), true);
    }
}
