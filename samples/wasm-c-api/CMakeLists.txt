# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

if (NOT WAMR_BUILD_PLATFORM STREQUAL "windows")
  project(c-api)
else()
  project (c-api C ASM)
  enable_language (ASM_MASM)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
################  runtime settings  ################
string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
if (APPLE)
  add_definitions(-DBH_PLATFORM_DARWIN)
endif ()

# Resetdefault linker flags
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# WAMR features switch
set(WAMR_BUILD_TARGET "X86_64")

if(NOT DEFINED WAMR_BUILD_INTERP)
  set(WAMR_BUILD_INTERP 1)
endif()

if(NOT DEFINED WAMR_BUILD_AOT)
  set(WAMR_BUILD_AOT 0)
endif()

if(NOT DEFINED WAMR_BUILD_JOT)
  set(WAMR_BUILD_JIT 0)
endif()

set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 0)

if(NOT DEFINED WAMR_BUILD_FAST_INTERP)
  set(WAMR_BUILD_FAST_INTERP 0)
endif()

if (NOT MSVC)
  # compiling and linking flags
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -fPIE")
  if (NOT (CMAKE_C_COMPILER MATCHES ".*clang.*" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
  endif ()
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")
  if (WAMR_BUILD_TARGET MATCHES "X86_.*" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
    if (NOT (CMAKE_C_COMPILER MATCHES ".*clang.*" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mindirect-branch-register")
    endif ()
  endif ()
endif()
# build out vmlib
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(WAMRC ${WAMR_ROOT_DIR}/wamr-compiler/build/wamrc)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

add_library(vmlib STATIC ${WAMR_RUNTIME_LIB_SOURCE})
if (MSVC)
  target_compile_definitions(vmlib PRIVATE WASM_API_EXTERN=)
endif()
################################################

################  application related  ################
file(GLOB SOURCES src/*.c)
add_library(c-api ${SOURCES})
target_include_directories(c-api
  PRIVATE ${C_API_PATH}/include
)
target_link_libraries(c-api PRIVATE vmlib -lpthread -lm)
if (MSVC)
  target_compile_definitions(c-api PRIVATE WASM_API_EXTERN=)
endif()

foreach(SRC ${SOURCES})
  get_filename_component(APPNAME ${SRC} NAME_WE)

  # build executable for each .c
  add_executable(${APPNAME} ${SRC})
  message("create executable about ${APPNAME}")
  target_link_libraries(${APPNAME} c-api)
  if (MSVC)
    target_compile_definitions(${APPNAME} PRIVATE WASM_API_EXTERN=)
  endif()

  # copy .wasm
  add_custom_command(TARGET ${APPNAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
                  ${CMAKE_CURRENT_SOURCE_DIR}/src/${APPNAME}.wasm
                  ${PROJECT_BINARY_DIR}/
      BYPRODUCTS ${APPNAME}.wasm
      COMMENT "Copy ${SRC} to the output directory"
  )

  # generate .aot file
  if(${WAMR_BUILD_AOT} EQUAL 1)
    if(EXISTS ${WAMRC})
      add_custom_command(TARGET ${APPNAME} POST_BUILD
          COMMAND ${WAMRC} -o ${APPNAME}.aot
              ${CMAKE_CURRENT_SOURCE_DIR}/src/${APPNAME}.wasm
          BYPRODUCTS ${APPNAME}.aot
          COMMENT "generate a aot file ${APPNAME}.aot"
      )
    endif()
  endif()

endforeach(SRC ${SOURCES})
################################################
