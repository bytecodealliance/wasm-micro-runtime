;; Copyright (C) 2023 Amazon Inc.  All rights reserved.
;; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
;;
;; Those tests verify if passing constant negative value
;; as a right parameter of the shift operator (along
;; with a constant value of the left operator) causes
;; any problems. See: https://github.com/bytecodealliance/wasm-micro-runtime/pull/2619
(module
  (memory (export "memory") 1 1)
  (func $assert_eq (param i32 i32)
    (i32.ne (local.get 0) (local.get 1))
    if
      unreachable
    end
  )

  (func $i32_shr_u
    (call $assert_eq
      (i32.shr_u (i32.const -1) (i32.const -5))
      (i32.const 31)
    )
  )

  (func $i32_shr_s
    (call $assert_eq
      (i32.shr_u (i32.const 32) (i32.const -30))
      (i32.const 8)
    )
  )

  (func $i32_shl
    (call $assert_eq
      (i32.shl (i32.const -1) (i32.const -30))
      (i32.const -4)
    )
  )

  (func (export "_start")
    call $i32_shr_u
    call $i32_shr_s
    call $i32_shl
  )
)
