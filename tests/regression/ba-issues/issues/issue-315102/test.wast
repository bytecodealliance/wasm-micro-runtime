(module
  (data $string_data "asdf")
  (func $print (import "spectest" "print_i32") (param $i i32))
  (memory $memory (export "memory") 1)
  (type $string (array (mut i8)))
  (func $init
    (local $str (ref null $string))
    (array.new_data $string $string_data (i32.const 0) (i32.const 4))
    (local.tee $str)
    (ref.as_non_null) 
    (array.len)
    (call $print)
  )
  (export "_start" (func $init))
)
