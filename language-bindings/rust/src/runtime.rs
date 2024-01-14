//! This is the main entry point for executing WebAssembly modules.

use crate::RuntimeError;
use wamr_sys::{
    mem_alloc_type_t_Alloc_With_System_Allocator, wasm_runtime_destroy, wasm_runtime_full_init,
    RunningMode_Mode_Fast_JIT, RunningMode_Mode_Interp, RunningMode_Mode_LLVM_JIT, RuntimeInitArgs,
};

pub struct Runtime {}

fn runtime_init_args_setup_for_fast_jit(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_Fast_JIT;
    init_args.fast_jit_code_cache_size = 10 * 1024 * 1024;
    *init_args
}

fn runtime_init_args_setup_for_interp(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_Interp;
    *init_args
}

fn runtime_init_args_setup_for_llvm_jit(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_LLVM_JIT;
    init_args.llvm_jit_opt_level = 3;
    init_args.llvm_jit_size_level = 3;
    *init_args
}

impl Runtime {
    /// Create a runtime with default `RuntimeInitArgs`
    ///
    /// # Errors
    /// `RuntimeError::InitializationFailure` If a *full init* failed
    pub fn new() -> Result<Runtime, RuntimeError> {
        let mut init_args: RuntimeInitArgs = RuntimeInitArgs::default();
        init_args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        init_args = runtime_init_args_setup_for_interp(&mut init_args);

        let ret;
        unsafe {
            ret = wasm_runtime_full_init(&mut init_args);
        }

        match ret {
            true => Ok(Runtime {}),
            false => Err(RuntimeError::InitializationFailure),
        }
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        unsafe {
            wasm_runtime_destroy();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_runtime() {
        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);

    }
}
