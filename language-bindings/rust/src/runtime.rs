/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! This is the main entry point for executing WebAssembly modules.
//! Every process should have only one instance of this runtime by call
//! `Runtime::new()` or `Runtime::builder().build()` once.

use std::ffi::c_void;
use std::sync::Mutex;

use wamr_sys::{
    mem_alloc_type_t_Alloc_With_Pool, mem_alloc_type_t_Alloc_With_System_Allocator,
    wasm_runtime_destroy, wasm_runtime_full_init, wasm_runtime_init, RunningMode_Mode_Interp,
    RunningMode_Mode_LLVM_JIT, RuntimeInitArgs,
};

use crate::RuntimeError;

#[derive(Clone, Debug)]
pub struct Runtime {}

static SINGLETON_REF_CNT: Mutex<i32> = Mutex::new(0);

impl Runtime {
    /// return a `RuntimeBuilder` instance
    ///
    /// has to
    /// - select a allocation mode
    /// - select a running mode
    pub fn builder() -> RuntimeBuilder {
        RuntimeBuilder::default()
    }

    /// create a new `Runtime` instance with the default configuration which includes:
    /// - system allocator mode
    /// - the default running mode
    ///
    /// # Errors
    ///
    /// if the runtime initialization failed, it will return `RuntimeError::InitializationFailure`
    pub fn new() -> Result<Self, RuntimeError> {
        let mut ref_cnt = SINGLETON_REF_CNT.lock().unwrap();

        *ref_cnt += 1;

        match *ref_cnt {
            1 => match unsafe { wasm_runtime_init() } {
                true => Ok(Runtime {}),
                false => Err(RuntimeError::InitializationFailure),
            },
            _ => Ok(Runtime {}),
        }
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        let mut ref_cnt = SINGLETON_REF_CNT.lock().unwrap();

        *ref_cnt -= 1;

        if *ref_cnt == 0 {
            unsafe {
                wasm_runtime_destroy();
            }
        }
    }
}

/// The builder of `Runtime`. It is used to configure the runtime.
/// Get one via `Runtime::builder()`
#[derive(Default)]
pub struct RuntimeBuilder {
    args: RuntimeInitArgs,
}

impl RuntimeBuilder {
    /// system allocator mode
    /// allocate memory from system allocator for runtime consumed memory
    pub fn use_system_allocator(mut self) -> RuntimeBuilder {
        self.args.mem_alloc_type = mem_alloc_type_t_Alloc_With_System_Allocator;
        self
    }

    /// system allocator mode
    /// allocate memory from pool, as a pre-allocated buffer, for runtime consumed memory
    pub fn use_memory_pool(mut self, mut pool: Vec<u8>, pool_size: u32) -> RuntimeBuilder {
        self.args.mem_alloc_type = mem_alloc_type_t_Alloc_With_Pool;
        self.args.mem_alloc_option.pool.heap_buf = pool.as_mut_ptr() as *mut c_void;
        self.args.mem_alloc_option.pool.heap_size = pool_size;
        self
    }

    /// use interpreter mode
    pub fn run_as_interpreter(mut self) -> RuntimeBuilder {
        self.args.running_mode = RunningMode_Mode_Interp;
        self
    }

    /// TODO: use fast-jit mode

    /// use llvm-jit mode
    pub fn run_as_llvm_jit(mut self, opt_level: u32, size_level: u32) -> RuntimeBuilder {
        self.args.running_mode = RunningMode_Mode_LLVM_JIT;
        self.args.llvm_jit_opt_level = opt_level;
        self.args.llvm_jit_size_level = size_level;
        self
    }

    /// create a `Runtime` instance with the configuration
    ///
    /// # Errors
    ///
    /// if the runtime initialization failed, it will return `RuntimeError::InitializationFailure`
    pub fn build(mut self) -> Result<Runtime, RuntimeError> {
        let mut ref_cnt = SINGLETON_REF_CNT.lock().unwrap();

        *ref_cnt += 1;

        match *ref_cnt {
            1 => match unsafe { wasm_runtime_full_init(&mut self.args) } {
                true => Ok(Runtime {}),
                false => Err(RuntimeError::InitializationFailure),
            },
            _ => Ok(Runtime {}),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use wamr_sys::{wasm_runtime_free, wasm_runtime_malloc};

    #[test]
    #[ignore]
    fn test_runtime_new() {
        let runtime = Runtime::new();
        assert!(runtime.is_ok());

        /* use malloc to confirm */
        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(!small_buf.is_null());
        unsafe { wasm_runtime_free(small_buf) };

        drop(runtime);

        /* runtime has been destroyed. malloc should be failed */
        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(small_buf.is_null());

        {
            let runtime = Runtime::new();
            assert!(runtime.is_ok());

            let runtime = Runtime::new();
            assert!(runtime.is_ok());

            let runtime = Runtime::new();
            assert!(runtime.is_ok());
        }

        /* runtime has been destroyed. malloc should be failed */
        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(small_buf.is_null());
    }

    #[test]
    fn test_runtime_builder_default() {
        // use Mode_Default
        let runtime = Runtime::builder().use_system_allocator().build();
        assert!(runtime.is_ok());

        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(!small_buf.is_null());
        unsafe { wasm_runtime_free(small_buf) };
    }

    #[test]
    fn test_runtime_builder_interpreter() {
        let runtime = Runtime::builder()
            .run_as_interpreter()
            .use_system_allocator()
            .build();
        assert!(runtime.is_ok());

        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(!small_buf.is_null());
        unsafe { wasm_runtime_free(small_buf) };
    }

    #[test]
    #[cfg(feature = "llvmjit")]
    #[ignore]
    fn test_runtime_builder_llvm_jit() {
        let runtime = Runtime::builder()
            .run_as_llvm_jit(3, 3)
            .use_system_allocator()
            .build();
        assert!(runtime.is_ok());

        let small_buf = unsafe { wasm_runtime_malloc(16) };
        assert!(!small_buf.is_null());
        unsafe { wasm_runtime_free(small_buf) };
    }
}
