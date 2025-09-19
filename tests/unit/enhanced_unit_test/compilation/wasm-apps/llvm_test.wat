(module
  ;; Memory definition: 64KB for basic LLVM optimization testing
  (memory 1)
  
  ;; Data section with test patterns for optimization
  (data (i32.const 0) "LLVM_TEST_DATA_PATTERN_FOR_OPTIMIZATION")
  (data (i32.const 64) "\01\02\03\04\05\06\07\08\09\0a\0b\0c\0d\0e\0f\10")
  
  ;; Global variables for optimization testing
  (global $counter (mut i32) (i32.const 0))
  (global $accumulator (mut i64) (i64.const 0))
  (global $float_state (mut f32) (f32.const 1.5))
  (global $double_state (mut f64) (f64.const 2.718281828))
  
  ;; Function 1: Basic arithmetic for constant propagation testing
  (func (export "test_constant_propagation") (result i32)
    (local $a i32)
    (local $b i32)
    (local $c i32)
    
    ;; Constants that should be propagated by LLVM
    (local.set $a (i32.const 10))
    (local.set $b (i32.const 20))
    (local.set $c (i32.add (local.get $a) (local.get $b)))
    
    ;; More complex constant expressions
    (i32.mul 
      (local.get $c)
      (i32.add (i32.const 5) (i32.const 3))
    )
  )
  
  ;; Function 2: Loop optimization testing
  (func (export "test_loop_optimization") (param $iterations i32) (result i32)
    (local $i i32)
    (local $sum i32)
    
    (local.set $i (i32.const 0))
    (local.set $sum (i32.const 0))
    
    (loop $loop_main
      (local.set $sum 
        (i32.add (local.get $sum) (local.get $i))
      )
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      
      (br_if $loop_main 
        (i32.lt_s (local.get $i) (local.get $iterations))
      )
    )
    
    (local.get $sum)
  )
  
  ;; Function 3: Dead code elimination testing
  (func (export "test_dead_code_elimination") (result i32)
    (local $live_var i32)
    (local $dead_var i32)
    (local $another_dead i32)
    
    ;; Live code path
    (local.set $live_var (i32.const 42))
    
    ;; Dead code that should be eliminated
    (local.set $dead_var (i32.const 999))
    (local.set $another_dead (i32.mul (local.get $dead_var) (i32.const 2)))
    
    ;; Only return live variable
    (local.get $live_var)
  )
  
  ;; Function 4: Function inlining candidate
  (func $inline_candidate (param $x i32) (result i32)
    (i32.add (local.get $x) (i32.const 1))
  )
  
  (func (export "test_function_inlining") (param $input i32) (result i32)
    ;; Small function calls that should be inlined
    (call $inline_candidate
      (call $inline_candidate
        (call $inline_candidate (local.get $input))
      )
    )
  )
  
  ;; Function 5: Memory operations for layout optimization
  (func (export "test_memory_layout_optimization") (param $offset i32) (result i32)
    (local $val1 i32)
    (local $val2 i32)
    (local $val3 i32)
    
    ;; Sequential memory accesses for optimization
    (local.set $val1 (i32.load (local.get $offset)))
    (local.set $val2 (i32.load (i32.add (local.get $offset) (i32.const 4))))
    (local.set $val3 (i32.load (i32.add (local.get $offset) (i32.const 8))))
    
    ;; Store optimized pattern
    (i32.store (i32.add (local.get $offset) (i32.const 12)) 
      (i32.add 
        (i32.add (local.get $val1) (local.get $val2))
        (local.get $val3)
      )
    )
    
    (i32.load (i32.add (local.get $offset) (i32.const 12)))
  )
  
  ;; Function 6: Branch optimization testing
  (func (export "test_branch_optimization") (param $condition i32) (result i32)
    (local $result i32)
    
    (if (local.get $condition)
      (then
        (local.set $result (i32.const 100))
      )
      (else
        (local.set $result (i32.const 200))
      )
    )
    
    ;; Predictable branch pattern for optimization
    (if (i32.gt_s (local.get $result) (i32.const 150))
      (then
        (local.set $result (i32.sub (local.get $result) (i32.const 50)))
      )
    )
    
    (local.get $result)
  )
  
  ;; Function 7: Floating point optimization
  (func (export "test_floating_point_optimization") (param $x f32) (param $y f32) (result f32)
    (local $temp1 f32)
    (local $temp2 f32)
    
    ;; Operations that can be optimized
    (local.set $temp1 (f32.mul (local.get $x) (f32.const 2.0)))
    (local.set $temp2 (f32.add (local.get $y) (f32.const 0.0)))
    
    ;; Should optimize to simple multiplication
    (f32.div 
      (f32.add (local.get $temp1) (local.get $temp2))
      (f32.const 1.0)
    )
  )
  
  ;; Function 8: Integer optimization patterns
  (func (export "test_integer_optimization") (param $a i64) (param $b i64) (result i64)
    (local $temp i64)
    
    ;; Patterns for strength reduction
    (local.set $temp (i64.mul (local.get $a) (i64.const 8)))  ;; Should become shift
    
    ;; Algebraic simplification opportunities
    (i64.add
      (i64.sub (local.get $temp) (local.get $b))
      (local.get $b)  ;; Should simplify to just temp
    )
  )
  
  ;; Function 9: Register allocation stress test
  (func (export "test_register_allocation") (param $p1 i32) (param $p2 i32) (param $p3 i32) (result i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32)
    (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    
    ;; Create register pressure
    (local.set $l1 (i32.add (local.get $p1) (i32.const 1)))
    (local.set $l2 (i32.add (local.get $p2) (i32.const 2)))
    (local.set $l3 (i32.add (local.get $p3) (i32.const 3)))
    (local.set $l4 (i32.mul (local.get $l1) (local.get $l2)))
    (local.set $l5 (i32.mul (local.get $l2) (local.get $l3)))
    (local.set $l6 (i32.mul (local.get $l3) (local.get $l1)))
    (local.set $l7 (i32.add (local.get $l4) (local.get $l5)))
    (local.set $l8 (i32.add (local.get $l6) (local.get $l7)))
    
    (local.get $l8)
  )
  
  ;; Function 10: Vectorization opportunity (if SIMD enabled)
  (func (export "test_vectorization_opportunity") (param $base_addr i32) (result i32)
    (local $i i32)
    (local $sum i32)
    
    (local.set $i (i32.const 0))
    (local.set $sum (i32.const 0))
    
    ;; Loop that could be vectorized
    (loop $vector_loop
      (local.set $sum
        (i32.add (local.get $sum)
          (i32.load (i32.add (local.get $base_addr) 
            (i32.mul (local.get $i) (i32.const 4))
          ))
        )
      )
      
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br_if $vector_loop (i32.lt_s (local.get $i) (i32.const 4)))
    )
    
    (local.get $sum)
  )
  
  ;; Function 11: Cross-function optimization
  (func $helper_function (param $x i32) (result i32)
    (i32.shl (local.get $x) (i32.const 1))  ;; Multiply by 2
  )
  
  (func (export "test_cross_function_optimization") (param $input i32) (result i32)
    (local $temp i32)
    
    (local.set $temp (call $helper_function (local.get $input)))
    (call $helper_function (local.get $temp))
  )
  
  ;; Function 12: Global variable optimization
  (func (export "test_global_optimization") (result i32)
    ;; Operations on globals that can be optimized
    (global.set $counter (i32.add (global.get $counter) (i32.const 1)))
    (global.set $accumulator 
      (i64.add (global.get $accumulator) 
        (i64.extend_i32_s (global.get $counter))
      )
    )
    
    (i32.wrap_i64 (global.get $accumulator))
  )
  
  ;; Function 13: Exception handling optimization (if supported)
  (func (export "test_exception_handling_optimization") (param $divisor i32) (result i32)
    (local $result i32)
    
    ;; Division that could trap - optimization should handle gracefully
    (if (i32.eqz (local.get $divisor))
      (then
        (local.set $result (i32.const -1))
      )
      (else
        (local.set $result (i32.div_s (i32.const 100) (local.get $divisor)))
      )
    )
    
    (local.get $result)
  )
  
  ;; Function 14: Instruction scheduling test
  (func (export "test_instruction_scheduling") (param $a i32) (param $b i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local $temp3 i32)
    
    ;; Independent operations that can be reordered
    (local.set $temp1 (i32.mul (local.get $a) (i32.const 3)))
    (local.set $temp2 (i32.add (local.get $b) (i32.const 7)))
    (local.set $temp3 (i32.sub (local.get $a) (local.get $b)))
    
    ;; Dependent operations
    (i32.add
      (i32.add (local.get $temp1) (local.get $temp2))
      (local.get $temp3)
    )
  )
  
  ;; Function 15: Complex control flow for optimization
  (func (export "test_complex_control_flow") (param $selector i32) (result i32)
    (local $result i32)
    
    (block $exit
      (block $case3
        (block $case2
          (block $case1
            (block $case0
              (br_table $case0 $case1 $case2 $case3 $exit
                (local.get $selector)
              )
            )
            (local.set $result (i32.const 10))
            (br $exit)
          )
          (local.set $result (i32.const 20))
          (br $exit)
        )
        (local.set $result (i32.const 30))
        (br $exit)
      )
      (local.set $result (i32.const 40))
    )
    
    (local.get $result)
  )
)