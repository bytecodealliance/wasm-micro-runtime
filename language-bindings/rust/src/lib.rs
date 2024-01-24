/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

//! # WAMR Rust SDK
//!
//! ## Overview
//!
//! WAMR Rust SDK provides Rust language bindings for WAMR. It is the wrapper
//! of *wasm_export.h* but with Rust style. It is more convenient to use WAMR
//! in Rust with this crate.
//!
//! This crate contains API used to interact with Wasm modules. You can compile
//! modules, instantiate modules, call their export functions, etc.
//! Plus, as an embedded of Wasm, you can provide Wasm module functionality by
//! creating host-defined functions, globals (memory and table are not supported
//! for now).
//!
//! WAMR Rust SDK will search for the WAMR runtime library in the *../..* path
//! for now since it is stored in *language-bindings/rust* in WAMR repo. And
//! then uses `rust-bindgen` durning the build process.
//!
//! This crate has similar concepts to the
//! [WebAssembly specification](https://webassembly.github.io/spec/core/).
//!
//! ### Core concepts
//!
//! - *Runtime*. It is the environment that hosts all the wasm modules. Each process has one runtime instance.
//! - *Module*. It is the compiled .wasm. It can be loaded into runtime and instantiated into instance.
//! - *Instance*. It is the running instance of a module. It can be used to call export functions.
//! - *Function*. It is the exported function.
//!
//! ### WASI concepts
//!
//! - *WASIArgs*. It is used to configure the WASI environment.
//!   - *pre-open*.
//!   - *environment variable*.
//!   - *allowed address*.
//!   - *allowed namespace*.
//!
//! ### WAMR private concepts
//!
//! - *loading linking* instead of *instantiation linking*. *instantiation linking* is
//! used in Wasm JS API and Wasm C API. It means that every instance has its own, maybe
//! variant, imports. But *loading linking* means that all instances share the same.
//!
//! - *RuntimeArg*.
//!
//! - *NativeFunction*.
//!
//! ## Examples
//!
//! ### Example: to run a wasm32-wasi .wasm
//!
//! *wasm32-wasi* is a most common target for Wasm. It means that the .wasm is compiled with
//! `cargo build --target wasm32-wasi` or `wasi-sdk/bin/clang --target wasm32-wasi`.
//!
//! Say there is a *test.wasm* includes a function named *add*.
//!
//! ``` wat
//!   (func (export "add") (param i32 i32) (result i32)
//!     (local.get 0)
//!     (local.get 1)
//!     (i32.add)
//!   )
//! ```
//! The rust code to call the *add* function is like this:
//!
//! ``` rust
//! use wamr_rust_sdk::*;
//!
//! fn main() -> Result<> {
//!   let runtime = Runtime::new()?;
//!
//!   let module = Module::from_file(&runtime, "test.wasm")?;
//!   module.set_wasi_arg_pre_open_path(vec![String::from(".")], vec![]);
//!
//!   let stack_size = 1024;
//!   let instance = Instance::new(&module, stack_size)?;
//!   let wasm_func = instance.find_export_func("add")?;
//!   let params: Vec<WasmValue> = vec![WasmValue::I32(1), WasmValue::I32(2)];
//!   let ret = wasm_func.call(&instance, &params)?;
//!   assert_eq!(ret , WasmValue::I32(3));
//!
//!   Ok()
//! }
//! ```
//!
//! ### Example: more configuration for runtime.
//!
//! with more configuration, runtime is capable to run .wasm with variant features, like
//! - Wasm without WASI. Some .wasm are compiled with `--nostdlib` to reduce the size.
//! - runtime under variant running modes. like LLVM_JIT.
//! - runtime with variant memory management, like other allocators, memory pool, or heap size.
//! - provides host-defined functions to meet import requirements.
//!
//! Say there is a *test.wasm*
//!
//! ``` wat
//! (module
//!   (func $extra (import "extra") (result i32))
//!   (func (export "add") (param i32 i32) (result i32)
//!     (local.get 0)
//!     (local.get 1)
//!     (i32.add)
//!     (call $extra)
//!     (i32.add)
//!   )
//! )
//! ```
//!
//! The rust code to call the *add* function is like this:
//!
//! ``` rust
//! use wamr_rust_sdk::*;
//!
//! fn main() -> Reulst<> {
//!   let runtime = Runtime::builder().run_as_interpreter().use_system_allocator().build()?;
//!
//!   let native_functions : Vec<NativeFunction> = Vec::new();
//!   native_functions.push(NativeFunction::new("extra", || => 10));
//!   runtime.register_native_functions(&native_functions);
//!
//!   let module = Module::from_file(&runtime, "test.wasm")?;
//!
//!   let stack_size = 1024;
//!   let instance = Instance::new(&module, stack_size)?;
//!   let wasm_func = instance.find_export_func("add")?;
//!   let params: Vec<WasmValue> = vec![WasmValue::I32(1), WasmValue::I32(2)];
//!   let ret = wasm_func.call(&instance, &params)?;
//!   assert_eq!(ret , WasmValue::I32(13));
//!
//!   Ok()
//! }
//! ```
//!

pub mod function;
mod helper;
pub mod instance;
pub mod module;
pub mod runtime;
pub mod value;

/// all kinds of exceptions raised by WAMR
#[derive(Debug)]
pub enum RuntimeError {
    NotImplemented,
    /// Runtime initialization error.
    InitializationFailure,
    /// file operation error. usually while loading(compilation) a .wasm
    WasmFileFSError(std::io::Error),
    /// A compilation error. usually means that the .wasm file is invalid
    CompilationError(String),
    /// instantiation failure
    InstantiationFailure(String),
    /// Error during execute wasm functions
    ExecutionError(String),
    /// usually returns by `find_export_func()`
    FunctionNotFound,
}
