cmake_minimum_required(VERSION 3.8)
project(sandbox CXX)

INCLUDE (CheckCXXSourceCompiles)

set(CHILD_SOURCES library_runner.cc)
set(NOSANDBOX_SOURCES nosandbox.cc)
set(EXAMPLE_SOURCES example.cc)
set(EXAMPLE_HEADERS shared.h)
set(EXAMPLE_LIB_SOURCES lib.cc)
set(LIBSANDBOX_SOURCES libsandbox.cc)
set(LIBSANDBOX_HEADERS sandbox.hh)

include_directories(AFTER "${CMAKE_SOURCE_DIR}/../../external/snmalloc/src")
set(CMAKE_CXX_STANDARD 17)


if (MSVC)
else ()
	add_compile_options(-mcx16 -Wall -Wextra -Werror "$<$<CONFIG:DEBUG>:-fno-inline>")
	set(LLD_NAMES
		lld70
		lld60
		lld-7.0
		lld-70
		lld)
	# Loop over each of the possible names of clang-format and try to find one.
	set(LLD LLD-NOTFOUND)
	foreach (NAME IN ITEMS ${LLD_NAMES})
		if (${LLD} STREQUAL "LLD-NOTFOUND")
			find_program(LLD "ld.${NAME}")
		endif ()
	endforeach()
		if (${LLD} STREQUAL "LLD-NOTFOUND")
			message(WARNING "Could not find lld, linking may fail")
		else ()
			message(STATUS "Linking with ${LLD}")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=${LLD}")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=${LLD}")
		endif ()
endif()

set(DEFAULT_KQUEUE false)
set(DEFAULT_PROCDESC false)
set(DEFAULT_CAPSICUM false)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	set(DEFAULT_KQUEUE true)
	set(DEFAULT_PROCDESC true)
	set(DEFAULT_CAPSICUM true)
endif()

set(ENABLE_KQUEUE ${DEFAULT_KQUEUE} CACHE BOOL "Use kqueue for for file descriptor event handling")
set(ENABLE_PROCDESC ${DEFAULT_PROCDESC} CACHE BOOL "Use process descriptors and kqueue for creating the child process")
set(ENABLE_CAPSICUM ${DEFAULT_CAPSICUM} CACHE BOOL "Use Capsicum to restruct child process privileges")

if (${ENABLE_KQUEUE})
	add_definitions(-DUSE_KQUEUE)
endif()
if (${ENABLE_PROCDESC})
	add_definitions(-DUSE_KQUEUE_PROCDESC)
endif()
if (${ENABLE_CAPSICUM})
	add_definitions(-DUSE_CAPSICUM)
endif()

add_library(sandbox SHARED ${LIBSANDBOX_SOURCES})
add_executable(library_runner ${CHILD_SOURCES})

add_executable(nosandbox ${NOSANDBOX_SOURCES})
add_library(example_lib SHARED ${EXAMPLE_LIB_SOURCES})
add_executable(example ${EXAMPLE_SOURCES})

target_link_libraries(nosandbox -lz)
target_link_libraries(example_lib -lz)
target_link_libraries(library_runner -pthread)
target_link_libraries(example sandbox)
target_link_libraries(sandbox -pthread)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_link_libraries(library_runner -ldl -lbsd)
	target_link_libraries(sandbox -ldl -lbsd)
endif()

set_target_properties(example_lib PROPERTIES PREFIX "")


add_custom_target(
	test
	COMMAND time ./example example && (sha1 example.Z ";" time ./nosandbox example && sha1 example.Z)
	DEPENDS nosandbox sandbox example library_runner
)


# The clang-format tool is installed under a variety of different names.  Try
# to find a sensible one.  Only look for 6.0 and 7.0 versions explicitly - we
# don't know whether our clang-format file will work with newer versions of the
# tool
set(CLANG_FORMAT_NAMES
	clang-format-10.0
	clang-format-9.0
	clang-format-8.0
	clang-format10
	clang-format90
	clang-format80
	clang-format)

# Loop over each of the possible names of clang-format and try to find one.
set(CLANG_FORMAT CLANG_FORMAT-NOTFOUND)
foreach (NAME IN ITEMS ${CLANG_FORMAT_NAMES})
	if (${CLANG_FORMAT} STREQUAL "CLANG_FORMAT-NOTFOUND")
		find_program(CLANG_FORMAT ${NAME})
	endif ()
endforeach()

# If we've found a clang-format tool, generate a target for it, otherwise emit
# a warning.
if (${CLANG_FORMAT} STREQUAL "CLANG_FORMAT-NOTFOUND")
	message(WARNING "Not generating clangformat target, no clang-format tool found")
else ()
	message(STATUS "Generating clangformat target using ${CLANG_FORMAT}")
	set(ALL_SOURCE_FILES
	${CMAKE_SOURCE_DIR}/${CHILD_SOURCES}
	${CMAKE_SOURCE_DIR}/${NOSANDBOX_SOURCES}
	${CMAKE_SOURCE_DIR}/${EXAMPLE_SOURCES}
	${CMAKE_SOURCE_DIR}/${EXAMPLE_HEADERS}
	${CMAKE_SOURCE_DIR}/${EXAMPLE_LIB_SOURCES}
	${CMAKE_SOURCE_DIR}/${LIBSANDBOX_SOURCES}
	${CMAKE_SOURCE_DIR}/${LIBSANDBOX_HEADERS})
	if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
		add_custom_target(copy_format_file)
	else ()
		add_custom_target(
			copy_format_file
			COMMAND cmake -E copy ${CMAKE_SOURCE_DIR}/../../.clang-format ${CMAKE_BINARY_DIR}/)
	endif ()
	add_custom_target(
		clangformat
		COMMAND ${CLANG_FORMAT}
		-i
		${ALL_SOURCE_FILES}
		DEPENDS copy_format_file
	)
endif()

