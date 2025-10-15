# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

function(check_ubuntu_version)
# ubuntu 2204 is the recommended environment for collecting coverage data for now
# otherwise, there will be ERRORs, when using 2404, like below and
#
# geninfo: ERROR: ('mismatch') mismatched end line for _ZN63compilation_aot_emit_memory_test_aot_check_memory_overflow_Test8TestBodyEv at /workspaces/wasm-micro-runtime/tests/unit/compilation/aot_emit_memory_test.cc:96: 96 -> 106
#        (use "geninfo --ignore-errors mismatch,mismatch ..." to suppress this warning)
# geninfo: ERROR: ('negative') Unexpected negative count '-3' for /workspaces/wasm-micro-runtime/core/iwasm/interpreter/wasm_interp_classic.c:5473.
#        Perhaps you need to compile with '-fprofile-update=atomic
#        (use "geninfo --ignore-errors negative,negative ..." to suppress this warning)
#
# For sure, `--ignore-errors` can be used to ignore these errors, but better to use the recommended environment.
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    string(REGEX MATCH "VERSION_ID=\"([0-9]+)\\.([0-9]+)\"" _ ${OS_RELEASE_CONTENT})
    if(NOT DEFINED CMAKE_MATCH_1 OR NOT DEFINED CMAKE_MATCH_2)
        message(WARNING "Unable to detect Ubuntu version. Please ensure you are using Ubuntu 22.04.")
        return()
    endif()

    set(UBUNTU_MAJOR_VERSION ${CMAKE_MATCH_1})
    set(UBUNTU_MINOR_VERSION ${CMAKE_MATCH_2})

    if(NOT (UBUNTU_MAJOR_VERSION EQUAL 22 AND UBUNTU_MINOR_VERSION EQUAL 04))
        message(WARNING "Ubuntu ${UBUNTU_MAJOR_VERSION}.${UBUNTU_MINOR_VERSION} detected. Ubuntu 22.04 is recommended for collecting coverage data.")
    else()
        message(STATUS "Ubuntu 22.04 detected. Proceeding with coverage data collection.")
    endif()
endfunction()

check_ubuntu_version()

# add compile options for code coverage globally
add_compile_options(--coverage -O0 -g)
link_libraries(gcov)
add_definitions (-DCOLLECT_CODE_COVERAGE)
