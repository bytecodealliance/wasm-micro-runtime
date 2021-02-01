set (IWASM_COMPL_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${IWASM_COMPL_DIR})

if (WAMR_BUILD_DEBUG_INFO EQUAL 1)
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

set (IWASM_COMPL_SOURCE ${source_all})

