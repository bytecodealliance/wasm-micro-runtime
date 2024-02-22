/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#[link(wasm_import_module = "host")]
extern "C" {
    fn extra() -> u32;
}

#[export_name = "add"]
pub fn add_ex(m: u32, n: u32) -> u32 {
    m + n + unsafe { extra() }
}

fn main() {
    println!("Hello, world! Please call add(10, 20) to see the result.");
}
