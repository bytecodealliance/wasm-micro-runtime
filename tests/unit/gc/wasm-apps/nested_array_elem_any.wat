(module
  (type $array_type (array (mut anyref)))

  (global $g_array
    (mut (ref $array_type))
    (array.new_fixed $array_type 2
      (ref.i31 (i32.const 10))
      (array.new_fixed $array_type 2
        (ref.i31 (i32.const 20))
        (array.new_default $array_type (i32.const 2))
      )
    )
  )

  ;; assert_return(invoke "get_elem0"), 10)
  (func (export "get_elem0") (result i32)
    (i31.get_s (ref.cast i31ref (array.get $array_type (global.get $g_array) (i32.const 0))))
  )

  ;; assert_return(invoke "get_elem1"), array.new_fixed $array_type ...)
  (func (export "get_elem1") (result anyref)
    (array.get $array_type (global.get $g_array) (i32.const 1))
  )

  ;; assert_return(invoke "get_elem1_elem0"), 20)
  (func (export "get_elem1_elem0") (result i32)
    (i31.get_s (ref.cast i31ref
      (array.get $array_type
        (ref.cast (ref $array_type)
          (array.get $array_type (global.get $g_array) (i32.const 1))
        )
        (i32.const 0)
      )
    ))
  )

  ;; assert_return(invoke "get_elem1_elem1"), array.new_default $array_type ...)
  (func (export "get_elem1_elem1") (result anyref)
    (array.get $array_type
      (ref.cast (ref $array_type)
        (array.get $array_type (global.get $g_array) (i32.const 1))
      )
      (i32.const 1)
    )
  )

  ;; assert_return(invoke "get_elem1_elem1_elem0"), 0)
  (func (export "get_elem1_elem1_elem0") (result i32)
    (i31.get_s (ref.cast i31ref
      (array.get $array_type
        (ref.cast (ref $array_type)
          (array.get $array_type
            (ref.cast (ref $array_type)
              (array.get $array_type (global.get $g_array) (i32.const 1))
            )
            (i32.const 1)
          )
        )
        (i32.const 0)
      )
    ))
  )
)