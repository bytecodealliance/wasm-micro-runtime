set (IWASM_COMPL_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${IWASM_COMPL_DIR})

file (GLOB_RECURSE source_all ${IWASM_COMPL_DIR}/*.c)

set (IWASM_COMPL_SOURCE ${source_all})

