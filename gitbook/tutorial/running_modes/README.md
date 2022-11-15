---
description: "This page is under construction/refinement. p.s. wanna hear a construction joke? we are still working on it"
---

# WAMR Running Modes

## Brief Introduction

In this section, we want to introduce running modes and their difference to you

### "iwasm" VM core running mode

It could run an AOT file(compiled by wamrc AOT compiler) in AOT running mode

- AOT: Ahead-of-Time compilation. As you can guess from the name, we first need to use the *wamrc* compiler to compile wasm file to the AOT file. Then it could be run with our *iwasm* vmcore. In this running mode, we could achieve the nearly native speed(the best of all running modes) with very small footprint and quick startup

It could run wasm applications in Interpreter/JIT running mode:

- Interpreter:
  Interpreters are very useful when debugging or studying, but their performance is relatively poor compared with other running modes. We support two running modes of the interpreter:
  - Classic Interpreter: plain interpreter running mode
  - Fast Interpreter: as you can guess from the name, the fast interpreter runs ~2X faster than the classic interpreter but consumes about 2X memory to hold the pre-compiled code.
- JIT:
  Using the Just-in-Time compilation technique, we could make iwasm run much faster than Interpreter mode and sometimes very close to the speed of AOT running mode. We support two running modes of JIT:
  - JIT(LLVM-JIT): implement JIT using LLVM. The (LLVM)JIT generally performs better than Fast-JIT, **executing ~2X faster** than Fast-JIT. But the startup time is slower than Fast-JIT.
  - Fast-JIT: implement JIT without getting extra dependencies(LLVM libraries) involved so that iwasm could run easier on some platforms. It is called fast for it **starts up very fast**. So as you can imagine, overall performance for some small applications could be better in this mode than in (LLVM)JIT mode.

<!-- TODO: incoming blog -->
For more detailed introduction, kindly refer to this article(**incoming**) in our blog.
