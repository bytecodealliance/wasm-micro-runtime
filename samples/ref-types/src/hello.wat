;; Copyright (C) 2019 Intel Corporation.  All rights reserved.
;; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

(module
  ;; import test_write function which is implemented by host
  (import "env" "test_write"
    (func $test_write (param externref i32 i32) (result i32)))

  ;; memory with one page (64KiB).
  (memory (export "memory") 1)

  (data (i32.const 0x8) "Hello, world!\n")

  ;; function that writes string to a given open file handle
  (func (export "test") (param externref)
     (local.get 0)
     (i32.const 0x8)
     (i32.const 14)
     (call $test_write)
     drop
  )
)
