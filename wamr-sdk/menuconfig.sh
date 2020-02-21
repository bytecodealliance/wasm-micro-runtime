#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
./build_sdk.sh ${args}