#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

X86_TARGET="x86"
STM32_TARGET="stm32"
QEMU_CORTEX_A53="qemu_cortex_a53"
XTENSA_QEMU_TARGET="xtensa-qemu"
ESP32_TARGET="esp32"

if [ $# != 1 ] ; then
        echo "USAGE:"
        echo "$0 $X86_TARGET|$STM32_TARGET|$QEMU_CORTEX_A53|$XTENSA_QEMU_TARGET|$ESP32_TARGET"
        echo "Example:"
        echo "        $0 $X86_TARGET"
        echo "        $0 $STM32_TARGET"
        echo "        $0 $QEMU_CORTEX_A53"
        echo "        $0 $XTENSA_QEMU_TARGET"
        echo "        $0 $ESP32_TARGET"
        exit 1
fi

TARGET=$1

if [ "$TARGET" = "$X86_TARGET" ] ; then
        cp prj_qemu_x86_nommu.conf prj.conf
        rm -fr build && mkdir build && cd build
        cmake -GNinja -DBOARD=qemu_x86_nommu -DWAMR_BUILD_TARGET=X86_32 ..
        ninja
        ninja run
elif [ "$TARGET" = "$STM32_TARGET" ] ; then
        cp prj_nucleo767zi.conf prj.conf
        rm -fr build && mkdir build && cd build
        cmake -GNinja -DBOARD=nucleo_f767zi -DWAMR_BUILD_TARGET=THUMBV7 ..
        ninja
        ninja flash
elif [ "$TARGET" = "$XTENSA_QEMU_TARGET" ] ; then
        cp prj_qemu_xtensa.conf prj.conf
        rm -fr build && mkdir build && cd build
        cmake -GNinja -DBOARD=qemu_xtensa -DWAMR_BUILD_TARGET=XTENSA ..
        ninja
        ninja run
elif [ "$TARGET" = "$ESP32_TARGET" ] ; then
        # suppose you have set environment variable ESP_IDF_PATH
        west build -b esp32 . -p always -- \
                -DESP_IDF_PATH=$ESP_IDF_PATH \
                -DCONF_FILE=prj_esp32.conf \
                -DWAMR_BUILD_TARGET=XTENSA
        # suppose the serial port is /dev/ttyUSB1 and you should change to
        # the real name accordingly
        west flash -d ./build --skip-rebuild --esp-device /dev/ttyUSB1
elif [ "$TARGET" = "$QEMU_CORTEX_A53" ] ; then
        cp prj_qemu_cortex_a53.conf prj.conf
        rm -fr build && mkdir build && cd build
        cmake -GNinja -DBOARD=qemu_cortex_a53 -DWAMR_BUILD_TARGET=AARCH64 ..
        ninja
        ninja run
else
        echo "unsupported target: $TARGET"
        exit 1
fi
