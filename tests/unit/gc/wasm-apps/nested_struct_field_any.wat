(module
  (type $struct_type (struct (field (mut i32)) (field (mut anyref))))

  (global $g_struct
    (mut (ref $struct_type))
    (struct.new $struct_type
      (i32.const 10)
      (struct.new $struct_type
        (i32.const 20)
        (struct.new_default $struct_type)
      )
    )
  )

  ;; assert_return(invoke "get_field1"), 10)
  (func (export "get_field1") (result i32)
    (struct.get $struct_type 0 (global.get $g_struct))
  )

  ;; assert_return(invoke "get_field1"), struct.new $struct_type ...)
  (func (export "get_field2") (result anyref)
    (struct.get $struct_type 1 (global.get $g_struct))
  )

  ;; assert_return(invoke "get_field2_field1"), 20)
  (func (export "get_field2_field1") (result i32)
    (struct.get $struct_type 0
      (ref.cast structref
        (struct.get $struct_type 1 (global.get $g_struct))
      )
    )
  )

  ;; assert_return(invoke "get_field2_field2"), struct.new_default $struct_type ...)
  (func (export "get_field2_field2") (result anyref)
    (struct.get $struct_type 1
      (ref.cast structref
        (struct.get $struct_type 1 (global.get $g_struct))
      )
    )
  )

  ;; assert_return(invoke "get_field2_field2_field1"), 0)
  (func (export "get_field2_field2_field1") (result i32)
    (struct.get $struct_type 0
      (ref.cast structref
        (struct.get $struct_type 1
          (ref.cast structref
            (struct.get $struct_type 1 (global.get $g_struct))
          )
        )
      )
    )
  )
)
