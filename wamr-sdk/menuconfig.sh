#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception


usage ()
{
    echo "menuconfig.sh [options]"
    echo " -x [config file path name]"
    exit 1
}


while getopts "x:" opt
do
    case $opt in
        x)
        wamr_config_cmake_file=$OPTARG
        ;;
        ?)
        echo "Unknown arg: $arg"
        usage
        exit 1
        ;;
    esac
done




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




sdk_root=$(cd "$(dirname "$0")/" && pwd)
wamr_root=${sdk_root}/..

if [ ! `command -v menuconfig` ]; then
    echo "Can't find kconfiglib python lib on this computer"
    echo "Downloading it through pip"
    echo "If this fails, you can try `pip install kconfiglib` to install it manually"
    echo "Or download the repo from https://github.com/ulfalizer/Kconfiglib"

    pip install kconfiglib
fi

if [ -f ".wamr_modules" ]; then
    rm -f .wamr_modules
fi

# get all modules under core/app-framework
for module in `ls ${wamr_root}/core/app-framework -F | grep "/$" | grep -v "base" | grep -v "app-native-shared" | grep -v "template"`
do
    module=${module%*/}
    echo "config APP_BUILD_${module^^}"   >>  .wamr_modules
    echo "    bool \"enable ${module}\""  >>  .wamr_modules
done

menuconfig Kconfig
[ $? -eq 0 ] || exit $?

if [ ! -e ".config" ]; then
    exit 0
fi


args=""
function args_add_bool()
{
    args="${args} -$1"
}

function args_add_one()
{
    args="${args} -$1 $2"
}

function args_add_array()
{
    args="${args} -$1 ${2#,}"
}

source .config

profile=`cat .config | grep "^CONFIG_WAMR_SDK_PROFILE"`
profile=${profile#CONFIG_WAMR_SDK_PROFILE=\"}
profile=${profile%*\"}
args_add_one n ${profile}

platform=`cat .config | grep "^CONFIG_PLATFORM"`
platform=${platform%*=y}
platform=${platform,,}
platform=${platform#config_platform_}
if [ -n "${platform}" ]; then
    args_add_one p ${platform#config_platform_}
fi

target=`cat .config | grep "^CONFIG_TARGET"`
target=${target%*=y}
target=${target#CONFIG_TARGET_}
if [ -n "${target}" ]; then
    args_add_one t ${target#CONFIG_TARGET_}
fi


modes=`cat .config | grep "^CONFIG_EXEC"`
arg_mode=""
for mode in ${modes}
do
    mode=${mode%*=y}
    mode=${mode#CONFIG_EXEC_}
    arg_mode="${arg_mode},${mode,,}"
done
if [ -z "${arg_mode}" ]; then
    echo "execution mode are not selected"
    exit 1
fi
args_add_array m "${arg_mode}"

libc=`cat .config | grep "^CONFIG_LIBC"`
libc=${libc%*=y}
if [ "${libc}" = "CONFIG_LIBC_WASI" ]; then
    args_add_bool w
fi

app_en=`cat .config | grep "^CONFIG_APP_FRAMEWORK"`
app_en=${app_en%*=y}
app_en=${app_en,,}
if [ -n "${app_en}" ]; then
    args_add_bool a
fi

apps=`cat .config | grep "^CONFIG_APP_BUILD"`
arg_app=""
for app in ${apps}
do
    app=${app%*=y}
    app=${app#CONFIG_APP_BUILD_}
    arg_app="${arg_app},${app,,}"
done

if [ -n "${app_en}" ]; then
    arg_app="${arg_app},base"
    args_add_array l "${arg_app}"
fi

extra_path=`cat .config | grep "^CONFIG_EXTRA_INCLUDE_PATH"`
if [ -n "${extra_path}" ]; then
    extra_path=${extra_path#CONFIG_EXTRA_INCLUDE_PATH=\"}
    extra_path=${extra_path%*\"}
    args_add_one e ${extra_path}
fi

args="-g ${args}"


if  [[ -f $wamr_config_cmake_file ]]; then
    rm  $wamr_config_cmake_file
fi

set_build_target        ${TARGET}
set_build_platform      ${PLATFORM}
set_exec_mode           "${MODES[*]}"
set_libc_support        ${LIBC_SUPPORT}
set_app_module          "${APP_LIST[*]}"
set_app_framework       ${APP}
