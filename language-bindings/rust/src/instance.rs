//! an instantiated module. The module is instantiated with the given imports.
//!

use ::core::ffi::c_char;

use wamr_sys::{wasm_module_inst_t, wasm_runtime_deinstantiate, wasm_runtime_instantiate};

use crate::{
    helper::error_buf_to_string, helper::DEFAULT_ERROR_BUF_SIZE, module::Module, RuntimeError,
};

#[derive(Debug)]
pub struct Instance {
    instance: wasm_module_inst_t,
}

impl Instance {
    pub fn new(module: &Module, stack_size: u32, heap_size: u32) -> Result<Self, RuntimeError> {
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
    use crate::runtime;

    #[test]
    fn test_instance_new() {
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

        let runtime = runtime::Runtime::new_as_interp();
        assert_eq!(runtime.is_ok(), true);

        let runtime = runtime.unwrap();
        let module = Module::from_buf(&runtime, &mut binary);
        println!("{module:?}");
        assert_eq!(module.is_ok(), true);

        let instance = Instance::new(&module.unwrap(), 1024, 1024);
        assert_eq!(instance.is_ok(), true);
    }
}
