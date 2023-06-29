#!/bin/bash

WGL_ROOT=$(cd "$(dirname "$0")/" && pwd)
LVGL_REPO_DIR=${WGL_ROOT}/../../../deps/lvgl
ls $LVGL_REPO_DIR

#if [ ! -d "${LVGL_REPO_DIR}" ]; then
#    echo "lvgl repo not exist, please git pull the lvgl v6.0 first"
#    exit 1
#fi

cd ${WGL_ROOT}/wa-inc/lvgl
pwd

if [ -d src ]; then
    rm -rf src
    echo "deleted the src folder from previous preparation."
fi

mkdir src
cd src

cp ${LVGL_REPO_DIR}/src/*.h ./

for folder in lv_core lv_draw lv_hal lv_objx lv_font lv_misc lv_themes
do
    echo "Prepare fold $folder...done"
    mkdir $folder
    cp ${LVGL_REPO_DIR}/src/${folder}/*.h ./${folder}/
done

cp -f ../lv_obj.h ./lv_core/lv_obj.h

echo "test the header files..."
cd ..

gcc test.c -o test.out
if [ $? != 0 ];then
    echo "failed to compile the test.c"
    exit 1
else
    echo "okay"
    rm test.out
fi

echo "lvgl header files for WASM application ready."
