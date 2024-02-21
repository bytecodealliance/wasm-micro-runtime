/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/// This is a wrapper of a host defined(Rust) function.
use std::ffi::{c_void, CString};
use std::ptr;

use wamr_sys::NativeSymbol;

#[allow(dead_code)]
#[derive(Debug)]
struct HostFunction {
    function_name: CString,
    function_ptr: *mut c_void,
}

#[derive(Debug)]
pub struct HostFunctionList {
    pub module_name: CString,
    // keep ownership of host functions
    host_functions: Vec<HostFunction>,
    pub native_symbols: Vec<NativeSymbol>,
}

impl HostFunctionList {
    pub fn new(module_name: &str) -> Self {
        HostFunctionList {
            module_name: CString::new(module_name).unwrap(),
            host_functions: Vec::new(),
            native_symbols: Vec::new(),
        }
    }

    pub fn add_host_function(&mut self, function_name: &str, function_ptr: *mut c_void) {
        self.host_functions.push(HostFunction {
            function_name: CString::new(function_name).unwrap(),
            function_ptr,
        });

        let last = self.host_functions.last().unwrap();
        self.native_symbols
            .push(pack_host_function(&(last.function_name), function_ptr));
    }

    pub fn get_native_symbols(&mut self) -> &mut Vec<NativeSymbol> {
        &mut self.native_symbols
    }

    pub fn get_module_name(&mut self) -> &CString {
        &self.module_name
    }
}

pub fn pack_host_function(function_name: &CString, function_ptr: *mut c_void) -> NativeSymbol {
    NativeSymbol {
        symbol: function_name.as_ptr(),
        func_ptr: function_ptr,
        signature: ptr::null(),
        attachment: ptr::null_mut(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        function::Function, instance::Instance, module::Module, runtime::Runtime, value::WasmValue,
    };
    use std::env;
    use std::path::PathBuf;

    extern "C" fn extra() -> i32 {
        100
    }

    #[test]
    #[ignore]
    fn test_host_function() {
        let runtime = Runtime::builder()
            .use_system_allocator()
            .add_host_function("extra", extra as *mut c_void)
            .build();
        assert!(runtime.is_ok());

        let mut d = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        d.push("resources/test");
        d.push("add_extra_wasm32_wasi.wasm");
        let module = Module::from_file(d.as_path());
        assert!(module.is_ok());
        let module = module.unwrap();

        let instance = Instance::new(&module, 1024 * 64);
        assert!(instance.is_ok());
        let instance: &Instance = &instance.unwrap();

        let function = Function::find_export_func(instance, "add");
        assert!(function.is_ok());
        let function = function.unwrap();

        let params: Vec<WasmValue> = vec![WasmValue::I32(8), WasmValue::I32(8)];
        let result = function.call(instance, &params);
        assert_eq!(result.unwrap(), WasmValue::I32(116));
    }
}
