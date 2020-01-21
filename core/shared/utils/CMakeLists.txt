# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include_directories (. ../include ../platform/include ../platform/${WAMR_BUILD_PLATFORM} coap/er-coap coap/extension)


file (GLOB_RECURSE source_all *.c )

add_library (vmutilslib ${source_all})

