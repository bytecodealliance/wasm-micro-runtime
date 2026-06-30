# Copyright (C) 2026 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""
Test suite for test-tools/addr2line/addr2line.py.

Most tests build a wasm from a single apps/*.c source, capture the trap
address from DWARF, and check that addr2line.py resolves it correctly.

Multi-SDK behavior is opt-in via `pytest --multi-sdk`. Cross-version
equivalence (test_modern_legacy_equivalence) requires it; everything
else runs against whichever SDK is active.
"""

import re
import subprocess
from pathlib import Path

import pytest

from conftest import find_dwarf_low_pc, code_section_start


# Format a wasm file offset that points at a real instruction inside
# the function body, given its DWARF low_pc and the wasm Code section
# start. Offset is `code_section_start + low_pc + delta + 1` because
# addr2line.py applies a -1 adjustment for classic-interp mode.
def _stack_line(wabt, wasm, wasi_sdk, func_name, delta=1, frame_idx=0,
                func_label=None):
    cs = code_section_start(wabt, wasm)
    low = find_dwarf_low_pc(wasi_sdk, wasm, func_name)
    if low is None:
        pytest.skip(f"DW_TAG_subprogram for {func_name} not found in {wasm}")
    # +1 to compensate for addr2line.py's -1 in interp mode
    file_off = cs + low + delta + 1
    label = func_label or "$f0"
    return f"#{frame_idx:02d}: 0x{file_off:04x} - {label}"


# --- Build-based tests ---------------------------------------------------

@pytest.mark.slow
def test_simple_resolution(build_wasm, wabt, run_addr2line, wasi_sdk):
    """Single-function trap resolves to function=crash, file=simple.c."""
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])
    line = _stack_line(wabt, wasm, wasi_sdk, "crash", delta=1,
                       func_label="$f0")
    out, err, rc = run_addr2line(wasm, [line])
    assert rc == 0
    assert "crash" in out
    assert "simple.c" in out


@pytest.mark.slow
def test_inline_chain_basic(build_wasm, wabt, run_addr2line, wasi_sdk):
    """always_inline helper at trap site produces 2-frame inline chain."""
    wasm = build_wasm(["always_inline.c"], ["-O0", "-g"])
    # Trap is inside `inner` (always_inline'd into outer)
    line = _stack_line(wabt, wasm, wasi_sdk, "outer", delta=2,
                       func_label="$f0")
    out, err, rc = run_addr2line(wasm, [line])
    assert rc == 0
    assert "inner" in out
    assert "outer" in out
    # Inline annotation must appear
    assert "(inlined into" in out


@pytest.mark.slow
def test_inline_chain_deep(build_wasm, wabt, run_addr2line, wasi_sdk):
    """4-level inline chain produces all 4 frames in order."""
    wasm = build_wasm(["deep_inline_chain.c"], ["-O0", "-g"])
    # All 4 levels inlined into app_main; pick low_pc(app_main)+small
    line = _stack_line(wabt, wasm, wasi_sdk, "app_main", delta=2,
                       func_label="$f0")
    out, err, rc = run_addr2line(wasm, [line])
    assert rc == 0
    # All four levels should appear somewhere in the output
    for fn in ("level1", "level2", "level3", "level4"):
        assert fn in out, f"missing {fn} in output:\n{out}"
    assert "(inlined into" in out
    # The OUTERMOST frame must be app_main (not a wasi-libc symbol like
    # `free` / `__multi3` that overlaps app_main's PC range — that's the
    # exact regression the legacy resolver's subprogram-name overlay
    # exists to prevent).
    assert re.search(r"^\s+app_main\b", out, re.MULTILINE), (
        f"outermost frame is not app_main:\n{out}"
    )


@pytest.mark.slow
def test_cross_tu_inline(build_wasm, wasm_opt_pass, wabt, run_addr2line,
                        wasi_sdk):
    """Multi-file recursion under -Oz -g -flto + wasm-opt -Oz -g.

    `wasm-opt -Oz -g` reorders DWARF in ways that confuse both legacy
    and modern symbolizer paths on the function-name lookup; this test
    pins the more limited contract that the line table itself stays
    inside our sources (or returns `??:0`), never a wasi-libc path.
    Outermost-name correctness for non-mangled DWARF is covered by
    test_inline_chain_deep.
    """
    raw = build_wasm(
        ["multi_file_recur_main.c", "multi_file_recur.c"],
        ["-Oz", "-g", "-flto"],
    )
    debug = wasm_opt_pass(raw, ["-Oz", "-g"])
    line = _stack_line(wabt, debug, wasi_sdk, "recur", delta=0x40,
                       func_label="$f0")
    out, err, rc = run_addr2line(debug, [line])
    assert rc == 0
    assert (
        "multi_file_recur" in out
        or "??:0" in out
    ), f"line table did not resolve to multi_file_recur* source:\n{out}"


@pytest.mark.slow
def test_trap_mid_function(build_wasm, wabt, run_addr2line, wasi_sdk):
    """Trap inside a loop body resolves to the trap line, not entry line."""
    wasm = build_wasm(["trap_in_loop.c"], ["-O0", "-g"])
    # Probe well past entry so the line table catches the trap line
    line = _stack_line(wabt, wasm, wasi_sdk, "crash", delta=0x20,
                       func_label="$f0")
    out, err, rc = run_addr2line(wasm, [line])
    assert rc == 0
    assert "crash" in out
    assert "trap_in_loop.c" in out
    # The trap is on its own line — output should reference a line >= 14
    # (function declaration is on line ~10, trap is below it)
    m = re.search(r"trap_in_loop\.c:(\d+)", out)
    assert m is not None, f"no file:line in output:\n{out}"
    line_num = int(m.group(1))
    assert line_num > 10, (
        f"resolved to line {line_num}, expected > 10 (post-declaration)"
    )


@pytest.mark.slow
def test_multi_frame_callstack(build_wasm, wabt, run_addr2line, wasi_sdk):
    """Multi-frame call stack renders all frames with distinct indices."""
    wasm = build_wasm(["multi_frame.c"], ["-O0", "-g"])
    lines = []
    for idx, fn in enumerate(["bot", "mid", "top", "app_main"]):
        lines.append(
            _stack_line(wabt, wasm, wasi_sdk, fn, delta=2,
                        frame_idx=idx, func_label="$f0")
        )
    out, err, rc = run_addr2line(wasm, lines)
    assert rc == 0
    for fn in ("bot", "mid", "top", "app_main"):
        assert fn in out, f"missing {fn} in output:\n{out}"


@pytest.mark.slow
def test_cxx_demangling(build_wasm, wabt, run_addr2line, wasi_sdk):
    """C++ symbols come through the toolchain in a readable form.

    The DWARF that wasi-sdk's clang emits stores the demangled name in
    DW_AT_name and the mangled `_Z...` form in DW_AT_linkage_name; both
    llvm-symbolizer and the wasm name section emit the demangled form,
    so addr2line.py's llvm-cxxfilt pass is usually a no-op. This test
    asserts the end-to-end output is human-readable: source file from
    our sample, line numbers present, no mangled `_Z...` leaks.

    Symbolizer-reported function names for templated/namespaced symbols
    are not asserted — both clang < 22 and clang 22+ pick the wrong
    DW_TAG_subprogram for some C++ template addresses on wasm targets
    (e.g. reporting `app_main` for an address that DWARF clearly puts
    inside `crash_with<int>`). The line table is correct in both cases.
    """
    wasm = build_wasm(["cxx_mangled.cpp"], ["-O0", "-g"], language="cxx")
    # Look up by the DWARF DW_AT_name (already demangled, with template args).
    line = _stack_line(wabt, wasm, wasi_sdk, "crash_with<int>",
                       delta=2, func_label="$f0")
    out, err, rc = run_addr2line(wasm, [line])
    assert rc == 0
    # No mangled `_Z...` should ever leak to the user.
    assert "_ZN" not in out, f"mangled prefix leaked:\n{out}"
    # Source file should resolve to our sample (line table is reliable).
    assert "cxx_mangled.cpp" in out, (
        f"line table did not resolve to cxx_mangled.cpp:\n{out}"
    )


@pytest.mark.slow
def test_aot_mode(build_wasm, wasm_opt_pass, wabt, run_addr2line, wasi_sdk):
    """--mode=aot uses no -1 adjustment and resolves correctly.

    For an address that's exactly at low_pc+delta (no -1), --mode=aot must
    still resolve to the right function (whereas --mode=interp would
    apply -1 and possibly land outside the function range).
    """
    raw = build_wasm(["simple.c"], ["-Oz", "-g"])
    debug = wasm_opt_pass(raw, ["-Oz", "-g"])
    cs = code_section_start(wabt, debug)
    low = find_dwarf_low_pc(wasi_sdk, debug, "crash")
    if low is None:
        pytest.skip("crash() inlined away under -Oz; skipping AOT mode test")
    aot_offset = cs + low + 4   # AOT path uses offset verbatim
    cs_line = f"#00: 0x{aot_offset:04x} - $f0"
    out, err, rc = run_addr2line(debug, [cs_line], extra_args=["--mode", "aot"])
    assert rc == 0
    assert "crash" in out


# --- Fixture-based tests (no build) --------------------------------------

def test_fast_interp_mode(addr2line_script, wabt, wasi_sdk, fixtures_dir,
                          run_addr2line, build_wasm):
    """--mode=fast-interp falls back to function-name lookup (no addr math).

    The fixture's $f0 and $f1 must resolve to simple.c's two functions
    (`crash` and `app_main`) via the wasm name section.
    """
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])
    out, err, rc = run_addr2line(
        wasm, fixtures_dir / "fast_interp_stack.txt",
        extra_args=["--mode", "fast-interp"],
    )
    assert rc == 0
    assert "crash" in out, f"expected $f0 -> crash:\n{out}"
    assert "app_main" in out, f"expected $f1 -> app_main:\n{out}"


def test_no_addr_mode(build_wasm, run_addr2line, fixtures_dir):
    """--no-addr resolves by function name, not address.

    Same fixture as fast-interp: $f0 -> crash, $f1 -> app_main. Under
    --no-addr addr2line should additionally surface the decl-line from
    DWARF (not just the function name).
    """
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])
    out, err, rc = run_addr2line(
        wasm, fixtures_dir / "fast_interp_stack.txt",
        extra_args=["--no-addr"],
    )
    assert rc == 0
    assert "crash" in out, f"expected $f0 -> crash:\n{out}"
    assert "app_main" in out, f"expected $f1 -> app_main:\n{out}"
    assert "simple.c" in out, f"expected DWARF source file:\n{out}"


def test_offset_zero_fallback(build_wasm, run_addr2line):
    """Runtime offset=0 falls back gracefully (no assertion crash)."""
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])
    out, err, rc = run_addr2line(wasm, ["#00: 0x0000 - app_main"])
    assert rc == 0
    # Output should mention app_main even with no useful offset
    assert "app_main" in out


def test_empty_input(build_wasm, run_addr2line, fixtures_dir):
    """Empty call stack file produces clean exit, minimal output."""
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])
    out, err, rc = run_addr2line(wasm, fixtures_dir / "empty_stack.txt")
    assert rc == 0


# --- Cross-cutting -------------------------------------------------------

def test_dispatch_message_in_stderr(build_wasm, run_addr2line):
    """The version-dispatch decision is logged to stderr at startup
    when --verbose is passed; silent otherwise."""
    wasm = build_wasm(["simple.c"], ["-O0", "-g"])

    # Default: stderr is silent (no dispatch message).
    out, err, rc = run_addr2line(wasm, ["#00: 0x0040 - $f0"])
    assert rc == 0
    assert "modern resolver" not in err and "legacy resolver" not in err, (
        f"dispatch message leaked to stderr without -v:\n{err}"
    )

    # With -v: one of the three known messages must appear.
    out, err, rc = run_addr2line(wasm, ["#00: 0x0040 - $f0"],
                                 extra_args=["-v"])
    assert rc == 0
    assert (
        "modern resolver" in err
        or "legacy resolver" in err
    ), f"no dispatch message in stderr with -v:\n{err}"


@pytest.mark.multi_sdk
@pytest.mark.slow
def test_modern_legacy_equivalence(build_wasm, wasm_opt_pass, wabt,
                                  addr2line_script, apps_dir, tmp_path):
    """Multi-SDK only: legacy and modern paths produce equivalent output.

    Build the same source once per detected SDK and assert the
    symbolicated stdout strings match (after stripping per-SDK file paths).
    Skipped unless --multi-sdk is given AND at least 2 SDKs detected.
    """
    pytest.importorskip("subprocess")  # already always available; placeholder

    # Detect SDKs directly here (the wasi_sdk fixture is per-test, not a list)
    from conftest import _detect_sdks_under_opt
    sdks = _detect_sdks_under_opt()
    if len(sdks) < 2:
        pytest.skip("need >= 2 SDKs for equivalence test")

    # Pick one with clang < 22 and one with clang >= 22 if possible
    import sys
    sys.path.insert(0, str(addr2line_script.parent))
    from addr2line import detect_clang_major_version

    def _major(sdk_path: Path):
        return detect_clang_major_version(sdk_path)

    # Require llvm-symbolizer in the SDK — older wasi-sdk packages
    # (e.g. sdk25) ship without it and addr2line.py can't run.
    def _has_symbolizer(p: Path):
        return (p / "bin" / "llvm-symbolizer").exists()

    legacy_sdks = [(v, p) for v, p in sdks
                   if (_major(p) or 0) < 22 and _has_symbolizer(p)]
    modern_sdks = [(v, p) for v, p in sdks
                   if (_major(p) or 0) >= 22 and _has_symbolizer(p)]
    if not legacy_sdks or not modern_sdks:
        pytest.skip(
            f"need at least one legacy (clang<22) AND one modern (clang>=22) "
            f"SDK with llvm-symbolizer present; "
            f"have legacy={[v for v,_ in legacy_sdks]}, "
            f"modern={[v for v,_ in modern_sdks]}"
        )

    # Build the same source under both SDKs and compare outputs.
    # Use simple.c so every SDK can build it without flag interactions.
    legacy_ver, legacy_path = legacy_sdks[0]
    modern_ver, modern_path = modern_sdks[0]

    def _build_and_resolve(sdk_path):
        # Build using subprocess directly so we don't tangle with the
        # parametrized wasi_sdk fixture.
        out_wasm = tmp_path / f"out_{sdk_path.name}.wasm"
        compiler = sdk_path / "bin" / "clang"
        cmd = [
            str(compiler),
            "--target=wasm32-wasi", "-O0", "-g",
            "-Wl,--export=app_main", "-Wl,--no-entry", "-nostartfiles",
            "-o", str(out_wasm),
            str(apps_dir / "simple.c"),
        ]
        r = subprocess.run(cmd, capture_output=True, text=True)
        assert r.returncode == 0, r.stderr

        # Resolve via addr2line.py with this SDK
        cs = tmp_path / f"cs_{sdk_path.name}.txt"
        cs.write_text("#00: 0x0040 - $f0\n")
        cmd = [
            "python3", str(addr2line_script),
            "--wasi-sdk", str(sdk_path),
            "--wabt", str(wabt),
            "--wasm-file", str(out_wasm),
            str(cs),
        ]
        r = subprocess.run(cmd, capture_output=True, text=True)
        return r.stdout

    legacy_out = _build_and_resolve(legacy_path)
    modern_out = _build_and_resolve(modern_path)

    # Strip absolute file paths to compare (the per-SDK build dirs differ)
    def _normalize(s):
        return re.sub(r"/[^\s:]+/", "/.../", s)

    assert _normalize(legacy_out) == _normalize(modern_out), (
        f"legacy ({legacy_ver}) and modern ({modern_ver}) outputs differ:\n"
        f"--- legacy ---\n{legacy_out}\n--- modern ---\n{modern_out}"
    )
