(module
  (memory 1)
  (export "memory" (memory 0))

  (func $test
    ;; Add 130 i64 locals (260 slots) to push v128 past offset 256
    (local $d0 i64) (local $d1 i64) (local $d2 i64) (local $d3 i64) (local $d4 i64)
    (local $d5 i64) (local $d6 i64) (local $d7 i64) (local $d8 i64) (local $d9 i64)
    (local $d10 i64) (local $d11 i64) (local $d12 i64) (local $d13 i64) (local $d14 i64)
    (local $d15 i64) (local $d16 i64) (local $d17 i64) (local $d18 i64) (local $d19 i64)
    (local $d20 i64) (local $d21 i64) (local $d22 i64) (local $d23 i64) (local $d24 i64)
    (local $d25 i64) (local $d26 i64) (local $d27 i64) (local $d28 i64) (local $d29 i64)
    (local $d30 i64) (local $d31 i64) (local $d32 i64) (local $d33 i64) (local $d34 i64)
    (local $d35 i64) (local $d36 i64) (local $d37 i64) (local $d38 i64) (local $d39 i64)
    (local $d40 i64) (local $d41 i64) (local $d42 i64) (local $d43 i64) (local $d44 i64)
    (local $d45 i64) (local $d46 i64) (local $d47 i64) (local $d48 i64) (local $d49 i64)
    (local $d50 i64) (local $d51 i64) (local $d52 i64) (local $d53 i64) (local $d54 i64)
    (local $d55 i64) (local $d56 i64) (local $d57 i64) (local $d58 i64) (local $d59 i64)
    (local $d60 i64) (local $d61 i64) (local $d62 i64) (local $d63 i64) (local $d64 i64)
    (local $d65 i64) (local $d66 i64) (local $d67 i64) (local $d68 i64) (local $d69 i64)
    (local $d70 i64) (local $d71 i64) (local $d72 i64) (local $d73 i64) (local $d74 i64)
    (local $d75 i64) (local $d76 i64) (local $d77 i64) (local $d78 i64) (local $d79 i64)
    (local $d80 i64) (local $d81 i64) (local $d82 i64) (local $d83 i64) (local $d84 i64)
    (local $d85 i64) (local $d86 i64) (local $d87 i64) (local $d88 i64) (local $d89 i64)
    (local $d90 i64) (local $d91 i64) (local $d92 i64) (local $d93 i64) (local $d94 i64)
    (local $d95 i64) (local $d96 i64) (local $d97 i64) (local $d98 i64) (local $d99 i64)
    (local $d100 i64) (local $d101 i64) (local $d102 i64) (local $d103 i64) (local $d104 i64)
    (local $d105 i64) (local $d106 i64) (local $d107 i64) (local $d108 i64) (local $d109 i64)
    (local $d110 i64) (local $d111 i64) (local $d112 i64) (local $d113 i64) (local $d114 i64)
    (local $d115 i64) (local $d116 i64) (local $d117 i64) (local $d118 i64) (local $d119 i64)
    (local $d120 i64) (local $d121 i64) (local $d122 i64) (local $d123 i64) (local $d124 i64)
    (local $d125 i64) (local $d126 i64) (local $d127 i64) (local $d128 i64) (local $d129 i64)

    (local $vec v128)

    ;; Should hit WASM_OP_TEE_LOCAL rather than EXT_OP_TEE_LOCAL_FAST_V128
    (v128.const i32x4 1 2 3 4)
    (local.tee $vec)
    (drop)
  )

  (export "_start" (func $test))
)
