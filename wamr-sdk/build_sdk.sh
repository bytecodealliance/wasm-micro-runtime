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
    echo " -p [platform]"
    echo " -t [target]"
    echo " -m [mode]"
    echo " -e [extra include path], files under this path will be copied into SDK package"
    echo " -c, clean"
    echo " -i, enter interactive config setting"
    exit 1
}


while getopts "e:x:n:p:t:m:l:awgicg" opt
do
    case $opt in
        n)
        PROFILE=$OPTARG
        ;;
        x)
        wamr_config_cmake_file=$OPTARG
        ;;
        p)
        PLATFORM=$OPTARG
        ;;
        t)
        TARGET=$OPTARG
        ;;
        m)
        OLD_IFS="$IFS"
        IFS=","
        MODES=($OPTARG)
        IFS="$OLD_IFS"
        ;;
        l)
        OLD_IFS="$IFS"
        IFS=","
        APP_LIST=($OPTARG)
        IFS="$OLD_IFS"
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
        g)
        FROM_GUI_MENU="TRUE"
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
#4: use the default config cmake file
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

function set_build_target () {
    target=$1

    if [[ "${target}" = "X86_64" ]]; then
        echo -e "set (WAMR_BUILD_TARGET \"X86_64\")" >> ${wamr_config_cmake_file}
    elif [[ "${target}" = "X86_32" ]]; then
        echo -e "set (WAMR_BUILD_TARGET \"X86_32\")" >> ${wamr_config_cmake_file}
    else
        echo "unknown build target."
        exit 1
    fi
}

function set_build_platform () {
    platform=$1

    if [[ "${platform}" = "linux" ]]; then
        echo -e "set (WAMR_BUILD_PLATFORM \"linux\")" >> ${wamr_config_cmake_file}
    # TODO: add other platforms
    else
        echo "${platform} platform currently not supported"
        exit 1
    fi
}

# input: array of selected exec modes [aot jit interp]
function set_exec_mode () {
    modes=($1)

    for mode in ${modes[@]}
    do
        if [[ "$mode" = "aot" ]]; then
            echo "set (WAMR_BUILD_AOT 1)" >> ${wamr_config_cmake_file}
        elif [[ "$mode" = "jit" ]]; then
            echo "set (WAMR_BUILD_JIT 1)" >> ${wamr_config_cmake_file}
            BUILD_LLVM="TRUE"
        elif [[ "$mode" = "interp" ]]; then
            echo "set (WAMR_BUILD_INTERP 1)" >> ${wamr_config_cmake_file}
        else
            echo "unknown execute mode."
            exit 1
        fi
    done
}

function set_libc_support () {
    libc=$1

    if [ "$libc" = "WASI" ]; then
        echo "set (WAMR_BUILD_LIBC_WASI 1)" >> ${wamr_config_cmake_file}
    else
        echo "set (WAMR_BUILD_LIBC_BUILTIN 1)" >> ${wamr_config_cmake_file}
    fi
}

function set_app_framework () {
    app_support=$1

    if [ "$app_support" = "TRUE" ]; then
        echo "set (WAMR_BUILD_APP_FRAMEWORK 1)" >> ${wamr_config_cmake_file}
    fi
}

# input: array of selected app modules
function set_app_module () {
    modules=($1)

    for module in ${modules[*]}
    do
        if [ "${module}" = "all" ]; then
            cmake_app_list="WAMR_APP_BUILD_ALL"
            break
        fi

        cmake_app_list="${cmake_app_list} WAMR_APP_BUILD_${module^^}"
    done

    # APP module list
    if [ -n "${cmake_app_list}" ]; then
        echo "set (WAMR_BUILD_APP_LIST ${cmake_app_list# })" >> ${wamr_config_cmake_file}
    fi
}

if [ ! -f "/opt/wasi-sdk/bin/clang" ]; then
        echo "Can't find wasi-sdk under /opt/wasi-sdk"
        echo "You can download wasi-sdk from here:"
        echo ""
        echo "https://github.com/CraneStation/wasi-sdk/releases/tag/wasi-sdk-7"
        echo ""
        echo "please install it to the default path for your convenience"
        echo ""
        exit 1
fi

if [ "${FROM_GUI_MENU}" = "TRUE" ]; then
    # called from gui based menuconfig,
    # all settings are passed from command line options

    if  [[ -f $wamr_config_cmake_file ]]; then
		rm 	$wamr_config_cmake_file
	fi

    set_build_target        ${TARGET}
    set_build_platform      ${PLATFORM}
    set_exec_mode           "${MODES[*]}"
    set_libc_support        ${LIBC_SUPPORT}
    set_app_module          "${APP_LIST[*]}"
    set_app_framework       ${APP}
fi


# No options passed, ask for user input
if [ "$MENUCONFIG" = "TRUE" ]; then

	if  [[ -f $wamr_config_cmake_file ]]; then
		rm 	$wamr_config_cmake_file
	fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "select a build target:"
    echo "[1] X86_64 (default)"
    echo "[2] X86_32"
    read -a select_target

    if [ "${select_target}" = "2" ]; then
        TARGET="X86_32"
    else
        TARGET="X86_64"
    fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "select a build platform:"
    echo "[1] linux (default)"
    echo "More platforms to be add here ..."

    read -a select_platform

    if [ "${select_platform}" = "1" ]; then
        PLATFORM="linux"
    # TODO: add more platforms
    else
        PLATFORM="linux"
    fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "select one or more execution mode of the WAMR runtime"

    enable_interp="y"
    enable_jit="n"
    enable_aot="n"

    read -p "enable interpreter mode [y]/n:  " -a enable_interp
    read -p "enable jit mode y/[n]:  " -a enable_jit
    read -p "enable aot mode y/[n]:  " -a enable_aot

    # by default the interpreter mode is selected
    if [[ ${enable_interp} != "n" ]] && [[ ${enable_interp} != "N" ]]; then
        enable_interp="y"
    fi

    if [[ ${enable_interp} != "y" ]] && [[ ${enable_aot} != "y" ]];
    then
        echo "WASM Interpreter and AOT must be enabled at least one"
        exit 1
    fi

    if [[ ${enable_interp} = "y" ]]; then
        MODES[${#MODES[@]}]=interp
    fi
    if [[ ${enable_jit} = "y" ]] || [[ ${enable_jit} = "Y" ]]; then
        MODES[${#MODES[@]}]=jit
    fi
    if [[ ${enable_aot} = "y" ]] || [[ ${enable_aot} = "Y" ]]; then
        MODES[${#MODES[@]}]=aot
    fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "select a libc support:"
    echo "[1] builtin libc (default)"
    echo "[2] WebAssembly System Interface (WASI)"
    read -a libc_select

    if [ "$libc_select" = "1" ]; then
        LIBC_SUPPORT="BUILTIN"
    elif [ "$libc_select" = "2" ]; then
        LIBC_SUPPORT="WASI"
    fi

    echo ""
    echo "-----------------------------------------------------------------"
    echo "enable app framework? [y]/n"
    read -a enable_app

    if [[ "$enable_app" != "n" ]] && [[ "$enable_app" != "N" ]]; then
        APP="TRUE"
    fi

    if [[ "$APP" = "TRUE" ]]; then
        echo ""
        echo "-----------------------------------------------------------------"
        echo "please input the name of the module you need, seperate by ',' "
        echo "type \"all\" if you want to enable all of them"
        echo "---------------"
        for folder in `ls ${wamr_root_dir}/core/app-framework -F | grep "/$" | grep -v "base" | grep -v "app-native-shared"  | grep -v "template"`
        do
            folder=${folder%*/}
            echo "${folder}"
        done
        echo "---------------"
        read -a app_select

        app_select=${app_select},base
        app_select=${app_select#,}
        OLD_IFS="$IFS"
        IFS=","
        APP_LIST=($app_select)
        IFS="$OLD_IFS"
    fi

    set_build_target        ${TARGET}
    set_build_platform      ${PLATFORM}
    set_exec_mode           "${MODES[*]}"
    set_libc_support        ${LIBC_SUPPORT}
    set_app_module          "${APP_LIST[*]}"
    set_app_framework       ${APP}
fi

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

