set (WASI_NN_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_WASI_NN=1)

set (LIBC_WASI_NN_SOURCE ${WASI_NN_DIR}/wasi_nn_native.c ${WASI_NN_DIR}/wasi_nn_tensorflow.cpp)
