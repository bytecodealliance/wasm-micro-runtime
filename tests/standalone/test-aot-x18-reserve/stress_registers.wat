;; Copyright (C) 2019 Intel Corporation. All rights reserved.
;; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

;; Test module that uses many local variables to stress register allocation.
;; On ARM64, this will force LLVM to use x18 register if +reserve-x18 is not set.
;; x18 is reserved by Apple on macOS for TLS, so using it causes crashes.

(module
  (memory (export "memory") 1)

  (func $stress_registers (export "stress_registers") (param $input i64) (result i64)
    (local $a i64) (local $b i64) (local $c i64) (local $d i64)
    (local $e i64) (local $f i64) (local $g i64) (local $h i64)
    (local $i i64) (local $j i64) (local $k i64) (local $l i64)
    (local $m i64) (local $n i64) (local $o i64) (local $p i64)
    (local $q i64) (local $r i64) (local $s i64) (local $t i64)
    (local $u i64) (local $v i64) (local $w i64) (local $x i64)

    ;; Initialize all locals with different values based on input
    (local.set $a (i64.add (local.get $input) (i64.const 1)))
    (local.set $b (i64.mul (local.get $a) (i64.const 2)))
    (local.set $c (i64.add (local.get $b) (i64.const 3)))
    (local.set $d (i64.mul (local.get $c) (i64.const 4)))
    (local.set $e (i64.add (local.get $d) (i64.const 5)))
    (local.set $f (i64.mul (local.get $e) (i64.const 6)))
    (local.set $g (i64.add (local.get $f) (i64.const 7)))
    (local.set $h (i64.mul (local.get $g) (i64.const 8)))
    (local.set $i (i64.add (local.get $h) (i64.const 9)))
    (local.set $j (i64.mul (local.get $i) (i64.const 10)))
    (local.set $k (i64.add (local.get $j) (i64.const 11)))
    (local.set $l (i64.mul (local.get $k) (i64.const 12)))
    (local.set $m (i64.add (local.get $l) (i64.const 13)))
    (local.set $n (i64.mul (local.get $m) (i64.const 14)))
    (local.set $o (i64.add (local.get $n) (i64.const 15)))
    (local.set $p (i64.mul (local.get $o) (i64.const 16)))
    (local.set $q (i64.add (local.get $p) (i64.const 17)))
    (local.set $r (i64.mul (local.get $q) (i64.const 18)))
    (local.set $s (i64.add (local.get $r) (i64.const 19)))
    (local.set $t (i64.mul (local.get $s) (i64.const 20)))
    (local.set $u (i64.add (local.get $t) (i64.const 21)))
    (local.set $v (i64.mul (local.get $u) (i64.const 22)))
    (local.set $w (i64.add (local.get $v) (i64.const 23)))
    (local.set $x (i64.mul (local.get $w) (i64.const 24)))

    ;; Now use all of them together to prevent optimization
    (i64.add
      (i64.add
        (i64.add
          (i64.add
            (i64.add
              (i64.add
                (i64.add
                  (i64.add
                    (i64.add
                      (i64.add
                        (i64.add
                          (i64.add
                            (local.get $a)
                            (local.get $b))
                          (local.get $c))
                        (local.get $d))
                      (local.get $e))
                    (local.get $f))
                  (local.get $g))
                (local.get $h))
              (local.get $i))
            (local.get $j))
          (local.get $k))
        (local.get $l))
      (i64.add
        (i64.add
          (i64.add
            (i64.add
              (i64.add
                (i64.add
                  (i64.add
                    (i64.add
                      (i64.add
                        (i64.add
                          (i64.add
                            (local.get $m)
                            (local.get $n))
                          (local.get $o))
                        (local.get $p))
                      (local.get $q))
                    (local.get $r))
                  (local.get $s))
                (local.get $t))
              (local.get $u))
            (local.get $v))
          (local.get $w))
        (local.get $x))))

  (func $_start (export "_start")
    (drop (call $stress_registers (i64.const 42)))
  )
)
