//! Reulst and error types
use std::string::String;

/// A compilation error.
pub enum CompilationError {
    /// Some opcodes are not supported. May need to recompile the wasm module(?)
    /// Or recompile the WAMR runtime library with different compilation options.
    Unsupported(String),
}

/// A wasm function execution error.
pub enum ExecutionError {}

/// A runtime error.
pub enum RuntimeError {}
