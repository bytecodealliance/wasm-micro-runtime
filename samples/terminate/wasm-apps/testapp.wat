;; Copyright (C) 2024 YAMAMOTO Takashi
;; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

(module
  (func $fd_read (import "wasi_snapshot_preview1" "fd_read") (param i32 i32 i32 i32) (result i32))
  (func (export "_start")
    ;; read from FD 0
    i32.const 100 ;; iov_base
    i32.const 200 ;; buffer
    i32.store
    i32.const 104 ;; iov_len
    i32.const 1
    i32.store
    i32.const 0 ;; fd 0
    i32.const 100 ;; iov_base
    i32.const 1   ;; iov count
    i32.const 300 ;; retp (out)
    call $fd_read
    unreachable
  )
  (memory (export "memory") 1)
)
