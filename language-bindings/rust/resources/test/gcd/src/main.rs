/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#[export_name = "gcd"]
pub fn gcd(m: u32, n: u32) -> u32 {
    let mut a = m;
    let mut b = n;

    while b != 0 {
        (a, b) = (b, a % b)
    }

    println!("gcd({}, {}) = {}", m, n, a);
    a
}

fn main() {
    println!("Hello, world! Please call gcd(10, 5) to see the result.");
}
