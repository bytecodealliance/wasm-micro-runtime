(module
  ;; Test functions for i64.const opcode validation
  ;; Each function tests different aspects of constant loading

  ;; Basic constant tests - typical values
  (func (export "test_const_zero") (result i64)
    i64.const 0)

  (func (export "test_const_one") (result i64)
    i64.const 1)

  (func (export "test_const_forty_two") (result i64)
    i64.const 42)

  (func (export "test_const_minus_one") (result i64)
    i64.const -1)

  (func (export "test_const_minus_hundred") (result i64)
    i64.const -100)

  ;; Boundary value tests - extreme limits
  (func (export "test_const_max_int64") (result i64)
    i64.const 9223372036854775807)  ;; INT64_MAX = 0x7FFFFFFFFFFFFFFF

  (func (export "test_const_min_int64") (result i64)
    i64.const -9223372036854775808) ;; INT64_MIN = 0x8000000000000000

  (func (export "test_const_max_int32") (result i64)
    i64.const 2147483647)           ;; INT32_MAX = 0x7FFFFFFF

  (func (export "test_const_min_int32") (result i64)
    i64.const -2147483648)          ;; INT32_MIN = 0x80000000

  (func (export "test_const_uint32_max") (result i64)
    i64.const 4294967295)           ;; UINT32_MAX = 0xFFFFFFFF

  ;; Bit pattern tests - special patterns
  (func (export "test_const_alternating_10") (result i64)
    i64.const -6148914691236517206) ;; 0xAAAAAAAAAAAAAAAA

  (func (export "test_const_alternating_01") (result i64)
    i64.const 6148914691236517205)  ;; 0x5555555555555555

  (func (export "test_const_all_ones") (result i64)
    i64.const -1)                   ;; 0xFFFFFFFFFFFFFFFF

  (func (export "test_const_high_word_only") (result i64)
    i64.const 1311768467463790592)  ;; 0x1234567800000000

  (func (export "test_const_low_word_only") (result i64)
    i64.const 2271560481)           ;; 0x0000000087654321

  ;; Sequential constant tests - stack behavior validation
  (func (export "test_sequential_first") (result i64)
    ;; Load two constants, drop the second, return the first
    i64.const 100
    i64.const 999
    drop)

  (func (export "test_sequential_second") (result i64)
    ;; Load two constants, return the top one (second loaded)
    i64.const 999
    drop
    i64.const 200)

  (func (export "test_sequential_sum") (result i64)
    ;; Load three constants and sum them: 100 + 200 + 300 = 600
    i64.const 100
    i64.const 200
    i64.add
    i64.const 300
    i64.add)

  ;; LEB128 encoding stress tests - maximum encoding length
  (func (export "test_const_large_positive") (result i64)
    i64.const 4611686018427387903)  ;; 0x3FFFFFFFFFFFFFFF

  (func (export "test_const_large_negative") (result i64)
    i64.const -4611686018427387904) ;; 0xC000000000000000

  ;; Power of 2 boundary tests
  (func (export "test_const_power_2_63_minus_1") (result i64)
    i64.const 9223372036854775807)  ;; 2^63 - 1

  (func (export "test_const_power_2_32") (result i64)
    i64.const 4294967296)           ;; 2^32

  (func (export "test_const_power_2_16") (result i64)
    i64.const 65536)                ;; 2^16

  ;; Sign extension tests
  (func (export "test_const_sign_extension_positive") (result i64)
    i64.const 2147483647)           ;; Positive value near 32-bit limit

  (func (export "test_const_sign_extension_negative") (result i64)
    i64.const -2147483648)          ;; Negative value at 32-bit limit

  ;; Zero and identity tests
  (func (export "test_const_identity_zero") (result i64)
    i64.const 0)

  (func (export "test_const_identity_positive_one") (result i64)
    i64.const 1)

  (func (export "test_const_identity_negative_one") (result i64)
    i64.const -1)
)