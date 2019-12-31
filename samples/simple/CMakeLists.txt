cmake_minimum_required (VERSION 2.8)

project (simple)

set (TARGET_PLATFORM "linux")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

set (BUILD_AS_64BIT_SUPPORT "YES")

# Set BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "ARM[sub]", "THUMB[sub]", "MIPS", "XTENSA"
if (NOT DEFINED BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (BUILD_AS_64BIT_SUPPORT STREQUAL "YES")
      # Build as X86_64 by default in 64-bit platform
      set (BUILD_TARGET "X86_64")
    else ()
      set (BUILD_TARGET "X86_32")
    endif ()
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (BUILD_TARGET "X86_32")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

if (NOT DEFINED WASM_ENABLE_INTERP)
  # Enable Interpreter by default
  set (WASM_ENABLE_INTERP 1)
endif ()

if (NOT DEFINED WASM_ENABLE_AOT)
  # Enable AOT by default.
  set (WASM_ENABLE_AOT 1)
endif ()

if (NOT DEFINED WASM_ENABLE_JIT)
  # Disable JIT by default.
  set (WASM_ENABLE_JIT 0)
endif ()

if (NOT DEFINED WASM_ENABLE_WASI)
  # Enable WASI support by default
  set (WASM_ENABLE_WASI 1)
endif ()

if (NOT PLATFORM)
  set (PLATFORM linux)
endif (NOT PLATFORM)
message ("-- PLATFORM = " ${PLATFORM})

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -Wno-pedantic")

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(WASM_DIR  ${WAMR_ROOT_DIR}/core/iwasm)
set(APP_MGR_DIR ${WAMR_ROOT_DIR}/core/app-mgr)
set(SHARED_DIR ${WAMR_ROOT_DIR}/core/shared-lib)

# include the build config template file
include (${WAMR_ROOT_DIR}/cmake/config_common.cmake)

if ("${ENABLE_GUI}" STREQUAL "YES")
    set (LV_DRIVERS_DIR ${WASM_DIR}/lib/3rdparty/lv_drivers)
    set (LVGL_DIR ${WASM_DIR}/lib/3rdparty/lvgl)
    file(GLOB_RECURSE LV_DRIVERS_SOURCES "${LV_DRIVERS_DIR}/*.c" )
endif()

enable_language (ASM)

include (${WASM_DIR}/runtime/utils/utils.cmake)
include (${WASM_DIR}/runtime/vmcore-wasm/vmcore.cmake)
include (${WASM_DIR}/lib/native/base/wasm_lib_base.cmake)
include (${WASM_DIR}/lib/native/libc/wasm_libc.cmake)
include (${WASM_DIR}/lib/native/libc/wasmtime-wasi-c/wasi.cmake)
include (${WASM_DIR}/lib/native/extension/sensor/wasm_lib_sensor.cmake)
if ("${ENABLE_GUI}" STREQUAL "YES")
    include (${WASM_DIR}/lib/native/extension/gui/wasm_lib_gui.cmake)
endif()
include (${WASM_DIR}/lib/native/extension/connection/wasm_lib_conn.cmake)
include (${WASM_DIR}/lib/native/extension/connection/${TARGET_PLATFORM}/connection_mgr.cmake)
include (${WASM_DIR}/lib/native-interface/native_interface.cmake)
include (${APP_MGR_DIR}/app-manager/app_mgr.cmake)
include (${APP_MGR_DIR}/app-mgr-shared/app_mgr_shared.cmake)
include (${SHARED_DIR}/platform/${TARGET_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/coap/lib_coap.cmake)

if (WASM_ENABLE_AOT EQUAL 1)
  include (${WASM_DIR}/runtime/aot/runtime/aot_runtime.cmake)
endif ()

include_directories(${SHARED_DIR}/include)
include_directories(${WASM_DIR}/runtime/platform/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

#Note: uncomment below line to use UART mode
#add_definitions (-DCONNECTION_UART)
add_definitions (-DWASM_ENABLE_BASE_LIB)
add_definitions (-Dattr_container_malloc=bh_malloc)
add_definitions (-Dattr_container_free=bh_free)
add_definitions (-DLV_CONF_INCLUDE_SIMPLE)

if ("${ENABLE_GUI}" STREQUAL "YES")
    add_definitions (-DWASM_ENABLE_GUI=1)
endif()

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASI_LIB_SOURCE}
             ${WASM_LIBC_SOURCE}
             ${APP_MGR_SOURCE}
             ${WASM_LIB_BASE_SOURCE}
             ${WASM_LIB_EXT_SOURCE}
             ${WASM_LIB_SENSOR_SOURCE}
             ${WASM_LIB_GUI_SOURCE}
             ${WASM_LIB_CONN_SOURCE}
             ${WASM_LIB_CONN_MGR_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${NATIVE_INTERFACE_SOURCE}
             ${AOT_RUNTIME_SOURCE}
            )

if ("${ENABLE_GUI}" STREQUAL "YES")
    add_executable (simple src/main.c src/iwasm_main.c src/ext_lib_export.c ${LV_DRIVERS_SOURCES})
    target_link_libraries (simple vmlib -lm -ldl -lpthread -lSDL2)
else ()
    add_executable (simple src/main.c src/iwasm_main.c src/ext_lib_export.c)
    target_link_libraries (simple vmlib -lm -ldl -lpthread)
endif ()


