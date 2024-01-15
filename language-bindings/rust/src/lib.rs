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
//! This crate has similar concepts to the [WebAssembly speicification](https://webassembly.github.io/spec/core/).
//!
//! ### Core concepts
//!
//! - Rutime.
//! - Module.
//! - Instance.
//! - Function.
//! - Global.
//! - Memory.
//! - Table.
//!
//! ### WASI concepts
//!
//! ### WAMR prviate concepts
//!
//! ## Examples
//!
//! Here are a few examples showing how to use WAMR Rust SDK.
//!
//! ### Example: Run a wasm function in a wasm module
//!
//! assume *test.wasm* `(func (export "run"))`
//!
//! ``` rust
//! use wamr::*;
//!
//! fn main() -> Result<> {
//!   let runtime = Runtime::new()?;
//!   let module = Module::from_file(&runtime, "test.wasm")?;
//!   let instancce = Instance::new(&module, &[])?;
//!   let func = instance.find_export_func("run")?;
//!   func.call(&[])?;
//!   Ok(())
//! }
//! ```
//!
//! ### Example: Import a host function and run it in a wasm module
//!
//! assume
//! - "test.wasm" needs to `(import "host" "host_func" (func (param i32 i32)))`
//! - *test.wasm* has a `(func (export "run"))`
//!
//! ``` rust
//! fn main() -> Reulst<> {
//!   let runtime = Runtime::new()?;
//!   let module = Module::from_file(&runtime, "test.wasm")?;
//!   let host_func = Func::new_native("host_func", |p1: i32, p2: i32| {
//!     println!("host_func called. incoming {p1:?}, {p2:?}");
//!   });
//!   let instancce = Instance::new(&module, &[host_func])?;
//!   let func = instance.find_export_func("run")?;
//!   func.call(&[])?;
//!   Ok(())
//! }
//! ```
//!
//! ### Example: Work with WASI
//!
//! ``` rust
//! // TODO
//! ```
//!
//! ## TODO:
//! - [ ] license
//! - [ ] may use downloading WAMR runtime library from github release for convenience
//! - [ ] may support a local WAMR runtime library for convenience
//! - [ ] shall we handle various compilation options with features?

mod func;
mod global;
mod instance;
mod memory;
mod module;
mod runtime;
mod table;

mod helper;

/// A wasm function execution error.
pub enum ExecutionError {}

/// A runtime error.
#[derive(Debug)]
pub enum RuntimeError {
    /// If a functionality hasn't been implemented yet
    NotImplemented,
    /// Runtime initialization error
    InitializationFailure,
    /// .wasm operation error
    WasmFileFSError(std::io::Error),
    /// A compilation error.
    CompilationError(String),
}
