#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

X86_TARGET="x86"
STM32_TARGET="stm32"
QEMU_CORTEX_A53="qemu_cortex_a53"
XTENSA_QEMU_TARGET="xtensa-qemu"
ESP32_TARGET="esp32"

usage ()
{
        echo "USAGE:"
        echo "$0 $X86_TARGET|$STM32_TARGET|$QEMU_CORTEX_A53|$XTENSA_QEMU_TARGET|$ESP32_TARGET"
        echo "Example:"
        echo "        $0 $X86_TARGET"
        echo "        $0 $STM32_TARGET"
        echo "        $0 $QEMU_CORTEX_A53"
        echo "        $0 $XTENSA_QEMU_TARGET"
        echo "        $0 $ESP32_TARGET"
        exit 1
}

if [ $# != 1 ] ; then
        usage
fi

TARGET=$1

case $TARGET in
        $X86_TARGET)
                west build -b qemu_x86_nommu \
                           . -p always -- \
                           -DCONF_FILE=prj_qemu_x86_nommu.conf \
                           -DWAMR_BUILD_TARGET=X86_32
                west build -t run
                ;;
        $STM32_TARGET)
                west build -b nucleo_f767zi \
                           . -p always -- \
                           -DCONF_FILE=prj_nucleo767zi.conf \
                           -DWAMR_BUILD_TARGET=THUMBV7
                west flash
                ;;
        $XTENSA_QEMU_TARGET)
                west build -b qemu_xtensa \
                           . -p always -- \
                           -DCONF_FILE=prj_qemu_xtensa.conf \
                           -DWAMR_BUILD_TARGET=XTENSA
                west build -t run
                ;;
        $ESP32_TARGET)
                # suppose you have set environment variable ESP_IDF_PATH
                west build -b esp32 \
                           . -p always -- \
                           -DESP_IDF_PATH=$ESP_IDF_PATH \
                           -DCONF_FILE=prj_esp32.conf \
                           -DWAMR_BUILD_TARGET=XTENSA
                # suppose the serial port is /dev/ttyUSB1 and you should change to
                # the real name accordingly
                west flash --esp-device /dev/ttyUSB1
                ;;
        $QEMU_CORTEX_A53)
                west build -b qemu_cortex_a53 \
                           . -p always -- \
                           -DCONF_FILE=prj_qemu_cortex_a53.conf \
                           -DWAMR_BUILD_TARGET=AARCH64
                west build -t run
                ;;
        *)
                echo "unsupported target: $TARGET"
                usage
                exit 1
                ;;
esac

