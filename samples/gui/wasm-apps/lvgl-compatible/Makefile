# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

CC = emcc
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
    -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
    -s TOTAL_MEMORY=65536 -s TOTAL_STACK=2048\
    -s "EXPORTED_FUNCTIONS=['_on_init', '_on_timer_callback', '_on_widget_event']" \
    -o ui_app_lvgl_compatible.wasm
