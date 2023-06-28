set (IWASM_COMPL_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${IWASM_COMPL_DIR})

if (WAMR_BUILD_DEBUG_AOT EQUAL 1)
    file (GLOB_RECURSE source_all
                    ${IWASM_COMPL_DIR}/*.c
                    ${IWASM_COMPL_DIR}/*.cpp)
else()
    file (GLOB source_all
                    ${IWASM_COMPL_DIR}/simd/*.c
                    ${IWASM_COMPL_DIR}/simd/*.cpp
                    ${IWASM_COMPL_DIR}/*.c
                    ${IWASM_COMPL_DIR}/*.cpp)
endif()

if (WAMR_BUILD_FAST_JIT EQUAL 1
      AND WAMR_BUILD_JIT EQUAL 1
      AND WAMR_BUILD_DYNAMIC_PGO EQUAL 1)
  list(APPEND source_all
    ${IWASM_COMPL_DIR}/dpgo/dpgo_impl.c
    ${IWASM_COMPL_DIR}/dpgo/dpgo_llvm_extra.cpp
  )
endif()

set (IWASM_COMPL_SOURCE ${source_all})

# Disalbe rtti to works with LLVM

if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

