# Wasm Proposals

This document is intended to describe the current status of WebAssembly proposals and WASI proposals in WAMR.

Only track proposals that are followed in the [WebAssembly proposals](https://github.com/WebAssembly/proposals) and [WASI proposals](https://github.com/WebAssembly/WASI/blob/main/Proposals.md).

Normally, the document tracks proposals that are in phase 4. However, if a proposal in an earlier phase receives support, it will be added to the list below.

The _status_ represents the configuration _product-mini/platforms/linux/CMakeLists.txt_. There may be minor differences between the top-level CMakeLists and platform-specific CMakeLists.

Users can turn those features on or off by using compilation options. If a relevant compilation option is not available(`N/A`), it indicates that the feature is permanently enabled.

## On-by-default Wasm Proposals

| Proposal                              | Phase 4 | Compilation Option         |
| ------------------------------------- | ------- | -------------------------- |
| Non-trapping float-to-int conversions | Yes     | N/A                        |
| Sign-extension operators              | Yes     | N/A                        |
| Multi-value                           | Yes     | N/A                        |
| Reference Types                       | Yes     | `WAMR_BUILD_REF_TYPES`     |
| Bulk memory operations                | Yes     | `WAMR_BUILD_BULK_MEMORY`   |
| Fixed-width SIMD[^1]                  | Yes     | `WAMR_BUILD_SIMD`          |
| Extended Constant Expressions         | Yes     | N/A                        |
| Typed Function References             | Yes     | `WAMR_BUILD_GC`            |
| Thread                                | Yes     | `WAMR_BUILD_SHARED_MEMORY` |
| Legacy Exception handling[^2]         | Yes     | `WAMR_BUILD_EXCE_HANDLING` |
| WebAssembly C and C++ API             | No      | N/A                        |

[^1]: jit and aot only
[^2]: interpreter only

## Off-by-default Wasm Proposals

| Proposal              | Phase 4 | Compilation Option        |
| --------------------- | ------- | ------------------------- |
| Tail call             | Yes     | `WAMR_BUILD_TAIL_CALL`    |
| Garbage collection    | Yes     | `WAMR_BUILD_GC`           |
| Multiple memories[^3] | Yes     | `WAMR_BUILD_MULTI_MEMORY` |
| Memory64              | Yes     | `WAMR_BUILD_MEMORY64`     |

[^3]: interpreter only

## Unimplemented Wasm Proposals

| Proposal                                    | Phase 4 |
| ------------------------------------------- | ------- |
| Import/Export of Mutable Globals            | Yes     |
| Relaxed SIMD                                | Yes     |
| Custom Annotation Syntax in the Text Format | Yes     |
| Branch Hinting                              | Yes     |
| JS String Builtins                          | Yes     |
| Exception handling                          | Yes     |

## On-by-default WASI Proposals

| Proposal | Phase 4 | Compilation Option |
| -------- | ------- | ------------------ |

## Off-by-default WASI Proposals

| Proposal                   | Phase 4 | Compilation Option            |
| -------------------------- | ------- | ----------------------------- |
| Machine Learning (wasi-nn) | No      | `WAMR_BUILD_WASI_NN`          |
| Threads                    | No      | `WAMR_BUILD_LIB_WASI_THREADS` |

## Unimplemented WASI Proposals

| Proposal | Phase 4 |
| -------- | ------- |

## WAMR features

WAMR offers a variety of customizable features to create a highly efficient runtime. For more details, please refer to [build_wamr](./build_wamr.md).
