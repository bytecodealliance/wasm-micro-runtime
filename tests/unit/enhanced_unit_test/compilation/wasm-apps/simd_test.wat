(module
  ;; Memory declaration for SIMD operations
  (memory 1)
  
  ;; SIMD Lane Access Operations
  (func (export "test_v128_extract_lane_i8") (param $vec v128) (result i32)
    local.get $vec
    i8x16.extract_lane_s 0
  )
  
  (func (export "test_v128_replace_lane_i16") (param $vec v128) (param $value i32) (result v128)
    local.get $vec
    local.get $value
    i16x8.replace_lane 3
  )
  
  ;; SIMD Bitmask Extracts
  (func (export "test_i8x16_bitmask") (param $vec v128) (result i32)
    local.get $vec
    i8x16.bitmask
  )
  
  (func (export "test_i32x4_bitmask") (param $vec v128) (result i32)
    local.get $vec
    i32x4.bitmask
  )
  
  ;; SIMD Bit Shifts
  (func (export "test_i16x8_shl") (param $vec v128) (param $shift i32) (result v128)
    local.get $vec
    local.get $shift
    i16x8.shl
  )
  
  (func (export "test_i32x4_shr_s") (param $vec v128) (param $shift i32) (result v128)
    local.get $vec
    local.get $shift
    i32x4.shr_s
  )
  
  ;; SIMD Bitwise Operations
  (func (export "test_v128_and") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    v128.and
  )
  
  (func (export "test_v128_or") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    v128.or
  )
  
  (func (export "test_v128_xor") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    v128.xor
  )
  
  (func (export "test_v128_not") (param $vec v128) (result v128)
    local.get $vec
    v128.not
  )
  
  ;; SIMD Boolean Reductions
  (func (export "test_v128_any_true") (param $vec v128) (result i32)
    local.get $vec
    v128.any_true
  )
  
  (func (export "test_i8x16_all_true") (param $vec v128) (result i32)
    local.get $vec
    i8x16.all_true
  )
  
  ;; SIMD Comparisons
  (func (export "test_i32x4_eq") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i32x4.eq
  )
  
  (func (export "test_f32x4_lt") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    f32x4.lt
  )
  
  (func (export "test_i16x8_gt_s") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i16x8.gt_s
  )
  
  ;; SIMD Conversions
  (func (export "test_f32x4_convert_i32x4_s") (param $vec v128) (result v128)
    local.get $vec
    f32x4.convert_i32x4_s
  )
  
  (func (export "test_i32x4_trunc_sat_f32x4_s") (param $vec v128) (result v128)
    local.get $vec
    i32x4.trunc_sat_f32x4_s
  )
  
  ;; SIMD Value Construction
  (func (export "test_i32x4_splat") (param $value i32) (result v128)
    local.get $value
    i32x4.splat
  )
  
  (func (export "test_f64x2_splat") (param $value f64) (result v128)
    local.get $value
    f64x2.splat
  )
  
  ;; SIMD Floating Point Operations
  (func (export "test_f32x4_add") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    f32x4.add
  )
  
  (func (export "test_f64x2_mul") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    f64x2.mul
  )
  
  (func (export "test_f32x4_sqrt") (param $vec v128) (result v128)
    local.get $vec
    f32x4.sqrt
  )
  
  ;; SIMD Integer Arithmetic
  (func (export "test_i32x4_add") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i32x4.add
  )
  
  (func (export "test_i16x8_mul") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i16x8.mul
  )
  
  (func (export "test_i8x16_neg") (param $vec v128) (result v128)
    local.get $vec
    i8x16.neg
  )
  
  ;; SIMD Load/Store Operations
  (func (export "test_v128_load") (param $addr i32) (result v128)
    local.get $addr
    v128.load
  )
  
  (func (export "test_v128_store") (param $addr i32) (param $vec v128)
    local.get $addr
    local.get $vec
    v128.store
  )
  
  (func (export "test_v128_load8_splat") (param $addr i32) (result v128)
    local.get $addr
    v128.load8_splat
  )
  
  ;; SIMD Saturated Arithmetic
  (func (export "test_i8x16_add_sat_s") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i8x16.add_sat_s
  )
  
  (func (export "test_i16x8_sub_sat_u") (param $a v128) (param $b v128) (result v128)
    local.get $a
    local.get $b
    i16x8.sub_sat_u
  )
  
  ;; Advanced Constant Operations
  (func (export "test_const_v128") (result v128)
    v128.const i32x4 0x12345678 0x9abcdef0 0x11111111 0x22222222
  )
  
  ;; Complex Memory Operations with SIMD
  (func (export "test_memory_fill_simd") (param $dest i32) (param $size i32)
    (local $vec v128)
    ;; Create a pattern vector
    i32.const 0x12345678
    i32x4.splat
    local.set $vec
    
    ;; Fill memory with SIMD vector
    local.get $dest
    local.get $vec
    v128.store
  )
  
  ;; Advanced Control Flow with SIMD
  (func (export "test_simd_conditional") (param $condition i32) (param $a v128) (param $b v128) (result v128)
    local.get $condition
    if (result v128)
      local.get $a
    else
      local.get $b
    end
  )
  
  ;; SIMD with Exception Handling Potential
  (func (export "test_simd_bounds_check") (param $addr i32) (result v128)
    ;; This function tests memory bounds with SIMD operations
    local.get $addr
    i32.const 65520  ;; Close to memory limit
    i32.add
    v128.load
  )
  
  ;; Simple test function for basic compilation
  (func (export "test_simple") (result i32)
    i32.const 42
  )
)