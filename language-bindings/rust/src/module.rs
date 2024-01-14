//! .wasm compiled, in-memory representation

use crate::{runtime::Runtime, RuntimeError, DEFAULT_ERROR_BUF_SIZE};
use std::{fs, path::Path};
use wamr_sys::{wasm_module_t, wasm_runtime_load, wasm_runtime_unload};

pub struct Module {
    module: wasm_module_t,
}

impl Module {
    /// compile a module with the given wasm file path
    pub fn from_file(runtime: &Runtime, wasm_file: &Path) -> Result<Self, RuntimeError> {
        let mut contents = match fs::read_to_string(wasm_file) {
            Ok(c) => c,
            Err(e) => return Err(RuntimeError::WasmFileFSError(e)),
        };

        let mut error_buf = [0i8; DEFAULT_ERROR_BUF_SIZE];
        let module = unsafe {
            wasm_runtime_load(
                contents.as_mut_ptr(),
                contents.len() as u32,
                error_buf.as_mut_ptr(),
                error_buf.len() as u32,
            )
        };

        if !error_buf.is_empty() {
            let error_buf = error_buf.map(|c| c as u8);
            let error_buf = match String::from_utf8(error_buf.to_vec()) {
                Ok(s) => s,
                Err(e) => return Err(RuntimeError::CompilationError(e.to_string())),
            };

            return Err(RuntimeError::CompilationError(error_buf));
        }

        Ok(Module { module })
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
    fn test_module() {
        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);

        let runtime = runtime.unwrap();
        let module = Module::from_file(&runtime, Path::new("not_exist"));
        assert_eq!(module.is_err(), true);
    }
}
