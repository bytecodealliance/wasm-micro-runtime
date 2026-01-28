set (IWASM_COMPL_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${IWASM_COMPL_DIR})
enable_language(CXX)

file (GLOB source_all
  ${IWASM_COMPL_DIR}/aot.c
  ${IWASM_COMPL_DIR}/aot_compiler.c
  ${IWASM_COMPL_DIR}/aot_llvm.c
  ${IWASM_COMPL_DIR}/aot_llvm_extra*.cpp
  ${IWASM_COMPL_DIR}/aot_stack_frame*.c
  ${IWASM_COMPL_DIR}/aot_emit_*.c
)

if (WAMR_BUILD_DEBUG_AOT EQUAL 1)
  file(GLOB debug_sources
    ${IWASM_COMPL_DIR}/debug/*.c
  )
  list(APPEND source_all ${debug_sources})
endif ()

if (WAMR_BUILD_SIMD EQUAL 1)
  file(GLOB simd_sources
    ${IWASM_COMPL_DIR}/simd/*.c
  )
  list(APPEND source_all ${simd_sources})
endif ()

if (WAMR_BUILD_LLVM EQUAL 1)
  message("Build with LLVM ORC JIT support")
  file(GLOB orc_jit_sources
    ${IWASM_COMPL_DIR}/aot_orc_extra*.cpp
  )
  list(APPEND source_all ${orc_jit_sources})
endif ()

set (IWASM_COMPL_SOURCE ${source_all})

# Disable rtti to works with LLVM

if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

