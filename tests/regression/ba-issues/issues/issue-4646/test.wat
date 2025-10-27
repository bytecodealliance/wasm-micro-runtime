;; define different reference types
(type $struct_a (struct (field (mut i32))))
(type $struct_b (struct (field (mut i64))))
(type $struct_c (struct (field (mut i32)) (field (mut i32))))

(func $main
  ;; prepare parameters: i32, ref_a, i32, ref_b
  (i32.const 10)
  (struct.new $struct_a (i32.const 100))
  (i32.const 20)
  (struct.new $struct_b (i64.const 200))

  ;; block with interleaved parameters: i32, ref_a, i32, ref_b -> ref_c
  (block (param i32 (ref $struct_a) i32 (ref $struct_b)) (result (ref $struct_c))
    ;; clean up parameters from stack
    drop  ;; drop ref_b
    drop  ;; drop i32
    drop  ;; drop ref_a
    drop  ;; drop i32

    ;; return new type reference struct_c
    (struct.new $struct_c (i32.const 300) (i32.const 400))
  )

  ;; drop return value
  drop
)

(memory 1)
(export "memory" (memory 0))
(export "_start" (func $main))