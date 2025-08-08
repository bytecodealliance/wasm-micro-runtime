# Copyright 2025 Sony Semiconductor Solutions Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Find ONNX Runtime library
#
# This module defines the following variables:
#
# ::
#
#   onnxruntime_FOUND        - True if onnxruntime is found
#   onnxruntime_INCLUDE_DIRS - Include directories for onnxruntime
#   onnxruntime_LIBRARIES    - List of libraries for onnxruntime
#   onnxruntime_VERSION      - Version of onnxruntime
#
# ::
#
# Example usage:
#
#   find_package(onnxruntime)
#   if(onnxruntime_FOUND)
#     target_link_libraries(app onnxruntime)
#   endif()

# First try to find ONNX Runtime using the CMake config file
# FIXME: This is a temporary workaround for ONNX Runtime's broken CMake config on Linux.
# See https://github.com/microsoft/onnxruntime/issues/25279
# Once the upstream issue is fixed, this conditional can be safely removed.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(onnxruntime CONFIG QUIET)
  if(onnxruntime_FOUND)
    return()
  endif()
endif()

# If not found via CMake config, try to find manually
find_path(onnxruntime_INCLUDE_DIR
  NAMES onnxruntime_c_api.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/onnxruntime/include
    $ENV{ONNXRUNTIME_ROOT}/include
    ${CMAKE_CURRENT_LIST_DIR}/../../../../..
)

find_library(onnxruntime_LIBRARY
  NAMES onnxruntime
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/onnxruntime/lib
    $ENV{ONNXRUNTIME_ROOT}/lib
    ${CMAKE_CURRENT_LIST_DIR}/../../../../..
)

# Try to determine version from header file
if(onnxruntime_INCLUDE_DIR)
  file(STRINGS "${onnxruntime_INCLUDE_DIR}/onnxruntime_c_api.h" onnxruntime_version_str
    REGEX "^#define[\t ]+ORT_API_VERSION[\t ]+[0-9]+")
  
  if(onnxruntime_version_str)
    string(REGEX REPLACE "^#define[\t ]+ORT_API_VERSION[\t ]+([0-9]+)" "\\1"
      onnxruntime_VERSION "${onnxruntime_version_str}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(onnxruntime
  REQUIRED_VARS onnxruntime_LIBRARY onnxruntime_INCLUDE_DIR
  VERSION_VAR onnxruntime_VERSION
)

if(onnxruntime_FOUND)
  set(onnxruntime_LIBRARIES ${onnxruntime_LIBRARY})
  set(onnxruntime_INCLUDE_DIRS ${onnxruntime_INCLUDE_DIR})

  if(NOT TARGET onnxruntime::onnxruntime)
    add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED)
    set_target_properties(onnxruntime::onnxruntime PROPERTIES
      IMPORTED_LOCATION "${onnxruntime_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(onnxruntime_INCLUDE_DIR onnxruntime_LIBRARY)
