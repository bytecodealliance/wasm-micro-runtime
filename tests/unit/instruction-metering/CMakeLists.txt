cmake_minimum_required(VERSION 3.14)

project(test-instruction-metering)

add_definitions(-DRUN_ON_LINUX)

set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_WASI 0)
set(WAMR_BUILD_APP_FRAMEWORK 0)
set(WAMR_BUILD_INSTRUCTION_METERING 1)

if(NOT DEFINED WASI_SDK_DIR)
  set(WASI_SDK_DIR "/opt/wasi-sdk")
endif()
set(WASISDK_TOOLCHAIN "${WASI_SDK_DIR}/share/cmake/wasi-sdk.cmake")

include(ExternalProject)
include(GoogleTest)
if (TARGET gtest)
  get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
  if (GTEST_INCLUDE_DIRS)
    include_directories(${GTEST_INCLUDE_DIRS})
  endif()
endif()
ExternalProject_Add(
  instruction_metering_wasm_apps
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps
  BUILD_ALWAYS YES
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps -B build
                    -DWASI_SDK_PREFIX=${WASI_SDK_DIR}
                    -DCMAKE_TOOLCHAIN_FILE=${WASISDK_TOOLCHAIN}
  BUILD_COMMAND     ${CMAKE_COMMAND} --build build
  INSTALL_COMMAND   ${CMAKE_COMMAND} --install build --prefix ${CMAKE_CURRENT_BINARY_DIR}
)

include(../unit_common.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE source_all ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(unit_test_sources
  ${source_all}
  ${WAMR_RUNTIME_LIB_SOURCE}
  ${UNCOMMON_SHARED_SOURCE}
)

add_executable(instruction_metering_test ${unit_test_sources})
target_link_libraries(instruction_metering_test gtest_main gtest)
add_dependencies(instruction_metering_test instruction_metering_wasm_apps)

gtest_discover_tests(instruction_metering_test)
