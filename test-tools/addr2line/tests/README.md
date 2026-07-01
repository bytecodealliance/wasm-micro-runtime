# addr2line.py tests

Pytest suite for `test-tools/addr2line/addr2line.py`. Replaces the older
diagnostic harness that lived under `samples/debug-tools/llvm-bug-experiment/`.

## What's covered

Each test case exercises one specific aspect of `addr2line.py`:

| Test | Source | Purpose |
|------|--------|---------|
| `test_simple_resolution` | `apps/simple.c` | Baseline. One function, one trap. Asserts the resolved frame names the function and points at its source file — proves the basic address → `(func, file:line)` plumbing works end to end. |
| `test_inline_chain_basic` | `apps/always_inline.c` | A trap inside an `__attribute__((always_inline))` helper produces a 2-frame inline chain (the helper plus its caller). Asserts addr2line emits BOTH names and the `(inlined into <next>)` annotation that ties them together — guards against future regressions in `print_frames`. |
| `test_inline_chain_deep` | `apps/deep_inline_chain.c` | Four nested always-inline functions all collapse into one WASM function under `-O0 -g`. Asserts all four names render in the inline chain AND that the outermost frame name is `app_main` (not a wasi-libc symbol like `__multi3` that overlaps app_main's PC range). This is the test that catches the "(inlined into free)" / wasi-libc-shadow class of legacy-resolver regressions. |
| `test_cross_tu_inline` | `apps/multi_file_recur_*.c` | The canonical clang-bug trigger: two TUs, `-Oz -flto`, then `wasm-opt -Oz -g` mangles the DWARF further. Both legacy AND modern symbolizers get confused on the outermost function name here, so the test only pins the limited contract that the *line table* stays inside our source files (or returns `??:0`) — it must NEVER reach into wasi-libc / wasi-sysroot. |
| `test_trap_mid_function` | `apps/trap_in_loop.c` | Trap is inside a loop body, several instructions past function entry. Asserts the resolved line is > 10 (i.e. NOT the function declaration line). Catches future regressions where addr2line might collapse mid-function addresses back to function-entry. |
| `test_multi_frame_callstack` | `apps/multi_frame.c` | Four distinct WASM functions in a call chain. Asserts every frame renders with the correct function name and a distinct frame index — catches frame-numbering or per-frame-resolution bugs that a single-frame test misses. |
| `test_cxx_demangling` | `apps/cxx_mangled.cpp` | A templated, namespaced C++ function exercises the cxxfilt pass. Asserts mangled `_Z...` never leaks to the user and that the line table resolves to our `.cpp` (not wasi-libc). Doesn't pin the symbolizer's function-name choice — both clang < 22 and clang 22+ pick the wrong subprogram on some C++ template addresses on wasm. |
| `test_aot_mode` | `apps/simple.c` (`-Oz -g`) | `--mode=aot` uses no `-1` adjustment (wamrc commits ip at instruction-start, not post-advance). The test picks an address at `low_pc+delta` and asserts the function still resolves under `--mode=aot`. Skips when `crash()` gets inlined away under `-Oz`. |
| `test_fast_interp_mode` | `fixtures/fast_interp_stack.txt` + `apps/simple.c` | The fixture has `$f0`/`$f1` indices; `--mode=fast-interp` forces function-name lookup (offsets aren't mappable). Asserts both indices resolve to the real wasm name-section entries (`crash`/`app_main`). |
| `test_no_addr_mode` | `fixtures/fast_interp_stack.txt` + `apps/simple.c` | Same fixture; `--no-addr` is the explicit form of function-name lookup. Additionally asserts the DWARF decl-file (`simple.c`) is surfaced (function-name lookup falls back to declaration-line, not call-site). |
| `test_offset_zero_fallback` | `apps/simple.c` (inline cs) | WAMR reports `offset=0` when `frame_ip` wasn't captured (trap at function entry, top frame of `iwasm -f`, etc.). The test asserts addr2line falls back to function-name resolution gracefully and prints the `app_main` name — does NOT crash or assert. |
| `test_empty_input` | `fixtures/empty_stack.txt` + `apps/simple.c` | A zero-line call stack should exit cleanly (rc=0) and produce no output. Defends against an `IndexError`-style crash when the loop body never runs. |
| `test_dispatch_message_in_stderr` | `apps/simple.c` (inline cs) | The version-dispatch decision (modern vs legacy resolver) goes to stderr ONLY when `-v` is passed; default is silent. Test runs addr2line twice with the same fixture address, once with and once without `-v`, and asserts the stderr-message visibility flips. |
| `test_modern_legacy_equivalence` | `apps/simple.c`, `--multi-sdk` only | Build `simple.c` once under a clang < 22 SDK and once under clang >= 22, run addr2line against both, and assert the outputs are identical after stripping per-SDK file paths. Proves the legacy-resolver overlay actually FIXES the bug rather than introducing its own divergence. Skipped unless `--multi-sdk` is passed AND at least one legacy + one modern SDK is installed. |

Sources under `apps/` are written for the test suite, not copied from
`samples/`. They're small (~10–25 lines each) and target specific edge
cases that aren't covered by the sample-CI integration tests.

## Running

```bash
# Default: against $WASI_SDK_PATH or /opt/wasi-sdk
./run_tests.sh
# or
python3 -m pytest -v

# Fixture-only (fast, no builds)
./run_tests.sh -m "not slow"

# Multi-SDK: parametrize build-based tests over every detected
# /opt/wasi-sdk-*-x86_64-linux installation. Required for the
# legacy/modern equivalence test.
./run_tests.sh --multi-sdk

# Verbose: print each test's addr2line input + stdout/stderr so you
# can see what got resolved. Use -v -s to surface it live; -v alone
# captures it and only shows it on failure.
./run_tests.sh -v -s
```

## Background: the LLVM symbolizer wasm bug

`addr2line.py` exists primarily to convert WAMR call-stack dumps into
source file:line:column. On most clang versions this is a thin wrapper
around `llvm-symbolizer -f -i`. But on **wasm targets emitted by clang
< 22**, the symbolizer's address-to-function resolver picks the wrong
`DW_TAG_subprogram` for some addresses (e.g., reports `recurse` as
`free`). The line table is correct; only the function name is wrong.

`addr2line.py` therefore detects the wasi-sdk's clang major version at
startup and routes through one of two resolvers:

| Resolver | Used when | What it does |
|----------|-----------|--------------|
| `resolve_address_modern` | clang ≥ 22 (wasi-sdk 33+) | Single `llvm-symbolizer` call per address |
| `resolve_address_legacy` | clang < 22 | Symbolizer call + an outermost-name overlay from a startup-built `DW_TAG_subprogram` interval table (`build_subprogram_intervals`) |

By default the dispatch decision is silent; pass `-v` to addr2line.py to
log which path was picked to stderr.

The `test_modern_legacy_equivalence` test (multi-SDK only) verifies
that for the same input both paths produce identical output (after
normalizing per-SDK file paths). This proves the legacy overlay
actually fixes the bug rather than introducing its own divergence.

## Why we left samples/debug-tools/llvm-bug-experiment behind

The earlier experiment was useful for diagnosing the bug but wasn't a
proper test suite — it was a one-shot script that printed status to
stdout. This pytest suite covers more cases, runs in CI per-PR (default
SDK) and nightly (multi-SDK), and is decoupled from sample evolution.

## Layout

```
test-tools/addr2line/tests/
├── README.md             # this file
├── conftest.py           # fixtures (wasi_sdk, build_wasm, run_addr2line, ...)
├── test_addr2line.py     # the tests
├── pytest.ini            # marker definitions
├── run_tests.sh          # convenience wrapper
├── apps/                 # purpose-built test sources
│   ├── simple.c
│   ├── always_inline.c
│   ├── deep_inline_chain.c
│   ├── multi_file_recur_main.c
│   ├── multi_file_recur.c
│   ├── trap_in_loop.c
│   ├── multi_frame.c
│   └── cxx_mangled.cpp
└── fixtures/             # plaintext call-stack inputs
    ├── fast_interp_stack.txt
    ├── aot_stack.txt
    └── empty_stack.txt
```
