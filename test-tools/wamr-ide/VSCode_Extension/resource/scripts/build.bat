@REM boot up debugger server docker image and execute the wasm target with iwasm
@REM mount current workspace explorer into docker container
echo off

set AoT_Binary_Name=%1

@REM start a container, mount current project path to container/mnt
docker run --name=wasm-toolchain-provider-ctr ^
                -it -v %cd%:/mnt ^
                wasm-toolchain-provider:1.0  ^
                /bin/bash -c "./build_wasm.sh %AoT_Binary_Name%"

@REM stop and remove wasm-toolchain-ctr container
docker stop wasm-toolchain-provider-ctr && docker rm wasm-toolchain-provider-ctr
