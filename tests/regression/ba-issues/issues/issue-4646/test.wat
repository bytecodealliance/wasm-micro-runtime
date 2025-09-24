;; 定义不同的引用类型
(type $struct_a (struct (field (mut i32))))
(type $struct_b (struct (field (mut i64))))
(type $struct_c (struct (field (mut i32)) (field (mut i32))))

(func $main
  ;; 准备参数：i32, ref_a, i32, ref_b
  (i32.const 10)
  (struct.new $struct_a (i32.const 100))
  (i32.const 20)
  (struct.new $struct_b (i64.const 200))

  ;; 带交错参数的block：i32, ref_a, i32, ref_b -> ref_c
  (block (param i32 (ref $struct_a) i32 (ref $struct_b)) (result (ref $struct_c))
    ;; 清理栈中的参数
    drop  ;; 丢弃 ref_b
    drop  ;; 丢弃 i32
    drop  ;; 丢弃 ref_a
    drop  ;; 丢弃 i32
    ;; 返回新的第三种类型引用

    (struct.new $struct_c (i32.const 300) (i32.const 400))
  )

  ;; 丢弃返回值
  drop
)

(memory 1)
(export "memory" (memory 0))
(export "_start" (func $main))