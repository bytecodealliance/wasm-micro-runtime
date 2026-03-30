# WAMR Component Model support

## Introduction

A WebAssembly (Wasm) component is an abstraction layer built over [standard WebAssembly](https://webassembly.github.io/spec/core/index.html) (often called WebAssembly Core). It is designed to enrich the exposed type system and improve interoperability across different programming languages and libraries. More details can be found [here](https://component-model.bytecodealliance.org/).

To distinguish between the two layers, the Component Model specification refers to standard WebAssembly entities by prefixing them with the word "core" (e.g., core modules, core functions, core types).

In short, a Wasm component uses WIT—an [Interface Definition Language](https://en.wikipedia.org/wiki/Interface_description_language) (IDL)—to define its public-facing interfaces. It then bundles one or more Wasm core modules to implement the underlying logic behind those interfaces.

WAMR implements binary parsing for the WebAssembly [Component Model](https://github.com/WebAssembly/component-model) proposal. The parser handles the component binary format as defined in the [Binary.md](https://github.com/WebAssembly/component-model/blob/main/design/mvp/Binary.md) specification, covering all 13 section types with validation and error reporting.

References:
- [Component Model design repository](https://github.com/WebAssembly/component-model)
- [Component Model binary format specification](https://github.com/WebAssembly/component-model/blob/main/design/mvp/Binary.md)
- [Canonical ABI specification](https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md)
- [Build WAMR vmcore](./build_wamr.md) for build flag reference

## Build

Enable the feature by setting the CMake flag `WAMR_BUILD_COMPONENT_MODEL` (enabled by default). This defines the C preprocessor macro `WASM_ENABLE_COMPONENT_MODEL=1`.

```cmake
set (WAMR_BUILD_COMPONENT_MODEL 1)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
```

Or pass it on the cmake command line:

```bash
cmake -DWAMR_BUILD_COMPONENT_MODEL=1 ..
```

## Component vs core module

A component binary shares the same magic number (`\0asm`) as a core WebAssembly module but is distinguished by its header fields:

|               | Core module   | Component     |
|---------------|---------------|---------------|
| magic         | `\0asm`       | `\0asm`       |
| version       | `0x0001`      | `0x000d`      |
| layer         | `0x0000`      | `0x0001`      |

WAMR uses `wasm_decode_header()` to read the 8-byte header and `is_wasm_component()` to determine whether a binary is a component or a core module.

## Binary parsing overview

The binary parser takes a raw component binary buffer and produces a `WASMComponent` structure that holds the decoded header and an array of parsed sections. The diagram below illustrates the high-level parsing flow:

<center><img src="./pics/binary_parser_hld.png" width="85%" height="85%"></img></center>

The parsing proceeds as follows:

1. **Header decode** -- `wasm_decode_header()` reads the 8-byte header (magic, version, layer) and populates a `WASMHeader` struct.
2. **Section loop** -- the parser iterates over the binary, reading a 1-byte section ID and a LEB128-encoded payload length for each section.
3. **Section dispatch** -- based on the section ID, the parser delegates to a dedicated per-section parser. Each parser decodes the section payload into a typed struct, validates its contents (UTF-8 names, index bounds, canonical options), and reports how many bytes were consumed.
4. **Core module delegation** -- when a Core Module section (0x01) is encountered, the parser delegates to the existing WAMR core module loader (`wasm_runtime_load_ex()`) to parse the embedded module.
5. **Recursive nesting** -- when a Component section (0x04) is encountered, the parser calls itself recursively with an incremented depth counter. Recursion depth is capped at 100.
6. **Result** -- on success, the `WASMComponent` struct holds the header and a dynamically-sized array of `WASMComponentSection` entries, each containing the raw payload pointer and a typed union with the parsed result.

### Section types

The component binary format defines 13 section types:

| ID   | Section          | Spec reference                    |
|------|------------------|-----------------------------------|
| 0x00 | Core Custom      | custom section (name + data)      |
| 0x01 | Core Module      | embedded core wasm module         |
| 0x02 | Core Instance    | core module instantiation         |
| 0x03 | Core Type        | core func types, module types     |
| 0x04 | Component        | nested component (recursive)      |
| 0x05 | Instance         | component instance definitions    |
| 0x06 | Alias            | export, core export, outer aliases|
| 0x07 | Type             | component type definitions        |
| 0x08 | Canon            | canonical lift/lower/resource ops |
| 0x09 | Start            | component start function          |
| 0x0A | Import           | component imports                 |
| 0x0B | Export           | component exports                 |
| 0x0C | Value            | value definitions                 |

### Current limitations

- **Core Type (0x03)**: only `moduletype` is supported; WebAssembly GC types (`rectype`, `subtype`) are rejected.
- **Canon (0x08)**: `async` and `callback` canonical options are rejected; all other canonical operations are supported.

## Source layout

All component model sources reside in `core/iwasm/common/component-model/`:

```
component-model/
  iwasm_component.cmake              # CMake build configuration
  wasm_component.h                   # type definitions, enums, struct declarations
  wasm_component.c                   # entry point: section dispatch and free
  wasm_component_helpers.c           # shared utilities: LEB128, names, value types
  wasm_component_validate.c          # validation: UTF-8, index bounds, canon options
  wasm_component_validate.h          # validation declarations
  wasm_component_core_custom_section.c   # section 0x00
  wasm_component_core_module_section.c   # section 0x01
  wasm_component_core_instance_section.c # section 0x02
  wasm_component_core_type_section.c     # section 0x03
  wasm_component_component_section.c     # section 0x04
  wasm_component_instances_section.c     # section 0x05
  wasm_component_alias_section.c         # section 0x06
  wasm_component_types_section.c         # section 0x07
  wasm_component_canons_section.c        # section 0x08
  wasm_component_start_section.c         # section 0x09
  wasm_component_imports_section.c       # section 0x0A
  wasm_component_exports_section.c       # section 0x0B
  wasm_component_values_section.c        # section 0x0C
  wasm_component_export.c               # export runtime helpers
  wasm_component_export.h               # export declarations
```
