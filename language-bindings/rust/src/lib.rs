//! # WAMR Rust SDK
//!
//! ## Overview
//!
//! WAMR Rust SDK provides Rust language bindings for WAMR
//!
//! This crate contains API used to interact with Wasm modules. You can compile
//! modules, instantiate modules, call their export functions, etc.
//! Plus, as an embedded of Wasm, you can provide Wasm module functionality by
//! creating host-defined functions, globals (memory and table are not supported
//! for now).
//!
//! WAMR Rust SDK will search for the WAMR runtime library in the *../..* path.
//! And then uses `rust-bindgen` durning the build process.
//!
//! This crate has similar concepts to the [WebAssembly speicification](https://webassembly.github.io/spec/core/).
//!
//! ## Examples
//!
//! Here is a few examples, showing how to use WAMR Rust SDK.
//!
//! ### Example: Run a wasm function in a wasm module
//!
//! ```
//! ```
//!
//! ### Example: Import a host function into a wasm module
//!
//! ```
//! ```
//!
//! ### Example: Work with WASI
//!
//! ```
//! ```
//!
//! ## TODO:
//! - [ ] license
//! - [ ] may use downloading WAMR runtime library from github release for convenience
//! - [ ] may support a local WAMR runtime library for convenience
//! - [ ] shall we handle various compilation options with features?

mod runtime;
mod module;
mod instance;
mod func;
mod global;
mod memory;
mod table;
