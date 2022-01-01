# Set WAMR's build options
if("${IDF_TARGET}" STREQUAL "esp32c3")
    set(WAMR_BUILD_TARGET "RISCV32")
else()
    set(WAMR_BUILD_TARGET "XTENSA")
    add_compile_options(-DWAMR_BUILD_TARGET_XTENSA=1)
endif()

set(WAMR_BUILD_PLATFORM "esp-idf")

if (NOT DEFINED WAMR_BUILD_INTERP)
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_FAST_INTERP)
  set (WAMR_BUILD_FAST_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  set (WAMR_BUILD_AOT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_APP_FRAMEWORK)
  set (WAMR_BUILD_APP_FRAMEWORK 0)
endif ()

if (NOT CMAKE_BUILD_EARLY_EXPANSION)
    # Set the compile time variable so that the right binary is selected
    add_compile_options(-DWAMR_BUILD_INTERP=${WAMR_BUILD_INTERP})

    set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
    include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
endif()

idf_component_register(SRCS ${WAMR_RUNTIME_LIB_SOURCE} ${PLATFORM_SHARED_SOURCE}
  INCLUDE_DIRS ${IWASM_DIR}/include ${UTILS_SHARED_DIR} ${PLATFORM_SHARED_DIR} ${PLATFORM_SHARED_DIR}/../include
  REQUIRES pthread
)


