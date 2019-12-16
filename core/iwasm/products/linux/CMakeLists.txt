# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

project (iwasm)

set (PLATFORM "linux")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

# Set BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "ARM_32", "MIPS_32", "XTENSA_32"
if (NOT DEFINED BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (BUILD_TARGET "X86_32")
  endif ()
endif ()

string(TOUPPER ${BUILD_TARGET} BUILD_TARGET)

# Add definitions for the build target
if (BUILD_TARGET STREQUAL "X86_64")
  add_definitions(-DBUILD_TARGET_X86_64)
elseif (BUILD_TARGET STREQUAL "AMD_64")
  add_definitions(-DBUILD_TARGET_AMD_64)
elseif (BUILD_TARGET STREQUAL "X86_32")
  add_definitions(-DBUILD_TARGET_X86_32)
elseif (BUILD_TARGET STREQUAL "ARM_32")
  add_definitions(-DBUILD_TARGET_ARM_32)
elseif (BUILD_TARGET STREQUAL "MIPS_32")
  add_definitions(-DBUILD_TARGET_MIPS_32)
elseif (BUILD_TARGET STREQUAL "XTENSA_32")
  add_definitions(-DBUILD_TARGET_XTENSA_32)
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  if (BUILD_TARGET STREQUAL "X86_64" OR BUILD_TARGET STREQUAL "AMD_64")
    # Add -fPIC flag if build as 64-bit
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -fPIC")
  else ()
    add_definitions (-m32)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
  endif ()
endif ()

if (NOT DEFINED WASM_ENABLE_WASI)
  # Enable wasi support by default
  set (WASM_ENABLE_WASI 1)
endif ()

if (WASM_ENABLE_WASI EQUAL 1)
  add_definitions(-DWASM_ENABLE_WASI=1)
  message ("-- WASI enabled")
else ()
  message ("-- WASI disabled")
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)
message ("-- CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -Wno-pedantic")

set (SHARED_LIB_DIR ../../../shared-lib)

include_directories (.
                     ../../runtime/include
                     ../../runtime/platform/include
                     ${SHARED_LIB_DIR}/include)

enable_language (ASM)

include (../../runtime/utils/utils.cmake)
include (../../runtime/vmcore-wasm/vmcore.cmake)
include (../../lib/native/libc/wasmtime-wasi-c/wasi.cmake)
include (../../lib/native/base/wasm_lib_base.cmake)
include (../../lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_DIR}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_DIR}/utils/shared_utils.cmake)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wconversion -Wsign-conversion")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch-register")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong --param ssp-buffer-size=4")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,noexecstack,-z,relro,-z,now")

# These flags will lead to about 0.1x ~ 0.25x slower in interpreter mode,
# but we enable them by default to enhance security
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -fPIE -ftrapv -D_FORTIFY_SOURCE=2")

# These flags will lead to about 4x ~ 6x slower in interpreter mode, we disable them by default
# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch=thunk -mfunction-return=thunk")

# message ("-- CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASI_LIB_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE})

add_executable (iwasm main.c ext_lib_export.c)

install (TARGETS iwasm DESTINATION bin)

target_link_libraries (iwasm vmlib -lm -ldl -lpthread)

add_library (libiwasm SHARED
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE})

install (TARGETS libiwasm DESTINATION lib)

set_target_properties (libiwasm PROPERTIES OUTPUT_NAME iwasm)

target_link_libraries (libiwasm -lm -ldl -lpthread)

