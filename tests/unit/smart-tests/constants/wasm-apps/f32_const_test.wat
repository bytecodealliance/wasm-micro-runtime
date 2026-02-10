(module
  ;; Basic f32.const test functions

  ;; Standard positive float constant
  (func (export "get_positive_const") (result f32)
    f32.const 1.5
  )

  ;; Standard negative float constant
  (func (export "get_negative_const") (result f32)
    f32.const -3.14159
  )

  ;; Zero constant
  (func (export "get_zero_const") (result f32)
    f32.const 0.0
  )

  ;; Integer-valued float constant
  (func (export "get_integer_const") (result f32)
    f32.const 42.0
  )

  ;; Boundary Values Tests

  ;; Maximum finite positive f32 value (FLT_MAX)
  (func (export "get_max_finite") (result f32)
    f32.const 3.4028235e+38
  )

  ;; Maximum finite negative f32 value (-FLT_MAX)
  (func (export "get_min_finite") (result f32)
    f32.const -3.4028235e+38
  )

  ;; Smallest positive normalized f32 value (FLT_MIN = 0x00800000)
  (func (export "get_min_normal") (result f32)
    f32.const 0x1p-126
  )

  ;; Smallest negative normalized f32 value (-FLT_MIN = 0x80800000)
  (func (export "get_min_normal_neg") (result f32)
    f32.const -0x1p-126
  )

  ;; Subnormal Values Tests

  ;; Smallest positive subnormal f32 value (0x00000001)
  (func (export "get_min_subnormal") (result f32)
    f32.const 1.401298e-45
  )

  ;; Smallest negative subnormal f32 value (0x80000001)
  (func (export "get_min_subnormal_neg") (result f32)
    f32.const -1.401298e-45
  )

  ;; Special IEEE 754 Values Tests

  ;; NaN (Not-a-Number)
  (func (export "get_nan") (result f32)
    f32.const nan
  )

  ;; Positive infinity
  (func (export "get_pos_inf") (result f32)
    f32.const inf
  )

  ;; Negative infinity
  (func (export "get_neg_inf") (result f32)
    f32.const -inf
  )

  ;; Positive zero (0x00000000)
  (func (export "get_pos_zero") (result f32)
    f32.const 0.0
  )

  ;; Negative zero (0x80000000)
  (func (export "get_neg_zero") (result f32)
    f32.const -0.0
  )

  ;; Bit Pattern Preservation Tests

  ;; Specific pattern for testing bit-level preservation
  (func (export "get_specific_pattern") (result f32)
    f32.const 1.23456789
  )

  ;; Signaling NaN (implementation may convert to quiet NaN)
  (func (export "get_signaling_nan") (result f32)
    f32.const nan:0x200000
  )

  ;; Quiet NaN
  (func (export "get_quiet_nan") (result f32)
    f32.const nan:0x400000
  )

  ;; Multiple Constants Test

  ;; Load multiple constants in sequence
  (func (export "get_multiple_constants") (result f32 f32 f32)
    f32.const 1.0
    f32.const 2.5
    f32.const -7.75
  )

  ;; Integration Tests with Operations

  ;; Add two constants: 2.5 + 3.7 = 6.2
  (func (export "add_two_constants") (result f32)
    f32.const 2.5
    f32.const 3.7
    f32.add
  )

  ;; Subtract constants: 10.0 - 3.5 = 6.5
  (func (export "subtract_constants") (result f32)
    f32.const 10.0
    f32.const 3.5
    f32.sub
  )

  ;; Multiply constants: 2.0 * 1.5 = 3.0
  (func (export "multiply_constants") (result f32)
    f32.const 2.0
    f32.const 1.5
    f32.mul
  )

  ;; Additional edge cases

  ;; Very small positive number close to subnormal boundary
  (func (export "get_small_normal") (result f32)
    f32.const 1.175495e-38
  )

  ;; Very large number close to infinity boundary
  (func (export "get_large_normal") (result f32)
    f32.const 3.4028234e+38
  )

  ;; Precision test - number with many decimal places
  (func (export "get_precision_test") (result f32)
    f32.const 1.2345678901234567890
  )

  ;; Mathematical constants
  (func (export "get_pi") (result f32)
    f32.const 3.141592653589793
  )

  (func (export "get_e") (result f32)
    f32.const 2.718281828459045
  )

  ;; Powers of 2 (exact representation)
  (func (export "get_power_of_two") (result f32)
    f32.const 1024.0
  )

  ;; Negative power of 2
  (func (export "get_negative_power_of_two") (result f32)
    f32.const -512.0
  )

  ;; Fractional power of 2
  (func (export "get_fractional_power_of_two") (result f32)
    f32.const 0.125
  )
)