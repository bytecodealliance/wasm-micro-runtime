# `addr2line.py` — symbolicating WAMR call stacks

`addr2line.py` turns a WAMR call-stack dump (`#00: 0x0040 - $f0` lines) back
into source-level information (function names, files, line numbers, inline
chains). It is a thin orchestrator over four LLVM/wabt tools, with workarounds
for a real LLVM symbolization bug on wasm targets.

The two `samples/debug-tools*/` samples are end-to-end demos of the workflow.
This README focuses on the tool itself: how it works, why each subprocess is
necessary, and what edge cases force a "legacy" code path.

## Quick reference

```bash
python3 addr2line.py \
    --wasi-sdk /opt/wasi-sdk \
    --wabt    /opt/wabt \
    --wasm-file /path/to/app.debug.wasm \
    call_stack.txt
```

| Flag | Default | Purpose |
|------|---------|---------|
| `--mode={interp,aot,fast-interp}` | `interp` | How to interpret runtime offsets — see *Three offset spaces* below. |
| `--no-addr` | off | Resolve by function name only (for very old iwasm dumps that lack offsets, or for fast-interp). |
| `-v` / `--verbose` | off | Print the resolver-dispatch decision (modern vs legacy) to stderr. |

## What the script does, step by step

1. **Parse each captured frame** (`#NN: 0xADDR - SYMBOL`).
2. **Convert the runtime offset to a DWARF address.** WAMR reports
   file-absolute byte offsets; DWARF addresses are relative to the start of
   the WASM Code section, so the script subtracts `code_section_start` (read
   from `wasm-objdump -h`).
3. **Apply a mode-dependent adjustment.** See *Three offset spaces* below.
4. **Resolve the address.** Either the modern resolver (one subprocess) or
   the legacy resolver (two subprocesses with a name overlay) — the script
   detects the wasi-sdk's clang version and picks one automatically.
5. **Demangle and render.** C++ symbols pass through `llvm-cxxfilt`; inline
   frames are annotated `(inlined into <next>)` so the chain reads top-down.

## Three offset spaces (`--mode`)

WAMR captures `frame_ip` differently in each execution mode, so the offset in
a captured frame means different things:

| Mode | Offset space | Adjustment | What addr2line does |
|------|--------------|-----------|----------------------|
| `interp` (default) | File-absolute, *post*-advance — interpreter has already incremented `frame_ip` past the trapping opcode (and past LEB reads inside the handler) before the trap fires. | `offset - code_start - 1` | Full symbolication: file, line, column, inline chain. |
| `aot` | File-absolute, *instruction-start* — `wamrc` emits a `commit_ip` before every WASM operation. | `offset - code_start` | Full symbolication. |
| `fast-interp` | Function-relative, into the *transformed* in-memory bytecode (opcodes replaced with handler indices, LEBs expanded to fixed widths). The mapping back to source is destroyed by the transform. | (none) | Function-name lookup only — equivalent to `--no-addr`. One frame per input; no inline expansion (the `DW_TAG_inlined_subroutine` overlay is address-keyed and unavailable here). |

Picking the wrong `--mode` lands the resolver inside an adjacent function and
silently produces wrong file/line.

## The two resolvers, and why both exist

`llvm-symbolizer -e <wasm> -f -i <addr>` is the natural way to map an address
to a function + source location, with `-i` expanding inline frames.

**This subprocess is buggy on wasm targets emitted by clang < 22.** For some
addresses it returns the wrong outermost function name — frequently a
declaration-only DW_TAG_subprogram from wasi-libc that overlaps the real
function's PC range. Concretely, you can ask for the address of `recurse` and
get back `free`; ask for `a()` in a small-trap demo and get back `_start`.
The line table is correct in both cases — only the function-name lookup is
wrong. The bug lives in the symbolizer/addr2line shared backend; switching
between the two binaries doesn't help.

The bug is fixed in clang 22 (wasi-sdk 33). For older toolchains we
build a `(low_pc, high_pc, name)` interval table from a single
`llvm-dwarfdump --debug-info` pass at startup, and overlay the
**outermost** frame's name from that table — `[low_pc, high_pc)` ranges
parsed straight from DWARF are canonical, regardless of any symbolizer
backend bug. Inner inline frames stay as-reported by the symbolizer:
they come from `DW_TAG_inlined_subroutine` entries that the symbolizer
renders correctly even on the buggy backend. When several intervals
cover the same address (wasi-libc declarations often have
`low_pc=0` and span much of the binary), the **innermost** (smallest
range) wins.

`addr2line.py` runs `<wasi-sdk>/bin/clang --version` once at startup and:
- clang ≥ 22 → **modern resolver** (one `llvm-symbolizer` subprocess per address).
- clang < 22 → **legacy resolver** (`llvm-symbolizer` + interval-table overlay).
- clang version undetectable → **legacy resolver** (safest fallback).

Pass `-v` to see which path is taken. The legacy resolver — and the
interval-table helpers `build_subprogram_intervals` /
`lookup_subprogram_name` — can be removed once the project's minimum
supported wasi-sdk is 33+ everywhere.

## Tools used by addr2line.py

| Tool | From | Why this tool, and not another |
|------|------|--------------------------------|
| `llvm-symbolizer` | wasi-sdk | The actual address → `(function, file:line:column)` resolver, with inline expansion via `-i`. Ships in wasi-sdk 29+ (the project's minimum supported version). |
| `llvm-dwarfdump` | wasi-sdk | Legacy path only: a single `--debug-info` pass at startup builds a `(low_pc, high_pc, name)` interval table for every `DW_TAG_subprogram`, used to overlay the outermost frame's name (the buggy lookup on clang < 22). No equivalent CLI exposes this as structured output, so we parse the text. |
| `wasm-objdump` | wabt | Reports the Code-section start offset (needed to convert file-absolute runtime offsets to DWARF addresses) and the function-index → name table (a last-resort fallback for declaration-only DW_TAG_subprograms like `__main_void`). |
| `llvm-cxxfilt` | wasi-sdk | C++ symbol demangling, applied as a final pass on resolved names. |

The script also detects an emscripten-style sourceMappingURL section and
falls back to `emsymbolizer` for that case.

## Behavior matrix

| Input | Resolver path | Output |
|-------|---------------|--------|
| `--mode=interp` or `--mode=aot`, clang ≥ 22, address in DWARF range | Modern | Full file:line:column with inline chain |
| `--mode=interp` or `--mode=aot`, clang < 22, address in DWARF range | Legacy | Same, but the outermost name is overlaid from a startup-built `DW_TAG_subprogram` interval table (innermost match wins) |
| `--mode=fast-interp` | Function-name | Single frame per input, no inline expansion, no source mapping |
| `--no-addr` | Function-name | Resolves the *function* — file:line refers to its declaration, not the trap site |
| Offset = 0 | Function-name fallback (with `(offset=0 — function entry)` note) | Used when WAMR couldn't capture `frame_ip` (e.g., trap at function entry, or top frame of an `iwasm -f` invocation) |
| Address outside any DW_TAG_subprogram | wasm-objdump function-index table | Single name; useful for declaration-only entries |
| Source has emscripten `sourceMappingURL` custom section | `emsymbolizer` source-map path | Single frame per address; no inline expansion (source maps don't carry it) |

## Tests

Run the pytest suite under `test-tools/addr2line/tests/`. It builds tiny
purpose-built C/C++ apps, captures their trap sites, and asserts the resolver
behaves correctly under a number of scenarios (single trap, always_inline,
deep inline chain, cross-TU LTO inlining, mid-function trap, multi-frame
call stack, C++ demangling, AOT mode, fast-interp mode, `--no-addr`,
`offset=0`, empty input). Multi-SDK runs (`pytest --multi-sdk`) cover the
modern-vs-legacy equivalence and require both an old and a new wasi-sdk to be
installed.

## See also

- `test-tools/addr2line/addr2line.py` — the script
- `test-tools/addr2line/tests/` — pytest harness
- `samples/debug-tools/` — minimal end-to-end sample
- `samples/debug-tools-optimized/` — production-realistic workflow (LTO, wasm-opt, strip, AOT, debug companion)
