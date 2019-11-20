cmake_minimum_required (VERSION 2.8)

project (wasm_runtime_wgl)

set(REPO_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)
set(WASM_DIR  ${REPO_ROOT_DIR}/core/iwasm)
set(APP_MGR_DIR ${REPO_ROOT_DIR}/core/app-mgr)
set(SHARED_DIR ${REPO_ROOT_DIR}/core/shared-lib)

set (LV_DRIVERS_DIR ${WASM_DIR}/lib/3rdparty/lv_drivers)
set (LVGL_DIR ${WASM_DIR}/lib/3rdparty/lvgl)

file(GLOB_RECURSE LV_DRIVERS_SOURCES "${LV_DRIVERS_DIR}/*.c" )

set (TARGET_PLATFORM "linux")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

# Set BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "ARM_32", "MIPS_32", "XTENSA_32"
if (NOT BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (BUILD_TARGET "X86_32")
  endif ()
endif ()

string(TOUPPER ${BUILD_TARGET} BUILD_TARGET)

# Add definitions for the build target
if (BUILD_TARGET STREQUAL "X86_64")
  add_definitions(-DBUILD_TARGET_X86_64)
elseif (BUILD_TARGET STREQUAL "AMD_64")
  add_definitions(-DBUILD_TARGET_AMD_64)
elseif (BUILD_TARGET STREQUAL "X86_32")
  add_definitions(-DBUILD_TARGET_X86_32)
elseif (BUILD_TARGET STREQUAL "ARM_32")
  add_definitions(-DBUILD_TARGET_ARM_32)
elseif (BUILD_TARGET STREQUAL "MIPS_32")
  add_definitions(-DBUILD_TARGET_MIPS_32)
elseif (BUILD_TARGET STREQUAL "XTENSA_32")
  add_definitions(-DBUILD_TARGET_XTENSA_32)
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  if (BUILD_TARGET STREQUAL "X86_64" OR BUILD_TARGET STREQUAL "AMD_64")
    # Add -fPIC flag if build as 64-bit
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -fPIC")
  else ()
    add_definitions (-m32)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
SET(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)
message ("-- CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

if (NOT PLATFORM)
SET(PLATFORM linux)
endif (NOT PLATFORM)
message ("-- PLATFORM = " ${PLATFORM})

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -Wno-pedantic")


enable_language (ASM)

include (${WASM_DIR}/runtime/platform/${TARGET_PLATFORM}/platform.cmake)
include (${WASM_DIR}/runtime/utils/utils.cmake)
include (${WASM_DIR}/runtime/vmcore-wasm/vmcore.cmake)
include (${WASM_DIR}/lib/native/base/wasm_lib_base.cmake)
include (${WASM_DIR}/lib/native/libc/wasm_libc.cmake)
include (${WASM_DIR}/lib/native/extension/sensor/wasm_lib_sensor.cmake)
include (${WASM_DIR}/lib/native/extension/gui/wasm_lib_gui.cmake)
include (${WASM_DIR}/lib/native/extension/connection/wasm_lib_conn.cmake)
include (${WASM_DIR}/lib/native/extension/connection/${TARGET_PLATFORM}/connection_mgr.cmake)
include (${WASM_DIR}/lib/native-interface/native_interface.cmake)
include (${APP_MGR_DIR}/app-manager/app_mgr.cmake)
include (${APP_MGR_DIR}/app-mgr-shared/app_mgr_shared.cmake)
include (${SHARED_DIR}/platform/${TARGET_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/coap/lib_coap.cmake)

set (PROJECT_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../src/platform/${TARGET_PLATFORM})

include_directories(${SHARED_DIR}/include)
include_directories(${PROJECT_SRC_DIR})

add_definitions (-DWASM_ENABLE_BASE_LIB)
add_definitions (-Dattr_container_malloc=bh_malloc)
add_definitions (-Dattr_container_free=bh_free)
add_definitions (-DLV_CONF_INCLUDE_SIMPLE)
add_definitions (-DWASM_ENABLE_GUI=1)

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
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
             )

set (SOURCES
     ${PROJECT_SRC_DIR}/main.c
     ${PROJECT_SRC_DIR}/iwasm_main.c
     ${PROJECT_SRC_DIR}/../../ext_lib_export.c
     ${LV_DRIVERS_SOURCES}
     )

add_executable (wasm_runtime_wgl ${SOURCES})

target_link_libraries (wasm_runtime_wgl vmlib -lm -ldl -lpthread -lSDL2)
#target_link_libraries(wasm_runtime_wgl PRIVATE SDL2 )

