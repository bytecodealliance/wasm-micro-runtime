;; define array type - i32 array
(type $i32_array (array (mut i32)))

;; main function
(func $main
  (local $arr (ref $i32_array))

  ;; create an i32 array of size 10 with initial value 0
  (array.new $i32_array
    (i32.const 0)    ;; initial value
    (i32.const 10))  ;; array size
  (local.set $arr)

  ;; fill array using array.fill
  (array.fill $i32_array
    (local.get $arr)  ;; array reference
    (i32.const 0)     ;; start index
    (i32.const 42)    ;; fill value
    (i32.const 10))   ;; fill length - fill entire array
)

(memory 1)
(export "memory" (memory 0))
(export "_start" (func $main))
