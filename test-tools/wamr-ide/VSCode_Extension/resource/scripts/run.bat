@REM boot up debugger server docker image and execute the wasm target with iwasm
@REM mount current workspace explorer into docker container
echo off

set target_name=%1

docker run -it --name=wasm-executor-ctr ^
           -v "%cd%":/mnt ^
           wasm-debug-server:1.0 ^
           /bin/bash -c "./run.sh %target_name%"

@REM stop and remove wasm-executor-ctr
docker stop wasm-executor-ctr && docker rm wasm-executor-ctr
