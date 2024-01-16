//! This is the main entry point for executing WebAssembly modules.

use std::option::Option;
use std::sync::{Arc, OnceLock};

use wamr_sys::{
    mem_alloc_type_t_Alloc_With_System_Allocator, wasm_runtime_destroy, wasm_runtime_full_init,
    RunningMode_Mode_Fast_JIT, RunningMode_Mode_Interp, RunningMode_Mode_LLVM_JIT, RuntimeInitArgs,
};

use crate::RuntimeError;
#[derive(Clone, Copy, Debug)]
pub struct Runtime {}

static SINGLETON_RUNTIME: OnceLock<Option<Arc<Runtime>>> = OnceLock::new();

fn init_args_as_aot(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    // TODO: Add AOT support
    *init_args
}

fn init_args_as_fast_jit(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_Fast_JIT;
    init_args.fast_jit_code_cache_size = 10 * 1024 * 1024;
    *init_args
}

fn init_args_as_interp(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_Interp;
    *init_args
}

fn init_args_as_llvm_jit(init_args: &mut RuntimeInitArgs) -> RuntimeInitArgs {
    init_args.running_mode = RunningMode_Mode_LLVM_JIT;
    init_args.llvm_jit_opt_level = 3;
    init_args.llvm_jit_size_level = 3;
    *init_args
}

impl Runtime {
    //TODO: use feature?
    /// Create a runtime under the aot running mode
    /// and use system provided allocations like `malloc`, `free`
    ///
    /// # Errors
    /// `RuntimeError::InitializationFailure` If a *full init* failed
    pub fn new_as_aot() -> Result<Arc<Runtime>, RuntimeError> {
        let mut init_args: RuntimeInitArgs = RuntimeInitArgs::default();
        init_args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        init_args = init_args_as_aot(&mut init_args);

        return Self::new_with_args(&mut init_args);
    }

    //TODO: use feature?
    /// Create a runtime under the fast_jit running mode
    /// and use system provided allocations like `malloc`, `free`
    ///
    /// # Errors
    /// `RuntimeError::InitializationFailure` If a *full init* failed
    pub fn new_as_fast_jit() -> Result<Arc<Runtime>, RuntimeError> {
        let mut init_args: RuntimeInitArgs = RuntimeInitArgs::default();
        init_args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        init_args = init_args_as_fast_jit(&mut init_args);

        return Self::new_with_args(&mut init_args);
    }

    //TODO: use feature?
    /// Create a runtime under the interpreter running mode
    /// and use system provided allocations like `malloc`, `free`
    ///
    /// # Errors
    /// `RuntimeError::InitializationFailure` If a *full init* failed
    pub fn new_as_interp() -> Result<Arc<Runtime>, RuntimeError> {
        let mut init_args: RuntimeInitArgs = RuntimeInitArgs::default();
        init_args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        init_args = init_args_as_interp(&mut init_args);

        return Self::new_with_args(&mut init_args);
    }

    //TODO: use feature?
    /// Create a runtime under the llvm_jit running mode
    /// and use system provided allocations like `malloc`, `free`
    ///
    /// # Errors
    /// `RuntimeError::InitializationFailure` If a *full init* failed
    pub fn new_as_llvm_jit() -> Result<Arc<Runtime>, RuntimeError> {
        let mut init_args: RuntimeInitArgs = RuntimeInitArgs::default();
        init_args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        init_args = init_args_as_llvm_jit(&mut init_args);

        return Self::new_with_args(&mut init_args);
    }

    fn new_with_args(init_args: &mut RuntimeInitArgs) -> Result<Arc<Runtime>, RuntimeError> {
        let runtime = SINGLETON_RUNTIME.get_or_init(|| {
            let ret;
            unsafe {
                println!("--> 🎇 call wasm_runtime_full_init");
                ret = wasm_runtime_full_init(init_args);
            }

            match ret {
                true => Some(Arc::new(Runtime {})),
                false => None,
            }
        });

        match runtime {
            Some(runtime) => Ok(Arc::clone(runtime)),
            None => Err(RuntimeError::InitializationFailure),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_runtime() {
        {
            let runtime = Runtime::new_as_interp();
            assert_eq!(runtime.is_ok(), true);
        }

        {
            let runtime = Runtime::new_as_interp();
            assert_eq!(runtime.is_ok(), true);
        }

        {
            let runtime = Runtime::new_as_interp();
            assert_eq!(runtime.is_ok(), true);
        }
    }
}
