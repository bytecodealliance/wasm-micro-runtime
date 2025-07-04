# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# global additional warnings. Keep those options consistent with wamr-compiler/CMakeLists.txt.
if (MSVC)
  # warning level 4
  add_compile_options(/W4)
else ()
  # refer to https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
  add_compile_options(-Wall -Wextra -Wformat -Wformat-security -Wshadow)
  # -pedantic causes warnings like "ISO C forbids initialization between function pointer and ‘void *’" which
  #   is widely used in the codebase.
  #
  # -fpermissive causes warnings like "-fpermissive is valid for C++/ObjC++ but not for C"
  #
  # _Static_assert and _Alignof requires c11 or later
  #
  #
  add_compile_options (
    -Wimplicit-function-declaration
    -Wincompatible-pointer-types
  )
  # waivers
  add_compile_options (
    -Wno-unused
    -Wno-unused-parameter
  )

  if (WAMR_BUILD_JIT EQUAL 1)
    # Friendly fire on LLVM libraries.
    add_compile_options (
      -Wno-error=shadow
    )
  endif ()
endif ()
