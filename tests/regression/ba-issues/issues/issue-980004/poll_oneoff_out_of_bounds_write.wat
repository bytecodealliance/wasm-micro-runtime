(module
  (import "wasi_snapshot_preview1" "poll_oneoff"
    (func $poll_oneoff (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit"
    (func $proc_exit (param i32)))
  (memory (export "memory") 1)
  (func (export "_start")
    ;; Two absolute clock subscriptions ensure poll_oneoff writes two events.
    i32.const 0
    i64.const 1
    i64.store
    i32.const 8
    i32.const 0
    i32.store8
    i32.const 40
    i32.const 1
    i32.store16

    i32.const 48
    i64.const 2
    i64.store
    i32.const 56
    i32.const 0
    i32.store8
    i32.const 88
    i32.const 1
    i32.store16

    ;; out points to exactly one wasi_event_t at the end of memory.
    i32.const 0
    i32.const 65504
    i32.const 2
    i32.const 65500
    call $poll_oneoff
    drop

    i32.const 0
    call $proc_exit))
