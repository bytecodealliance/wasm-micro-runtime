#!/bin/bash

DEPS_ROOT=$(cd "$(dirname "$0")/" && pwd)
cd ${DEPS_ROOT}


if [ ! -d "lvgl" ]; then
        echo "git pull lvgl..."
        git clone https://github.com/littlevgl/lvgl.git --branch v6.0.1
        [ $? -eq 0 ] || exit $?

        ../app-framework/wgl/app/prepare_headers.sh
fi
if [ ! -d "lv_drivers" ]; then
        echo "git pull lv_drivers..."
        git clone https://github.com/littlevgl/lv_drivers.git
        [ $? -eq 0 ] || exit $?
fi

if [ ! -d "tlsf" ]; then
    echo "git pull tlsf..."
    git clone https://github.com/mattconte/tlsf
    [ $? -eq 0 ] || exit $?
    #cd ${WAMR_DIR}/core/shared/mem-alloc
fi


exit 0
