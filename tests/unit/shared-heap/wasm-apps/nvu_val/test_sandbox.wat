(module
  ;; Define memory with initial size = 1 page (64KiB) and max size = 4 pages
  (memory 1 4)

  ;; Function to grow memory dynamically
  (func (export "grow_memory") (param $pages i32) (result i32)
    (memory.grow (local.get $pages)))

  ;; Function to return current memory size in pages
  (func (export "memory_size") (result i32)
    (memory.size))

  ;; Function to store an integer value at a specific address
  (func (export "store") (param $addr i32) (param $val i32)
    (i32.store align=4 (local.get $addr) (local.get $val)))

  ;; Function to load an integer value from a specific address
  (func (export "load") (param $addr i32) (result i32)
    (i32.load align=4 (local.get $addr)))

  ;; Data segment initialization (valid offset)
  (data (i32.const 0) "valid_data")
)

;; ;; Test initial memory size
;; (assert_return (invoke "memory_size") (i32.const 1))

;; ;; Test memory growth
;; (assert_return (invoke "grow_memory" (i32.const 1)) (i32.const 1)) ;; Grow by 1 page
;; (assert_return (invoke "memory_size") (i32.const 2)) ;; Verify new size
;; (assert_return (invoke "grow_memory" (i32.const 2)) (i32.const 2)) ;; Grow to max size
;; (assert_return (invoke "memory_size") (i32.const 4)) ;; Verify max size
;; (assert_return (invoke "grow_memory" (i32.const 1)) (i32.const -1)) ;; Exceed max size

;; ;; Test unaligned store and load
;; (invoke "store" (i32.const 4) (i32.const 100)) ;; Store value 100 at address 4
;; (assert_return (invoke "load" (i32.const 4)) (i32.const 100)) ;; Load value from address 4