cmake_minimum_required (VERSION 2.8)

project (simple)

set (TARGET_PLATFORM "linux")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Enable repl mode if want to test spec cases
# add_definitions(-DWASM_ENABLE_REPL)

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

# Currently build as 64-bit by default.
set (BUILD_AS_64BIT_SUPPORT "YES")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
if (${BUILD_AS_64BIT_SUPPORT} STREQUAL "YES")
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
message ("CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

if (NOT PLATFORM)
SET(PLATFORM linux)
endif (NOT PLATFORM)
message ("PLATFORM = " ${PLATFORM})

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -Wno-pedantic")

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(WASM_DIR  ${WAMR_ROOT_DIR}/core/iwasm)
set(APP_MGR_DIR ${WAMR_ROOT_DIR}/core/app-mgr)
set(SHARED_DIR ${WAMR_ROOT_DIR}/core/shared-lib)

set (lv_drivers_name lv_drivers)
set (lv_name lvgl)
set (LV_DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/${lv_drivers_name})
set (LVGL_DIR ${WASM_DIR}/lib/3rdparty/${lv_name})

if ((NOT EXISTS ${LVGL_DIR}) OR (NOT EXISTS ${LV_DRIVERS_DIR}))
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl_drivers/CMakeLists.txt)

        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl_drivers )
        if(result)
                message(FATAL_ERROR "CMake step for lvgl drivers failed: ${result}")
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/download_lvgl_drivers )
        if(result)
                message(FATAL_ERROR "Build step for lvgl drivers failed: ${result}")
        endif()
endif()

file(GLOB_RECURSE LV_DRIVERS_SOURCES "${LV_DRIVERS_DIR}/*.c" )

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


include_directories(${SHARED_DIR}/include)

#Note: uncomment below line to use UART mode
#add_definitions (-DCONNECTION_UART)
add_definitions (-DWASM_ENABLE_BASE_LIB)
add_definitions (-Dattr_container_malloc=bh_malloc)
add_definitions (-Dattr_container_free=bh_free)

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

    add_executable (simple src/main.c src/iwasm_main.c src/ext_lib_export.c ${LV_DRIVERS_SOURCES})

target_link_libraries (simple vmlib -lm -ldl -lpthread -lSDL2)

