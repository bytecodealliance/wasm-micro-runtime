(module
  (type $elem (array i32))
  (type $outer (array (ref $elem)))

  (func (export "new_invalid_array")
    (array.new_default $outer (i32.const 1))
    drop)
)
