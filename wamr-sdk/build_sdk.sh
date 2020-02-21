#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

sdk_root=$(cd "$(dirname "$0")/" && pwd)
wamr_root_dir=${sdk_root}/..
out_dir=${sdk_root}/out
profile_path=${out_dir}/profile.cmake
wamr_config_cmake_file=""
# libc support, default builtin-libc
LIBC_SUPPORT="BUILTIN"
CMAKE_DEXTRA_SDK_INCLUDE_PATH=""

# menuconfig will pass options to this script
MENUCONFIG=""

usage ()
{
    echo "build.sh [options]"
    echo " -n [profile name]"
    echo " -x [config file path name]"
    echo " -e [extra include path], files under this path will be copied into SDK package"
    echo " -c, clean"
    echo " -i, enter interactive config setting"
    exit 1
}


while getopts "e:x:n:t:m:l:awgicg" opt
do
    case $opt in
        n)
        PROFILE=$OPTARG
        ;;
        x)
        wamr_config_cmake_file=$OPTARG
        ;;
        e)
        CMAKE_DEXTRA_SDK_INCLUDE_PATH="-DEXTRA_SDK_INCLUDE_PATH=${OPTARG}"
        ;;
        a)
        APP="TRUE"
        ;;
        c)
        CLEAN="TRUE"
        ;;
        w)
        LIBC_SUPPORT="WASI"
        ;;
        i)
        MENUCONFIG="TRUE"
        ;;
        ?)
        echo "Unknown arg: $arg"
        usage
        exit 1
        ;;
    esac
done

if [ ! -d "${out_dir}" ]; then
    mkdir -p ${out_dir}
fi

echo "CMAKE_DEXTRA_SDK_INCLUDE_PATH=${CMAKE_DEXTRA_SDK_INCLUDE_PATH}"


if [ -z "$PROFILE" ]; then
    PROFILE="default"
    echo "PROFILE argument not set, using DEFAULT"
fi

curr_profile_dir=${out_dir}/${PROFILE}
wamr_app_out_dir=${curr_profile_dir}/app-sdk/wamr-app-framework
sysroot_dir=${curr_profile_dir}/app-sdk/libc-builtin-sysroot

if [[ "$CLEAN" = "TRUE" ]]; then
    rm -rf ${curr_profile_dir}
fi

# cmake config file for wamr runtime:
# 1. use the users provided the config cmake file path.
# 2. if user set MENU CONFIG, enter menu config to generate menu_config.cmake in the profile output folder
# 3. If the menu_config.cmake is already in the profile folder, use it
# 4. Use the default config cmake file
if [[ -n "$wamr_config_cmake_file" ]]; then
	echo "User config file: [${wamr_config_cmake_file}]"
else
	wamr_config_cmake_file=${curr_profile_dir}/wamr_config_menu.cmake
	if [[ "$MENUCONFIG" = "TRUE" ]] || [[ "$FROM_GUI_MENU" = "TRUE" ]]; then
		echo "MENUCONFIG: user config file: [${wamr_config_cmake_file}]"
	elif  [[ -f $wamr_config_cmake_file ]]; then
		echo "use existing config file: [$wamr_config_cmake_file]"
 	else
 		wamr_config_cmake_file=${sdk_root}/wamr_config_default.cmake
 		echo "use default config file: [$wamr_config_cmake_file]"
    fi
fi

# if called by gui menuconfig, overwrite the exist profile
if [ "${FROM_GUI_MENU}" != "TRUE" ]; then
    if [[ "$PROFILE" != "default" ]] && [[ -d "$curr_profile_dir" ]]; then
        echo "#########################################################"
        echo "profile ${curr_profile_dir} already exists"
        echo "  skip the build process and use the previous settings: [y]"
        echo "  or delete the profile and generate a new one:          n"
        read -a erase_exist
        if [[ "$erase_exist" != "n" ]] && [[ "$erase_exist" != "N" ]]; then
            exit 0
        fi

        rm -rf ${curr_profile_dir}
    fi
fi


mkdir -p ${curr_profile_dir}
mkdir -p ${curr_profile_dir}/app-sdk
mkdir -p ${curr_profile_dir}/runtime-sdk


if [ "${BUILD_LLVM}" = "TRUE" ]; then
    if [ ! -d "${wamr_root_dir}/core/deps/llvm" ]; then
        echo -e "\n"
        echo "######  build llvm (this will take a long time)  #######"
        echo ""
        cd ${wamr_root_dir}/wamr-compiler
        ./build_llvm.sh
    fi
fi

echo -e "\n\n"
echo "##############  Start to build wasm app sdk  ###############"
cd ${sdk_root}/app
rm -fr build && mkdir build
cd build
if [ "${LIBC_SUPPORT}" = "WASI" ]; then
    echo "using wasi toolchain"
    cmake .. $CMAKE_DEXTRA_SDK_INCLUDE_PATH -DWAMR_BUILD_SDK_PROFILE=${PROFILE} -DCONFIG_PATH=${wamr_config_cmake_file} -DCMAKE_TOOLCHAIN_FILE=../wasi_toolchain.cmake
else
    echo "using builtin libc toolchain"
    cmake .. $CMAKE_DEXTRA_SDK_INCLUDE_PATH -DWAMR_BUILD_SDK_PROFILE=${PROFILE} -DCONFIG_PATH=${wamr_config_cmake_file} -DCMAKE_TOOLCHAIN_FILE=../wamr_toolchain.cmake
fi
[ $? -eq 0 ] || exit $?
make

if (( $? == 0 )); then
    echo -e "\033[32mSuccessfully built app-sdk under ${curr_profile_dir}/app-sdk\033[0m"
else
    echo -e "\033[31mFailed to build app-sdk for wasm application\033[0m"
    exit 1
fi

cd ..
rm -fr build

echo -e "\n\n"
echo "##############  Start to build runtime sdk  ###############"
cd ${sdk_root}/runtime
rm -fr build_runtime_sdk && mkdir build_runtime_sdk
cd build_runtime_sdk
cmake .. $CMAKE_DEXTRA_SDK_INCLUDE_PATH -DWAMR_BUILD_SDK_PROFILE=${PROFILE} -DCONFIG_PATH=${wamr_config_cmake_file}
[ $? -eq 0 ] || exit $?
make

if (( $? == 0 )); then
    echo -e "\033[32mSuccessfully built runtime library under ${curr_profile_dir}/runtime-sdk/lib\033[0m"
else
    echo -e "\033[31mFailed to build runtime sdk\033[0m"
    exit 1
fi

cd ..
rm -fr build_runtime_sdk

if [ "$APP" = "TRUE" ]; then
    # Generate defined-symbol list for app-sdk
    cd ${wamr_app_out_dir}/share
    cat ${curr_profile_dir}/runtime-sdk/include/*.inl | egrep "^ *EXPORT_WASM_API *[(] *[a-zA-Z_][a-zA-Z0-9_]* *?[)]" | cut -d '(' -f2 | cut -d ')' -f1 > defined-symbols.txt
    echo "wasm_register_resource"       >> defined-symbols.txt
    echo "wasm_response_send"           >> defined-symbols.txt
    echo "wasm_post_request"            >> defined-symbols.txt
    echo "wasm_sub_event"               >> defined-symbols.txt
    echo "wasm_create_timer"            >> defined-symbols.txt
    echo "wasm_timer_destroy"           >> defined-symbols.txt
    echo "wasm_timer_cancel"            >> defined-symbols.txt
    echo "wasm_timer_restart"           >> defined-symbols.txt
    echo "wasm_get_sys_tick_ms"         >> defined-symbols.txt
fi

