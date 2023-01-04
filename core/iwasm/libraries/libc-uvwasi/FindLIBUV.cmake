# Find libuv library
# This module defines
#  LIBUV_FOUND, if false, do not try to link to libuv
#  UV_A_LIBS
#  LIBUV_INCLUDE_DIR, where to find uv.h

find_path(LIBUV_INCLUDE_DIR NAMES uv.h)
find_library(UV_A_LIBS NAMES uv libuv)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  LIBUV
  FOUND_VAR LIBUV_FOUND
  REQUIRED_VARS
    UV_A_LIBS
    LIBUV_INCLUDE_DIR
)

if(WIN32)
  list(APPEND UV_A_LIBS iphlpapi)
  list(APPEND UV_A_LIBS psapi)
  list(APPEND UV_A_LIBS userenv)
  list(APPEND UV_A_LIBS ws2_32)
endif()
