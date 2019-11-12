# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR wasm32)
SET (CMAKE_SYSROOT                ${CMAKE_CURRENT_LIST_DIR}/sysroot)

SET (CMAKE_C_FLAGS                "-nostdlib" CACHE INTERNAL "")
SET (CMAKE_C_COMPILER_TARGET      "wasm32")
SET (CMAKE_C_COMPILER             "clang-8")

SET (CMAKE_CXX_FLAGS                "-nostdlib" CACHE INTERNAL "")
SET (CMAKE_CXX_COMPILER_TARGET      "wasm32")
SET (CMAKE_CXX_COMPILER             "clang++-8")

SET (CMAKE_EXE_LINKER_FLAGS "-Wl,--no-entry,--export-all,--allow-undefined-file=${CMAKE_SYSROOT}/share/defined-symbols.txt" CACHE INTERNAL "")
SET (CMAKE_LINKER  "/usr/bin/wasm-ld-8" CACHE INTERNAL "")

SET (CMAKE_AR      "/usr/bin/llvm-ar-8" CACHE INTERNAL "")
SET (CMAKE_NM      "/usr/bin/llvm-nm-8" CACHE INTERNAL "")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump-8" CACHE INTERNAL "")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib-8" CACHE INTERNAL "")
