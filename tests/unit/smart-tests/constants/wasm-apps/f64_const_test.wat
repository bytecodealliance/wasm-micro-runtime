(module
  ;; Enhanced f64.const opcode test functions
  ;; Tests comprehensive double-precision floating-point constant loading
  ;; including IEEE 754 special values, boundary conditions, and edge cases

  ;; Basic Constant Loading Tests - Main Category

  ;; Positive one - fundamental identity value
  (func (export "test_f64_const_pos_one") (result f64)
    f64.const 1.0
  )

  ;; Negative one - fundamental negative identity value
  (func (export "test_f64_const_neg_one") (result f64)
    f64.const -1.0
  )

  ;; Zero - additive identity value
  (func (export "test_f64_const_zero") (result f64)
    f64.const 0.0
  )

  ;; Mathematical constant pi - high-precision double
  (func (export "test_f64_const_pi") (result f64)
    f64.const 3.14159265358979323846
  )

  ;; Mathematical constant e (Euler's number) - high-precision double
  (func (export "test_f64_const_e") (result f64)
    f64.const 2.7182818284590452354
  )

  ;; Boundary Values Tests - Corner Category

  ;; Maximum finite positive f64 value (DBL_MAX = 0x7FEFFFFFFFFFFFFF)
  (func (export "test_f64_const_max") (result f64)
    f64.const 1.7976931348623157e+308
  )

  ;; Maximum finite negative f64 value (-DBL_MAX = 0xFFEFFFFFFFFFFFFF)
  (func (export "test_f64_const_min") (result f64)
    f64.const -1.7976931348623157e+308
  )

  ;; Smallest positive normalized f64 value (DBL_MIN = 0x0010000000000000)
  (func (export "test_f64_const_min_normal") (result f64)
    f64.const 2.2250738585072014e-308
  )

  ;; Smallest positive subnormal f64 value (0x0000000000000001)
  (func (export "test_f64_const_smallest_subnormal") (result f64)
    f64.const 4.9406564584124654e-324
  )

  ;; Largest positive subnormal f64 value (0x000FFFFFFFFFFFFF)
  (func (export "test_f64_const_largest_subnormal") (result f64)
    f64.const 2.225073858507201e-308
  )

  ;; Special IEEE 754 Values Tests - Edge Category

  ;; Positive zero (0x0000000000000000)
  (func (export "test_f64_const_pos_zero") (result f64)
    f64.const 0.0
  )

  ;; Negative zero (0x8000000000000000) - distinct from positive zero
  (func (export "test_f64_const_neg_zero") (result f64)
    f64.const -0.0
  )

  ;; Positive infinity (0x7FF0000000000000)
  (func (export "test_f64_const_pos_inf") (result f64)
    f64.const inf
  )

  ;; Negative infinity (0xFFF0000000000000)
  (func (export "test_f64_const_neg_inf") (result f64)
    f64.const -inf
  )

  ;; Quiet NaN (0x7FF8000000000000 - canonical quiet NaN)
  (func (export "test_f64_const_qnan") (result f64)
    f64.const nan
  )

  ;; Signaling NaN (0x7FF0000000000001) - signaling NaN with payload
  ;; Note: WAT syntax for specific NaN bit patterns
  (func (export "test_f64_const_snan") (result f64)
    f64.const nan:0x0000000000000001
  )

  ;; Multiple Constants and Stack Order Tests - Edge Category

  ;; First constant in sequence
  (func (export "test_f64_const_first") (result f64)
    f64.const 1.0
  )

  ;; Second constant in sequence
  (func (export "test_f64_const_second") (result f64)
    f64.const 2.0
  )

  ;; Third constant in sequence
  (func (export "test_f64_const_third") (result f64)
    f64.const 3.0
  )

  ;; Stack order test - load multiple constants and return sum
  ;; This tests that constants are loaded in proper stack order
  (func (export "test_f64_const_stack_order") (result f64)
    f64.const 10.0
    f64.const 20.0
    f64.const 30.0
    f64.add    ;; 30.0 + 20.0 = 50.0
    f64.add    ;; 50.0 + 10.0 = 60.0
  )

  ;; Additional Edge Case Tests

  ;; Very small positive number close to zero
  (func (export "test_f64_const_tiny_positive") (result f64)
    f64.const 1e-300
  )

  ;; Very small negative number close to zero
  (func (export "test_f64_const_tiny_negative") (result f64)
    f64.const -1e-300
  )

  ;; Large positive number (but still finite)
  (func (export "test_f64_const_large_positive") (result f64)
    f64.const 1e+300
  )

  ;; Large negative number (but still finite)
  (func (export "test_f64_const_large_negative") (result f64)
    f64.const -1e+300
  )

  ;; Fractional constant with high precision
  (func (export "test_f64_const_high_precision") (result f64)
    f64.const 0.1234567890123456
  )

  ;; Integration Tests with Arithmetic Operations

  ;; Test constant used in addition
  (func (export "test_f64_const_with_add") (result f64)
    f64.const 5.5
    f64.const 4.5
    f64.add
  )

  ;; Test constant used in multiplication
  (func (export "test_f64_const_with_mul") (result f64)
    f64.const 3.0
    f64.const 7.0
    f64.mul
  )

  ;; Test constant used in comparison
  (func (export "test_f64_const_with_eq") (result i32)
    f64.const 1.0
    f64.const 1.0
    f64.eq
  )

  ;; Test loading same constant multiple times
  (func (export "test_f64_const_duplicate_loading") (result f64)
    f64.const 2.5
    f64.const 2.5
    f64.add    ;; Should result in 5.0
  )

  ;; Precision Testing Functions

  ;; Machine epsilon for double precision (smallest diff between 1.0 and next representable)
  (func (export "test_f64_const_machine_epsilon") (result f64)
    f64.const 2.220446049250313e-16
  )

  ;; One plus machine epsilon
  (func (export "test_f64_const_one_plus_epsilon") (result f64)
    f64.const 1.0000000000000002
  )

  ;; One minus half machine epsilon (should round to exactly 1.0)
  (func (export "test_f64_const_one_minus_half_epsilon") (result f64)
    f64.const 0.9999999999999999
  )
)