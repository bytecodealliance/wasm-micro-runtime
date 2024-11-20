# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)
add_executable(raw main.c ${UNCOMMON_SHARED_SOURCE})
target_link_libraries(raw PRIVATE vmlib)
