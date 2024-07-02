set (LIB_SHARED_HEAP ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_SHARED_HEAP=1)

include_directories(${LIB_SHARED_HEAP_DIR})

file (GLOB source_all ${LIB_SHARED_HEAP}/*.c)

set (LIB_SHARED_HEAP_SOURCE ${source_all})