# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include(FetchContent)

set(CJSON_SOURCE_DIR "${WAMR_ROOT_DIR}/core/deps/cjson")

FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG        v1.7.18
  SOURCE_DIR     ${CJSON_SOURCE_DIR}
)

set(ENABLE_CJSON_TEST OFF CACHE INTERNAL "Turn off tests")
set(ENABLE_CJSON_UNINSTALL OFF CACHE INTERNAL "Turn off uninstall to avoid targets conflict")
FetchContent_MakeAvailable(cjson)
