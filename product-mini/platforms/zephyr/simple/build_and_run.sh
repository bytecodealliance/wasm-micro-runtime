#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

X86_TARGET="x86"
STM32_TARGET="stm32"
ESP32_TARGET="esp32"
QEMU_CORTEX_A53="qemu_cortex_a53"
QEMU_XTENSA_TARGET="qemu_xtensa"
QEMU_RISCV64_TARGET="qemu_riscv64"
QEMU_RISCV32_TARGET="qemu_riscv32"
QEMU_ARC_TARGET="qemu_arc"

usage ()
{
        echo "USAGE:"
        echo "$0 $X86_TARGET|$STM32_TARGET|$ESP32_TARGET|$QEMU_CORTEX_A53|$QEMU_XTENSA_TARGET|$QEMU_RISCV64_TARGET|$QEMU_RISCV32_TARGET|$QEMU_ARC_TARGET"
        echo "Example:"
        echo "        $0 $X86_TARGET"
        echo "        $0 $STM32_TARGET"
        echo "        $0 $ESP32_TARGET"
        echo "        $0 $QEMU_CORTEX_A53"
        echo "        $0 $QEMU_XTENSA_TARGET"
        echo "        $0 $QEMU_RISCV64_TARGET"
        echo "        $0 $QEMU_RISCV32_TARGET"
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
        $QEMU_XTENSA_TARGET)
                west build -b qemu_xtensa \
                           . -p always -- \
                           -DCONF_FILE=prj_qemu_xtensa.conf \
                           -DWAMR_BUILD_TARGET=XTENSA
                west build -t run
                ;;
        $QEMU_CORTEX_A53)
                west build -b qemu_cortex_a53 \
                           . -p always -- \
                           -DCONF_FILE=prj_qemu_cortex_a53.conf \
                           -DWAMR_BUILD_TARGET=AARCH64
                west build -t run
                ;;
        $QEMU_RISCV64_TARGET)
                west build -b qemu_riscv64 \
                            . -p always -- \
                            -DCONF_FILE=prj_qemu_riscv64.conf \
                            -DWAMR_BUILD_TARGET=RISCV64_LP64 \
                            -DWAMR_BUILD_AOT=0
                west build -t run
                ;;
        $QEMU_RISCV32_TARGET)
                west build -b qemu_riscv32 \
                            . -p always -- \
                            -DCONF_FILE=prj_qemu_riscv32.conf \
                            -DWAMR_BUILD_TARGET=RISCV32_ILP32 \
                            -DWAMR_BUILD_AOT=0
                west build -t run
                ;;
        $QEMU_ARC_TARGET)
                west build -b qemu_arc_em \
                            . -p always -- \
                            -DCONF_FILE=prj_qemu_arc.conf \
                            -DWAMR_BUILD_TARGET=ARC \
                            -DWAMR_BUILD_AOT=0
                west build -t run
                ;;
        *)
                echo "unsupported target: $TARGET"
                usage
                exit 1
                ;;
esac

