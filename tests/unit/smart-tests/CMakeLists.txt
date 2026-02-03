# Enhanced Unit Test CMakeLists.txt
cmake_minimum_required(VERSION 3.12)

add_subdirectory(interpreter)

if (WAMR_BUILD_TARGET STREQUAL "X86_64")
  add_subdirectory(constants)
endif ()
