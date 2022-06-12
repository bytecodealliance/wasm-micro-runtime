set (WASI_NN_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_WASI_NN=1)

file (GLOB_RECURSE source_all ${WASI_NN_DIR}/*.c)

set (LIBC_WASI_NN_SOURCE ${source_all})
