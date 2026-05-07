(module
  ;; Export a function for division operation
  (func (export "divide_by_zero") (param $numerator i32) (result i32)
    ;; Directly perform division by zero
    (i32.div_s (local.get $numerator) (i32.const 0))
  )
)