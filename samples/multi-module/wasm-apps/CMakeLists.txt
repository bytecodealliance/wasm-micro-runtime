cmake_minimum_required(VERSION 2.8)
project(wasm-apps)

set(CMAKE_VERBOSE_MAKEFILE on)

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(CLANG_COMMAND "/opt/wasi-sdk/bin/clang")

set(CLANG_FLAGS --target=wasm32 -nostdlib)
set(CLANG_FLAGS ${CLANG_FLAGS} -Wl,--no-entry,--allow-undefined,--export-all)

set(SOURCE_A ${CMAKE_CURRENT_SOURCE_DIR}/mA.c)
add_custom_command(
  OUTPUT mA.wasm
  COMMENT "Transform mA.C to mA.WASM"
  COMMAND ${CLANG_COMMAND} ${CLANG_FLAGS} -o mA.wasm ${SOURCE_A}
  DEPENDS ${SOURCE_A}
  VERBATIM
)

set(SOURCE_B ${CMAKE_CURRENT_SOURCE_DIR}/mB.c)
add_custom_command(
  OUTPUT mB.wasm
  COMMENT "Transform mB.C to mB.WASM"
  COMMAND ${CLANG_COMMAND} ${CLANG_FLAGS} -o mB.wasm ${SOURCE_B}
  DEPENDS ${SOURCE_B}
  VERBATIM
)

set(SOURCE_C ${CMAKE_CURRENT_SOURCE_DIR}/mC.c)
add_custom_command(
  OUTPUT mC.wasm
  COMMENT "Transform mC.C to mC.WASM"
  COMMAND ${CLANG_COMMAND} ${CLANG_FLAGS} -o mC.wasm ${SOURCE_C}
  DEPENDS ${SOURCE_C}
  VERBATIM
)

add_custom_target(wasm-modules ALL
  DEPENDS mA.wasm mB.wasm mC.wasm
)