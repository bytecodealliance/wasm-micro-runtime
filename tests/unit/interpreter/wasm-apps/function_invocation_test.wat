(module
  ;; Import functions
  (import "env" "test_import_add" (func $test_import_add (param i32 i32) (result i32)))
  (import "env" "test_import_mul" (func $test_import_mul (param i32 i32) (result i32)))
  (import "env" "malloc" (func $malloc (param i32) (result i32)))
  (import "env" "free" (func $free (param i32)))
  (import "env" "native_func" (func $native_func (param i32) (result i32)))
  
  ;; Type definitions
  (type $void_to_void (func))
  (type $i32_to_i32 (func (param i32) (result i32)))
  (type $i32_i32_to_i32 (func (param i32 i32) (result i32)))
  
  ;; Memory and table
  (memory 1)
  (table 4 funcref)
  
  ;; Local functions for table
  (func $add_func (type $i32_to_i32) (param $x i32) (result i32)
    local.get $x
    i32.const 10
    i32.add)
  
  (func $mul_func (type $i32_to_i32) (param $x i32) (result i32)
    local.get $x
    i32.const 2
    i32.mul)
  
  (func $identity_func (type $i32_to_i32) (param $x i32) (result i32)
    local.get $x)
  
  (func $void_func (type $void_to_void))
  
  ;; Initialize table
  (elem (i32.const 0) $add_func $mul_func $identity_func $void_func)
  
  ;; Test functions matching the C++ test expectations
  (func (export "test_call_indirect_valid") (param $idx i32) (param $val i32) (result i32)
    local.get $val
    local.get $idx
    call_indirect (type $i32_to_i32))
  
  (func (export "test_call_indirect_invalid_index") (param $val i32) (result i32)
    local.get $val
    i32.const 10
    call_indirect (type $i32_to_i32))
  
  (func (export "test_call_indirect_type_mismatch") (param $val i32) (result i32)
    local.get $val
    i32.const 3
    call_indirect (type $i32_to_i32))
  
  (func (export "test_import_function_call") (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    call $test_import_add)
  
  (func (export "test_import_function_mul") (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    call $test_import_mul)
  
  (func (export "test_native_function_call") (param $a i32) (result i32)
    local.get $a
    call $native_func)
  
  (func (export "test_malloc_operation") (param $size i32) (result i32)
    local.get $size
    call $malloc)
  
  (func (export "test_free_operation") (param $ptr i32)
    local.get $ptr
    call $free)
  
  (func (export "test_malloc_free_cycle") (param $size i32) (result i32)
    (local $ptr i32)
    local.get $size
    call $malloc
    local.set $ptr
    
    local.get $ptr
    i32.const 42
    i32.store
    
    local.get $ptr
    i32.load
    
    local.get $ptr
    call $free)
  
  (func (export "test_stack_operations") (param $val1 i32) (param $val2 i32) (result i32)
    local.get $val1
    local.get $val2
    call $test_import_add
    
    i32.const 5
    call $add_func
    i32.add
    
    local.get $val2
    i32.const 3
    call $test_import_mul
    i32.add)
  
  (func (export "test_complex_indirect_calls") (param $selector i32) (param $value i32) (result i32)
    local.get $selector
    i32.const 0
    i32.eq
    if (result i32)
      local.get $value
      i32.const 0
      call_indirect (type $i32_to_i32)
    else
      local.get $selector
      i32.const 1
      i32.eq
      if (result i32)
        local.get $value
        i32.const 1
        call_indirect (type $i32_to_i32)
      else
        local.get $value
        i32.const 2
        call_indirect (type $i32_to_i32)
      end
    end)
  
  (func (export "test_large_param_stack") 
    (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32)
    (param $p5 i32) (param $p6 i32) (param $p7 i32) (param $p8 i32)
    (result i32)
    local.get $p1
    local.get $p2
    call $test_import_add
    
    local.get $p3
    local.get $p4
    call $test_import_add
    i32.add
    
    local.get $p5
    local.get $p6
    call $test_import_add
    i32.add
    
    local.get $p7
    local.get $p8
    call $test_import_add
    i32.add)
)