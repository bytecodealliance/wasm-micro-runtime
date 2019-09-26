# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
