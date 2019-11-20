# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

CC = clang-8
APP_DIR = ${shell pwd}
IWASM_DIR=../../../../core/iwasm
CFLAGS += -O3 \
	  -Wno-int-conversion \
	  -DLV_CONF_INCLUDE_SIMPLE \
	  -DENABLE_WGL=1 \
	  -I$(APP_DIR)/src/ \
	  -I$(IWASM_DIR)/lib/app-libs/base/ \
	  -I$(IWASM_DIR)/lib/native-interface/ \
	  -I$(IWASM_DIR)/lib/app-libs/extension/sensor \
	  -I$(IWASM_DIR)/lib/app-libs/extension/gui \
	  -I$(IWASM_DIR)/lib/app-libs/extension/connection

SRCS += $(APP_DIR)/src/main.c

# For app size consideration, not all but necessary app libs are included
SRCS += $(IWASM_DIR)/lib/app-libs/base/timer.c
SRCS += $(IWASM_DIR)/lib/app-libs/extension/gui/src/*.c

all: 
	@$(CC) $(CFLAGS) $(SRCS) \
    --target=wasm32 -O3 -z stack-size=2048 -Wl,--initial-memory=65536 \
    -Wl,--allow-undefined \
    -Wl,--no-threads,--strip-all,--no-entry -nostdlib \
    -Wl,--export=on_init -Wl,--export=on_timer_callback \
    -Wl,--export=on_widget_event \
    -o ui_app.wasm
