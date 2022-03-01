#!/bin/bash

DEPS_ROOT=$(cd "$(dirname "$0")/" && pwd)
cd ${DEPS_ROOT}


if [ ! -d "lvgl" ]; then
        echo "git pull lvgl..."
        git clone https://github.com/lvgl/lvgl.git --branch v6.0.1
        [ $? -eq 0 ] || exit $?

        ../app-framework/wgl/app/prepare_headers.sh
fi
if [ ! -d "lv_drivers" ]; then
        echo "git pull lv_drivers..."
        git clone https://github.com/lvgl/lv_drivers.git --branch v6.0.1
        [ $? -eq 0 ] || exit $?
fi

exit 0
