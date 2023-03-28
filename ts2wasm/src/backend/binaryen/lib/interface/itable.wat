(module
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (import "env" "strcmp" (func $strcmp (param i32 i32) (result i32)))
 (memory $0 1)
 (export "find_index" (func $find_index))
 (func $find_index (; 1 ;) (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$0
   (block $label$1
    (br_if $label$1
     (i32.lt_s
      (local.tee $3
       (i32.load offset=4
        (local.get $0)
       )
      )
      (i32.const 1)
     )
    )
    (local.set $0
     (i32.add
      (local.get $0)
      (i32.const 16)
     )
    )
    (local.set $4
     (i32.const 0)
    )
    (loop $label$2
     (block $label$3
      (br_if $label$3
       (call $strcmp
        (i32.load
         (i32.add
          (local.get $0)
          (i32.const -8)
         )
        )
        (local.get $1)
       )
      )
      (br_if $label$0
       (i32.eq
        (i32.load
         (i32.add
          (local.get $0)
          (i32.const -4)
         )
        )
        (local.get $2)
       )
      )
     )
     (local.set $0
      (i32.add
       (local.get $0)
       (i32.const 12)
      )
     )
     (br_if $label$2
      (i32.lt_s
       (local.tee $4
        (i32.add
         (local.get $4)
         (i32.const 1)
        )
       )
       (local.get $3)
      )
     )
    )
   )
   (return
    (i32.const -1)
   )
  )
  (i32.load
   (local.get $0)
  )
 )
)
