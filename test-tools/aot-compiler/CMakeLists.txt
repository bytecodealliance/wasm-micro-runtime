# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
cmake_minimum_required (VERSION 2.8)

project (aot-compiler)

set (PLATFORM "linux")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_definitions(-DWASM_ENABLE_INTERP=1)
add_definitions(-DBUILD_AOT_COMPILER=1)

# Set BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "ARM_32", "MIPS_32", "XTENSA_32"
if (NOT BUILD_TARGET)
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

if (NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)
message ("-- CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

# Enable LLVM
set (LLVM_SRC_ROOT "${PROJECT_SOURCE_DIR}/../../core/iwasm/lib/3rdparty/llvm")
if (NOT EXISTS "${LLVM_SRC_ROOT}/build")
  message (FATAL_ERROR "Cannot find LLVM dir: ${LLVM_SRC_ROOT}/build")
endif ()
set (CMAKE_PREFIX_PATH "${LLVM_SRC_ROOT}/build;${CMAKE_PREFIX_PATH}")
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections \
                    -Wall -Wno-unused-parameter -Wno-pedantic")

set (SHARED_LIB_DIR ../../core/shared-lib)
set (IWASM_DIR ../../core/iwasm)

include_directories (.
                     ${IWASM_DIR}/runtime/include
                     ${IWASM_DIR}/runtime/platform/include
                     ${SHARED_LIB_DIR}/include)

enable_language (ASM)

include (${IWASM_DIR}/runtime/utils/utils.cmake)
include (${IWASM_DIR}/runtime/vmcore-wasm/vmcore.cmake)
include (${IWASM_DIR}/runtime/aot/runtime/aot_runtime.cmake)
include (${IWASM_DIR}/runtime/aot/compiler/aot_compiler.cmake)
include (${IWASM_DIR}/lib/native/base/wasm_lib_base.cmake)
include (${IWASM_DIR}/lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_DIR}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_DIR}/mem-alloc/mem_alloc.cmake)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wconversion -Wsign-conversion")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch-register")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong --param ssp-buffer-size=4")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,noexecstack,-z,relro,-z,now")

# We disable these flags by default to stay the same with wasm runtime
# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch=thunk -mfunction-return=thunk")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -fPIE -ftrapv -D_FORTIFY_SOURCE=2")

# message ("-- CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${AOT_RUNTIME_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE})

add_library (aotclib ${AOT_COMPILER_SOURCE})

add_executable (wamrc main.c ext_lib_export.c)

target_link_libraries (wamrc aotclib vmlib ${LLVM_AVAILABLE_LIBS} -lm -ldl -lpthread)

