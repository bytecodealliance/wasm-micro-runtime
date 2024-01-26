/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! .wasm compiled, in-memory representation
//! get one via `Module::from_file()` or `Module::from_buf()`

use crate::{helper::error_buf_to_string, helper::DEFAULT_ERROR_BUF_SIZE, RuntimeError};
use std::{ffi::CString, fs::File, io::Read, path::Path, ptr, string::String, vec::Vec};
use wamr_sys::{
    wasm_module_t, wasm_runtime_load, wasm_runtime_set_wasi_addr_pool, wasm_runtime_set_wasi_args,
    wasm_runtime_set_wasi_ns_lookup_pool, wasm_runtime_unload,
};

#[derive(Default, Debug)]
struct PreOpen {
    real_paths: Vec<CString>,
    mapped_paths: Vec<CString>,
}

// keep CString in memory. There maybe is another way to maintain
// the right lifetime
#[derive(Default, Debug)]
struct WASIArg {
    pre_open: PreOpen,
    allowed_address: Vec<CString>,
    allowed_dns: Vec<CString>,
    env: Vec<CString>,
}

#[derive(Debug)]
pub struct Module {
    module: wasm_module_t,
    // used to keep the module content in memory
    content: Vec<u8>,
    wasi_arg: WASIArg,
}

impl Module {
    /// compile a module with the given wasm file path
    ///
    /// # Error
    ///
    /// If the file does not exist or the file cannot be read, an `RuntimeError::WasmFileFSError` will be returned.
    /// If the wasm file is not a valid wasm file, an `RuntimeError::CompilationError` will be returned.
    pub fn from_file(wasm_file: &Path) -> Result<Self, RuntimeError> {
        let mut wasm_file = match File::open(wasm_file) {
            Ok(f) => f,
            Err(e) => return Err(RuntimeError::WasmFileFSError(e)),
        };

        let mut binary: Vec<u8> = Vec::new();
        match wasm_file.read_to_end(&mut binary) {
            Ok(_) => Self::from_buf(&binary),
            Err(e) => Err(RuntimeError::WasmFileFSError(e)),
        }
    }

    /// compile a module int the given buffer
    ///
    /// # Error
    ///
    /// If the file does not exist or the file cannot be read, an `RuntimeError::WasmFileFSError` will be returned.
    /// If the wasm file is not a valid wasm file, an `RuntimeError::CompilationError` will be returned.
    pub fn from_buf(buf: &Vec<u8>) -> Result<Self, RuntimeError> {
        let mut content = buf.clone();
        let mut error_buf = [0i8; DEFAULT_ERROR_BUF_SIZE];
        let module = unsafe {
            wasm_runtime_load(
                content.as_mut_ptr(),
                content.len() as u32,
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

        Ok(Module {
            module,
            content,
            wasi_arg: WASIArg::default(),
        })
    }

    /// set pre-open directories and files, which are part of WASI arguments, for the module.
    /// the format of each map entry: <guest-path>::<host-path>
    ///
    /// This function should be called before `Instance::new`
    pub fn set_wasi_arg_pre_open_path(
        &mut self,
        real_paths: Vec<String>,
        mapped_paths: Vec<String>,
    ) {
        self.wasi_arg.pre_open.real_paths = real_paths
            .iter()
            .map(|s| CString::new(s.as_bytes()).unwrap())
            .collect::<Vec<CString>>();

        self.wasi_arg.pre_open.mapped_paths = real_paths
            .iter()
            .map(|s| CString::new(s.as_bytes()).unwrap())
            .collect::<Vec<CString>>();

        let real_paths_ptr: *mut *const i8 = if real_paths.is_empty() {
            ptr::null_mut()
        } else {
            self.wasi_arg.pre_open.real_paths.as_ptr() as *mut *const i8
        };

        let mapped_paths_ptr: *mut *const i8 = if mapped_paths.is_empty() {
            ptr::null_mut()
        } else {
            self.wasi_arg.pre_open.mapped_paths.as_ptr() as *mut *const i8
        };

        unsafe {
            wasm_runtime_set_wasi_args(
                self.get_inner_module(),
                real_paths_ptr,
                real_paths.len() as u32,
                mapped_paths_ptr,
                mapped_paths.len() as u32,
                ptr::null_mut(),
                0,
                ptr::null_mut(),
                0,
            );
        }
    }

    /// set environment variables, which are part of WASI arguments, for the module
    ///
    /// This function should be called before `Instance::new`
    ///
    /// all wasi args of a module will be spread into the environment variables of the module
    pub fn set_wasi_arg_env_vars(&mut self, envs: Vec<String>) {
        self.wasi_arg.env = envs
            .iter()
            .map(|s| CString::new(s.as_bytes()).unwrap())
            .collect::<Vec<CString>>();

        let envs_ptr = if envs.is_empty() {
            ptr::null_mut()
        } else {
            self.wasi_arg.env.as_ptr() as *mut *const i8
        };

        unsafe {
            wasm_runtime_set_wasi_args(
                self.get_inner_module(),
                ptr::null_mut(),
                0,
                ptr::null_mut(),
                0,
                envs_ptr,
                envs.len() as u32,
                ptr::null_mut(),
                0,
            );
        }
    }

    /// set allowed ns , which are part of WASI arguments, for the module
    ///
    /// This function should be called before `Instance::new`
    ///
    /// all wasi args of a module will be spread into the environment variables of the module
    pub fn set_wasi_arg_allowed_dns(&mut self, dns: Vec<String>) {
        self.wasi_arg.allowed_dns = dns
            .iter()
            .map(|s| CString::new(s.as_bytes()).unwrap())
            .collect::<Vec<CString>>();

        let ns_pool_ptr = if dns.is_empty() {
            ptr::null_mut()
        } else {
            self.wasi_arg.allowed_dns.as_ptr() as *mut *const i8
        };

        unsafe {
            wasm_runtime_set_wasi_ns_lookup_pool(
                self.get_inner_module(),
                ns_pool_ptr,
                dns.len() as u32,
            );
        }
    }

    /// set allowed ip addresses, which are part of WASI arguments, for the module
    ///
    /// This function should be called before `Instance::new`
    ///
    /// all wasi args of a module will be spread into the environment variables of the module
    pub fn set_wasi_arg_allowed_address(&mut self, addresses: Vec<String>) {
        self.wasi_arg.allowed_address = addresses
            .iter()
            .map(|s| CString::new(s.as_bytes()).unwrap())
            .collect::<Vec<CString>>();

        let addr_pool_ptr = if addresses.is_empty() {
            ptr::null_mut()
        } else {
            self.wasi_arg.allowed_address.as_ptr() as *mut *const i8
        };

        unsafe {
            wasm_runtime_set_wasi_addr_pool(
                self.get_inner_module(),
                addr_pool_ptr,
                addresses.len() as u32,
            );
        }
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
    use crate::runtime::Runtime;
    use std::path::PathBuf;

    #[test]
    fn test_module_not_exist() {
        let module = Module::from_file(Path::new("not_exist"));
        assert_eq!(module.is_err(), true);
    }

    #[test]
    fn test_module_from_buf() {
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
    }

    #[test]
    fn test_module_from_file() {
        let _ = Runtime::new().expect("runtime init failed");

        let mut d = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        d.push("resources/test");
        d.push("hello_wasm32-wasi.wasm");
        let module = Module::from_file(d.as_path());
        assert_eq!(module.is_ok(), true);
    }

    #[test]
    fn test_wasi_args() {
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
        let mut module = module.unwrap();

        module.set_wasi_arg_pre_open_path(vec![String::from(".")], vec![]);
        module.set_wasi_arg_env_vars(vec![]);
        module.set_wasi_arg_allowed_address(vec![]);
        module.set_wasi_arg_allowed_dns(vec![]);
    }
}
