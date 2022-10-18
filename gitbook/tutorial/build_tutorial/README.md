# build tutorial

In this chapter, we provide detailed tutorial to how to build [iwasm vmcore](../../../doc/build_wamr.md) and [wamrc](../build_tutorial/build_wamrc.md).

Our powerful **iwasm vmcore** provide various running mode you could choose using the compile CMake option. Here is the matrix for different running mode and their attributes:

| Running mode | CMake build options | Pros and Cons  |
| -----------  | -----------         | ---------      |
|  AOT         | none(default)       |                |
|  Interpreter | -DWAMR_BUILD_FAST_INTERP=0 |         |
|  Fast Interpreter | none(default)  |                |
|  JIT         | -DWAMR_BUILD_JIT=1  |                |
|  Fast JIT    | -DWAMR_BUILD_FAST_JIT=1 |            |
