set (AOT_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${AOT_LIB_DIR})

file (GLOB_RECURSE source_all ${AOT_LIB_DIR}/*.c)

set (AOT_LIB_SOURCE ${source_all})

