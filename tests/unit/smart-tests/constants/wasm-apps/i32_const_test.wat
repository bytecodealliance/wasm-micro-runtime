(module
  ;; Test functions for basic constant loading
  (func (export "const_positive_one") (result i32)
    i32.const 1)

  (func (export "const_positive_42") (result i32)
    i32.const 42)

  (func (export "const_positive_100") (result i32)
    i32.const 100)

  (func (export "const_negative_one") (result i32)
    i32.const -1)

  (func (export "const_negative_42") (result i32)
    i32.const -42)

  (func (export "const_negative_100") (result i32)
    i32.const -100)

  (func (export "const_zero") (result i32)
    i32.const 0)

  ;; Test functions for boundary values
  (func (export "const_int32_max") (result i32)
    i32.const 2147483647) ;; INT32_MAX

  (func (export "const_int32_min") (result i32)
    i32.const -2147483648) ;; INT32_MIN

  (func (export "const_int32_max_minus_one") (result i32)
    i32.const 2147483646) ;; INT32_MAX - 1

  (func (export "const_int32_min_plus_one") (result i32)
    i32.const -2147483647) ;; INT32_MIN + 1

  ;; Test functions for special bit patterns
  (func (export "const_all_bits_set") (result i32)
    i32.const 0xFFFFFFFF) ;; All bits set (-1 in two's complement)

  (func (export "const_alternating_01") (result i32)
    i32.const 0x55555555) ;; Alternating 01010101...

  (func (export "const_alternating_10") (result i32)
    i32.const 0xAAAAAAAA) ;; Alternating 10101010...

  (func (export "const_power_of_two_0") (result i32)
    i32.const 1) ;; 2^0

  (func (export "const_power_of_two_10") (result i32)
    i32.const 1024) ;; 2^10

  (func (export "const_power_of_two_30") (result i32)
    i32.const 1073741824) ;; 2^30

  ;; Test functions for sequential loading and stack order
  (func (export "const_sequential_first") (result i32)
    ;; Return 30 to simulate first popped from sequence [10, 20, 30]
    i32.const 30)

  (func (export "const_sequential_second") (result i32)
    ;; Return 20 to simulate second popped from sequence [10, 20, 30]
    i32.const 20)

  (func (export "const_sequential_third") (result i32)
    ;; Return 10 to simulate third popped from sequence [10, 20, 30]
    i32.const 10)

  (func (export "const_empty_stack_load") (result i32)
    ;; Load constant on empty stack
    i32.const 99)
)