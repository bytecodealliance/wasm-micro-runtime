(module
  (type $ft (func))
  (type $st (struct))
  (type $at (array i8))

  (table $ta 10 anyref)
  (table $tf 10 funcref)
  (table $te 10 externref)

  (elem declare func $f)
  (func $f)

  (func (export "init") (param $x externref)
    (table.set $ta (i32.const 0) (ref.null any))
    (table.set $ta (i32.const 1) (ref.null struct))
    (table.set $ta (i32.const 2) (ref.null none))
    (table.set $ta (i32.const 3) (i31.new (i32.const 7)))
    (table.set $ta (i32.const 4) (struct.new_canon_default $st))
    (table.set $ta (i32.const 5) (array.new_canon_default $at (i32.const 0)))
    (table.set $ta (i32.const 6) (extern.internalize (local.get $x)))
    (table.set $ta (i32.const 7) (extern.internalize (ref.null extern)))

    (table.set $tf (i32.const 0) (ref.null nofunc))
    (table.set $tf (i32.const 1) (ref.null func))
    (table.set $tf (i32.const 2) (ref.func $f))

    (table.set $te (i32.const 0) (ref.null noextern))
    (table.set $te (i32.const 1) (ref.null extern))
    (table.set $te (i32.const 2) (local.get $x))
    (table.set $te (i32.const 3) (extern.externalize (i31.new (i32.const 8))))
    (table.set $te (i32.const 4) (extern.externalize (struct.new_canon_default $st)))
    (table.set $te (i32.const 5) (extern.externalize (ref.null any)))
  )

  (func (export "ref_test_null_data") (param $i i32) (result i32)
    (i32.add
      (ref.is_null (table.get $ta (local.get $i)))
      (ref.test null none (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_any") (param $i i32) (result i32)
    (i32.add
      (ref.test any (table.get $ta (local.get $i)))
      (ref.test null any (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_eq") (param $i i32) (result i32)
    (i32.add
      (ref.test eq (table.get $ta (local.get $i)))
      (ref.test null eq (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_i31") (param $i i32) (result i32)
    (i32.add
      (ref.test i31 (table.get $ta (local.get $i)))
      (ref.test null i31 (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_struct") (param $i i32) (result i32)
    (i32.add
      (ref.test struct (table.get $ta (local.get $i)))
      (ref.test null struct (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_array") (param $i i32) (result i32)
    (i32.add
      (ref.test array (table.get $ta (local.get $i)))
      (ref.test null array (table.get $ta (local.get $i)))
    )
  )

  (func (export "ref_test_null_func") (param $i i32) (result i32)
    (i32.add
      (ref.is_null (table.get $tf (local.get $i)))
      (ref.test null nofunc (table.get $tf (local.get $i)))
    )
  )

  (func (export "ref_test_func") (param $i i32) (result i32)
    (i32.add
      (ref.test func (table.get $tf (local.get $i)))
      (ref.test null func (table.get $tf (local.get $i)))
    )
  )

  (func (export "ref_test_null_extern") (param $i i32) (result i32)
    (i32.add
      (ref.is_null (table.get $te (local.get $i)))
      (ref.test null noextern (table.get $te (local.get $i)))
    )
  )

  (func (export "ref_test_extern") (param $i i32) (result i32)
    (i32.add
      (ref.test extern (table.get $te (local.get $i)))
      (ref.test null extern (table.get $te (local.get $i)))
    )
  )
)
