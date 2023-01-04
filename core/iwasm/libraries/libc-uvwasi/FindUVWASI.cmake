# Find libuvwasi library
# This module defines
#  UVWASI_FOUND, if false, do not try to link to libuvwasi
#  UVWASI_LIBS
#  UVWASI_INCLUDE_DIR, where to find headers

find_path(UVWASI_INCLUDE_DIR NAMES uvwasi.h wasi_serdes.h wasi_types.h PATH_SUFFIXES uvwasi)
find_library(UVWASI_LIBS NAMES uvwasi_a)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  UVWASI
  FOUND_VAR UVWASI_FOUND
  REQUIRED_VARS
    UVWASI_LIBS
    UVWASI_INCLUDE_DIR
)

if(UVWASI_FOUND)
  set(UVWASI_INCLUDE_DIR ${UVWASI_INCLUDE_DIR}/uvwasi)
endif()
