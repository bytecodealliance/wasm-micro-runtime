# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include_directories (.
                     include
                     platform/include
                     platform/${PLATFORM}
                     ../classlib/include
                     utils/coap/er-coap
                     utils/coap/extension
                     ../external/iwasm/include)


file (GLOB_RECURSE source_all ../../app-manager/*.c)

add_library (appmgrlib ${source_all})

