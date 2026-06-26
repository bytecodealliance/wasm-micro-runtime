(module
  (import "wasi_snapshot_preview1" "poll_oneoff"
    (func $poll_oneoff (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit"
    (func $proc_exit (param i32)))
  (memory (export "memory") 1)
  (func (export "_start")
    ;; Place exactly one subscription at the end of memory.
    i32.const 65488
    i64.const 1
    i64.store
    i32.const 65496
    i32.const 0
    i32.store8
    i32.const 65528
    i32.const 1
    i32.store16

    ;; nsubscriptions=2 forces validation of the full subscription array.
    i32.const 65488
    i32.const 64
    i32.const 2
    i32.const 60
    call $poll_oneoff
    drop

    i32.const 0
    call $proc_exit))
