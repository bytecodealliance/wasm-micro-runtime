/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! This is the main entry point for executing WebAssembly modules.
//! Every process should have only one instance of this runtime by call
//! `Runtime::new()` or `Runtime::builder().build()` once.

use std::option::Option;
use std::sync::{Arc, OnceLock};

use wamr_sys::{
    mem_alloc_type_t_Alloc_With_System_Allocator, wasm_runtime_destroy, wasm_runtime_full_init,
    wasm_runtime_init, RunningMode_Mode_Fast_JIT, RunningMode_Mode_Interp,
    RunningMode_Mode_LLVM_JIT, RuntimeInitArgs,
};

use crate::RuntimeError;

/// The builder of `Runtime`. It is used to configure the runtime.
pub struct RuntimeBuilder {
    args: RuntimeInitArgs,
}

impl Default for RuntimeBuilder {
    /// Default configuration of `RuntimeBuilder` uses system allocator mode
    fn default() -> Self {
        let builder = RuntimeBuilder {
            args: RuntimeInitArgs::default(),
        };
        builder.use_system_allocator()
    }
}

impl RuntimeBuilder {
    /// system allocator mode
    /// allocate memory from system allocator for runtime consumed memory
    pub fn use_system_allocator(mut self) -> RuntimeBuilder {
        self.args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        self
    }

    //TODO: feature
    pub fn set_max_threads(mut self, num: u32) -> RuntimeBuilder {
        self.args.max_thread_num = num;
        self
    }

    //TODO: feature
    /// use interpreter mode
    pub fn run_as_interpreter(mut self) -> RuntimeBuilder {
        self.args.running_mode = RunningMode_Mode_Interp;
        self
    }

    //TODO: feature
    /// use fast-jit mode
    pub fn run_as_fast_jit(mut self, code_cache_size: u32) -> RuntimeBuilder {
        self.args.running_mode = RunningMode_Mode_Fast_JIT;
        self.args.fast_jit_code_cache_size = code_cache_size;
        self
    }

    //TODO: feature
    /// use llvm-jit mode
    pub fn run_as_llvm_jit(mut self, opt_level: u32, size_level: u32) -> RuntimeBuilder {
        self.args.running_mode = RunningMode_Mode_LLVM_JIT;
        self.args.llvm_jit_opt_level = opt_level;
        self.args.llvm_jit_size_level = size_level;
        self
    }

    /// create a new `Runtime` instance with the configuration
    pub fn build(mut self) -> Result<Arc<Runtime>, RuntimeError> {
        let runtime = SINGLETON_RUNTIME.get_or_init(|| {
            let ret;
            unsafe {
                ret = wasm_runtime_full_init(&mut self.args);
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

#[derive(Clone, Debug)]
pub struct Runtime {}

static SINGLETON_RUNTIME: OnceLock<Option<Arc<Runtime>>> = OnceLock::new();

impl Runtime {
    pub fn builder() -> RuntimeBuilder {
        RuntimeBuilder::default()
    }

    /// create a new `Runtime` instance with the default configuration which includes:
    /// - system allocator mode
    /// - the default running mode
    pub fn new() -> Result<Arc<Self>, RuntimeError> {
        let runtime = SINGLETON_RUNTIME.get_or_init(|| {
            let ret;
            unsafe {
                ret = wasm_runtime_init();
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
    fn test_runtime_new() {
        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);
        drop(runtime);

        {
            let runtime = Runtime::new();
            assert_eq!(runtime.is_ok(), true);
            let runtime = Runtime::new();
            assert_eq!(runtime.is_ok(), true);
            let runtime = Runtime::new();
            assert_eq!(runtime.is_ok(), true);
            let runtime = Runtime::new();
            assert_eq!(runtime.is_ok(), true);
            let runtime = Runtime::new();
            assert_eq!(runtime.is_ok(), true);
        }
    }

    #[test]
    fn test_runtime_builder_default() {
        let runtime = Runtime::builder().build();
        assert_eq!(runtime.is_ok(), true);
        drop(runtime);
    }

    #[test]
    fn test_runtime_builder_interpreter() {
        let runtime = Runtime::builder()
            .run_as_interpreter()
            .use_system_allocator()
            .build();
        assert_eq!(runtime.is_ok(), true);
        drop(runtime);
    }

    #[test]
    fn test_runtime_builder_fast_jit() {
        let runtime = Runtime::builder()
            .run_as_fast_jit(1024)
            .use_system_allocator()
            .build();
        assert_eq!(runtime.is_ok(), true);
        drop(runtime);
    }

    #[test]
    fn test_runtime_builder_llvm_jit() {
        let runtime = Runtime::builder()
            .run_as_llvm_jit(3, 3)
            .use_system_allocator()
            .build();
        assert_eq!(runtime.is_ok(), true);
        drop(runtime);
    }
}
