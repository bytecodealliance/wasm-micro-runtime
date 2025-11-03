# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(unsupported_features_tests)

include(CMakePrintHelpers)

# fake target
include(../unit_common.cmake)
add_library(unsupported_features_tests EXCLUDE_FROM_ALL ${WAMR_RUNTIME_LIB_SOURCE})

enable_testing()

# Define a function to add tests with unsupported features
function(add_unsupported_feature_test test_name flags)
  cmake_print_variables(test_name flags)
  add_test(
    NAME verify_${test_name}
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_LIST_DIR} -B build_${test_name} ${flags} --fresh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_tests_properties(verify_${test_name} PROPERTIES WILL_FAIL TRUE)
endfunction()

# List of unsupported feature tests
set(UNSUPPORTED_FEATURE_TESTS
  "exce_handling_aot -DWAMR_BUILD_EXCE_HANDLING=1 -DWAMR_BUILD_AOT=1"
  "exce_handling_fast_interp -DWAMR_BUILD_EXCE_HANDLING=1 -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=1"
  "exce_handling_fast_jit -DWAMR_BUILD_EXCE_HANDLING=1 -DWAMR_BUILD_FAST_JIT=1"
  "exce_handling_llvm_jit -DWAMR_BUILD_EXCE_HANDLING=1 -DWAMR_BUILD_JIT=1"
  "gc_fast_jit -DWAMR_BUILD_GC=1 -DWAMR_BUILD_FAST_JIT=1"
  "memory64_fast_interp -DWAMR_BUILD_MEMORY64=1 -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=1"
  "memory64_fast_jit -DWAMR_BUILD_MEMORY64=1 -DWAMR_BUILD_FAST_JIT=1"
  "memory64_llvm_jit -DWAMR_BUILD_MEMORY64=1 -DWAMR_BUILD_JIT=1"
  "multi_memory_aot -DWAMR_BUILD_MULTI_MEMORY=1 -DWAMR_BUILD_AOT=1"
  "multi_memory_fast_interp -DWAMR_BUILD_MULTI_MEMORY=1 -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=1"
  "multi_memory_fast_jit -DWAMR_BUILD_MULTI_MEMORY=1 -DWAMR_BUILD_FAST_JIT=1"
  "multi_memory_llvm_jit -DWAMR_BUILD_MULTI_MEMORY=1 -DWAMR_BUILD_JIT=1"
  "multi_module_fast_jit -DWAMR_BUILD_MULTI_MODULE=1 -DWAMR_BUILD_FAST_JIT=1"
  "multi_module_llvm_jit -DWAMR_BUILD_MULTI_MODULE=1 -DWAMR_BUILD_JIT=1"
  "simd_classic_interp -DWAMR_BUILD_SIMD=1 -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_FAST_INTERP=0"
  "simd_fast_jit -DWAMR_BUILD_SIMD=1 -DWAMR_BUILD_FAST_JIT=1"
)

# Add each test using the function
foreach(test ${UNSUPPORTED_FEATURE_TESTS})
  # string -> list by replacing space with ;
  string(REPLACE " " ";" test_parts "${test}")
  # list[0]
  list(GET test_parts 0 test_name)
  # list[1:]
  list(REMOVE_AT test_parts 0)
  # pass list to cmake and let cmake split it
  set(flags ${test_parts})

  add_unsupported_feature_test(${test_name} "${flags}")
endforeach()

