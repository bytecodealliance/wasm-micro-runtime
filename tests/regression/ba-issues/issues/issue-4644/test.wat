;; 定义数组类型 - i32数组
(type $i32_array (array (mut i32)))

;; main函数
(func $main
  (local $arr (ref $i32_array))

  ;; 创建一个大小为10的i32数组，初始值为0
  (array.new $i32_array
    (i32.const 0)    ;; 初始值
    (i32.const 10))  ;; 数组大小
  (local.set $arr)

  ;; 使用 array.fill 填充数组
  (array.fill $i32_array
    (local.get $arr)  ;; 数组引用
    (i32.const 0)     ;; 起始索引
    (i32.const 42)    ;; 填充值
    (i32.const 10))   ;; 填充长度 - 填满整个数组
)

(memory 1)
(export "memory" (memory 0))
(export "_start" (func $main))
