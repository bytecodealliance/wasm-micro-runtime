//! This is the main entry point for executing WebAssembly modules.

use RuntimeError;
use wamr_sys;

pub struct Runtime {}

impl Runtime {
    fn new() -> Result<Runtime, RuntimeError> {
        wamr_sys::wasm_runtime_full_init(None);

        Err(RuntimeError::NotImplemented)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let runtime = Runtime::new();
        assert_eq!(runtime.is_ok(), true);
    }
}
