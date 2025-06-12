set(wasi_socket_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../core/iwasm/libraries/lib-socket)
set(wasi_socket_header_dir ${wasi_socket_dir}/inc)

set(srcs ${wasi_socket_dir}/src/wasi/wasi_socket_ext.c)
set(headers
    ${wasi_socket_header_dir}/wasi_socket_ext.h
)

add_library(wamr-wasi-socket STATIC ${srcs})
set_property(TARGET wamr-wasi-socket PROPERTY PUBLIC_HEADER ${headers})
target_include_directories(wamr-wasi-socket
                           PUBLIC
                           $<BUILD_INTERFACE:${wasi_socket_header_dir}>
                           $<INSTALL_INTERFACE:include>)

install(TARGETS wamr-wasi-socket
        EXPORT wamr-wasi-socket-config
        PUBLIC_HEADER DESTINATION include)
install(EXPORT wamr-wasi-socket-config
        DESTINATION lib/cmake/wamr-wasi-socket)
