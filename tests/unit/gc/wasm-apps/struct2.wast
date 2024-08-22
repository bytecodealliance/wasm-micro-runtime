(module
  (type $vec (struct (field f32) (field $y (mut f32)) (field $z f32)))

  ;;(global (ref $vec) (struct.new_canon $vec (f32.const 1) (f32.const 2) (f32.const 3)))
  (global (ref $vec) (struct.new_canon_default $vec))

  (func (export "new") (result anyref)
    (struct.new_canon_default $vec)
  )

  (func $get_0 (param $v (ref $vec)) (result f32)
    (struct.get $vec 0 (local.get $v))
  )
  (func (export "get_0") (result f32)
    (call $get_0 (struct.new_canon_default $vec))
  )

  (func $set_get_y (param $v (ref $vec)) (param $y f32) (result f32)
    (struct.set $vec $y (local.get $v) (local.get $y))
    (struct.get $vec $y (local.get $v))
  )
  (func (export "set_get_y") (param $y f32) (result f32)
    (call $set_get_y (struct.new_canon_default $vec) (local.get $y))
  )

  (func $set_get_1 (param $v (ref $vec)) (param $y f32) (result f32)
    (struct.set $vec 1 (local.get $v) (local.get $y))
    (struct.get $vec $y (local.get $v))
  )
  (func (export "set_get_1") (param $y f32) (result f32)
    (call $set_get_1 (struct.new_canon_default $vec) (local.get $y))
  )
)
