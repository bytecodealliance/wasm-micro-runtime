# wamr-rust-sdk

## WAMR Rust SDK

### Overview

WAMR Rust SDK provides Rust language bindings for WAMR. It is the wrapper
of [*wasm_export.h*](../../../core/iwasm/include/wasm_export.h) but with Rust style.
It is more convenient to use WAMR in Rust with this crate.

This crate contains API used to interact with Wasm modules. You can compile
modules, instantiate modules, call their export functions, etc.
Plus, as an embedded of Wasm, you can provide Wasm module functionality by
creating host-defined functions.

WAMR Rust SDK includes a [*wamr-sys*](../crates/wamr-sys) crate. It will search for
the WAMR runtime source in the path *../..*. And then uses `rust-bindgen` durning
the build process to make a .so.

This crate has similar concepts to the
[WebAssembly specification](https://webassembly.github.io/spec/core/).

#### Core concepts

- *Runtime*. It is the environment that hosts all the wasm modules. Each process has one runtime instance.
- *Module*. It is the compiled .wasm or .aot. It can be loaded into runtime and instantiated into instance.
- *Instance*. It is the running instance of a module. It can be used to call export functions.
- *Function*. It is the exported function.

#### WASI concepts

- *WASIArgs*. It is used to configure the WASI environment.
  - *pre-open*. All files and directories in the list will be opened before the .wasm or .aot loaded.
  - *allowed address*. All ip addresses in the *allowed address* list will be allowed to connect with a socket.
  - *allowed DNS*.

#### WAMR private concepts

- *loading linking* instead of *instantiation linking*. *instantiation linking* is
used in Wasm JS API and Wasm C API. It means that every instance has its own, maybe
variant, imports. But *loading linking* means that all instances share the same *imports*.

- *RuntimeArg*. Control runtime behavior.
  - *running mode*.
  - *allocator*.

- *NativeFunction*.

- *WasmValues*.

### Examples

#### Example: to run a wasm32-wasi .wasm

*wasm32-wasi* is a most common target for Wasm. It means that the .wasm is compiled with
`cargo build --target wasm32-wasi` or `wasi-sdk/bin/clang --target wasm32-wasi`.

Say there is a gcd_wasm32_wasi.wasm which includes a function named *gcd*. It returns the GCD
of two parameters.

The rust code to call the function would be:

```rust
use wamr_rust_sdk::{
    runtime::Runtime, module::Module, instance::Instance, function::Function,
    value::WasmValue, RuntimeError
};
use std::path::PathBuf;

fn main() -> Result<(), RuntimeError> {
    let runtime = Runtime::new()?;

    let mut d = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    d.push("resources/test");
    d.push("gcd_wasm32_wasi.wasm");

    let module = Module::from_file(d.as_path())?;

    let instance = Instance::new(&module, 1024 * 64)?;

    let function = Function::find_export_func(&instance, "gcd")?;

    let params: Vec<WasmValue> = vec![WasmValue::I32(9), WasmValue::I32(27)];
    let result = function.call(&instance, &params)?;
    assert_eq!(result, WasmValue::I32(9));

    Ok(())
}
```

#### Example: more configuration for runtime.

With more configuration, runtime is capable to run .wasm with variant features, like
- Wasm without WASI requirement. Usually, it means that the .wasm is compiled with `-nostdlib`
  or `--target wasm32-unknown-unknown`
- Configure runtime.
- Provides host-defined functions to meet import requirements.

Say there is an add_extra_wasm32_wasi.wasm. Its exported function, `add()`,
requires an imported function, `extra()`, during the execution. The `add()`
adds two parameters and the result of `extra()` . It is like `a + b + extra()`.

The rust code to call the *add* function is like this:

```rust
use wamr_rust_sdk::{
    runtime::Runtime, module::Module, instance::Instance, function::Function,
    value::WasmValue, RuntimeError
};
use std::path::PathBuf;
use std::ffi::c_void;

extern "C" fn extra() -> i32 {
    100
}

fn main() -> Result<(), RuntimeError> {
    let runtime = Runtime::builder()
        .use_system_allocator()
        .register_host_function("extra", extra as *mut c_void)
        .build()?;

    let mut d = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    d.push("resources/test");
    d.push("add_extra_wasm32_wasi.wasm");
    let module = Module::from_file(d.as_path())?;

    let instance = Instance::new(&module, 1024 * 64)?;

    let function = Function::find_export_func(&instance, "add")?;

    let params: Vec<WasmValue> = vec![WasmValue::I32(9), WasmValue::I32(27)];
    let result = function.call(&instance, &params)?;
    assert_eq!(result, WasmValue::I32(136));

    Ok(())
}
```
