#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

X86_TARGET="x86"
STM32_TARGET="stm32"

if [ $# != 1 ] ; then
        echo "USAGE:"
        echo "$0 $X86_TARGET|$STM32_TARGET"
        echo "Example:"
        echo "        $0 $X86_TARGET"
        echo "        $0 $STM32_TARGET"
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
else
        echo "unsupported target: $TARGET"
        exit 1
fi
