---
description: "This page is under construction/refinement. p.s. wanna hear a construction joke? we are still working on it"
---

# WAMR Running Modes

## Brief Introduction

In this section, we want to introduce running modes and their difference to you

### "iwasm" VM core running mode

It could run an AOT file(compiled by wamrc AOT compiler) in AOT running mode

- AOT: Ahead-of-Time compilation. As you can guess from the name, we first need to use the *wamrc* compiler to compile wasm file to the AOT file. Then it could be run with our *iwasm* vmcore. In this running mode, we could achieve nearly native speed(the best of all running modes) and a smaller runtime footprint.

It could run wasm applications in Interpreter/JIT running mode:

- Interpreter:
  We support two running modes of the interpreter:
  - Classic Interpreter: plain interpreter running mode
  - Fast Interpreter: as you can guess from the name, the fast interpreter runs ~2X faster than the classic interpreter but consumes about 2X memory to hold the pre-compiled code.
- JIT:
  Using the Just-in-Time compilation technique, we could make iwasm run much faster than Interpreter mode and sometimes very close to the speed of AOT running mode. We support two running modes of JIT:
  - JIT(LLVM-JIT): implement JIT using LLVM
  - Fast-JIT: implement JIT without get extra dependencies(LLVM libraries) involved so that iwasm could run easier in some platforms

<!-- TODO: incoming blog -->
For more detailed introduction, kindly refer to this article(incoming) in our blog.
