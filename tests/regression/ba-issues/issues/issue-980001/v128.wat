(module
  (type (;0;) (func))
  (func (;0;) (type 0)
    i32.const 0
    i32.const 16
    v128.load
    i32.const 32
    v128.load
    i64x2.eq
    v128.store)
  (memory (;0;) 1 1)
  (export "mem" (memory 0))
  (export "main" (func 0)))
