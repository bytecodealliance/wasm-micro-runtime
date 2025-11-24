(module
  (global $g0 (mut i32) (i32.const 0))
  (global $g1 (mut i32) (i32.const 0))
  (global $g2 (mut i32) (i32.const 0))
  (global $g3 (mut i32) (i32.const 0))
  (global $g4 (mut i32) (i32.const 0))
  (global $g5 (mut i32) (i32.const 0))
  (global $g6 (mut i32) (i32.const 0))
  (global $g7 (mut i32) (i32.const 0))

  (export "test" (func $0))
  (func $0
    (local i32)

    global.get $g0
    global.get $g1
    global.get $g2
    global.get $g3
    global.get $g4
    global.get $g5
    global.get $g6
    global.get $g7
    global.get $g0
    global.get $g1
    global.get $g2
    global.get $g3
    global.get $g4
    global.get $g5
    global.get $g6
    global.get $g7
    global.get $g0
    global.get $g1
    global.get $g2
    global.get $g3
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g4
    global.get $g0

    ;; has consumed 30 elements, left 2 elements on stack
    block
     block
        f64.const 3.14
        ;; RESET current block stack and mark polymorphic
        unreachable
        ;; PUSH ANY
        select

        loop (param i64) (result i32)
          ;; NOW, unmatched stacks. Enlarge frame_ref stack. Keep frame_offset stack unchanged.
          global.get $g0
          i32.eqz
          ;; OUT-OF-BOUNDS
          if
            unreachable
          end
          i32.wrap_i64
        end
        local.set 0
      end
    end 
    unreachable
  )
)
