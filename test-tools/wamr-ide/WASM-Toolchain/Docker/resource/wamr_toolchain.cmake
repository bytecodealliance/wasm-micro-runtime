# INTEL CONFIDENTIAL
#
# Copyright (C) 2022 Intel Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents
# related to the source code ("Material") are owned by Intel
# Corporation or its suppliers or licensors. Title to the Material
# remains with Intel Corporation or its suppliers and licensors. The
# Material contains trade secrets and proprietary and confidential
# information of Intel or its suppliers and licensors. The Material
# is protected by worldwide copyright and trade secret laws and
# treaty provisions. No part of the Material may be used, copied,
# reproduced, modified, published, uploaded, posted, transmitted,
# distributed, or disclosed in any way without Intel's prior express
# written permission.
#
# No license under any patent, copyright, trade secret or other
# intellectual property right is granted to or conferred upon you by
# disclosure or delivery of the Materials, either expressly, by
# implication, inducement, estoppel or otherwise. Any license under
# such intellectual property rights must be express and approved by
# Intel in writing.

SET (CMAKE_SYSTEM_NAME Linux)
SET (CMAKE_SYSTEM_PROCESSOR wasm32)
SET (CMAKE_SYSROOT ${CMAKE_CURRENT_LIST_DIR}/libc-builtin-sysroot)

IF (NOT (DEFINED WASI_SDK_DIR OR DEFINED CACHE{WASI_SDK_DIR}))
  SET (WASI_SDK_DIR "/opt/wasi-sdk")
ELSE ()
  MESSAGE (STATUS "WASI_SDK_DIR=${WASI_SDK_DIR}")
  LIST (APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES "WASI_SDK_DIR")
ENDIF ()


SET (CMAKE_C_FLAGS                  "-nostdlib"   CACHE INTERNAL "")
SET (CMAKE_C_COMPILER_TARGET        "wasm32")
SET (CMAKE_C_COMPILER               "${WASI_SDK_DIR}/bin/clang")

SET (CMAKE_CXX_FLAGS                "-nostdlib"   CACHE INTERNAL "")
SET (CMAKE_CXX_COMPILER_TARGET      "wasm32")
SET (CMAKE_CXX_COMPILER             "${WASI_SDK_DIR}/bin/clang++")

SET (CMAKE_EXE_LINKER_FLAGS
    "-Wl,--no-entry,--fatal-warnings" CACHE INTERNAL "")

SET (CMAKE_LINKER  "${WASI_SDK_DIR}/bin/wasm-ld"                     CACHE INTERNAL "")
SET (CMAKE_AR      "${WASI_SDK_DIR}/bin/llvm-ar"                     CACHE INTERNAL "")
SET (CMAKE_NM      "${WASI_SDK_DIR}/bin/llvm-nm"                     CACHE INTERNAL "")
SET (CMAKE_OBJDUMP "${WASI_SDK_DIR}/bin/llvm-dwarfdump"              CACHE INTERNAL "")
SET (CMAKE_RANLIB  "${WASI_SDK_DIR}/bin/llvm-ranlib"                 CACHE INTERNAL "")
SET (CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS},--allow-undefined-file=${CMAKE_SYSROOT}/share/defined-symbols.txt" CACHE INTERNAL "")